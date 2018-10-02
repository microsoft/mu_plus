/** @file
ZeroTouchSettings.c

Library instance for ZeroTouch to support enabling, display, and deleting
the Zero Touch Certificate.

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

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Guid/ZeroTouchSettingsGuid.h>
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

#include <Settings/ZeroTouchSettings.h>

///// --- Inernal functions ------

/**
 * Install _ZT_CERT_INSTALL variable if the variable is not locked.
 *
 */
VOID
EFIAPI
ZeroTouchOnReadyToBoot (
    IN EFI_EVENT        Event,
    IN VOID             *Context
    ) {

    UINT8   State;
    EFI_STATUS Status;

    State = 1;

    Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME,
                              &gZeroTouchVariableGuid,
                               ZERO_TOUCH_VARIABLE_ATTRIBUTES,
                               sizeof(State),
                              &State);

    switch (Status) {
    case EFI_SUCCESS:
        DEBUG((DEBUG_INFO,"%a - Enabling install of Zero touch certificate.\n", __FUNCTION__));
        break;
    case EFI_ACCESS_DENIED:
        DEBUG((DEBUG_INFO,"%a - Unable to install Zero touch certificate.\n", __FUNCTION__));
        break;
    default:
        DEBUG((DEBUG_ERROR,"%a - Error setting %s. Code=%r\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME, Status));
        break;
    }

    gBS->CloseEvent(Event);
}


///// --- No Dfci Settings.  Must be linked with SettingsManager to get a proper constructor call.

/////---------------------Interface for Library  ---------------------//////

/**
 * GetZeroTouchState
 *
 * Checks to see if Zero Touch can be installed..
 *
 * @author miketur (7/18/2018)
 * @param
 *
 * @return BOOLEAN EFIAPI
 */
BOOLEAN
EFIAPI
GetZeroTouchInstallState (VOID)
{
    EFI_STATUS      Status;
    UINTN           ValueSize;
    UINT8           OptOutState;
    UINT8           InstallState;
    UINT32          Attributes;
    BOOLEAN         Installable;


    OptOutState = 0;
    ValueSize = sizeof(OptOutState);
    Status = gRT->GetVariable (ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME,
                              &gZeroTouchVariableGuid,
                              &Attributes,
                              &ValueSize,
                              &OptOutState );
    if (!EFI_ERROR(Status)) {                          // We have a variable
        if (ZERO_TOUCH_VARIABLE_ATTRIBUTES != Attributes) {  // Check if Attributes are wrong
            OptOutState = 0;               // Don't trust value received
            // Delete invalid variable
            Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME,
                                      &gZeroTouchVariableGuid,
                                       0,
                                       0,
                                       NULL);
            if (EFI_ERROR(Status)) {                   // What???
                DEBUG((DEBUG_ERROR,"%a - Unable to delete invalid variable %s\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME));
            }
        } else {
            OptOutState = 1;
        }
    } else {
        DEBUG((DEBUG_ERROR,"%a - error getting %s. Code=%r\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME, Status));
    }

    InstallState = 0;
    ValueSize = sizeof(InstallState);
    Status = gRT->GetVariable (ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME,
                              &gZeroTouchVariableGuid,
                              &Attributes,
                              &ValueSize,
                              &InstallState );
    if (!EFI_ERROR(Status)) {                          // We have a variable
        if (ZERO_TOUCH_VARIABLE_ATTRIBUTES != Attributes) {  // Check if Attributes are wrong
            InstallState = 0;                          // Don't trust the setting obtained
            // Delete invalid URL variable
            Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME,
                                      &gZeroTouchVariableGuid,
                                       0,
                                       0,
                                       NULL);
            if (EFI_ERROR(Status)) {                   // What???
                DEBUG((DEBUG_ERROR,"%a - Unable to delete invalid variable %s\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME));
            }
        }
    } else {
        DEBUG((DEBUG_ERROR,"%a - error getting %s. Code=%r\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME, Status));
    }

    if ((OptOutState == 0) && (InstallState == 1)) {
        Installable = TRUE;
    } else {
        Installable = FALSE;
    }

    return Installable;
}

/**
 * GetZeroTouchCertificate
 *
 * Checks if the user has opted out of Zero Touch enrollment. If
 * opted out, return EFI_NOT_FOUND to indicate no Certificate;
 *
 * Otherwize, the Zero Touch certificate is returned.
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
    BOOLEAN    Installable;

    Installable =  GetZeroTouchInstallState();

    if (!Installable) {
        return EFI_NOT_FOUND;
    }

    CertFile = (EFI_GUID *) PcdGetPtr(PcdZeroTouchCertificateFile);

    Status = GetSectionFromAnyFv(CertFile,
                                 EFI_SECTION_RAW,
                                 0,
                                 Certificate,
                                 CertificateSize
                                );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a - Unable to get the Zero Touch certificate\n", __FUNCTION__));
    }

    return Status;
}

/**
Function to Set Zero Touch Installed.

Calling this function set the variable state to Installed.

@retval: Success - State recorded
@retval: EFI_ERROR.  Error occurred.
**/
EFI_STATUS
EFIAPI
SetZeroTouchInstalled (VOID)
{
    EFI_STATUS      Status;
    UINT8           State;

    State = 0;
    Status = EFI_SUCCESS;

    Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME,
                              &gZeroTouchVariableGuid,
                               ZERO_TOUCH_VARIABLE_ATTRIBUTES,
                               sizeof(State),
                              &State);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a - Unable to set Install to 0. Code=%r\n", __FUNCTION__, Status));
    } else {
        DEBUG((DEBUG_INFO,"%a - Zero touch marked installed.\n", __FUNCTION__));
    }

    return Status;
}

/**
Function to Set Zero Touch OptOut.

SetZeroTouchOptOut sets the _ZT_CERT_OPT_OUT variable. Once it is set, it can only be
deleted if in MFG mode.

@retval: Success - Zero Touch is disabled
@retval: EFI_ERROR.  Error occurred.
**/
EFI_STATUS
EFIAPI
SetZeroTouchOptOut (VOID)
{
    EFI_STATUS      Status;
    UINT8           State;


    State = 0;
    Status = EFI_SUCCESS;

    Status = gRT->SetVariable (ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME,
                              &gZeroTouchVariableGuid,
                               ZERO_TOUCH_VARIABLE_ATTRIBUTES,
                               sizeof(State),
                              &State);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"%a - Unable to disable Zero Touch. Code=%r\n", __FUNCTION__, Status));
    } else {
        DEBUG((DEBUG_INFO,"%a - Zero touch disabled.\n", __FUNCTION__));
    }

    return Status;
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is only needed for DfciSettingsManager support.
 * The design is to have the PCD false for all modules except the 1 anonymously liked to the DfciettingsManager.
 *
 * @param  ImageHandle   The firmware allocated handle for the EFI image.
 * @param  SystemTable   A pointer to the EFI System Table.
 *
 * @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
 *
 **/
EFI_STATUS
EFIAPI
ZeroTouchSettingsConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

    EFI_EVENT  InitEvent;
    EFI_STATUS Status;
    UINT8      State;
    UINT32     Attributes;
    UINTN      ValueSize;

    if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {

        // Only do these things once.  This is accomplished by only running this code
        // when attached to the Settings Manager.

        // Try to delete the _ZT_CERT_OPT_OUT variable

        Status = gRT->SetVariable(ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME, &gZeroTouchVariableGuid, 0, 0, NULL);

        switch (Status) {
        case EFI_SUCCESS:
            DEBUG((DEBUG_ERROR, "Zero Touch re-enabled.\n"));
            break;
        case EFI_ACCESS_DENIED:
            DEBUG((DEBUG_ERROR, "Zero Touch is disabled.\n"));
            break;
        case EFI_NOT_FOUND:
            DEBUG((DEBUG_ERROR, "Zero Touch is enabled.\n"));
            break;
        default:
            DEBUG((DEBUG_ERROR, "%a - Initialize Zero Touch Var failed. %r.\n", __FUNCTION__, Status));
            break;
        }

        ValueSize = sizeof(State);
        Status = gRT->GetVariable (ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME,
                                  &gZeroTouchVariableGuid,
                                  &Attributes,
                                  &ValueSize,
                                  &State );
        switch (Status) {
        case EFI_NOT_FOUND:
            //
            // Register notify function to set _ZT_CERT_INSTALL variable at ReadyToBoot.
            //
            Status = gBS->CreateEventEx(
              EVT_NOTIFY_SIGNAL,
              TPL_CALLBACK,
              ZeroTouchOnReadyToBoot,
              ImageHandle,  //set the context to the image handle
              &gEfiEventReadyToBootGuid,
              &InitEvent
              );

            if (InitEvent == NULL) {
              DEBUG((DEBUG_ERROR, "%a - Create Event Ex for Ready to Boot failed\n", __FUNCTION__));
            }
            break;
        case EFI_SUCCESS:
            DEBUG((DEBUG_INFO, "%a - %s state = %d\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME, State));
            break;
        default:
            DEBUG((DEBUG_ERROR, "%a - Error checking %s. Code=%r\n", __FUNCTION__, ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME, Status));
        }
    }

    return EFI_SUCCESS;
}

