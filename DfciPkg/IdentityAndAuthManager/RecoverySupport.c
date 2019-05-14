/** @file
RecoverySupport.c

Manage the brute force recovery to unlock a provisioned system that fails to boot.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include "IdentityAndAuthManager.h"
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DfciRecoveryLib.h>
#include <Library/BaseMemoryLib.h>

#include <Settings/DfciSettings.h>

#define MAX_TRIES_FOR_RECOVERY (3)

//2 hours
#define RECOVERY_TIMEOUT_IN_SECONDS (60*60*2)


UINT64  mResponseValidationCount = 0;
DFCI_RECOVERY_CHALLENGE  *mRecoveryChallenge = NULL;
DFCI_IDENTITY_ID mRecoveryId = DFCI_IDENTITY_INVALID;


/**
DFCI recovery needs to happen.  User has performed valid
operations to invoke recovery.

- Need to clear permissions
- Need to reset to defaults for all settings which do not have a FrontPage UI element
- Need to clear all DFCI Auth and Keys
**/
EFI_STATUS
DoDfciRecovery()
{
  DFCI_AUTH_TOKEN Random = DFCI_AUTH_TOKEN_INVALID;
  EFI_STATUS Status;
  gBS->SetWatchdogTimer(0x0000, 0x0000, 0x0000, NULL);

  if (mRecoveryChallenge != NULL)
  {
    FreePool(mRecoveryChallenge);
    mRecoveryChallenge = NULL;
  }

  //Make a new auth using this key so the Clear operation has authority
  Random = CreateAuthTokenWithMapping(mRecoveryId);
  Status = ClearDFCI(&Random);
  mRecoveryId = DFCI_IDENTITY_INVALID;
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a: DFCI Recovery FAILED!!!! Status = %r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
Function to shutdown the system to protect it as unauthorized hammering has been
detected.

**/
VOID
ShutdownDueToHammering()
{
  gRT->ResetSystem(EfiResetShutdown, EFI_SECURITY_VIOLATION, 0, NULL);
  ASSERT(FALSE);
}

/**
This function returns a dynamically allocated Recovery Packet.
caller should free the Packet once finished.
Identity must be a valid key and have permission to do recovery

@param This               Auth Protocol Instance Pointer
@param Identity           identity to use to create recovery packet
@param Packet             Dynamically allocated Encrypted Recovery Packet

@retval EFI_SUCCESS   Packet is valid
@retval ERROR         no packet created
**/
EFI_STATUS
EFIAPI
GetRecoveryPacket(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL       *This,
  IN       DFCI_IDENTITY_ID                   Identity,
  OUT      DFCI_AUTH_RECOVERY_PACKET          **Packet
  )
{
  EFI_STATUS Status = EFI_ACCESS_DENIED;
  DFCI_PERMISSION_MASK Mask = DFCI_PERMISSION_MASK__NONE;
  DFCI_RECOVERY_CHALLENGE *Challenge = NULL;
  UINT8  *EData = NULL;
  UINTN  EDataSize = 0;
  DFCI_AUTH_RECOVERY_PACKET *LocalPacket = NULL;
  UINTN LocalSize = 0;
  UINT8 *CertData = NULL;  //DONT FREE THIS AS IT POINTS TO MODULE DATA
  UINTN CertDataSize = 0;
  UINTN ChallengeSize = 0;

  //Check input parameters
  if ((This == NULL) || (Packet == NULL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //Check to make sure not already started.
  if (mRecoveryChallenge != NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Recovery Process already started.  Only 1 process per boot.\n", __FUNCTION__));
    Status = EFI_ALREADY_STARTED;
    goto CLEANUP;
  }

  //Make sure identity is a key
  if ((Identity & DFCI_IDENTITY_MASK_KEYS) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Identity is not a key.  Not supported.\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  //Make sure this identity is provisioned
  if ((Provisioned() & Identity) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Identity is not provisioned at this time.\n", __FUNCTION__));
    Status = EFI_UNSUPPORTED;
    goto CLEANUP;
  }

  //Make sure this identity is enabled from Recovery
  if (mDfciSettingsPermissionProtocol == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Permission Protocol still NULL\n", __FUNCTION__));
    Status = EFI_NOT_READY;
    goto CLEANUP;
  }

  Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__DFCI_RECOVERY, &Mask);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get permission for recovery %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  // If DFCI_RECOVERY_MASK is 0, this is a DFCI recovery request.  Get permissions for ZTD_RECOVERY
  if (Mask == 0) {
    Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__ZTD_RECOVERY, &Mask);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - Failed to get permission for Dfci recovery %r\n", __FUNCTION__, Status));
      goto CLEANUP;
    }
  }

  if ((Mask & Identity) == 0)
  {
    DEBUG((DEBUG_ERROR, "%a - Identity not supported for recovery. Id (%d) \n", __FUNCTION__, Identity));
    Status = EFI_ACCESS_DENIED;
    goto CLEANUP;
  }

  //Get the Key info so that we can call lib to generate recovery packet
  //Do not free CertData as it is just an internal ptr
  Status = GetProvisionedCertDataAndSize(&CertData, &CertDataSize, Identity);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Cert Data for Identity (0x%X) %r\n", __FUNCTION__, Identity, Status));
    goto CLEANUP;
  }

  //Make the Challenge Packet
  Status = GetRecoveryChallenge(&Challenge, &ChallengeSize);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to get Recovery Challenge %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  //Encrypt Challenge
  Status = EncryptRecoveryChallenge(Challenge, ChallengeSize, CertData, CertDataSize, &EData, &EDataSize);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Encrypt Recovery Challenge %r\n", __FUNCTION__, Status));
    goto CLEANUP;
  }

  //Allocate memory for the caller structure
  LocalSize = sizeof(DFCI_AUTH_RECOVERY_PACKET) + EDataSize;
  LocalPacket = AllocatePool(LocalSize);
  if (LocalPacket == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - Failed to Allocate Memory for Recovery Packet (0x%X)\n", __FUNCTION__, LocalSize));
    Status = EFI_OUT_OF_RESOURCES;
    goto CLEANUP;
  }

  //Assign Structure data
  LocalPacket->Identity = Identity;
  LocalPacket->DataLength = EDataSize;
  CopyMem(&(LocalPacket->Data), EData, EDataSize);

  //Set global ptr - do after encryption so we know it was successful
  mRecoveryChallenge = Challenge;

  //Set a watchdog for our timeout period.  System will reset in that period.
  gBS->SetWatchdogTimer(RECOVERY_TIMEOUT_IN_SECONDS, 0x0000, 0x00, NULL);

  //set return values
  *Packet = LocalPacket;

  //Set the identity so we can use later once authenticated
  mRecoveryId = Identity;

  //TODO: remove this code once tool is written to decode
  //DEBUG((DEBUG_INFO, "%a: DEBUG FEATURE - Print response.  todo: remove before production\n", __FUNCTION__));
  //DEBUG_BUFFER(DEBUG_INFO, &(Challenge->Nonce.Parts.Key), sizeof(Challenge->Nonce.Parts.Key), DEBUG_DM_PRINT_ASCII);


CLEANUP:
  if (EFI_ERROR(Status))
  {
    //free the challenge if error.
    if (Challenge != NULL) { FreePool(Challenge); }
  }

  if (EData != NULL) { FreePool(EData); }

  return Status;
}

/**
This function validates the user provided Recovery response against
the active recovery packet for this session.  (1 packet at a given time/boot)

@param This               Auth Protocol Instance Pointer
@param RecoveryResponse   binary bytes of the recovery response
@param Size               Size of the response buffer in bytes.

@retval EFI_SUCCESS            - Recovery successful.  DFCI is unenrolled
@retval EFI_SECURITY_VIOLATION - All valid attempts have been exceeded.  Device needs rebooted.  Recovery session over
@retval EFI_ACCESS_DENIED      - Incorrect RecoveryResponse.  Try again.
@retval ERROR                  - Other error
**/
EFI_STATUS
EFIAPI
SetRecoveryResponse(
  IN CONST DFCI_AUTHENTICATION_PROTOCOL     *This,
  IN CONST UINT8                            *RecoveryResponse,
  IN       UINTN                            Size
  )
{
  EFI_STATUS Status = EFI_ACCESS_DENIED;

  //Check input parameters
  if ((This == NULL) || (RecoveryResponse == NULL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  if (Size != RECOVERY_RESPONSE_SIZE)
  {
    DEBUG((DEBUG_ERROR, "Size is not correct\n"));
    Status = EFI_INVALID_PARAMETER;
    goto CLEANUP;
  }

  //already exceeded.  Anti-hammering check...
  if (mResponseValidationCount >= MAX_TRIES_FOR_RECOVERY)
  {
    mResponseValidationCount++;
    DEBUG((DEBUG_ERROR, "Exceeded Max tries for recovery! ANTI-HAMMERING Check.\n"));
    Status = EFI_SECURITY_VIOLATION;
    goto CLEANUP;
  }

  //Make sure there is an active recovery session going on.
  if (mRecoveryChallenge == NULL)
  {
    DEBUG((DEBUG_ERROR, "No Recovery Packet Session Active.  Error\n"));
    Status = EFI_NOT_READY;
    goto CLEANUP;
  }

  if (CompareMem(&(mRecoveryChallenge->Nonce.Parts.Key), RecoveryResponse, Size) != 0)
  {
    DEBUG((DEBUG_ERROR, "Bad Recovery Response.  Not correct\n"));
    mResponseValidationCount++;
    Status = EFI_ACCESS_DENIED;
    goto CLEANUP;
  }

  //All test passed.  Looks good.
  DEBUG((DEBUG_INFO, "Recovery Response Valid.  DFCI Recovery Process Started\n"));
  Status = EFI_SUCCESS;
  DoDfciRecovery();

CLEANUP:

  if (mResponseValidationCount > MAX_TRIES_FOR_RECOVERY)
  {
    DEBUG((DEBUG_ERROR, "%a: Hammering detected.  Shutdown now!\n", __FUNCTION__));
    ShutdownDueToHammering();
  }

  if (mResponseValidationCount == MAX_TRIES_FOR_RECOVERY)
  {
    gBS->SetWatchdogTimer(0x0000, 0x0000, 0x0000, NULL);  //clear watchdog so our UI can reset
    Status = EFI_SECURITY_VIOLATION;
  }
  return Status;
}