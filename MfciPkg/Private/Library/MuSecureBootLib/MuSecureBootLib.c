/*++ @file MuSecureBootLib.c

Module Name:

  MuSecureBootLib.c

Abstract:

  This module contains functions for setting and clearing the secure boot
  variables

Environment:

  Driver Execution Environment (DXE)

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
--*/

#include <PiDxe.h>                               // This has to be here so Protocol/FirmwareVolume2.h doesn't cause errors.

#include <Guid/ImageAuthentication.h>            // gEfiCertPkcs7Guid
#include <Guid/AuthenticatedVariableFormat.h>    // EFI_CUSTOM_MODE stuff.
#include <Guid/MuVarPolicyFoundationDxe.h>       // Used to determine ReadyToBoot status.

#include <Library/BaseMemoryLib.h>               // CopyMem, etc.
#include <Library/MemoryAllocationLib.h>         // AllocateZeroPool, etc.
#include <Library/DebugLib.h>                    // Tracing
#include <Library/UefiRuntimeServicesTableLib.h> // gRT
#include <Library/UefiBootServicesTableLib.h>    // gBS
#include <Library/MuSecureBootLib.h>             // Our header
//  ***** START: not needed by MfciPkg **
//  #include <Library/PlatformKeyLib.h>              // Project-Specific PKs
//  ***** END:   not needed by MfciPkg */

#include <Protocol/VariablePolicy.h>

//  ***** START: not needed by MfciPkg **
//  #include "MuSecureBootDefaultVars.h"             // Byte arrays of the default keys.
//  ***** END:   not needed by MfciPkg */

// Definitions to make code self-documenting.
#define PK_UPDATE_AUTHORIZED        TRUE
#define PK_UPDATE_NOT_AUTHORIZED    FALSE

//
// MS Default Signature Owner GUID
// NOTE: This is a placeholder GUID that doesn't correspond to anything else.
//
#define MS_DEFAULT_SIGNATURE_OWNER_GUID \
  { \
    0x5577A8B5, 0x6828, 0x4D03, {0x80, 0xC3, 0x8A, 0xE3, 0xA8, 0x13, 0x29, 0xAA} \
  }

//
// MS Default Time-Based Payload Creation Date
// This is the date that is used when creating SecureBoot default variables.
// NOTE: This is a placeholder date that doesn't correspond to anything else.
//
EFI_TIME  mDefaultPayloadTimestamp = {
  15,   // Year (2015)
  8,    // Month (Aug)
  28,   // Day (28)
  0,    // Hour
  0,    // Minute
  0,    // Second
  0,    // Pad1
  0,    // Nanosecond
  0,    // Timezone (Dummy value)
  0,    // Daylight (Dummy value)
  0     // Pad2
};

//  ***** START: not needed by MfciPkg **
//  
//  //
//  // SELECT PLATFORM KEY
//  //
//  #define PLATFORM_KEY_BUFFER   ProductionPlatformKeyCertificate
//  #define PLATFORM_KEY_SIZE     ProductionSizeOfPlatformKeyCertificate
//  
//  //
//  // QUICK PROTOTYPES
//  // So that we don't have to create a whole header file...
//  //
//  STATIC
//  EFI_STATUS
//  InstallSecureBootVariable (
//    IN CHAR16                  *VariableName,
//    IN EFI_GUID                *VendorGuid,
//    IN UINTN                   DataSize,
//    IN VOID                    *Data
//    );
//  
//  ***** END:   not needed by MfciPkg */

/**
  NOTE: Copied from SecureBootConfigImpl.c, then modified.

  Create a time based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION_2
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out]   DataSize       On input, the size of Data buffer in bytes.
                                   On output, the size of data returned in Data
                                   buffer in bytes.
  @param[in, out]   Data           On input, Pointer to data buffer to be wrapped or
                                   pointer to NULL to wrap an empty payload.
                                   On output, Pointer to the new payload date buffer allocated from pool,
                                   it's caller's responsibility to free the memory when finish using it.
  @param[in]        Time           [Optional] If provided, will be used as the timestamp for the payload.
                                   If NULL, a new timestamp will be generated using GetTime().

  @retval EFI_SUCCESS              Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES     There are not enough memory resources to create time based payload.
  @retval EFI_INVALID_PARAMETER    The parameter is invalid.
  @retval Others                   Unexpected error happens.

**/
STATIC
EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN            *DataSize,
  IN OUT UINT8            **Data,
  IN     EFI_TIME         *Time OPTIONAL
  )
{
  EFI_STATUS                       Status;
  UINT8                            *NewData;
  UINT8                            *Payload;
  UINTN                            PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2    *DescriptorData;
  UINTN                            DescriptorSize;
  EFI_TIME                         NewTime;

  if (Data == NULL || DataSize == NULL) {
    DEBUG((EFI_D_ERROR, "CreateTimeBasedPayload(), invalid arg\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // In Setup mode or Custom mode, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize    = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8*) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    DEBUG((EFI_D_ERROR, "CreateTimeBasedPayload() Out of resources.\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  
  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  //
  // Use or create the timestamp.
  //
  // If Time is NULL, create a new timestamp.
  if (Time == NULL)
  {
    ZeroMem (&NewTime, sizeof (EFI_TIME));
    Status = gRT->GetTime (&NewTime, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "CreateTimeBasedPayload(), GetTime() failed, status = '%r'\n",
             Status));
      FreePool(NewData);
      NewData = NULL;
      return Status;
    }
    NewTime.Pad1       = 0;
    NewTime.Nanosecond = 0;
    NewTime.TimeZone   = 0;
    NewTime.Daylight   = 0;
    NewTime.Pad2       = 0;
    Time = &NewTime;        // Use the new timestamp.
  }
  CopyMem (&DescriptorData->TimeStamp, Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  if (Payload != NULL) {
    FreePool(Payload);
    Payload = NULL;
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
} // CreateTimeBasedPayload()


/**
  Signals the Variable services that an "authorized" PK
  modification is about to occur. Before ReadyToBoot this
  *should* allow an update to the PK without validating the
  full signature.

  @param[in]  State   TRUE = PK update is authorized. Set indication tokens appropriately.
                      FALSE = PK update is not authorized. Clear all indication tokens.

  @retval     EFI_SUCCESS             State has been successfully updated.
  @retval     EFI_INVALID_PARAMETER   State is something other than TRUE or FALSE.
  @retval     EFI_SECURITY_VIOLATION  Attempting to an invalid state at an invalid time (eg. post-ReadyToBoot).
  @retval     Others                  Error returned from LocateProtocol or DisableVariablePolicy.

**/
STATIC
EFI_STATUS
SetAuthorizedPkUpdateState (
  IN  BOOLEAN   State
  )
{
  EFI_STATUS                        TempStatus, Status = EFI_SUCCESS;
  UINT32                            Attributes;
  PHASE_INDICATOR                   PhaseIndicator;
  UINTN                             DataSize;
  VARIABLE_POLICY_PROTOCOL          *VariablePolicy;

  DEBUG(( DEBUG_INFO, "[SB] %a()\n", __FUNCTION__ ));

  // Make sure that the State makes sense (because BOOL can technically be neither TRUE nor FALSE).
  if (State != PK_UPDATE_NOT_AUTHORIZED && State != PK_UPDATE_AUTHORIZED)
  {
    DEBUG(( DEBUG_ERROR, "%a - Invalid State passed: %d\n", __FUNCTION__, State ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Step 1: Determine whether we are post-ReadyToBoot.
  //  If so, only allow the state to be cleared, not set.
  //
  DataSize = sizeof( PhaseIndicator );
  TempStatus = gRT->GetVariable( READY_TO_BOOT_INDICATOR_VAR_NAME,
                                 &gMuVarPolicyDxePhaseGuid,
                                 &Attributes,
                                 &DataSize,
                                 &PhaseIndicator );

  // If we're past ReadyToBoot, make sure we're not attempting to allow an update.
  // Assume we are post-ReadyToBoot as long as the variable is not "missing".
  // This leaves the possibility of other errors tripping this mechanism, but
  //   if the variables infrastructure is failing, what else are we to do?
  if (TempStatus != EFI_NOT_FOUND && State == PK_UPDATE_AUTHORIZED)
  {
    DEBUG(( DEBUG_ERROR, "%a - Cannot set state to %d when ReadyToBoot indicator test returns %r.\n", __FUNCTION__, State, Status ));
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Step 2: If we are enabling, let's do that.
  //
  // NOTE: This is fine if it's called twice in a row.
  if (State == PK_UPDATE_AUTHORIZED)
  {
    // IMPORTANT NOTE: This operation is sticky and leaves variable protections disabled.
    //                  The system *MUST* be reset after performing this operation.
    Status = gBS->LocateProtocol( &gVariablePolicyProtocolGuid, NULL, (VOID **) &VariablePolicy );
    if (!EFI_ERROR( Status )) {
      Status = VariablePolicy->DisableVariablePolicy();
      // EFI_ALREADY_STARTED means that everything is currently disabled.
      // This should be considered SUCCESS.
      if (Status == EFI_ALREADY_STARTED) {
        Status = EFI_SUCCESS;
      }
    }
  }

  //
  // Step 3: If we are disabling, let's do that.
  //
  else
  {
    // NOTE: Currently, there's no way to disable the suspension of protections.
    //        This will be revisited in later versions of the VariablePolicy protocol.
    //        For now, the caller is responsible for resetting the system after attempting.
  }

  return Status;
} // SetAuthorizedPkUpdateState()


EFI_STATUS
EFIAPI
DeleteSecureBootVariables()
/*++

Routine Description:

    This function will attempt to delete the secure boot variables, thus
    disabling secure boot.


Arguments:


Return Value:

    EFI_SUCCESS or underlying failure code.

--*/
{
  EFI_STATUS                      Status, TempStatus;
  UINT32                          Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
  UINTN                           DataSize;
  UINT8                           *Data;

  DEBUG((DEBUG_INFO, "INFO: Attempting to delete the Secure Boot variables.\r\n"));

  //
  // Step 1: Create a dummy payload.
  // This payload should be a valid cert/auth header and nothing more.
  // It is effectively DataSize = 0 and Data = NULL, but for authenticated variables.
  DataSize = 0;
  Data = NULL;
  Status = CreateTimeBasedPayload( &DataSize, &Data, NULL );
  if (EFI_ERROR( Status ) || Data == NULL)
  {
    DEBUG((DEBUG_ERROR, "DeleteSecureBoot: - Failed to build payload! %r\r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
  }

  //
  // Step 2: Notify that a PK update is coming shortly...
  if (!EFI_ERROR( Status ))
  {
    Status = SetAuthorizedPkUpdateState( PK_UPDATE_AUTHORIZED );
    if (EFI_ERROR( Status ))
    {
      DEBUG((DEBUG_ERROR, "DeleteSecureBoot - Failed to signal PK update start! %r\r\n", Status));
      // Classify this as a PK deletion error.
      Status = EFI_ABORTED;
    }
  }

  //
  // Step 3: Attempt to delete the PK.
  // Let's try to nuke the PK, why not...
  if (!EFI_ERROR( Status ))
  {
    Status = gRT->SetVariable( EFI_PLATFORM_KEY_NAME,
                               &gEfiGlobalVariableGuid,
                               Attributes,
                               DataSize,
                               Data );
    DEBUG((DEBUG_INFO, "DeleteSecureBoot - %s Delete = %r\r\n", EFI_PLATFORM_KEY_NAME, Status));
    // If the PK is not found, then our work here is done.
    if (Status == EFI_NOT_FOUND) Status = EFI_SUCCESS;
    // If any other error occurred, let's inform the caller that the PK delete in particular failed.
    else if (EFI_ERROR( Status )) Status = EFI_ABORTED;
  }

  //
  // Step 4: Regardless of whether the PK update succeeded, notify that the update is done.
  if (EFI_ERROR( SetAuthorizedPkUpdateState( PK_UPDATE_NOT_AUTHORIZED ) ))
  {
    DEBUG((DEBUG_ERROR, "DeleteSecureBoot - Failed to signal PK update stop! %r\r\n", Status));
    // In this case, assert, because this is bad: the PK is still unlocked.
    // It's not the end of the world, though... PK will lock at ReadyToBoot.
    ASSERT_EFI_ERROR( Status );
    // Classify this as a PK deletion error.
    Status = EFI_ABORTED;
  }

  //
  // Step 5: Attempt to delete remaining keys/databases...
  // Now that the PK is deleted (assuming Status == EFI_SUCCESS) the system is in SETUP_MODE.
  // Arguably we could leave these variables in place and let them be deleted by whoever wants to
  // update all the SecureBoot variables. However, for cleanliness sake, let's try to
  // get rid of them here.
  if (!EFI_ERROR( Status ))
  {
    //
    // If any of THESE steps have an error, report the error but attempt to delete all keys.
    // Using TempStatus will prevent an error from being trampled by an EFI_SUCCESS.
    // Overwrite Status ONLY if TempStatus is an error.
    //
    // If the error is EFI_NOT_FOUND, we can safely ignore it since we were trying to delete
    // the variables anyway.
    //
    TempStatus = gRT->SetVariable( EFI_KEY_EXCHANGE_KEY_NAME,
                                   &gEfiGlobalVariableGuid,
                                   Attributes,
                                   DataSize,
                                   Data );
    DEBUG((DEBUG_INFO, "DeleteSecureBoot - %s Delete = %r\r\n", EFI_KEY_EXCHANGE_KEY_NAME, TempStatus));
    if (EFI_ERROR( TempStatus ) && TempStatus != EFI_NOT_FOUND) Status = EFI_ACCESS_DENIED;
    
    TempStatus = gRT->SetVariable( EFI_IMAGE_SECURITY_DATABASE,
                                   &gEfiImageSecurityDatabaseGuid,
                                   Attributes,
                                   DataSize,
                                   Data );
    DEBUG((DEBUG_INFO, "DeleteSecureBoot - %s Delete = %r\r\n", EFI_IMAGE_SECURITY_DATABASE, TempStatus));
    if (EFI_ERROR( TempStatus ) && TempStatus != EFI_NOT_FOUND) Status = EFI_ACCESS_DENIED;

    TempStatus = gRT->SetVariable( EFI_IMAGE_SECURITY_DATABASE1,
                                   &gEfiImageSecurityDatabaseGuid,
                                   Attributes,
                                   DataSize,
                                   Data );
    DEBUG((DEBUG_INFO, "DeleteSecureBoot - %s Delete = %r\r\n", EFI_IMAGE_SECURITY_DATABASE1, TempStatus));
    if (EFI_ERROR( TempStatus ) && TempStatus != EFI_NOT_FOUND) Status = EFI_ACCESS_DENIED;
  }

  //
  // Always Put Away Your Toys
  if (Data != NULL)
  {
    FreePool(Data);
    Data = NULL;
  }

  return Status;
}//DeleteSecureBootVariables()


/**
  Helper function to quickly determine whether SecureBoot is enabled.

  @retval     TRUE    SecureBoot is verifiably enabled.
  @retval     FALSE   SecureBoot is either disabled or an error prevented checking.

**/
BOOLEAN
IsSecureBootEnable (
  VOID
  )
{
  UINT8                   SecureBoot;
  UINTN                   VarSize;
  EFI_STATUS              Status;

  VarSize = sizeof(SecureBoot);

  Status = gRT->GetVariable (
                  L"SecureBoot",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &VarSize,
                  &SecureBoot
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,"Cannot check SecureBoot variable %r \n ", Status));
    return FALSE;
  }

  if (SecureBoot == SECURE_BOOT_MODE_ENABLE) {
    return TRUE;
  } else {
    return FALSE;
  }
}


//  ***** START: not needed by MfciPkg
//  
//  /**
//    Returns the current config of the SecureBoot variables, if it can be determined.
//  
//    @retval     UINTN   Will return an MS_SB_CONFIG token or -1 if the config cannot be determined.
//  
//  **/
//  UINTN
//  GetCurrentSecureBootConfig (
//    VOID
//    )
//  {
//    EFI_STATUS    Status;
//    UINTN         Config = (UINTN)-1;     // Default to "Unknown"
//    UINTN         DbVarSize;
//    UINT8         *DbVar = NULL;
//  
//    //
//    // Determine whether the PK is set.
//    // If it's not set, we'll indicate that we're in NONE regardless of db state.
//    // NOTE: We don't care about getting the variable, we just want to see if it exists.
//    DbVarSize = 0;
//    Status = gRT->GetVariable( EFI_PLATFORM_KEY_NAME,
//                               &gEfiGlobalVariableGuid,
//                               NULL,
//                               &DbVarSize,
//                               DbVar );
//    if (Status == EFI_NOT_FOUND)
//    {
//      return MS_SB_CONFIG_NONE;
//    }
//  
//    //
//    // Load the current db.
//    DbVarSize = 0;
//    Status = gRT->GetVariable( EFI_IMAGE_SECURITY_DATABASE,
//                               &gEfiImageSecurityDatabaseGuid,
//                               NULL,
//                               &DbVarSize,
//                               DbVar );
//    // Only proceed if the error was buffer too small.
//    if (Status == EFI_BUFFER_TOO_SMALL)
//    {
//      DbVar = AllocatePool( DbVarSize );
//      if (DbVar != NULL)
//      {
//        Status = gRT->GetVariable( EFI_IMAGE_SECURITY_DATABASE,
//                                   &gEfiImageSecurityDatabaseGuid,
//                                   NULL,
//                                   &DbVarSize,
//                                   DbVar );
//      }
//    }
//    // If it's missing, there are no keys installed.
//    else if (Status == EFI_NOT_FOUND)
//    {
//      Config = MS_SB_CONFIG_NONE;
//    }
//  
//    //
//    // Compare the current db to the stored dbs and determine whether either matches.
//    if (!EFI_ERROR( Status ))
//    {
//      if (DbVarSize == mDbDefaultSize && CompareMem( DbVar, mDbDefault, DbVarSize ) == 0)
//      {
//        Config = MS_SB_CONFIG_MS_ONLY;
//      }
//      else if (DbVarSize == mDb3PDefaultSize && CompareMem( DbVar, mDb3PDefault, DbVarSize ) == 0)
//      {
//        Config = MS_SB_CONFIG_MS_3P;
//      }
//    }
//  
//    // Clean up if necessary.
//    if (DbVar != NULL)
//    {
//      FreePool( DbVar );
//    }
//  
//    return Config;
//  }
//  
//  
//  /**
//    Similar to DeleteSecureBootVariables, this function is used to unilaterally
//    force the state of all 4 SB variables. Use built-in, hardcoded default vars.
//  
//    NOTE: The UseThirdParty parameter can be used to set either strict MS or
//          MS+3rdParty keys.
//  
//    @param[in]  UseThirdParty  Flag to indicate whether to use 3rd party keys or
//                               strict MS-only keys.
//  
//    @retval     EFI_SUCCESS               SecureBoot keys are now set to defaults.
//    @retval     EFI_ABORTED               SecureBoot keys are not empty. Please delete keys first
//                                          or follow standard methods of altering keys (ie. use the signing system).
//    @retval     EFI_SECURITY_VIOLATION    Failed to create the PK.
//    @retval     Others                    Something failed in one of the subfunctions.
//  
//  **/
//  EFI_STATUS
//  SetDefaultSecureBootVariables (
//    IN  BOOLEAN    UseThirdParty
//    )
//  {
//    EFI_STATUS          Status;
//    UINT8               *Data;
//    UINT32              DataSize;
//    EFI_GUID            EfiX509Guid = EFI_CERT_X509_GUID, MsDefaultOwner = MS_DEFAULT_SIGNATURE_OWNER_GUID;
//    EFI_SIGNATURE_LIST  *SigListBuffer = NULL;
//  
//    DEBUG(( DEBUG_INFO, "MuSecureBootLib::%a()\n", __FUNCTION__ ));
//  
//    //
//    // Right off the bat, if SecureBoot is currently enabled, bail.
//    if (IsSecureBootEnable())
//    {
//      DEBUG(( DEBUG_ERROR, "%a - Cannot set default keys while SecureBoot is enabled!\n", __FUNCTION__ ));
//      return EFI_ABORTED;
//    }
//  
//    //
//    // Start running down the list, creating variables in our wake.
//    // dbx is a good place to start.
//    Data      = (UINT8*)mDbxDefault;
//    DataSize  = mDbxDefaultSize;
//    Status    = InstallSecureBootVariable( EFI_IMAGE_SECURITY_DATABASE1,
//                                           &gEfiImageSecurityDatabaseGuid,
//                                           DataSize,
//                                           Data );
//  
//    // If that went well, try the db (make sure to pick the right one!).
//    if (!EFI_ERROR( Status ))
//    {
//      if (!UseThirdParty)
//      {
//        Data      = (UINT8*)mDbDefault;
//        DataSize  = mDbDefaultSize;
//      }
//      else
//      {
//        Data      = (UINT8*)mDb3PDefault;
//        DataSize  = mDb3PDefaultSize;
//      }
//      Status    = InstallSecureBootVariable( EFI_IMAGE_SECURITY_DATABASE,
//                                             &gEfiImageSecurityDatabaseGuid,
//                                             DataSize,
//                                             Data );
//    }
//  
//    // Keep it going. Keep it going. KEK...
//    if (!EFI_ERROR( Status ))
//    {
//      Data      = (UINT8*)mKekDefault;
//      DataSize  = mKekDefaultSize;
//      Status    = InstallSecureBootVariable( EFI_KEY_EXCHANGE_KEY_NAME,
//                                             &gEfiGlobalVariableGuid,
//                                             DataSize,
//                                             Data );
//    }
//  
//    //
//    // Finally! The Big Daddy of them all.
//    // The PK!
//    //
//    if (!EFI_ERROR( Status ))
//    {
//      //
//      // First, we must build the PK buffer with the correct data.
//      //
//      // Calculate the size of the necessary buffer.
//      // We will need:
//      //  - The EFI_SIGNATURE_LIST structure as a header.
//      //  - The initial section of EFI_SIGNATURE_DATA.
//      //  - The key itself in DER format (same format as stored in the lib).
//      // NOTE: We currently use the PLATFORM_KEY_SIZE and PLATFORM_KEY_BUFFER #defines to make it
//      //        easier to switch between development and production.
//      DataSize = PLATFORM_KEY_SIZE + sizeof( EFI_SIGNATURE_LIST ) + OFFSET_OF( EFI_SIGNATURE_DATA, SignatureData );
//      // This should NEVER happen, but let's make sure that our sum fits in a UINT32.
//      ASSERT( (DataSize >= PLATFORM_KEY_SIZE) &&
//              (DataSize >= sizeof( EFI_SIGNATURE_LIST )) &&
//              (DataSize >= OFFSET_OF( EFI_SIGNATURE_DATA, SignatureData )) );
//  
//      // Allocate a sufficient buffer.
//      SigListBuffer = AllocateZeroPool( DataSize );
//      if (!SigListBuffer)
//      {
//        DEBUG(( DEBUG_ERROR, "%a - Failed to allocate memory for the PK payload!\n", __FUNCTION__ ));
//        Status = EFI_OUT_OF_RESOURCES;
//      }
//    }
//    // If we've made it this far, we're ready to create the PK.
//    if (!EFI_ERROR( Status ))
//    {
//      // Copy the cert owner.
//      // Adjust the Data pointer to be the start of the SignatureOwner section of the buffer.
//      Data = (UINT8*)SigListBuffer + sizeof( EFI_SIGNATURE_LIST ) + OFFSET_OF( EFI_SIGNATURE_DATA, SignatureOwner );
//      CopyGuid( (EFI_GUID*)Data, &MsDefaultOwner );
//  
//      // Copy the cert data.
//      // Adjust the Data pointer to be the start of the SignatureData section of the buffer.
//      Data = (UINT8*)SigListBuffer + sizeof( EFI_SIGNATURE_LIST ) + OFFSET_OF( EFI_SIGNATURE_DATA, SignatureData );
//      CopyMem( Data, PLATFORM_KEY_BUFFER, PLATFORM_KEY_SIZE );
//  
//      // Set up the rest of the header.
//      CopyGuid( &SigListBuffer->SignatureType, &EfiX509Guid );
//      SigListBuffer->SignatureListSize    = DataSize;
//      SigListBuffer->SignatureHeaderSize  = 0;       // Always 0 for x509 type certs.
//      SigListBuffer->SignatureSize        = PLATFORM_KEY_SIZE + OFFSET_OF( EFI_SIGNATURE_DATA, SignatureData );
//  
//      //
//      // Finally, install the key.
//      Status = SetAuthorizedPkUpdateState( PK_UPDATE_AUTHORIZED );
//      if (!EFI_ERROR( Status ))
//      {
//        Status = InstallSecureBootVariable( EFI_PLATFORM_KEY_NAME,
//                                            &gEfiGlobalVariableGuid,
//                                            DataSize,
//                                            SigListBuffer );
//      }
//      // No matter what, make sure that we [attempt to] clear the update authorized state.
//      if (EFI_ERROR( SetAuthorizedPkUpdateState( PK_UPDATE_NOT_AUTHORIZED ) ))
//      {
//        // Don't trample the status from InstallSecureBootVariable(), because it's more likely to be useful.
//        if (!EFI_ERROR( Status ))
//        {
//          Status = EFI_ABORTED;
//        }
//      }
//  
//      //
//      // Report PK creation errors.
//      if (EFI_ERROR( Status ))
//      {
//        DEBUG(( DEBUG_ERROR, "%a - Failed to update the PK!%r\n", __FUNCTION__, Status ));
//        Status = EFI_SECURITY_VIOLATION;
//      }
//    }
//  
//    //
//    // Always put away your toys.
//    if (SigListBuffer)
//    {
//      FreePool( SigListBuffer );
//    }
//  
//    return Status;
//  } // SetDefaultSecureBootVariables()
//  
//  ****** END:   not needed by MfciPkg

/**
  A helper function to take in a variable payload, wrap it in the
  proper authenticated variable structure, and install it in the
  EFI variable space.

  NOTE: Does not actually sign anything. Requires system to be in setup mode.

  @param[in]  VariableName  Same name parameter that would be passed to SetVariable.
  @param[in]  VendorGuid    Same GUID parameter that would be passed to SetVariable.
  @param[in]  DataSize      Same size parameter that would be passed to SetVariable.
  @param[in]  Data          Same data parameter that would be passed to SetVariable.

  @retval     EFI_SUCCESS             Everything went well!
  @retval     EFI_OUT_OF_RESOURCES    There was a problem constructing the payload.

**/
STATIC
EFI_STATUS
InstallSecureBootVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
{
  EFI_STATUS    Status;
  UINT8         *Payload;
  UINTN         PayloadSize;
  UINT32        Attributes = (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                              EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS);

  DEBUG(( DEBUG_INFO, "MuSecureBootLib::%a()\n", __FUNCTION__ ));

  // Bring in the noise...
  PayloadSize = DataSize;
  Payload     = AllocateZeroPool( DataSize );
  // Bring in the funk...
  if (Payload == NULL) return EFI_OUT_OF_RESOURCES;
  CopyMem( Payload, Data, DataSize );

  //
  // Step 1: Create a wrapped payload.
  Status = CreateTimeBasedPayload( &PayloadSize, &Payload, &mDefaultPayloadTimestamp );
  if (EFI_ERROR( Status ) || Payload == NULL)
  {
    DEBUG(( DEBUG_ERROR, "%a - Failed to build payload! %r\n", Status ));
    Payload = NULL;
    Status = EFI_OUT_OF_RESOURCES;
  }

  //
  // Step 2: Attempt to set the variable.
  if (!EFI_ERROR( Status ))
  {
    Status = gRT->SetVariable( VariableName,
                               VendorGuid,
                               Attributes,
                               PayloadSize,
                               Payload );
    DEBUG(( DEBUG_VERBOSE, "%a - SetVariable(%s) = %r\n", __FUNCTION__, VariableName, Status ));
    if (EFI_ERROR( Status ))
    {
      DEBUG(( DEBUG_ERROR, "%a - SetVariable(%s) failed! %r\n", __FUNCTION__, VariableName, Status ));
    }
  }

  //
  // Always Put Away Your Toys
  // Payload will be reassigned by CreateTimeBasedPayload()...
  if (Payload != NULL)
  {
    FreePool( Payload );
    Payload = NULL;
  }

  return Status;
} // InstallSecureBootVariable()
