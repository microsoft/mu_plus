/** @file
DfciSettingPermissionProvisionXml

Thsi file supports the tool input path for setting permissions.  
Permissions are set using XML.  That xml is written to a variable and then passed to UEFI to be applied. 
This code supports that.  

Copyright (c) 2015, Microsoft Corporation. 

**/
#include "DfciSettingPermission.h"
#include <Guid/DfciPermissionManagerVariables.h>
#include <Library/DfciXmlPermissionSchemaSupportLib.h>
#include <Library/DfciSerialNumberSupportLib.h>


//Internal state tracking of incoming request
//Lower nibble is good status.  Upper nibble means error state.  
typedef enum {
  PERMISSION_STATE_UNINITIALIZED = 0x00,
  PERMISSION_STATE_DATA_PRESENT = 0x01,
  PERMISSION_STATE_DATA_AUTHENTICATED = 0x02,
  PERMISSION_STATE_DATA_APPLIED = 0x03,
  PERMISSION_STATE_DATA_COMPLETE = 0x0F,   //Complete
  PERMISSION_STATE_VERSION_ERROR = 0xF0,   //LSV blocked processing settings
  PERMISSION_STATE_NOT_CORRECT_TARGET = 0xFA, //Serial Number not correct
  PERMISSION_STATE_SYSTEM_ERROR = 0xFB,   //Some sort of system error blocked processing XML
  PERMISSION_STATE_BAD_XML = 0xFC,   //Bad XML data.  Didn't follow rules
  PERMISSION_STATE_DATA_INVALID = 0xFD,   //Invalid Data
  PERMISSION_STATE_DATA_AUTH_FAILED = 0xFE,
  PERMISSION_STATE_ACCESS_DENIED = 0xFF    //identity that signed doesn't have permission to update
} PERMISSION_STATE;

//Internal global object to handle incoming request
typedef struct {
  DFCI_PERMISSION_POLICY_APPLY_VAR *Var;
  UINTN                          VarSize;
  UINT32                         SessionId;
  BOOLEAN                        NewStore; 
  PERMISSION_STATE               State;
  EFI_STATUS                     StatusCode;
  DFCI_AUTH_TOKEN                  IdentityToken;
  DFCI_PERMISSION_STORE            *Store;  //this is just pointer to externally managed data..dont free it.
} PERMISSION_INSTANCE_DATA;


//
// Check to see if we have pending input
//
EFI_STATUS
EFIAPI
GetPendingInputPermission(
IN PERMISSION_INSTANCE_DATA *Data)
{
  EFI_STATUS Status;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetVariable2(DFCI_PERMISSION_POLICY_APPLY_VAR_NAME,
    &gDfciPermissionManagerVarNamespace,
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
      Data->State = PERMISSION_STATE_DATA_INVALID;
      Status = EFI_ABORTED;
      Data->StatusCode = Status;
    }
    return Status;
  }

  if (Data->VarSize > MAX_ALLOWABLE_DFCI_PERMISSION_POLICY_VAR_INPUT_SIZE)
  {
    DEBUG((DEBUG_ERROR, "%a - Incomming Setting Apply var is too big (%d bytes)\n", __FUNCTION__, Data->VarSize));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  Data->State = PERMISSION_STATE_DATA_PRESENT;
  DEBUG((DEBUG_INFO, "%a - Incomming Permission Apply var Size: 0x%X\n", __FUNCTION__, Data->VarSize));
  Data->StatusCode = Status;
  return Status;
}

/**
Function to authenticate the data and get an identity based on the xml payload and signature
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticatePermissions(
IN PERMISSION_INSTANCE_DATA *Data)
{
  UINTN SignedDataLength = 0;
  UINTN SigLen = 0;
  WIN_CERTIFICATE   *SignaturePtr;
  EFI_STATUS Status;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != PERMISSION_STATE_DATA_PRESENT)
  {
    DEBUG((DEBUG_ERROR, "%a - Wrong start state.\n", __FUNCTION__));
    Data->State = PERMISSION_STATE_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }


  //verify variable header signature
  if (Data->Var->HeaderSignature != DFCI_PERMISSION_POLICY_APPLY_VAR_SIGNATURE)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Header Signature\n", __FUNCTION__));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
    return EFI_INCOMPATIBLE_VERSION;
  }

  //Verify variable header version
  if (Data->Var->HeaderVersion != DFCI_PERMISSION_POLICY_VAR_VERSION)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Header Version.  %d\n", __FUNCTION__, Data->Var->HeaderVersion));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
    return EFI_INCOMPATIBLE_VERSION;
  }

  //Verify variable payload size vs varsize.  can't be larger. 
  if ((UINTN)(Data->Var->PayloadSize) > Data->VarSize)
  {
    DEBUG((DEBUG_ERROR, "%a - Bad Payload Size(0x%x).  Larger than VarSize.\n", __FUNCTION__, Data->Var->PayloadSize));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  //do basic size checking here.  Do enough that we can claim the pointers are valid...but don't 
  //check the WIN CERT.  Leave that to auth manager. 
  SignedDataLength = sizeof(DFCI_PERMISSION_POLICY_APPLY_VAR) + Data->Var->PayloadSize;
  DEBUG((DEBUG_INFO, "%a - SignedDataLength = 0x%X\n", __FUNCTION__, SignedDataLength));
  if (SignedDataLength > (Data->VarSize - sizeof(WIN_CERTIFICATE_UEFI_GUID)))
  {
    DEBUG((DEBUG_ERROR, "%a - SignedDataLength is too long compared to VarSize\n", __FUNCTION__));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_BAD_BUFFER_SIZE;
  }

  SignaturePtr = (WIN_CERTIFICATE *)(((UINT8*)Data->Var) + SignedDataLength); //first byte after payload
  SigLen = Data->VarSize - SignedDataLength;  //find out the max size of sig data based on var size and start of sig data.  
  if (SigLen != SignaturePtr->dwLength)
  {
    DEBUG((DEBUG_ERROR, "%a - Signature Data not expected size (0x%X) (0x%X)\n", __FUNCTION__, SigLen, SignaturePtr->dwLength));
    Data->State = PERMISSION_STATE_DATA_INVALID;
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
      Data->State = PERMISSION_STATE_SYSTEM_ERROR;
      return Data->StatusCode;
    }

    DEBUG((DEBUG_INFO, "%a - Device SN: %ld\n", __FUNCTION__, DeviceSerialNumber));

    //have serial number now compare to packet
    if (Data->Var->SerialNumber != (UINT64)DeviceSerialNumber)
    {
      DEBUG((DEBUG_ERROR, "Permission Packet not for this device.  Packet SN Target: %ld\n", Data->Var->SerialNumber));
      Data->StatusCode = EFI_ABORTED;
      Data->State = PERMISSION_STATE_NOT_CORRECT_TARGET;
      return Data->StatusCode;
    }
  }


  Status = mAuthenticationProtocol->AuthWithSignedData(mAuthenticationProtocol, (UINT8*)Data->Var, SignedDataLength, SignaturePtr, &(Data->IdentityToken));
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Authenticate Permissions Packet %r\n", __FUNCTION__, Status));
    Data->State = PERMISSION_STATE_DATA_AUTH_FAILED;  //Auth Error
    Data->StatusCode = EFI_SECURITY_VIOLATION;
    return Data->StatusCode;
  }

  Data->State = PERMISSION_STATE_DATA_AUTHENTICATED; //authenticated
  return EFI_SUCCESS;
}

//
// Apply all Permissions from XML to their associated setting providers
//
EFI_STATUS
EFIAPI
ApplyPermissionsInXml(
IN PERMISSION_INSTANCE_DATA *Data)
{
  XmlNode                   *InputRootNode = NULL;        //The root xml node for the Input list.
  XmlNode                   *InputPacketNode = NULL;      //The PermissionPacket node in the Input list
  XmlNode                   *InputPermissionsListNode = NULL;    //The Permissions node for the Input list.
  XmlNode                   *InputTempNode = NULL;        //Temp node ptr to use when moving thru the Input list

  LIST_ENTRY                  *Link = NULL;

  EFI_STATUS                  Status;
  UINTN                       StrLen = 0;
  UINTN                       Version = 0;
  UINTN                       Lsv = 0;
  UINTN                       NewLsv = 0;
  DFCI_IDENTITY_PROPERTIES      IdProps;
  BOOLEAN                     AppendToExistingPermission = FALSE;
  DFCI_PERMISSION_MASK          PMask = 0;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != PERMISSION_STATE_DATA_AUTHENTICATED)
  {
    DEBUG((DEBUG_ERROR, "%a - Wrong start state (0x%X)\n", __FUNCTION__, Data->State));
    Data->State = PERMISSION_STATE_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  //Check the auth.  Permission Updates can only be done by the Owner Identity
  Status = mAuthenticationProtocol->GetIdentityProperties(mAuthenticationProtocol, &(Data->IdentityToken), &IdProps);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Id properties using the Auth Token %r\n", __FUNCTION__, Status));
    Data->State = PERMISSION_STATE_SYSTEM_ERROR;  // Code error. this shouldn't happen.
    Data->StatusCode = EFI_ABORTED;
    return Data->StatusCode;
  }

  if ((IdProps.Identity & DFCI_IDENTITY_SIGNER_OWNER) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Permission can't be applied.  Only Owner Identity is allowed to change Permissiosn. (0x%X)\n", __FUNCTION__, IdProps.Identity));
    Data->State = PERMISSION_STATE_ACCESS_DENIED; 
    Data->StatusCode = EFI_ACCESS_DENIED;
    return Data->StatusCode;
  }

  StrLen = AsciiStrnLenS((CHAR8*)(&(Data->Var->Payload)), Data->Var->PayloadSize);
  DEBUG((DEBUG_INFO, "%a - StrLen = 0x%X PayloadSize = 0x%X\n", __FUNCTION__, StrLen, Data->Var->PayloadSize));

  //
  // Create Node List from input
  //
  Status = CreateXmlTree((CONST CHAR8 *)Data->Var->Payload, StrLen, &InputRootNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Couldn't create a node list from the payload xml  %r\n", __FUNCTION__, Status));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //print the list
  DEBUG((DEBUG_INFO, "PRINTING PERMISSION INPUT XML - Start\n"));
  DebugPrintXmlTree(InputRootNode, 0);
  DEBUG((DEBUG_INFO, "PRINTING PERMISSION INPUT XML - End\n"));

  //Get Input SettingsPacket Node
  InputPacketNode = GetPermissionPacketNode(InputRootNode);
  if (InputPacketNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input PermissionsPacket Node\n"));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  //
  //Get input version
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, PERMISSIONS_VERSION_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Version Node\n"));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming Version: %a\n", InputTempNode->Value));
  Version = AsciiStrDecimalToUintn(InputTempNode->Value);

  //
  // Compare against save LSV
  //
  if (Version < Data->Store->Lsv)
  {
    DEBUG((DEBUG_INFO, "%a - Incomming Permission Packet Has Lower Version (0x%X) than allowed LSV (0x%X). Can't apply\n", __FUNCTION__, Version, Data->Store->Lsv));
    Data->State = PERMISSION_STATE_VERSION_ERROR;
    Status = EFI_ACCESS_DENIED;
    goto EXIT;
  }

  //
  //Get Incomming LSV
  //
  InputTempNode = FindFirstChildNodeByName(InputPacketNode, PERMISSIONS_LSV_ELEMENT_NAME);
  if (InputTempNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get LSV Node\n"));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }
  DEBUG((DEBUG_INFO, "Incomming LSV: %a\n", InputTempNode->Value));
  Lsv = AsciiStrDecimalToUintn(InputTempNode->Value);

  if (Lsv > Version)
  {
    DEBUG((DEBUG_ERROR, "%a - LSV (%a) can't be larger than current version\n", __FUNCTION__, InputTempNode->Value));
    Data->State = PERMISSION_STATE_DATA_INVALID;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  NewLsv = MAX(Data->Store->Lsv, Lsv);

  // Get the Xml Node for the PermissionsList
  InputPermissionsListNode = GetPermissionsListNodeFromPacketNode(InputPacketNode);

  if (InputPermissionsListNode == NULL)
  {
    DEBUG((DEBUG_INFO, "Failed to Get Input Permissions List Node\n"));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  //if request is to replace then initialize a new perm store
  Status = PermissionListEntriesAppend(InputPermissionsListNode, &AppendToExistingPermission);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to get Required Append Attribute in Permission XML.  Bad XML Data. %r\n", Status));
    Data->State = PERMISSION_STATE_BAD_XML;
    Status = EFI_NO_MAPPING;
    goto EXIT;
  }

  if (!AppendToExistingPermission)
  {
    //incomming validated policy is requesting
    //to replace rather than append. 
    DFCI_PERMISSION_STORE *TempStore = NULL; //temp to hold new store
    Status = InitPermStore(&TempStore);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to Init New Perm Store %r\n", __FUNCTION__, Status));
      Data->State = PERMISSION_STATE_SYSTEM_ERROR;
      Status = EFI_ABORTED;
      goto EXIT;
    }
    //Initialized new store...    
    Data->Store = TempStore;
    Data->NewStore = TRUE;
  }

  Data->Store->Lsv = (UINT32)NewLsv;  //Update LSV 
  Data->Store->Version = (UINT32)Version;  //Update Version

  //Handle Default Mask if set
  Status = GetPermissionsListDefaultPMask(InputPermissionsListNode, &PMask);
  if (EFI_ERROR(Status))
  {
    if (Status == EFI_NOT_FOUND)
    {
      //this is ok.  New Permission Xml doesn't have default
      DEBUG((DEBUG_INFO, "%a - New Permissions doesn't define a default\n", __FUNCTION__));
    }
    else
    {
      DEBUG((DEBUG_INFO, "%a - Error while trying to get default entry %r\n", __FUNCTION__, Status));
      Data->State = PERMISSION_STATE_BAD_XML;
      Status = EFI_NO_MAPPING;
      goto EXIT;
    }
  }
  else
  {  //have a good mask value
    Data->Store->Default = PMask;
  }
  

  //All verified.   Now lets walk thru the Permission Entries and add them to our Permission List.  
  for (Link = InputPermissionsListNode->ChildrenListHead.ForwardLink; Link != &(InputPermissionsListNode->ChildrenListHead); Link = Link->ForwardLink)
  {
    XmlNode *NodeThis = NULL;
    DFCI_SETTING_ID_ENUM Id = 0;
    DFCI_PERMISSION_MASK  Mask = 0;  
    DFCI_PERMISSION_ENTRY *Entry = NULL;

   
    NodeThis = (XmlNode*)Link;   //Link is first member so just cast it.  this is the <Setting> node
    Status = GetInputPermission(NodeThis, &Id, &Mask);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "Failed to Get Input Permission.  Bad XML Data. %r\n", Status));
      Data->State = PERMISSION_STATE_BAD_XML;
      Status = EFI_NO_MAPPING;
      goto EXIT;
    }

    DEBUG((DEBUG_INFO, "%a - Setting Permission for ID %d to 0x%X\n", __FUNCTION__, Id, Mask));

    //Check if it already exists
    Entry = FindPermissionEntry(Data->Store, Id);
    if (Entry)
    {
      //Exists.  just update the mask
      Entry->Perm = Mask;
    }
    else
    {
      //Doesn't exist.  Add new
      Status = AddPermissionEntry(Data->Store, Id, Mask);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "%a - Failed to Add Entry to Perm Store %r\n", __FUNCTION__, Status));
        Data->State = PERMISSION_STATE_SYSTEM_ERROR;
        Status = EFI_ABORTED;
        goto EXIT;
      }
    }
  } //end for loop

  Data->State = PERMISSION_STATE_DATA_APPLIED;

  //PRINT OUT PERMISSION STORE HERE
  DEBUG((DEBUG_INFO, "PRINTING OUT Permission Store\n"));
  DebugPrintPermissionStore(Data->Store);

  Status = EFI_SUCCESS;

EXIT:
  if (InputRootNode)
  {
    FreeXmlTree(&InputRootNode);
  }

  Data->StatusCode = Status;
  return Status;
}

//
// Create the Permission Result var
//
VOID
EFIAPI
UpdatePermissionResult(
IN PERMISSION_INSTANCE_DATA *Data
)
{
  DFCI_PERMISSION_POLICY_RESULT_VAR *ResultVar = NULL;
  UINTN VarSize = 0;
  EFI_STATUS Status;
  if (Data == NULL)
  {
    return;
  }

  if (Data->State == PERMISSION_STATE_UNINITIALIZED)
  {
    return;
  }

  VarSize = sizeof(DFCI_PERMISSION_POLICY_RESULT_VAR);
  ResultVar = AllocatePool(VarSize);
  if (ResultVar == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to allocate memory for Var\n", __FUNCTION__));
    return;
  }

  ResultVar->HeaderSignature = DFCI_PERMISSION_POLICY_RESULT_VAR_SIGNATURE;
  ResultVar->HeaderVersion = DFCI_PERMISSION_POLICY_VAR_VERSION;
  ResultVar->Status = Data->StatusCode;
  ResultVar->SessionId = Data->SessionId;
  
  //save var to var store
  Status = gRT->SetVariable(DFCI_PERMISSION_POLICY_RESULT_VAR_NAME, &gDfciPermissionManagerVarNamespace, DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_ATTRIBUTES, VarSize, ResultVar);
  DEBUG((DEBUG_INFO, "%a - Writing Variable for Results %r\n", __FUNCTION__, Status));

  if (ResultVar)
  {
    FreePool(ResultVar);
  }

}

//
// Clean up the incomming variable
//
VOID
EFIAPI
FreeNvVarsForIncommingPermissions(
IN PERMISSION_INSTANCE_DATA *Data)
{
  EFI_STATUS Status;
  if (Data == NULL)
  {
    return;
  }

  if (Data->State != PERMISSION_STATE_UNINITIALIZED)
  {
    //delete the variable
    Status = gRT->SetVariable(DFCI_PERMISSION_POLICY_APPLY_VAR_NAME, &gDfciPermissionManagerVarNamespace, 0, 0, NULL);
    DEBUG((DEBUG_INFO, "Delete Xml Permission Apply Input variable %r\n", Status));
  }
}

//
// Free locally allocated memory
//  -- this function only gets called when system is not resetting. 
//
VOID
EFIAPI
FreePermissionInstanceMemory(
IN PERMISSION_INSTANCE_DATA *Data)
{
  if (Data == NULL)
  {
    return;
  }

  if (Data->Var != NULL)
  {
    FreePool(Data->Var);
    Data->Var = NULL;
    Data->VarSize = 0;
  }
  //don't free the Store.
}


/**
  Main Entry point into the Xml Provisioning code. 
  This will check the incomming variables, authenticate them, and apply permission settings.
**/
VOID
EFIAPI
CheckForPendingPermissionChanges()
{

  EFI_STATUS Status;

  PERMISSION_INSTANCE_DATA InstanceData = { NULL, 0, 0, FALSE, PERMISSION_STATE_UNINITIALIZED, EFI_SUCCESS, DFCI_AUTH_TOKEN_INVALID, NULL };
  InstanceData.Store = mPermStore;

  if (mAuthenticationProtocol == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Trying to access Auth Protocol too early.\n", __FUNCTION__));
    return;
  }

  //check if incomming settings
  Status = GetPendingInputPermission(&InstanceData);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_INFO, "No Valid Pending Input Permissions\n"));
    goto CLEANUP;
  }

  Status = ValidateAndAuthenticatePermissions(&InstanceData);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Input Permission failed Authentication\n"));
    goto CLEANUP;
  }

  Status = ApplyPermissionsInXml(&InstanceData);
  if (EFI_ERROR(Status))
  {  
      DEBUG((DEBUG_ERROR, "Input Permissions Apply Error\n"));

    if (!InstanceData.NewStore)
    { //Since the apply was modifiying the store it must be reloaded. 
      FreePermissionStore(mPermStore);
      LoadFromFlash(&mPermStore);
    }
    else 
    {
       //Free the newly created Perm Store.
        FreePermissionStore(InstanceData.Store);
        InstanceData.Store = NULL;
     }
    
    goto CLEANUP;
  }
  
  //NO Errors. 
  //all completed successfully.
  if (InstanceData.NewStore)
  { //Free old store and point to new perm store
    FreePermissionStore(mPermStore);
    mPermStore = InstanceData.Store;  //set global to new one
  }

  mPermStore->Modified = TRUE;
  SaveToFlash(mPermStore); //save it
CLEANUP:    
  UpdatePermissionResult(&InstanceData);
  FreeNvVarsForIncommingPermissions(&InstanceData);
  if (InstanceData.IdentityToken != DFCI_AUTH_TOKEN_INVALID)
  {
    mAuthenticationProtocol->DisposeAuthToken(mAuthenticationProtocol, &(InstanceData.IdentityToken));
  }
  FreePermissionInstanceMemory(&InstanceData);

  return;
}
