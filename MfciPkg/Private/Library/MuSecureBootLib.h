/*++ @file MuSecureBootLib.h

Module Name:

  MuSecureBootLib.h

Abstract:

  Secure Boot functions.

Environments:

  Driver Execution Environment (DXE)

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
--*/

#ifndef __MS_SECURE_BOOT_LIB_INC__
#define __MS_SECURE_BOOT_LIB_INC__


enum {
    MS_SB_CONFIG_MS_ONLY = 0,   // Starts at 0 so it can be used as an index.
    MS_SB_CONFIG_MS_3P,
    MS_SB_CONFIG_NONE,

    // ALWAYS LAST OPTION!!
    MS_SB_CONFIG_COUNT
};


/**
    This function will delete the secure boot keys, thus
    disabling it.


  @return EFI_SUCCESS or underlying failure code.

**/
EFI_STATUS
EFIAPI
DeleteSecureBootVariables();


/**
  Helper function to quickly determine whether SecureBoot is enabled.

  @retval     TRUE    SecureBoot is verifiably enabled.
  @retval     FALSE   SecureBoot is either disabled or an error prevented checking.

**/
BOOLEAN
IsSecureBootEnable (
  VOID
  );


/**
  Returns the current config of the SecureBoot variables, if it can be determined.

  @retval     UINTN   Will return an MS_SB_CONFIG token or -1 if the config cannot be determined.

**/
UINTN
GetCurrentSecureBootConfig (
  VOID
  );


/**
  Similar to DeleteSecureBootVariables, this function is used to unilaterally
  force the state of all 4 SB variables. Use build-in, hardcoded default vars.

  NOTE: The UseThirdParty parameter can be used to set either strict MS or
        MS+3rdParty keys.

  @param[in]  UseThirdParty  Flag to indicate whether to use 3rd party keys or
                             strict MS-only keys.

  @retval     EFI_SUCCESS               SecureBoot keys are now set to defaults.
  @retval     EFI_ABORTED               SecureBoot keys are not empty. Please delete keys first
                                        or follow standard methods of altering keys (ie. use the signing system).
  @retval     EFI_SECURITY_VIOLATION    Failed to create the PK.
  @retval     Others                    Something failed in one of the subfunctions.

**/
EFI_STATUS
SetDefaultSecureBootVariables (
  IN  BOOLEAN    UseThirdParty
  );

#endif //__MS_SECURE_BOOT_LIB_INC__
