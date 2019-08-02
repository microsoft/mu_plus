/** @file Header file Ms Boot Options library

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_BOOT_OPTIONS_LIB_H_
#define _MS_BOOT_OPTIONS_LIB_H_

#include <Library/UefiBootManagerLib.h>

/**
 * GetDefaultBootApp - the application that implements the default boot order
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
MsBootOptionsLibGetDefaultBootApp (
    IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    IN     CHAR8                        *Parameter
);

/**
  Return the boot option corresponding to the Boot Manager Menu.

  @param BootOption    Return a created Boot Manager Menu with the parameter passed
  @param Parameter     The parameter to add to the BootOption

  @retval EFI_SUCCESS   The Boot Manager Menu is successfully returned.
  @retval Status        Return status of gRT->SetVariable (). BootOption still points
                        to the Boot Manager Menu even the Status is not EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MsBootOptionsLibGetBootManagerMenu (
    IN OUT EFI_BOOT_MANAGER_LOAD_OPTION *BootOption,
    IN     CHAR8                        *Parameter
);

/**
 * Register Default Boot Options
 *
 * @param
 *
 * @return VOID EFIAPI
 */
VOID
EFIAPI
MsBootOptionsLibRegisterDefaultBootOptions (
  VOID
);

/**
 * Get default boot options
 *
 * @param OptionCount
 *
 * @return EFI_BOOT_MANAGER_LOAD_OPTION*EFIAPI
 */
EFI_BOOT_MANAGER_LOAD_OPTION *
EFIAPI
MsBootOptionsLibGetDefaultOptions (
    OUT UINTN    *OptionCount
);

#endif  // _MS_BOOT_OPTIONS_LIB_H_

