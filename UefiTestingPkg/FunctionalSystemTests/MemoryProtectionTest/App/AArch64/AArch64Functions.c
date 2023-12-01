/** @file

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

/**
  Registers the interrupt handler which will be invoked when a page fault occurs.

  @retval     EFI_SUCCESS         Interrupt handler successfully installed
  @retval     EFI_UNSUPPORTED     Installing the interrupt handler is not supported.
  @retval     others              Error occurred during installation.
**/
EFI_STATUS
EFIAPI
RegisterMemoryProtectionTestAppInterruptHandler (
  VOID
  )
{
  // TODO: Add support for reset test method on Arm
  return EFI_UNSUPPORTED;
}

/**
  Checks if the hardware NX protection is enabled.

  @param[in]  Context   The unit test context.

  @retval     UNIT_TEST_PASSED    Hardware NX protection is enabled.
  @retval     others              Hardware NX protection is not enabled.
**/
UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  // ARM UEFI only has Stage 1 translation, so the FEAT XNX configuration bit does not apply.
  return UNIT_TEST_PASSED;
}
