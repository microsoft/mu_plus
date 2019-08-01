/** @file
ZeroTouchSettings.c

Library instance for ZeroTouch to support enabling, display, and deleting
the Zero Touch Certificate.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/ZeroTouchVariables.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ZeroTouchSettingsLib.h>   // Just for header information, no library access.

///// --- Inernal functions ------

///// --- No Dfci Settings.  Must not be linked with SettingsManager.
/////                        Should be linked with IdentityAndAuthManager
/////                        and DfciMenu

/////---------------------Interface for Library  ---------------------//////

/**
 * GetZeroTouchCertificate
 *
 * Returns the built in Zero Touch certificate
 *
 * @param Certificate
 * @param CertificateSize
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
GetZeroTouchCertificate(UINT8 **Certificate, UINTN *CertificateSize) {

    EFI_STATUS Status;
    EFI_GUID  *CertFile;


    CertFile = (EFI_GUID *) PcdGetPtr(PcdZeroTouchCertificateFile);

    Status = GetSectionFromAnyFv(CertFile,
                                 EFI_SECTION_RAW,
                                 0,
                                 (VOID **) Certificate,
                                 CertificateSize
                                );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a - Unable to get the Zero Touch certificate\n", __FUNCTION__));
    }

    return Status;
}

/**
 * Function to Get Zero Touch OptOut.
 *
 * @retval: TRUE    User has NOT opted out of Zero Touch.
 * @retval: FALSE   User has opted out of Zero Touch.
 *
 **/
ZERO_TOUCH_STATE
EFIAPI
GetZeroTouchState (
    VOID
  ) {
    UINT32            Attributes;
    EFI_STATUS        Status;
    UINT8             State;
    UINTN             StateSize;
    ZERO_TOUCH_STATE  CurrentState;

    State = 0;
    StateSize = sizeof(State);
    CurrentState = ZERO_TOUCH_INACTIVE;
    Status = gRT->GetVariable (ZERO_TOUCH_VARIABLE_OPT_IN_VAR_NAME,
                              &gZeroTouchVariableGuid,
                              &Attributes,
                              &StateSize,
                              &State);

    if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL) {
        DEBUG((DEBUG_INFO,"%a - retrieving ZTD State. Code=%r\n", __FUNCTION__, Status));
    } else {
        if ((Status == EFI_BUFFER_TOO_SMALL) || (Attributes != ZERO_TOUCH_VARIABLE_ATTRIBUTES)) {
            DEBUG((DEBUG_ERROR,"%a - Invalid variable size or attributes.\n", __FUNCTION__, Status));
            Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_OPT_IN_VAR_NAME,
                                      &gZeroTouchVariableGuid,
                                       0,
                                       0,
                                       NULL);
            if (EFI_ERROR(Status)) {
                DEBUG((DEBUG_ERROR,"%a - error deleting invalid variable. Code=%r\n", __FUNCTION__, Status));
            }
        } else {
            if (State == 0) {
                CurrentState = ZERO_TOUCH_OPT_OUT;
            } else {
                CurrentState = ZERO_TOUCH_OPT_IN;
            }
            DEBUG((DEBUG_INFO,"%a - Zero touch marked as %d.\n", __FUNCTION__, State));
        }
    }

    return CurrentState;
}

/**
Function to Set Zero Touch State.

Sets Zero Touch state to Opt in or Opt Out

@retval: Success - Zero Touch state set
@retval: EFI_INVALID_PARAMETER  - Attempt to set state to InActive.
**/
EFI_STATUS
EFIAPI
SetZeroTouchState (
    IN  ZERO_TOUCH_STATE NewState
  ) {
    EFI_STATUS      Status;
    UINT8           State;

    switch (NewState) {
        case ZERO_TOUCH_OPT_IN:
            State = 1;
            break;
        case ZERO_TOUCH_OPT_OUT:
            State = 0;
            break;
        default:
            return EFI_INVALID_PARAMETER;
    }

    Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_OPT_IN_VAR_NAME,
                              &gZeroTouchVariableGuid,
                               ZERO_TOUCH_VARIABLE_ATTRIBUTES,
                               sizeof(State),
                              &State);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a - Unable to set state of Zero Touch. Code=%r\n", __FUNCTION__, Status));
    } else {
        DEBUG((DEBUG_INFO,"%a - Zero touch marked as %d.\n", __FUNCTION__, State));
    }

    return Status;
}