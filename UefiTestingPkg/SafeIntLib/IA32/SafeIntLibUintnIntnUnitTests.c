/**
@file
IA32-specific functions for unit-testing INTN and UINTN functions in
SafeIntLib.


Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SafeIntLibUnitTests.h"

UNIT_TEST_STATUS
EFIAPI
TestSafeInt32ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT32 Operand = 0x5bababab;
  UINTN Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeInt32ToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeInt32ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt32ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT32 Operand = 0x5bababab;
  INTN Result = 0;

  //
  // If Operand is <= MAX_INTN, then it's a cast
  //
  Status = SafeUInt32ToIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUInt32ToIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5bababab;
  INT32 Result = 0;

  //
  // INTN is same as INT32 in IA32, so this is just a cast
  //
  Status = SafeIntNToInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Operand = 0x5bababab;
  UINT32 Result = 0;

  //
  // If Operand is non-negative, then it's a cast
  //
  Status = SafeIntNToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (-1537977259);
  Status = SafeIntNToUInt32(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToUInt32(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0xabababab;
  UINT32 Result = 0;

  //
  // UINTN is same as UINT32 in IA32, so this is just a cast
  //
  Status = SafeUIntNToUInt32(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0x5bababab;
  INTN Result = 0;

  //
  // If Operand is <= MAX_INTN, then it's a cast
  //
  Status = SafeUIntNToIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xabababab);
  Status = SafeUIntNToIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNToInt64(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Operand = 0xabababab;
  INT64 Result = 0;

  //
  // UINTN is same as UINT32 in IA32, and UINT32 is a subset of
  // INT64, so this is just a cast
  //
  Status = SafeUIntNToInt64(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0x5bababab;
  INTN Result = 0;

  //
  // If Operand is between MIN_INTN and  MAX_INTN2 inclusive, then it's a cast
  //
  Status = SafeInt64ToIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  Operand = (-1537977259);
  Status = SafeInt64ToIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-1537977259), Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeInt64ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INT64 Operand = 0xabababab;
  UINTN Result = 0;

  //
  // If Operand is between 0 and  MAX_UINTN inclusive, then it's a cast
  //
  Status = SafeInt64ToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0x5babababefefefef);
  Status = SafeInt64ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Operand =  (-6605562033422200815);
  Status = SafeInt64ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0x5bababab;
  INTN Result = 0;

  //
  // If Operand is <= MAX_INTN, then it's a cast
  //
  Status = SafeUInt64ToIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x5bababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUInt64ToUIntN(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINT64 Operand = 0xabababab;
  UINTN Result = 0;

  //
  // If Operand is <= MAX_UINTN, then it's a cast
  //
  Status = SafeUInt64ToUIntN(Operand, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0xabababab, Result);

  //
  // Otherwise should result in an error status
  //
  Operand = (0xababababefefefef);
  Status = SafeUInt64ToUIntN(Operand, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNAdd(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Augend = 0x3a3a3a3a;
  UINTN Addend = 0x3a3a3a3a;
  UINTN Result = 0;

  //
  // If the result of addition doesn't overflow MAX_UINTN, then it's addition
  //
  Status = SafeUIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74747474, Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0xabababab;
  Addend = 0xbcbcbcbc;
  Status = SafeUIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNAdd(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Augend = 0x3a3a3a3a;
  INTN Addend = 0x3a3a3a3a;
  INTN Result = 0;

  //
  // If the result of addition doesn't overflow MAX_INTN
  // and doesn't underflow MIN_INTN, then it's addition
  //
  Status = SafeIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x74747474, Result);

  Augend = (-976894522);
  Addend = (-976894522);
  Status = SafeIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-1953789044), Result);

  //
  // Otherwise should result in an error status
  //
  Augend = 0x5a5a5a5a;
  Addend = 0x5a5a5a5a;
  Status = SafeIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Augend = (-1515870810);
  Addend = (-1515870810);
  Status = SafeIntNAdd(Augend, Addend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNSub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Minuend = 0x5a5a5a5a;
  UINTN Subtrahend = 0x3b3b3b3b;
  UINTN Result = 0;

  //
  // If Minuend >= Subtrahend, then it's subtraction
  //
  Status = SafeUIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x1f1f1f1f, Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = 0x5a5a5a5a;
  Subtrahend = 0x6d6d6d6d;
  Status = SafeUIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNSub(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Minuend = 0x5a5a5a5a;
  INTN Subtrahend = 0x3a3a3a3a;
  INTN Result = 0;

  //
  // If the result of subtractions doesn't overflow MAX_INTN or
  // underflow MIN_INTN, then it's subtraction
  //
  Status = SafeIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x20202020, Result);

  Minuend = 0x3a3a3a3a;
  Subtrahend = 0x5a5a5a5a;
  Status = SafeIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL((-538976288), Result);

  //
  // Otherwise should result in an error status
  //
  Minuend = (-2054847098);
  Subtrahend = 2054847098;
  Status = SafeIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  Minuend = (2054847098);
  Subtrahend = (-2054847098);
  Status = SafeIntNSub(Minuend, Subtrahend, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeUIntNMult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  UINTN Multiplicand = 0xa122a;
  UINTN Multiplier = 0xd23;
  UINTN Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_UINTN, it will succeed
  //
  Status = SafeUIntNMult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x844c9dbe, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0xa122a;
  Multiplier = 0xed23;
  Status = SafeUIntNMult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
TestSafeIntNMult(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  EFI_STATUS Status;
  INTN Multiplicand = 0x123456;
  INTN Multiplier = 0x678;
  INTN Result = 0;

  //
  // If the result of multiplication doesn't overflow MAX_INTN and doesn't
  // underflow MIN_UINTN, it will succeed
  //
  Status = SafeIntNMult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(0x75c28c50, Result);

  //
  // Otherwise should result in an error status
  //
  Multiplicand = 0x123456;
  Multiplier = 0xabc;
  Status = SafeIntNMult(Multiplicand, Multiplier, &Result);
  UT_ASSERT_EQUAL(RETURN_BUFFER_TOO_SMALL, Status);

  return UNIT_TEST_PASSED;
}