
#include "SettingsManager.h"
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/XmlTreeQueryLib.h>
#include <Library/DfciXmlSettingSchemaSupportLib.h>
#include <Library/PrintLib.h>
#include <Library/DfciSerialNumberSupportLib.h>
#include <Guid/WinCertificate.h>
#include <Guid/DfciSettingsManagerVariables.h>


//Internal state tracking of incoming request
//Lower nibble is good status.  Upper nibble means error state.  
typedef enum {
  SETTING_STATE_UNINITIALIZED         = 0x00,
  SETTING_STATE_DATA_PRESENT          = 0x01,
  SETTING_STATE_DATA_AUTHENTICATED    = 0x02,
  SETTING_STATE_DATA_APPLIED          = 0x03,
  SETTING_STATE_DATA_COMPLETE         = 0x0F,   //Complete
  SETTING_STATE_VERSION_ERROR         = 0xF0,   //LSV blocked processing settings
  SETTING_STATE_NOT_CORRECT_TARGET    = 0xFA,   //Packet target value doesn't match device
  SETTING_STATE_SYSTEM_ERROR          = 0xFB,   //Some sort of system error blocked processing XML
  SETTING_STATE_BAD_XML               = 0xFC,   //Bad XML data.  Didn't follow rules
  SETTING_STATE_DATA_INVALID          = 0xFD,   //Invalid Data
  SETTING_STATE_DATA_AUTH_FAILED      = 0xFE    
}SETTING_STATE;

//Internal global object to handle incoming request
typedef struct {
  DFCI_SECURED_SETTINGS_APPLY_VAR  *Var;
  UINTN                          VarSize;
  UINT32                         SessionId;
  SETTING_STATE                  State;
  DFCI_SETTING_INTERNAL_DATA *InternalData;
  EFI_STATUS                     StatusCode;
  BOOLEAN                        ResetRequired; 
  DFCI_AUTH_TOKEN                  IdentityToken;
  CHAR8                          *ResultXml; 
  UINTN                          ResultXmlSize;
} SETTING_INSTANCE_DATA;

//
// Check to see if we have pending input
//
EFI_STATUS
EFIAPI
GetPendingInputSettings(
  IN SETTING_INSTANCE_DATA *Data)
{
  EFI_STATUS Status;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetVariable2(XML_SETTINGS_APPLY_INPUT_VAR_NAME,
    &gDfciSettingsManagerVarNamespace,
    &Data->Var,
    &Data->VarSize
    );

  if (EFI_ERROR(Status))
  {
    if (Status == EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_INFO, "%a - No Incoming Data.\n", __FUNCTION__));
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a - Error getting variable - %r\n", __FUNCTION__, Status));
      Data->State = SETTING_STATE_DATA_INVALID;
      Status = EFI_ABORTED;
      Data->StatusCode = Status;  
    }
    return Status;
  }

  if (Data->VarSize > MAX_ALLOWABLE_VAR_INPUT_SIZE)
  {
    DEBUG((DEBUG_ERROR, "%a - Incomming Setting Apply var is too big (%d bytes)\n", __FUNCTION__, Data->VarSize));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  Data->State = SETTING_STATE_DATA_PRESENT;
  DEBUG((DEBUG_INFO, "%a - Incomming Settings Apply var Size: 0x%X\n", __FUNCTION__, Data->VarSize));
  Data->StatusCode = Status;
  return Status;
}


/**
Function to authenticate the data and get an identity based on the xml payload and signature
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticateSettings(
  IN SETTING_INSTANCE_DATA *Data)
{
  UINTN SignedDataLength  = 0;
  UINTN SigLen = 0;
  WIN_CERTIFICATE   *SignaturePtr;
  EFI_STATUS Status; 

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != SETTING_STATE_DATA_PRESENT)
  {
    DEBUG((DEBUG_ERROR, "%a - Wrong start state.\n", __FUNCTION__));
    Data->State = SETTING_STATE_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  //verify variable header signature
  if (Data->Var->HeaderSignature != DFCI_SECURED_SETTINGS_APPLY_VAR_SIGNATURE)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Header Signature\n", __FUNCTION__));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
    return EFI_INCOMPATIBLE_VERSION;
  }

  //Verify variable header version
  if (Data->Var->HeaderVersion != DFCI_SECURED_SETTINGS_VAR_VERSION)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Header Version.  %d\n", __FUNCTION__, Data->Var->HeaderVersion));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
    return EFI_INCOMPATIBLE_VERSION;
  }

  //Verify variable payload size vs varsize.  can't be larger. 
  if ((UINTN)(Data->Var->PayloadSize) > Data->VarSize)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Payload Size(0x%x).  Larger than VarSize.\n", __FUNCTION__, Data->Var->PayloadSize));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  //do basic size checking here.  Do enough that we can claim the pointers are valid...but don't 
  //check the WIN CERT.  Leave that to auth manager. 
  SignedDataLength = sizeof(DFCI_SECURED_SETTINGS_APPLY_VAR) + Data->Var->PayloadSize;
  DEBUG((DEBUG_INFO, "%a - SignedDataLength = 0x%X\n", __FUNCTION__, SignedDataLength));
  if (SignedDataLength > (Data->VarSize - sizeof(WIN_CERTIFICATE_UEFI_GUID)))
  {
    DEBUG((DEBUG_ERROR, "%a - SignedDataLength is too long compared to VarSize\n", __FUNCTION__));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  SignaturePtr = (WIN_CERTIFICATE *)(((UINT8*)Data->Var) + SignedDataLength); //first byte after payload
  SigLen = Data->VarSize - SignedDataLength;  //find out the max size of sig data based on var size and start of sig data.  
  if (SigLen != SignaturePtr->dwLength)
  {
    DEBUG((DEBUG_ERROR, "%a - Signature Data not expected size (0x%X) (0x%X)\n", __FUNCTION__, SigLen, SignaturePtr->dwLength));
    Data->State = SETTING_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  //Get the session Id from the variable and then zero it before signature validation
  Data->SessionId = Data->Var->SessionId;
  Data->Var->SessionId = 0;

  DEBUG((DEBUG_INFO, "%a - Session ID = 0x%X\n", __FUNCTION__, Data->SessionId));

  //Lets check for device specific targetting using Serial Number
  if (Data->Var->SerialNumber != 0)
  {
    UINTN DeviceSerialNumber = 0;
    DEBUG((DEBUG_INFO, "%a - Target Packet with sn %ld\n", __FUNCTION__, Data->Var->SerialNumber));
    Status = GetSerialNumber(&DeviceSerialNumber);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to get device serial number %r\n", Status));
      Data->StatusCode = EFI_OUT_OF_RESOURCES;
      Data->State = SETTING_STATE_SYSTEM_ERROR;
      return Data->StatusCode;
    }

    DEBUG((DEBUG_INFO, "%a - Device SN: %ld\n", __FUNCTION__, DeviceSerialNumber));

    //have serial number now compare to packet
    if (Data->Var->SerialNumber != (UINT64)DeviceSerialNumber)
    {
      DEBUG((DEBUG_ERROR, "Setting Packet not for this device.  Packet SN Target: %ld\n", Data->Var->SerialNumber));
      Data->StatusCode = EFI_ABORTED;
      Data->State = SETTING_STATE_NOT_CORRECT_TARGET;
      return Data->StatusCode;
    }
  }

  
  Status = CheckAuthAndGetToken((UINT8*) Data->Var, SignedDataLength, SignaturePtr, &(Data->IdentityToken));
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Authenticate Settings %r\n", __FUNCTION__, Status));
    Data->State = SETTING_STATE_DATA_AUTH_FAILED;  //Auth Error
    Data->StatusCode = EFI_SECURITY_VIOLATION;
    return Data->StatusCode;
  }

  Data->State = SETTING_STATE_DATA_AUTHENTICATED; //authenticated
  return EFI_SUCCESS;
}

//
// Apply all settings from XML to their associated setting providers
//
EFI_STATUS
EFIAPI
ApplySettings(
  IN SETTING_INSTANCE_DATA *Data)
{
  XmlNode                   *InputRootNode = NULL;        //The root xml node for the Input list.
  XmlNode                   *InputPacketNode = NULL;      //The SettingsPacket node in the Input list
  XmlNode                   *InputSettingsNode = NULL;    //The Settings node for the Input list.
  XmlNode                   *InputTempNode = NULL;        //Temp node ptr to use when moving thru the Input list

  XmlNode                   *ResultRootNode = NULL;       //The root xml node in the result list
  XmlNode                   *ResultPacketNode = NULL;     //The ResultsPacket node in the result list
  XmlNode                   *ResultSettingsNode = NULL;   //The Settings Node in the result list

  LIST_ENTRY                  *Link = NULL;

  EFI_STATUS                  Status;
  UINTN                       StrLen = 0;
  DFCI_SETTING_FLAGS     Flags = 0;
  EFI_TIME                    ApplyTime;

  UINTN                       Version = 0;
  UINTN                       Lsv = 0;


  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != SETTING_STATE_DATA_AUTHENTICATED)
  {
    DEBUG((DEBUG_ERROR, "%a - Wrong start state (0x%X)\n", __FUNCTION__, Data->State));
    Data->State = SETTING_STATE_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  StrLen = AsciiStrnLenS( (CHAR8*)(&(Data->Var->Payload)), Data->Var->PayloadSize);
  DEBUG((DEBUG_INFO, "%a - StrLen = 0x%X PayloadSize = 0x%X\n", __FUNCTION__, StrLen, Data->Var->PayloadSize));

  //
  // Create Node List from input
  //
  Status = CreateXmlTree((CONST CHAR8 *)Data->Var->Payload, StrLen, &InputRootNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create a node list from the payload xml  %r\n", __FUNCTION__, Status));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //print the list
  DEBUG((DEBUG_INFO, "PRINTING INPUT XML - Start\n"));
  DebugPrintXmlTree(InputRootNode, 0);
  DEBUG((DEBUG_INFO, "PRINTING INPUT XML - End\n"));

  //
  // Create Node List for output
  //
  Status = gRT->GetTime(&ApplyTime, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get time. %r\n", __FUNCTION__, Status));
    Data->State = SETTING_STATE_SYSTEM_ERROR;
    Status = EFI_ABORTED;
    goto EXIT;
  }

  ResultRootNode = New_ResultPacketNodeList(&ApplyTime);
  if(ResultRootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create a node list from the result xml.\n", __FUNCTION__));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //Get Input SettingsPacket Node
  InputPacketNode = GetSettingsPacketNode(InputRootNode);
  if (InputPacketNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input SettingsPacket Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //Get Output ResultsPacket Node
  ResultPacketNode = GetResultsPacketNode(ResultRootNode);
  if (ResultPacketNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Output ResultsPacket Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  //
  //Get input version
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, SETTINGS_VERSION_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Version Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming Version: %a\n", InputTempNode->Value));
  Version = AsciiStrDecimalToUintn(InputTempNode->Value);

  if (Version > 0xFFFFFFFF)
  {
    DEBUG((DEBUG_INFO, "Version Value invalid.  0x%x\n", Version));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  
  //check against lsv
  if (Data->InternalData->LSV > (UINT32)Version)
  {
    DEBUG((DEBUG_INFO, "Setting Version Less Than System LSV\n"));
    Data->State = SETTING_STATE_VERSION_ERROR;
    Status = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  //
  //Get Incomming LSV
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, SETTINGS_LSV_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get LSV Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming LSV: %a\n", InputTempNode->Value));
  Lsv = AsciiStrDecimalToUintn(InputTempNode->Value);

  if (Lsv > 0xFFFFFFFF)
  {
    DEBUG((DEBUG_INFO, "Lowest Supported Version Value invalid.  0x%x\n", Lsv));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (Lsv > Version)
  {
    DEBUG((DEBUG_ERROR, "%a - LSV (%a) can't be larger than current version\n", __FUNCTION__, InputTempNode->Value));
    Data->State = SETTING_STATE_DATA_INVALID;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //set the new version
  if (Data->InternalData->CurrentVersion != (UINT32)Version)
  {
    Data->InternalData->CurrentVersion = (UINT32)Version;
    Data->InternalData->Modified = TRUE;
  }
  
  //If new LSV is larger set it
  if ((UINT32)Lsv > Data->InternalData->LSV)
  {
    DEBUG((DEBUG_INFO, "%a - Setting New LSV (0x%X)\n", __FUNCTION__, Lsv));
    Data->InternalData->LSV = (UINT32)Lsv;
    Data->InternalData->Modified = TRUE;
  }

  // Get the Xml Node for the SettingsList
  InputSettingsNode  = GetSettingsListNodeFromPacketNode(InputPacketNode);
  ResultSettingsNode = GetSettingsListNodeFromPacketNode(ResultPacketNode);

  if (ResultSettingsNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Result Settings List Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status= EFI_ABORTED;  //internal xml..should never fail
    goto EXIT;
  }

  if (InputSettingsNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input Settings List Node\n"));
    Data->State = SETTING_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //All verified.   Now lets walk thru the Settings and try to apply each one.  
  for (Link = InputSettingsNode->ChildrenListHead.ForwardLink; Link != &(InputSettingsNode->ChildrenListHead); Link = Link->ForwardLink)
  {
    XmlNode *NodeThis = NULL;
    CHAR8*    Id = NULL;
    CHAR8*    Value = NULL;
    CHAR8     StatusString[25];   //0xFFFFFFFFFFFFFFFF\n
    CHAR8     FlagString[25];
    Flags = 0;
    
    NodeThis = (XmlNode*)Link;   //Link is first member so just cast it.  this is the <Setting> node
    Status = GetInputSettings(NodeThis, &Id, &Value);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to GetInputSettings.  Bad XML Data. %r\n", Status));
      Data->State = SETTING_STATE_BAD_XML;
      Status = EFI_NO_MAPPING;
      goto EXIT;
    }

    //Now we have an Id and Value
    Status = SetSettingFromAscii(Id, Value, &(Data->IdentityToken), &Flags);
    DEBUG((DEBUG_INFO, "%a - Set %a = %a. Result = %r\n", __FUNCTION__, Id, Value, Status));

    //Record Status result
    ZeroMem(StatusString, sizeof(StatusString));
    ZeroMem(FlagString, sizeof(FlagString));
    StatusString[0] = '0';
    StatusString[1] = 'x';
    FlagString[0] = '0';
    FlagString[1] = 'x';

    AsciiValueToString(&(StatusString[2]), RADIX_HEX, (INT64)Status, 18);
    AsciiValueToString(&(FlagString[2]), RADIX_HEX, (INT64)Flags, 18);
    Status = SetOutputSettingsStatus(ResultSettingsNode, Id, &(StatusString[0]), &(FlagString[0]));
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to SetOutputSettingStatus.  %r\n", Status));
      Data->State = SETTING_STATE_SYSTEM_ERROR;
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }
 
    if (Flags & DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED)
    {
      Data->ResetRequired = TRUE;
    }
    //all done. 
  } //end for loop
  
  Data->State = SETTING_STATE_DATA_APPLIED;

  //PRINT OUT XML HERE
  DEBUG((DEBUG_INFO, "PRINTING OUT XML - Start\n"));
  DebugPrintXmlTree(ResultRootNode, 0);
  DEBUG((DEBUG_INFO, "PRINTING OUTPUT XML - End\n"));

  //convert result xml node list to string
  Status = XmlTreeToString(ResultRootNode, TRUE, &(Data->ResultXmlSize), &(Data->ResultXml));
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to convert Result XML to String.  Status = %r\n", Status));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //make sure its a good size
  if (Data->ResultXmlSize > MAX_ALLOWABLE_OUTPUT_PAYLOAD_SIZE)
  {
    DEBUG((DEBUG_ERROR, "Size of result XML doc is too large (0x%X).\n", Data->ResultXmlSize));
    Status = EFI_ABORTED;
    goto EXIT;
  }

  StrLen = AsciiStrSize(Data->ResultXml);
  if (Data->ResultXmlSize != StrLen)
  {
    DEBUG((DEBUG_ERROR, "ResultXmlSize is not the correct size\n"));
  }
  DEBUG((DEBUG_INFO, "%a - ResultXmlSize = 0x%X  ResultXml String Length = 0x%X\n", __FUNCTION__, Data->ResultXmlSize, StrLen));
  Status = EFI_SUCCESS;

EXIT:
  if (InputRootNode)
  {
	  FreeXmlTree(&InputRootNode);
  }

  if (ResultRootNode)
  {
	  FreeXmlTree(&ResultRootNode);
  }
  Data->StatusCode = Status;
  return Status;
}

//
// Create the Setting Result var
//
VOID
EFIAPI
UpdateSettingsResult(
  IN SETTING_INSTANCE_DATA *Data
  )
{
  DFCI_SECURED_SETTINGS_RESULT_VAR *ResultVar = NULL;
  UINTN VarSize = 0;
  EFI_STATUS Status;
  if (Data == NULL)
  {
    return;
  }

  if (Data->State == SETTING_STATE_UNINITIALIZED)
  {
    return;
  }
  VarSize = Data->ResultXmlSize + sizeof(DFCI_SECURED_SETTINGS_RESULT_VAR);
  ResultVar = AllocatePool(VarSize);
  if (ResultVar == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Var\n", __FUNCTION__));
    return;
  }

  ResultVar->HeaderSignature = DFCI_SECURED_SETTINGS_RESULT_VAR_SIGNATURE;
  ResultVar->HeaderVersion = DFCI_SECURED_SETTINGS_VAR_VERSION;
  ResultVar->Status = Data->StatusCode;
  ResultVar->SessionId = Data->SessionId;
  ResultVar->PayloadSize = (UINT16)Data->ResultXmlSize;
  if (Data->ResultXml != NULL)
  {
    CopyMem(ResultVar->Payload, Data->ResultXml, Data->ResultXmlSize);
  }

  //save var to var store
  Status = gRT->SetVariable(XML_SETTINGS_APPLY_OUTPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, DFCI_SECURED_SETTINGS_VAR_ATTRIBUTES, VarSize, ResultVar);
  DEBUG((DEBUG_INFO, "%a - Writing Variable for Results %r\n",__FUNCTION__, Status));

  if (ResultVar)
  {
    FreePool(ResultVar);
  }

  if (!EFI_ERROR(Data->StatusCode))
  {
    Status = SMID_SaveToFlash(Data->InternalData);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Writing New Internal Data to Flash Error %r\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR(Status);
    }
  }
}

//
// Clean up the incomming variable
//
VOID
EFIAPI
FreeSettings(
  IN SETTING_INSTANCE_DATA *Data)
{
  EFI_STATUS Status;
  if (Data == NULL)
  {
    return;
  }

  if (Data->State != SETTING_STATE_UNINITIALIZED)
  {
    //delete the variable
    Status = gRT->SetVariable(XML_SETTINGS_APPLY_INPUT_VAR_NAME, &gDfciSettingsManagerVarNamespace, 0, 0, NULL);
    DEBUG((DEBUG_INFO, "Delete Xml Settings Apply Input variable %r\n", Status));
  }
}

//
// Free locally allocated memory
//  -- this function only gets called when system is not resetting. 
//
VOID
EFIAPI
FreeInstanceMemory(
  IN SETTING_INSTANCE_DATA *Data)
{
  if (Data == NULL)
  {
    return;
  }

  if (Data->Var != NULL)
  {
    FreePool(Data->Var);
  }

  if (Data->ResultXml != NULL)
  {
    FreePool(Data->ResultXml);
  }

  if (Data->InternalData != NULL)
  {
    FreePool(Data->InternalData);
  }
}


VOID
EFIAPI
CheckForPendingUpdates()
{

  EFI_STATUS Status;
  DFCI_SETTING_INTERNAL_DATA *InternalData = NULL;

  SETTING_INSTANCE_DATA InstanceData = { NULL, 0, 0, SETTING_STATE_UNINITIALIZED, NULL, EFI_SUCCESS, FALSE, DFCI_AUTH_TOKEN_INVALID, NULL, 0 };

  //check if incomming settings
  Status = GetPendingInputSettings(&InstanceData);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "No Valid Pending Input Settings\n"));
    goto CLEANUP;
  }

  //Load current internal data info
  Status = SMID_LoadFromFlash(&InternalData);
  if (EFI_ERROR(Status))
  {
    if (Status != EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to load Settings Manager Internal Data. %r\n", __FUNCTION__, Status));
    }

    //If load failed - init store 
    Status = SMID_InitInternalData(&InternalData);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Couldn't Init Settings Internal Data %r\n", __FUNCTION__, Status));
      InternalData = NULL;
      ASSERT(InternalData != NULL);
      goto CLEANUP;
    }
  }
  InstanceData.InternalData = InternalData;

  Status = ValidateAndAuthenticateSettings(&InstanceData);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Input Settings failed Authentication\n"));
    goto CLEANUP;
  }

  Status = ApplySettings(&InstanceData);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Input Settings Apply Error\n"));
    goto CLEANUP;
  }

  //clear current settings
  ClearCacheOfCurrentSettings();

CLEANUP:
  UpdateSettingsResult(&InstanceData);
  FreeSettings(&InstanceData);
  if (InstanceData.ResetRequired)
  {
    gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
  }
  AuthTokenDispose(&(InstanceData.IdentityToken));
  FreeInstanceMemory(&InstanceData);

  return;
}
