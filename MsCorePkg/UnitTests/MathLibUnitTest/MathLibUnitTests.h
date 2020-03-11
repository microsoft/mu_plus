/**
Unit-tests UEFI shell app for MathLib


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MATH_LIB_UNIT_TESTS_H
#define MATH_LIB_UNIT_TESTS_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>

#include <Library/MathLib.h>


#define UNIT_TEST_APP_NAME        "Math Lib Unit Test Application"
#define UNIT_TEST_APP_VERSION     "0.1"

EFI_STATUS
EFIAPI
RegisterAttributeTests(UNIT_TEST_SUITE_HANDLE   TestSuite);

EFI_STATUS
EFIAPI
RegisterElementTests(UNIT_TEST_SUITE_HANDLE   TestSuite);

#endif