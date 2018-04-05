
#include "IdentityAndAuthManager.h"
#include <Guid/EventGroup.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/ZeroGuid.h>
#include <Private/DfciGlobalPrivate.h>
#include <Library/BaseLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Settings/DfciSettings.h>

EFI_EVENT                          mWMProtocolEvent;


/**
Event callback for Window Manager Protocol.
This is needed when a privisioning request that requires
user confirmation.
**/
VOID
EFIAPI
WindowManagerCallback (
  IN EFI_EVENT Event,
  IN VOID* Context
  )
{

  EFI_TPL OldTpl;

  //
  // Check if the UI components we need are available. If not, bail.
  //
  if (DfciUiIsUiAvailable() == FALSE) {
    DEBUG((DEBUG_ERROR, "%a - Callback trigggered. UI not available\n", __FUNCTION__));
    return;
  }

  //
  // Try again to process provisioning input.
  //
  OldTpl = gBS->RaiseTPL(TPL_NOTIFY);
  gBS->RestoreTPL(TPL_APPLICATION);
  CheckForNewProvisionInput();
  gBS->RaiseTPL(OldTpl);
  gBS->CloseEvent(Event);
}


/**
 Convert the Identity values used in the Provisioning Variable
 to the Identity values used by the Authentication Manager
**/
DFCI_IDENTITY_ID
VarIdentityToDfciIdentity(
  IN UINT8 VarIdentity
  )
{
  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER)
  {
    return DFCI_IDENTITY_SIGNER_USER;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_OWNER)
  {
    return DFCI_IDENTITY_SIGNER_OWNER;
  }


  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER1)
  {
    return DFCI_IDENTITY_SIGNER_USER1;
  }

  if (VarIdentity == DFCI_SIGNER_PROVISION_IDENTITY_USER2)
  {
    return DFCI_IDENTITY_SIGNER_USER2;
  }

  return DFCI_IDENTITY_INVALID;
}

UINT8
DfciIdentityToVarIdentity(
  IN DFCI_IDENTITY_ID DfciIdentity)
{
  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER)
  {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_OWNER)
  {
    return DFCI_SIGNER_PROVISION_IDENTITY_OWNER;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER1)
  {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER1;
  }

  if (DfciIdentity == DFCI_IDENTITY_SIGNER_USER2)
  {
    return DFCI_SIGNER_PROVISION_IDENTITY_USER2;
  }

  return DFCI_SIGNER_PROVISION_IDENTITY_INVALID;
}

DFCI_SETTING_ID_STRING
DfciIdentityToSettingId(
  IN DFCI_IDENTITY_ID Identity)
{
  if (Identity == DFCI_IDENTITY_SIGNER_USER)
  {
    return DFCI_SETTING_ID__USER_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_OWNER)
  {
    return DFCI_SETTING_ID__OWNER_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_USER1)
  {
    return DFCI_SETTING_ID__USER1_KEY;
  }

  if (Identity == DFCI_IDENTITY_SIGNER_USER2)
  {
    return DFCI_SETTING_ID__USER2_KEY;
  }

  return NULL;
}

/**
Write the provisioning response variable with parameter info
**/
EFI_STATUS
SetProvisionResponse(
  IN AUTH_MAN_PROV_INSTANCE_DATA* Data
  )
{
  DFCI_SIGNER_PROVISION_RESULT_VAR Var;
  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  //Don't want to write a status when we didn't have any data
  //
  if (Data->State == AUTH_MAN_PROV_STATE_UNINITIALIZED)
  {
    return EFI_SUCCESS;
  }

  //If user confirmation pending..don't write status as this should be run again once user input is enabled
  if (Data->State == AUTH_MAN_PROV_STATE_DATA_DELAYED_PROCESSING)
  {
    return EFI_SUCCESS;
  }

  Var.HeaderSignature = DFCI_IDENTITY_AUTH_PROVISION_RESULT_VAR_SIGNATURE;
  Var.HeaderVersion = DFCI_IDENTITY_AUTH_PROVISION_RESULT_VERSION;
  Var.Identity = DfciIdentityToVarIdentity(Data->Identity);
  DEBUG((DEBUG_INFO, "%a - Set Result Var Identity 0x%X.  DFCI Identity 0x%X\n", __FUNCTION__, Var.Identity, Data->Identity));
  Var.StatusCode = (UINT64)(Data->StatusCode);
  Var.SessionId = Data->SessionId;

  return gRT->SetVariable(DFCI_IDENTITY_AUTH_PROVISION_SIGNER_RESULT_VAR_NAME, &gDfciAuthProvisionVarNamespace, DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_ATTRIBUTES, sizeof(DFCI_SIGNER_PROVISION_RESULT_VAR), &Var);
}


EFI_STATUS
EFIAPI
GetPendingProvisionData(
  IN AUTH_MAN_PROV_INSTANCE_DATA* Data
  )
{
  EFI_STATUS Status;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  //Get the variable
  Status = GetVariable3(DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_NAME,
    &gDfciAuthProvisionVarNamespace,
    &Data->Var,
    &Data->VarSize,
    NULL
    );

  if (EFI_ERROR(Status))
  {
    if (Status == EFI_NOT_FOUND)
    {
      DEBUG((DEBUG_INFO, "Auth Manager - No Pending Provision Data.\n"));
    }
    else
    {
      DEBUG((DEBUG_ERROR, "%a - Error getting variable - %r\n", __FUNCTION__, Status));
      Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
      Data->StatusCode = Status;
    }
    return Status;
  }

  //Check incomming size
  if (Data->VarSize > MAX_ALLOWABLE_DFCI_IDENTITY_AUTH_PROVISION_APPLY_VAR_SIZE)
  {
    DEBUG((DEBUG_ERROR, "%a - Incomming Provision apply var is too big (%d bytes)\n", __FUNCTION__, Data->VarSize));
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    Data->StatusCode = EFI_BAD_BUFFER_SIZE;
    return EFI_NOT_FOUND;
  }

  Data->State = AUTH_MAN_PROV_STATE_DATA_PRESENT;
  DEBUG((DEBUG_INFO, "%a - Provision Variable Size: 0x%X\n", __FUNCTION__, Data->VarSize));
  return Status;
}

/**
 * CHeck signature of the packet
 *
 * @param Data
 * @param SignedDataLength
 *
 * @return EFI_STATUS
 */
EFI_STATUS
SignChecks (
    IN       AUTH_MAN_PROV_INSTANCE_DATA       *Data,
    IN CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *SettingPermissionProtocol,
    UINTN                                       SignedDataLength
  )
{
  EFI_STATUS                      Status;
  DFCI_PERMISSION_MASK            PermMask = 0;
  DFCI_IDENTITY_PROPERTIES        Properties;
  WIN_CERTIFICATE                *Signature = NULL;  //Ptr to UEFI cert structure
  WIN_CERTIFICATE                *TestSignature = NULL;

  Data->Identity = VarIdentityToDfciIdentity(Data->VarIdentity);  //Set the Identity

  //Lets check that the auth packet is either 1. Owner Identity or if not owner identity then an Owner already exists.
  //Can't provision User Key without Owner key already provisioned.
  if ((Data->VarIdentity != DFCI_SIGNER_PROVISION_IDENTITY_OWNER) && (!(Provisioned() & DFCI_IDENTITY_SIGNER_OWNER)))
  {
    DEBUG((DEBUG_ERROR, "[AM] - Can't provision User Auth Packet when Owner auth isn't already provisioned.\n"));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State = AUTH_MAN_PROV_STATE_DATA_NO_OWNER;
    return Data->StatusCode;
  }

  //Lets check that if its an unenroll packet that this system is DFCI enrolled.
  if ((Data->TrustedCertSize == 0) && (!(Provisioned() & DFCI_IDENTITY_SIGNER_OWNER)))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - Can't un-enroll a device that isn't enrolled in DFCI (no owner).\n", __FUNCTION__));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State = AUTH_MAN_PROV_STATE_DATA_NO_OWNER;
    return Data->StatusCode;
  }

  //Lets check the test signature
  // - this is to confirm new Cert Data (Trusted Cert) isn't in bad format (user/tool error) which would cause future validation errors and possible "brick"
  // - This is not present when this is a unenroll request (no New Trusted Cert)
  //
  if (Data->TrustedCertSize > 0)
  {
    if (Data->VarSize <= (SignedDataLength + sizeof(WIN_CERTIFICATE)))
    {
      //Invalid....Where the signature data???
      DEBUG((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold any signature data\n", __FUNCTION__));
      Data->StatusCode = EFI_COMPROMISED_DATA;
      Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
      return Data->StatusCode;
    }
    //Now we can check if we have a WIN_CERT
    TestSignature = (WIN_CERTIFICATE*)(((UINT8*)(Data->Var)) + SignedDataLength);
    //check Test Signature length
    if ((TestSignature->dwLength + SignedDataLength) > Data->VarSize)
    {
      //Invalid....Where the signature data???
      DEBUG((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold the declared test signature data\n", __FUNCTION__));
      Data->StatusCode = EFI_COMPROMISED_DATA;
      Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
      return Data->StatusCode;
    }

    //Check the test signature
    Status = VerifySignature(PKT_FIELD_FROM_OFFSET(Data->Var,Data->TrustedCertOffset), Data->TrustedCertSize, TestSignature, PKT_FIELD_FROM_OFFSET(Data->Var,Data->TrustedCertOffset), Data->TrustedCertSize);
    if (EFI_ERROR(Status))
    {
      //Test Signature Fails Validation
      DEBUG((DEBUG_ERROR, "[AM] %a - Test Signature Failed Validation.  %r\n", __FUNCTION__, Status));
      Data->StatusCode = EFI_CRC_ERROR;  //special return code for this case.  Probably should create a new status code
      Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
      return Data->StatusCode;
    }

    DEBUG((DEBUG_INFO, "[AM] Test Signature passed Validation.\n"));
    SignedDataLength += TestSignature->dwLength;  //Update the SignedDataLength based on valid Signature Length
  }

  //Check Signed Data length vs variable Length
  DEBUG((DEBUG_INFO, "[AM] %a - SignedDataLength = 0x%X\n", __FUNCTION__, SignedDataLength));
  if ((SignedDataLength + sizeof(WIN_CERTIFICATE_UEFI_GUID)) >= Data->VarSize)
  {
    //Where is the cert data?
    DEBUG((DEBUG_ERROR, "[AM] %a - Variable isn't big enough to hold the declared var signature data\n", __FUNCTION__));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }
  //Check the Identity to make sure it's supported
  if (Data->Identity == DFCI_IDENTITY_INVALID)
  {
    DEBUG((DEBUG_ERROR, "%a - Identity is not supported 0x%X\n", __FUNCTION__, Data->Identity));
    Data->StatusCode = EFI_UNSUPPORTED;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  //Get Permissions for this provisioned data
  Status = SettingPermissionProtocol->GetPermission(SettingPermissionProtocol, DfciIdentityToSettingId(Data->Identity), &PermMask);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Permission for Identity 0x%X.  Status = %r\n", __FUNCTION__, Data->Identity, Status));
    Data->StatusCode = Status;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  DEBUG((DEBUG_INFO, "%a - Permission for this Key is 0x%X\n", __FUNCTION__, PermMask));

  Signature = (WIN_CERTIFICATE*)(((UINT8*)Data->Var + SignedDataLength));

  //Check to make sure Signature is contained within Var Data
  if (Data->VarSize != (Signature->dwLength + SignedDataLength))
  {
    //Var Length isn't correct
    DEBUG((DEBUG_ERROR, "[AM] %a - Variable Size (0x%X) doesn't match calculated size (0x%X)\n", __FUNCTION__, Data->VarSize, (Signature->dwLength + SignedDataLength)));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  //ALL WIN_CERT Support and Verification is handled in the Auth Protocol

  //Now lets ask the auth manager to verify
  Status = AuthWithSignedData(&mAuthProtocol,
    (UINT8*)Data->Var,        //signed data ptr
    SignedDataLength, //signed data length
    Signature,       //Win Cert ptr
    &(Data->AuthToken));

  if (!EFI_ERROR(Status))
  {
    //Success.  now get Identity
    Status = GetIdentityProperties(&mAuthProtocol, &(Data->AuthToken), &Properties);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_INFO, "%a - Auth Passed but Identity failed. Should never happen. %r\n", Status));
      Data->StatusCode = EFI_ABORTED;
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_FAILED;
      return Data->StatusCode;
    }

    //Auth is valid.  Now check if auth has permission
    if ((Properties.Identity & PermMask) != 0)
    {
      //Permission is good.  Apply
      DEBUG((DEBUG_INFO, "%a - Permission is good. Applying without requiring user interaction.\n", __FUNCTION__));
      Data->UserConfirmationRequired = FALSE;
      Data->StatusCode = EFI_SUCCESS;
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTHENTICATED;
      return Data->StatusCode;
    }

    //Auth was good but Permission wasn't
    DEBUG((DEBUG_INFO, "%a - Auth Good but Permission not set for this identity\n", __FUNCTION__));
  }

  //Auth wasn't good enough
  DEBUG((DEBUG_INFO, "%a - Crypto Supplied Auth wasn't enough.\n", __FUNCTION__));
  if ((PermMask & DFCI_IDENTITY_LOCAL) != 0)
  {
    DEBUG((DEBUG_INFO, "%a - Local User Auth allowed.  Will prompt for User approval.\n", __FUNCTION__));
    Data->UserConfirmationRequired = TRUE;
    Data->StatusCode = EFI_SUCCESS;
    Data->State = AUTH_MAN_PROV_STATE_DATA_AUTHENTICATED;
    return Data->StatusCode;
  }

  //UNKNOWN ERROR
  //FAIL - Unsupported Identity
  DEBUG((DEBUG_INFO, "%a - Unsupported Key Provision\n", __FUNCTION__));
  Data->StatusCode = EFI_ACCESS_DENIED;
  Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_FAILED;
  return Data->StatusCode;
}

/**
Check for provision data variable is valid and then check authentication if required.
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticatePendingProvisionData_V1 (
  IN AUTH_MAN_PROV_INSTANCE_DATA             *Data,
  IN CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *SettingPermissionProtocol
  )
{
  UINTN SignedDataLength = 0;    //length of the hashed data for signature validation
  EFI_STATUS Status;

  //Save the session id
  Data->SessionId = Data->Var->v1.SessionId;
  Data->Var->v1.SessionId = 0;  //zero out the var data so signature validation works

  //Save the Identity early in case of error so response packet has valid response
  Data->VarIdentity = Data->Var->v1.Identity;

  Data->TrustedCertOffset = OFFSET_OF(DFCI_SIGNER_PROVISION_APPLY_VAR_V1, TrustedCert);
  Data->TrustedCertSize = Data->Var->v1.TrustedCertSize;
  Data->TrustedCert = Data->Var->v1.TrustedCert;

  //Lets check for device specific targetting using Serial Number
  if (Data->Var->v1.SerialNumber != 0)
  {
    UINTN DeviceSerialNumber = 0;

    DEBUG((DEBUG_INFO, "[AM] %a - Targetted Packet for sn %ld\n", __FUNCTION__, Data->Var->v1.SerialNumber));
    Status = DfciSupportGetDeviceId (&Data->DeviceId);

    if (EFI_ERROR(Status))
    {
      Data->StatusCode = EFI_ABORTED;
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
      return Data->StatusCode;
    }

    Status = GetSerialNumber(&DeviceSerialNumber);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "[AM] - Failed to get device serial number %r\n", Status));
      Data->StatusCode = EFI_OUT_OF_RESOURCES;
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
      return Data->StatusCode;
    }

    DEBUG((DEBUG_INFO, "[AM] %a - Device SN: %ld\n", __FUNCTION__, DeviceSerialNumber));

    //have serial number now compare to packet
    if (Data->Var->v1.SerialNumber != (UINT64)DeviceSerialNumber)
    {
      DEBUG((DEBUG_ERROR, "[AM] - Auth Packet not for this device.  Packet SN Target: %ld\n", Data->Var->v1.SerialNumber));
      Data->StatusCode = EFI_ABORTED;
      Data->State = AUTH_MAN_PROV_STATE_DATA_NOT_CORRECT_TARGET;
      return Data->StatusCode;
    }
  }

  SignedDataLength = sizeof(DFCI_SIGNER_PROVISION_APPLY_VAR_V1) + Data->Var->v1.TrustedCertSize;  //SignedData will be at least this big.

  Status = SignChecks (Data, SettingPermissionProtocol, SignedDataLength);

  return Status;
}

/**
Check for provision data variable is valid and then check authentication if required.
**/
EFI_STATUS
EFIAPI
ValidateAndAuthenticatePendingProvisionData_V2 (
  IN       AUTH_MAN_PROV_INSTANCE_DATA       *Data,
  IN CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *SettingPermissionProtocol
  )
{
  UINTN                      SignedDataLength = 0;    //length of the hashed data for signature validation
  EFI_STATUS                 Status;
  EFI_GUID                   SystemUuid;
  DFCI_DEVICE_ID_ELEMENTS   *DeviceId;

  //Save the session id
  Data->SessionId = Data->Var->v2.SessionId;
  Data->Var->v2.SessionId = 0;  //zero out the var data so signature validation works

  //Save the Identity early in case of error so response packet has valid response
  Data->VarIdentity = Data->Var->v2.Identity;

  Data->TrustedCertOffset = Data->Var->v2.TrustedCertOffset;
  Data->TrustedCertSize = Data->Var->v2.TrustedCertSize;
  Data->TrustedCert = PKT_FIELD_FROM_OFFSET(Data->Var,Data->TrustedCertOffset);

  // Check if the packet is for this DeviceId.  For V2, must be an exact match for all componentes of
  // the device Id.

  Status = DfciSupportGetDeviceId ( &DeviceId );
  if (EFI_ERROR(Status))
  {
    Data->StatusCode = EFI_ABORTED;
    Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
    return Data->StatusCode;
  }

  CopyGuid (&SystemUuid, &gZeroGuid);  // Insure ZeroGuid if no string guid
  Status = AsciiStrToGuid (DeviceId->Uuid, &SystemUuid);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] %a - Error convertion Uuid to Guid. Ignored. %r\n", __FUNCTION__, Status));
  }

  DEBUG((DEBUG_ERROR, "[AM] %a - Current -- Target\n", __FUNCTION__));
  DEBUG((DEBUG_ERROR, "Mfg  %a - %a\n",  DeviceId->Manufacturer, PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemMfgOffset)));
  DEBUG((DEBUG_ERROR, "Pn   %a - %a\n",  DeviceId->ProductName,  PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemProductOffset)));
  DEBUG((DEBUG_ERROR, "Sn   %a - %a\n",  DeviceId->SerialNumber, PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemSerialOffset)));
  DEBUG((DEBUG_ERROR, "Uuid %g - %g\n",  &SystemUuid, Data->Var->v2.SystemUuid));
  if ((0 != CompareMem (DeviceId->Manufacturer, PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemMfgOffset),     DeviceId->ManufacturerSize)) ||
      (0 != CompareMem (DeviceId->ProductName,  PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemProductOffset), DeviceId->ProductNameSize )) ||
      (0 != CompareMem (DeviceId->SerialNumber, PKT_FIELD_FROM_OFFSET(Data->Var,Data->Var->v2.SystemSerialOffset),  DeviceId->SerialNumberSize)) ||
      (!CompareGuid (&SystemUuid, &Data->Var->v2.SystemUuid)))
  {
    Data->StatusCode = EFI_ABORTED;
    Data->State = AUTH_MAN_PROV_STATE_DATA_NOT_CORRECT_TARGET;
    return Data->StatusCode;
  }

  SignedDataLength = Data->TrustedCertOffset + Data->TrustedCertSize;  //SignedData will be this big.

  Status = SignChecks (Data, SettingPermissionProtocol, SignedDataLength);

  return Status;
}

/**
 * Perform basic checks on packet
 *
 * @param Data
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ValidateAndAuthenticatePendingProvisionData (
    IN       AUTH_MAN_PROV_INSTANCE_DATA       *Data,
    IN CONST DFCI_SETTING_PERMISSIONS_PROTOCOL *SettingPermissionProtocol
  )
{
  EFI_STATUS  Status;
  UINTN       MinSize;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != AUTH_MAN_PROV_STATE_DATA_PRESENT)
  {
    DEBUG((DEBUG_ERROR, "%a called with data in wrong state 0x%x\n", __FUNCTION__, Data->State));
    return EFI_UNSUPPORTED;
  }

  if ((Data->Var == NULL) || (Data->VarSize == 0))
  {
    Data->StatusCode = EFI_INVALID_PARAMETER;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  MinSize = sizeof(DFCI_SIGNER_PROVISION_APPLY_VAR_V1);
  if ( (Data->VarSize > sizeof(DFCI_SIGNER_PROVISION_APPLY_VAR_HEADER)) &&
       (Data->Var->vh.HeaderVersion == DFCI_IDENTITY_AUTH_PROVISION_VAR_VERSION_V2) )
  {
    MinSize = sizeof(DFCI_SIGNER_PROVISION_APPLY_VAR_V2);
  }

  if (Data->VarSize < MinSize)
  {
    DEBUG((DEBUG_ERROR, "[AM] Auth Provision VarSize too small. Size: 0x%X MinSize: 0x%X\n", Data->VarSize, MinSize));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }

  //Check ascii signature to make sure var looks as expected.
  if (Data->Var->vh.HeaderSignature != DFCI_IDENTITY_AUTH_PROVISION_APPLY_VAR_SIGNATURE)
  {
    DEBUG((DEBUG_ERROR, "[AM] Auth Provision Var HeaderSignature not valid.\n"));
    Data->StatusCode = EFI_COMPROMISED_DATA;
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    return Data->StatusCode;
  }
  //Check Version

  switch (Data->Var->vh.HeaderVersion)
  {
    case DFCI_IDENTITY_AUTH_PROVISION_VAR_VERSION_V1:
      Status = ValidateAndAuthenticatePendingProvisionData_V1 (Data, SettingPermissionProtocol);
      break;
    case DFCI_IDENTITY_AUTH_PROVISION_VAR_VERSION_V2:
      Status = ValidateAndAuthenticatePendingProvisionData_V2 (Data, SettingPermissionProtocol);
      break;
    default:
      DEBUG((DEBUG_INFO, "[AM] Auth Provision Var Version not current.\n"));
      Data->StatusCode = EFI_INCOMPATIBLE_VERSION;
      Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
      return Data->StatusCode;
    break;
  }

  return Status;
}

/**
Function sets the new data into the Internal Cert Store and save to NV ram
**/
EFI_STATUS
EFIAPI
ApplyProvisionData(
  IN AUTH_MAN_PROV_INSTANCE_DATA* Data
  )
{
  UINT8 Index = CERT_INVALID_INDEX;
  UINT8*      NewCertData = NULL;
  EFI_STATUS Status;

  if (Data == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->State != AUTH_MAN_PROV_STATE_DATA_USER_APPROVED)
  {
    DEBUG((DEBUG_ERROR, "ApplyProvisionData called with data in wrong state 0x%x\n", Data->State));
    return EFI_UNSUPPORTED;
  }

  DEBUG((DEBUG_INFO, "Applying Provision Data for Identity %d\n", Data->Identity));

  // Special case for when a user is unenrolling in DFCI - which is done by removing owner key
  if ((Data->TrustedCertSize == 0) && (Data->Identity == DFCI_IDENTITY_SIGNER_OWNER))
  {
    Status = ClearDFCI(&(Data->AuthToken));

    Data->RebootRequired = TRUE;  //After clear force reboot...even in error case
    Data->StatusCode = Status;
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "[AM] - Failed to Clear DFCI.  System in bad state. %r\n", Status));
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
      ASSERT_EFI_ERROR(Status);
      return Status;
    }
    Data->State = AUTH_MAN_PROV_STATE_DATA_COMPLETE;
    return Status;
  }

  
  Index = DfciIdentityToCertIndex(Data->Identity);
  if (Index == CERT_INVALID_INDEX)
  {
    DEBUG((DEBUG_INFO, "Invalid Cert Index\n"));
    Data->State = AUTH_MAN_PROV_STATE_DATA_INVALID;
    Data->StatusCode = EFI_UNSUPPORTED;
    return Data->StatusCode;
  }

  //Only allocate new memory if this request has new cert data
  if (Data->TrustedCertSize > 0)
  {
    //allocate new data just in case of error we will not delete old yet
    NewCertData = AllocatePool(Data->TrustedCertSize);
    if (NewCertData == NULL)
    {
      Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
      Data->StatusCode = EFI_OUT_OF_RESOURCES;
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //Remove old if present
  if (mInternalCertStore.Certs[Index].Cert != NULL)
  {
    FreePool(mInternalCertStore.Certs[Index].Cert);
    mInternalCertStore.Certs[Index].Cert = NULL;
    mInternalCertStore.Certs[Index].CertSize = 0;

    mInternalCertStore.PopulatedIdentities &= ~(Data->Identity); //unset the PopulatedIdentities
    //Destroy any auth handle that is using the old Identity
  }

  //Dont try to copy if it was a delete operation
  if (Data->TrustedCertSize > 0)
  {
    mInternalCertStore.Certs[Index].Cert = NewCertData;
    mInternalCertStore.Certs[Index].CertSize = Data->TrustedCertSize;

    CopyMem(mInternalCertStore.Certs[Index].Cert, PKT_FIELD_FROM_OFFSET(Data->Var,Data->TrustedCertOffset), mInternalCertStore.Certs[Index].CertSize);
    mInternalCertStore.PopulatedIdentities |= (Data->Identity);  //Set the populatedIdentities
  }

  //Dispose of all mappings for the Identity that changed
  Status = DisposeAllIdentityMappings(Data->Identity);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - Failed to dispose of identites for Id 0x%X.  Status = %r\n", Data->Identity, Status));
    //continue on.  
  }

  //Save it
  Status = SaveProvisionedData();
  if (EFI_ERROR(Status))
  {
    Data->StatusCode = Status;
    Data->State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
    if (NewCertData != NULL)
    {
      FreePool(NewCertData);
    }
    return Status;
  }

  Data->StatusCode = EFI_SUCCESS;
  Data->State = AUTH_MAN_PROV_STATE_DATA_COMPLETE;
  return EFI_SUCCESS;
}


/**
Delete the variable for NV space
**/
VOID
DeleteProvisionVariable(
  IN AUTH_MAN_PROV_INSTANCE_DATA* Data)
{
  if ((Data == NULL) || (Data->State == AUTH_MAN_PROV_STATE_UNINITIALIZED))
  {
    return;
  }

  if (Data->State == AUTH_MAN_PROV_STATE_DATA_DELAYED_PROCESSING)
  {
    //don't delete the variable since we should come and try again later
    return;
  }

  gRT->SetVariable(DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_NAME, &gDfciAuthProvisionVarNamespace, 0, 0, NULL);
}

/*
Check for incoming provisioning request for User Cert or Owner Cert
Provision request could be Provision or Change/Delete.

*/
VOID
CheckForNewProvisionInput (
  VOID
  )
{

  AUTH_MAN_PROV_INSTANCE_DATA Data;
  BOOLEAN LocalAuthNeeded;
  EFI_STATUS Status;
  BOOLEAN Unenroll;

  Data.Var = NULL;
  Data.VarSize = 0;
  Data.State = AUTH_MAN_PROV_STATE_UNINITIALIZED;
  Data.StatusCode = 0;
  Data.Identity = DFCI_IDENTITY_INVALID;
  Data.UserConfirmationRequired = TRUE;
  Data.SessionId = 0;
  Data.RebootRequired = FALSE;
  Data.AuthToken = DFCI_AUTH_TOKEN_INVALID;

  //
  // 1 - Check mailbox for data.
  //
  Status = GetPendingProvisionData(&Data);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "No valid pending Input settings\n"));
    goto CLEANUP;
  }

  //
  // 2 - Validate mailbox data.
  //
  Status = ValidateAndAuthenticatePendingProvisionData(
             &Data, 
             mDfciSettingsPermissionProtocol);

  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "ValidateAndAuthenticatePendingProvisionData failed %r\n", Status));
    goto CLEANUP;
  }

  //
  // 3 - Check if delayed processing is required.
  // 
  // If handling this provisioning request cannot be completed at this time,
  // register a callback to take care of it at end of DXE.
  //
  Unenroll = FALSE;
  LocalAuthNeeded = FALSE;
  if ((Data.TrustedCertSize == 0) && (Data.Identity == DFCI_IDENTITY_SIGNER_OWNER)) {
    Unenroll = TRUE;

  } else if (Data.UserConfirmationRequired != FALSE) {
    LocalAuthNeeded = TRUE;
  }

  Status = DfciUiCheckForDelayProcessingNeeded(Unenroll, LocalAuthNeeded);
  if (EFI_ERROR(Status)) {

    //
    // If a delay is needed, setup the delay callback.
    // 
    // NOTE: If this fails, the provisioning instance data object is left in the
    //       DELAYED_PROCESSING state.
    //
    Data.State = AUTH_MAN_PROV_STATE_DATA_DELAYED_PROCESSING;
    Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                                TPL_CALLBACK,
                                WindowManagerCallback,
                                NULL,
                                &gEfiEndOfDxeEventGroupGuid,
                                &mWMProtocolEvent );

    if (EFI_ERROR( Status )) {
      DEBUG(( DEBUG_ERROR, "ERROR %a - EndOfDxe callback registration failed! %r\n", __FUNCTION__, Status ));
    }

    DEBUG((DEBUG_INFO, "Delay Processing %r\n", Status));
    goto CLEANUP;
  }

  //
  // 4 - Handle User Input
  //
  // If user confirmation is required, get the answer from the user.
  //
  if (Data.UserConfirmationRequired == FALSE) {
    DEBUG((DEBUG_VERBOSE, "USER APPROVAL NOT NECESSARY\n"));
    Data.State = AUTH_MAN_PROV_STATE_DATA_USER_APPROVED;

  } else {
    Status = DfciUiGetAnswerFromUser(&mAuthProtocol,
                                     PKT_FIELD_FROM_OFFSET(Data.Var,Data.TrustedCertOffset),
                                     Data.TrustedCertSize,
                                     &Data.AuthToken);

    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "DfciUiGetAnswerFromUser failed %r\n", Status));
      if (Status == EFI_NOT_READY) {
        Data.State = AUTH_MAN_PROV_STATE_DATA_AUTH_SYSTEM_ERROR;
        Data.StatusCode = EFI_NOT_READY;

      } else {
        Data.State = AUTH_MAN_PROV_STATE_DATA_USER_REJECTED;
        Data.StatusCode = EFI_ABORTED;
      }

      goto CLEANUP;

    } else {
      Data.RebootRequired = TRUE;
      Data.State = AUTH_MAN_PROV_STATE_DATA_USER_APPROVED;
    }
  }

  if (Data.State != AUTH_MAN_PROV_STATE_DATA_USER_APPROVED) {
    DEBUG((DEBUG_ERROR, "DfciUiGetAnswerFromUser - User Rejected Change\n"));
    goto CLEANUP;
  }

  //
  // 5 - Apply the change
  //
  Status = ApplyProvisionData(&Data);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "ApplyProvisionData failed %r\n", Status));
    goto CLEANUP;
  }

CLEANUP:
  DeleteProvisionVariable(&Data);
  Status = SetProvisionResponse(&Data);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "SetProvisionResponse failed %r\n", Status));
  }

  if (Data.RebootRequired) {
    gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
  }

  if (Data.AuthToken != DFCI_AUTH_TOKEN_INVALID) {
    DisposeAuthToken(&mAuthProtocol, &(Data.AuthToken));
  }

  if (Data.Var != NULL) {
    FreePool(Data.Var);
  }

  return;
}

/**
Clear all DFCI from the System.  

This requires an Auth token that has permission to change the owner key and/or permission for recovery.

All settings need a DFCI reset (only reset the settings that are DFCI only)
All Permissions need a DFCI Reset (clear all permissions and internal data)
All Auth needs a DFCI reset (Cleaer all keys and internal data)

**/
EFI_STATUS
EFIAPI
ClearDFCI (
  IN CONST DFCI_AUTH_TOKEN *AuthToken
  )
{
  DFCI_SETTING_ACCESS_PROTOCOL *SettingsAccess = NULL;
  EFI_STATUS Status = EFI_SUCCESS;

  if (*AuthToken == DFCI_AUTH_TOKEN_INVALID)
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - ClearDFCI requires valid auth token\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //
  // Check to make sure we have necessary protocols
  //
  if (mDfciSettingsPermissionProtocol == NULL)
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - requires Settings Permission Protocol\n", __FUNCTION__ ));
    Status = EFI_NOT_READY;
    goto CLEANUP;
  }

  //
  // Get SettingsAccess
  //
  Status = gBS->LocateProtocol(&gDfciSettingAccessProtocolGuid, NULL, &SettingsAccess);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - requires Settings Access Protocol (Status = %r)\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  //Must Reset Settings (including settings internal data)
  Status = SettingsAccess->Reset(SettingsAccess, AuthToken);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - FAILED to clear Settings.  Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status);
    goto CLEANUP;
  }
  DEBUG((DEBUG_INFO, "[AM] Settings Cleared\n"));

  //Must clear permissions (including internal data)
  Status = mDfciSettingsPermissionProtocol->ResetPermissions(mDfciSettingsPermissionProtocol, AuthToken);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - FAILED to Reset Permissions. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status);
    goto CLEANUP;
  }
  DEBUG((DEBUG_INFO, "[AM] Permissions Reset\n"));

  //Must delete keys (including internal data)
  Status = InitializeProvisionedData();
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - FAILED to Reset All Auth. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status);
    goto CLEANUP;
  }
  DEBUG((DEBUG_INFO, "[AM] All Stored Auths Keys Reset\n"));

  //Dispose all Key based Identity Mappings in the system
  Status = DisposeAllIdentityMappings(DFCI_IDENTITY_MASK_KEYS);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "[AM] - %a - FAILED to dispose all existing key based auth tokens. Status = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR(Status);
    goto CLEANUP;
  }

CLEANUP:
  return Status;
}

