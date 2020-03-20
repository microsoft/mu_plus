/** @file

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>

UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_CONTEXT           Context
  )
{
    //  TODO: Does ARM have an equivelent "EFER BIT" to check if NX protections are on?
    return UNIT_TEST_PASSED;
}