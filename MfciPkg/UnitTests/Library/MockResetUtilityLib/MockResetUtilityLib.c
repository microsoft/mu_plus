/** @file
  Unit tests of the MFCI instance of the ResetUtilityLib class.

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UnitTestLib.h>
#include <Library/ResetUtilityLib.h>

/**
  This is a shorthand helper function to reset with reset type and a subtype
  so that the caller doesn't have to bother with a function that has half
  a dozen parameters.

  This will generate a reset with status EFI_SUCCESS, a NULL string, and
  no custom data. The subtype will be formatted in such a way that it can be
  picked up by notification registrations and custom handlers.

  NOTE: This call will fail if the architectural ResetSystem underpinnings
        are not initialized. For DXE, you can add gEfiResetArchProtocolGuid
        to your DEPEX.

  @param[in]  ResetType     The default EFI_RESET_TYPE of the reset.
  @param[in]  ResetSubtype  GUID pointer for the reset subtype to be used.

**/
VOID
EFIAPI
ResetSystemWithSubtype (
  IN EFI_RESET_TYPE  ResetType,
  IN CONST  GUID     *ResetSubtype
  )
{
  BASE_LIBRARY_JUMP_BUFFER  *JumpBuf;

  assert_non_null (ResetSubtype);

  check_expected (ResetType);
  check_expected_ptr (ResetSubtype);

  JumpBuf = (BASE_LIBRARY_JUMP_BUFFER *)mock ();

  LongJump (JumpBuf, 1);
}
