/** @file -- ArchSpecificFunctions.h
Shared definition of the method between x64 and ARM.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARCH_SPECIFIC_FUNCTIONS_H_
#define ARCH_SPECIFIC_FUNCTIONS_H_

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
  );

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
  );

#endif // ARCH_SPECIFIC_FUNCTIONS_H_
