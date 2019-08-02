/**
@file
UEFI Shell based application for unit testing the SafeIntLib.


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/SafeIntLib.h>

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNAdd(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNAdd(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNSub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNSub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNMult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNMult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  );
