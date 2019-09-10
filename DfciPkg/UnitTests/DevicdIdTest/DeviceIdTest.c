/** @file
DeviceIdTestApp.c

This is a Unit Test for the DfciDeviceIdSupportLib library.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <UnitTestTypes.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciDeviceIdSupportLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME        L"Device Id Library test cases"
#define UNIT_TEST_APP_VERSION     L"1.0"

// A Constant used to insure a field is not disturbed.
#define TEST_CONSTANT_ONE  0xDeadBea7Ba5eBa11

// The minimum string is a single character with a NULL
#define TEST_MIN_STRING_SIZE 2

// The maximum string is 64 characters with a NULL
#define TEST_MAX_STRING_SIZE 65

// Characters that are always invalid in a UTF-8 string
UINT8   mInvalidUTF8[] = {0xc0, 0xc1, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
                          0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

/**

Device Id Library rules:

  1.  The following characters are not allowed "'<>&

  2.  The maximum string length is 64 characters

  3.  NULL (0x00) is a required terminator.  The interfaces return
      the string and the size of the string (including the NULL).

  4.  The string is a valid UTF-8 string (ie, no 8-bit ASCII)

 */

typedef
EFI_STATUS
(EFIAPI *GET_NAME) (
    CHAR8   **ProductName,
    UINTN    *ProductNameSize  OPTIONAL
  );

typedef struct {
    GET_NAME              GetString;
    CHAR8               **Parameter1;
    UINTN                *Parameter2;
    CHAR8                *IdString;
    UINTN                 IdStringSize;
    EFI_STATUS            ExpectedStatus;
} BASIC_TEST_CONTEXT;

//*----------------------------------------------------------------------------------*
//* Test Contexts                                                                    *
//*----------------------------------------------------------------------------------*
static BASIC_TEST_CONTEXT mTest1  = { NULL,                         NULL,              NULL,                  NULL, 0, EFI_SUCCESS };

static BASIC_TEST_CONTEXT mTest2  = { DfciIdSupportGetSerialNumber, NULL,              NULL,                  NULL, 0, EFI_INVALID_PARAMETER };
static BASIC_TEST_CONTEXT mTest3  = { DfciIdSupportGetSerialNumber, &mTest3.IdString,  &mTest3.IdStringSize,  NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest4  = { DfciIdSupportGetSerialNumber, &mTest4.IdString,  NULL,                  NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest5  = { DfciIdSupportGetSerialNumber, &mTest5.IdString,  &mTest5.IdStringSize,  NULL, 0, EFI_SUCCESS };

static BASIC_TEST_CONTEXT mTest6  = { DfciIdSupportGetProductName,  NULL,              NULL,                  NULL, 0, EFI_INVALID_PARAMETER };
static BASIC_TEST_CONTEXT mTest7  = { DfciIdSupportGetProductName,  &mTest7.IdString,  &mTest7.IdStringSize,  NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest8  = { DfciIdSupportGetProductName,  &mTest8.IdString,  NULL,                  NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest9  = { DfciIdSupportGetProductName,  &mTest9.IdString,  &mTest9.IdStringSize,  NULL, 0, EFI_SUCCESS };

static BASIC_TEST_CONTEXT mTest10 = { DfciIdSupportGetManufacturer, NULL,              NULL,                  NULL, 0, EFI_INVALID_PARAMETER };
static BASIC_TEST_CONTEXT mTest11 = { DfciIdSupportGetManufacturer, &mTest11.IdString, &mTest11.IdStringSize, NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest12 = { DfciIdSupportGetManufacturer, &mTest12.IdString, NULL,                  NULL, 0, EFI_SUCCESS };
static BASIC_TEST_CONTEXT mTest13 = { DfciIdSupportGetManufacturer, &mTest13.IdString, &mTest13.IdStringSize, NULL, 0, EFI_SUCCESS };

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/*
    IsValidUTF8

    Validate the the string is a valid UTF-8 string

    Based on https://en.wikipedia.org/wiki/UTF-8
*/
static
BOOLEAN
IsValidUTF8(CHAR8 *InputString) {

    CONST UINT8 *String;

    // The code counts on "unsigned" comparisons. The Dfci library uses CHAR8 *.
    // Re-cast the input string to an UNSIGNED INT8 * for the comparisons to work correctly.

    String = (CONST UINT8 *) InputString;

   // Testing based on https://en.wikipedia.org/wiki/UTF-8

    while (*String != '\0') {
        //ASCII -- NULL is terminator
        if ((0x01 <= String[0] && String[0] <= 0x7E)) {
            String += 1;
            continue;
        }

        // non-overlong 2-byte
        if ((0xC2 <= String[0] && String[0] <= 0xDF) &&
            (0x80 <= String[1] && String[1] <= 0xBF)) {
            String += 2;
            continue;
        }

        if ((// excluding overlongs
                String[0] == 0xE0 &&
                (0xA0 <= String[1] && String[1] <= 0xBF) &&
                (0x80 <= String[2] && String[2] <= 0xBF)
            ) ||
            (// straight 3-byte
                ((0xE1 <= String[0] && String[0] <= 0xEC) ||
                    String[0] == 0xEE ||
                    String[0] == 0xEF) &&
                (0x80 <= String[1] && String[1] <= 0xBF) &&
                (0x80 <= String[2] && String[2] <= 0xBF)
            ) ||
            (// excluding surrogates
                String[0] == 0xED &&
                (0x80 <= String[1] && String[1] <= 0x9F) &&
                (0x80 <= String[2] && String[2] <= 0xBF)
            )) {
            String += 3;
            continue;
        }

        if ((// planes 1-3
                String[0] == 0xF0 &&
                (0x90 <= String[1] && String[1] <= 0xBF) &&
                (0x80 <= String[2] && String[2] <= 0xBF) &&
                (0x80 <= String[3] && String[3] <= 0xBF)
            ) ||
            (// planes 4-15
                (0xF1 <= String[0] && String[0] <= 0xF3) &&
                (0x80 <= String[1] && String[1] <= 0xBF) &&
                (0x80 <= String[2] && String[2] <= 0xBF) &&
                (0x80 <= String[3] && String[3] <= 0xBF)
            ) ||
            (// plane 16
                String[0] == 0xF4 &&
                (0x80 <= String[1] && String[1] <= 0x8F) &&
                (0x80 <= String[2] && String[2] <= 0xBF) &&
                (0x80 <= String[3] && String[3] <= 0xBF)
            )) {
            String += 4;
            continue;
        }

        return FALSE;
    }

    return TRUE;
}

/*
    CleanUpTestContext

    Cleans up after a test case.  Free's any allocated buffers if a test
    takes the error exit.

*/
static
UNIT_TEST_STATUS
EFIAPI
CleanUpTestContext (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    if (NULL != Btc->IdString) {
        FreePool (Btc->IdString);
    }

    Btc->IdString = NULL;
    Btc->IdStringSize = 0;

    return UNIT_TEST_PASSED;
}

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

/*
    VerifyUTF8

    Validate the UTF-8 checking catches certain invalid characters, and
    does not access memory outside the string buffer.

    For the memory access portion of the test to work correctly, HEAP_GUARD
    must be enabled.
*/
static
UNIT_TEST_STATUS
EFIAPI
VerifyUTF8 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    UINTN               i;

#define TEST_STRING_SIZE 128

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Btc->IdString = AllocatePool (TEST_STRING_SIZE);
    UT_ASSERT_NOT_NULL (Btc->IdString);
    SetMem (Btc->IdString, TEST_STRING_SIZE, 'A');
    Btc->IdString[TEST_STRING_SIZE-1] = '\0';

    // Make sure a normal ASCII string is valid
    UT_ASSERT_TRUE (IsValidUTF8 (Btc->IdString));

    // There are 13 invalid character that cannot be in any UTF-8 string
    for (i = 0; i< ARRAY_SIZE(mInvalidUTF8); i++) {
        Btc->IdString[0] = mInvalidUTF8[i];
        UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));
    }

    Btc->IdString[0] = 'A';  // Restore first character to a valid character

    // The following tests count on HEAP GUARD to page fault if the string
    // is accessed beyond the '\0'.  We also make sure that any tested string
    // is properly '\0' terminated. The IsValidUTF8 code doesn't check the length,
    // but counts on the ordering of the compares to not access beyond the '\0' character.

    // Place a starting 2 byte code in the last character position.
    Btc->IdString[TEST_STRING_SIZE-2] = 0xC2;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place a starting 3 byte code in the last character position.
    Btc->IdString[TEST_STRING_SIZE-2] = 0xE0;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place a starting 4 byte code in the last character position.
    Btc->IdString[TEST_STRING_SIZE-2] = 0xF0;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place two valid bytes of a 3 byte code in the last two character positions.
    Btc->IdString[TEST_STRING_SIZE-3] = 0xE0;
    Btc->IdString[TEST_STRING_SIZE-2] = 0xA0;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place two valid bytes of a 4 byte code in the last two character positions.
    Btc->IdString[TEST_STRING_SIZE-3] = 0xF0;
    Btc->IdString[TEST_STRING_SIZE-2] = 0x90;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place three valid bytes of a 4 byte code in the last three character positions.
    Btc->IdString[TEST_STRING_SIZE-4] = 0xF1;
    Btc->IdString[TEST_STRING_SIZE-3] = 0x80;
    Btc->IdString[TEST_STRING_SIZE-2] = 0x80;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    // Place an invalid 4 byte code in the last 4 character positions.
    // The maximum character is U+10FFFF.  That means, U+110000 is invalid.
    // Place U+110000 in the string:
    Btc->IdString[TEST_STRING_SIZE-5] = 0xF5;
    Btc->IdString[TEST_STRING_SIZE-4] = 0x80;
    Btc->IdString[TEST_STRING_SIZE-3] = 0x80;
    Btc->IdString[TEST_STRING_SIZE-2] = 0x80;
    UT_ASSERT_FALSE (IsValidUTF8 (Btc->IdString));

    return UNIT_TEST_PASSED;
}

/*
    ValidateNull

    Verify that the GetString function return EFI_INVALID_PARAMETER when
    the first parameter is NULL.  The second parameter is optional.
*/
static
UNIT_TEST_STATUS
EFIAPI
ValidateNull (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Btc->IdStringSize = TEST_CONSTANT_ONE;
    Status = Btc->GetString (Btc->Parameter1, Btc->Parameter2);

    UT_LOG_INFO ("\nGetString return code %r, expected %r\n", Status, Btc->ExpectedStatus);

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
    UT_ASSERT_EQUAL (Btc->IdStringSize, TEST_CONSTANT_ONE);

    return UNIT_TEST_PASSED;
}

/*
    ValidateSize

    Verify that the size is correct, and matches the result of AsciiStrnSizeS().
*/
static
UNIT_TEST_STATUS
EFIAPI
ValidateSize (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;
    UINTN               MeasuredSize;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Btc->IdString = (CHAR8 *) TEST_CONSTANT_ONE;
    Status = Btc->GetString (Btc->Parameter1, Btc->Parameter2);

    UT_LOG_INFO ("\nGetString return code %r, expected %r\n", Status, Btc->ExpectedStatus);
    UT_LOG_INFO ("StringSize = %d\n", Btc->IdStringSize);

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
    UT_ASSERT_NOT_NULL (Btc->IdString);
    UT_ASSERT_NOT_EQUAL ((UINTN) Btc->IdString, TEST_CONSTANT_ONE);
    UT_ASSERT_TRUE (Btc->IdStringSize >= TEST_MIN_STRING_SIZE);
    UT_ASSERT_TRUE (Btc->IdStringSize <= TEST_MAX_STRING_SIZE);

    MeasuredSize = AsciiStrnSizeS(Btc->IdString, Btc->IdStringSize);
    UT_LOG_INFO ("MeasuredSize = %d\n", MeasuredSize);

    UT_ASSERT_EQUAL (MeasuredSize, Btc->IdStringSize);

    return UNIT_TEST_PASSED;
}

/*
    ValidateNullP2

    Verify that Parameter two is optional.

    We have already verified that the size of the string is valid.  Just make sure a valid
    string is returned when parameter two is NULL.
*/
static
UNIT_TEST_STATUS
EFIAPI
ValidateNullP2 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;
    UINTN               MeasuredSize;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Btc->IdStringSize = TEST_CONSTANT_ONE;
    Status = Btc->GetString (Btc->Parameter1, Btc->Parameter2);

    UT_LOG_INFO ("\nGetString return code %r, expected %r\n", Status, Btc->ExpectedStatus);

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
    UT_ASSERT_EQUAL (Btc->IdStringSize, TEST_CONSTANT_ONE);
    UT_ASSERT_NOT_NULL (Btc->IdString);

    MeasuredSize = AsciiStrnSizeS(Btc->IdString, TEST_MAX_STRING_SIZE);
    UT_ASSERT_TRUE (MeasuredSize >= TEST_MIN_STRING_SIZE);
    UT_ASSERT_TRUE (MeasuredSize <= TEST_MAX_STRING_SIZE);

    return UNIT_TEST_PASSED;
}

/*
    ValidateCharacters

    Verify that the characters are allowed.
*/
static
UNIT_TEST_STATUS
EFIAPI
ValidateCharacters (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;
    UINTN               i;
    CHAR8               TestChar;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    Status = Btc->GetString (Btc->Parameter1, Btc->Parameter2);

    UT_LOG_INFO ("\nGetString return code %r, expected %r\n", Status, Btc->ExpectedStatus);

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
    UT_LOG_INFO ("StringSize = %d\n", Btc->IdStringSize);

    UT_ASSERT_STATUS_EQUAL (Status, Btc->ExpectedStatus);
    UT_ASSERT_NOT_NULL (Btc->IdString);
    UT_ASSERT_NOT_EQUAL ((UINTN) Btc->IdString, TEST_CONSTANT_ONE);
    UT_ASSERT_TRUE (Btc->IdStringSize >= TEST_MIN_STRING_SIZE);
    UT_ASSERT_TRUE (Btc->IdStringSize <= TEST_MAX_STRING_SIZE);

    // Sample String:   "ABC"
    //
    //   AsciiStrSize = 4
    //   AsciiStrLen = 3
    //
    //   Index of each character:
    //      [0] [1] [2] [3]
    //      A   B   C   '\0'
    //   So, NULL at end of string is StrSize - 1

    // String must end in a NULL;
    UT_ASSERT_TRUE (Btc->IdString[Btc->IdStringSize - 1] == '\0');

    UT_ASSERT_TRUE (IsValidUTF8 (Btc->IdString));

    for (i=0; i < Btc->IdStringSize -2; i++) {
        TestChar = Btc->IdString[i];
        UT_ASSERT_NOT_EQUAL ( TestChar, '"');
        UT_ASSERT_NOT_EQUAL ( TestChar, '\'');
        UT_ASSERT_NOT_EQUAL ( TestChar, '<');
        UT_ASSERT_NOT_EQUAL ( TestChar, '>');
        UT_ASSERT_NOT_EQUAL ( TestChar, '&');
    }

    return UNIT_TEST_PASSED;
}


///================================================================================================
///================================================================================================
///
/// TEST ENGINE
///
///================================================================================================
///================================================================================================


/**
  DeviceIdTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
DeviceIdTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    UNIT_TEST_FRAMEWORK       *Fw = NULL;
    UNIT_TEST_SUITE           *DeviceIdTests;
    CHAR16                     ShortName[100];
    EFI_STATUS                 Status;

    ShortName[0] = L'\0';
    UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName);
    DEBUG(( DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
        goto EXIT;
    }

    //
    // Populate the DeviceId Library Test Suite.
    //
    Status = CreateUnitTestSuite( &DeviceIdTests, Fw, L"Validate DeviceId Library returns valid data", L"DeviceId.Test", NULL, NULL);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Device Id Tests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    //-----------Suite----------Description-----------------Class------------Test Function-------Pre---Clean-Context
    AddTestCase( DeviceIdTests, L"UTF8 SelfCheck",          L"SelfCheck",    VerifyUTF8,         NULL, CleanUpTestContext, &mTest1);

    AddTestCase( DeviceIdTests, L"GetSerialNumber NULL",    L"GetSN.NULL",   ValidateNull,       NULL, CleanUpTestContext, &mTest2);
    AddTestCase( DeviceIdTests, L"GetSerialNumber Size",    L"GetSN.Size",   ValidateSize,       NULL, CleanUpTestContext, &mTest3);
    AddTestCase( DeviceIdTests, L"GetSerialNumber NULL P2", L"GetSN.NULL",   ValidateNullP2,     NULL, CleanUpTestContext, &mTest4);
    AddTestCase( DeviceIdTests, L"GetSerialNumber Chars",   L"GetSN.Chars",  ValidateCharacters, NULL, CleanUpTestContext, &mTest5);

    AddTestCase( DeviceIdTests, L"GetProductName NULL",     L"GetPN.NULL",   ValidateNull,       NULL, CleanUpTestContext, &mTest6);
    AddTestCase( DeviceIdTests, L"GetProductName Size",     L"GetPN.Size",   ValidateSize,       NULL, CleanUpTestContext, &mTest7);
    AddTestCase( DeviceIdTests, L"GetSerialNumber NULL P2", L"GetSN.NULL",   ValidateNullP2,     NULL, CleanUpTestContext, &mTest8);
    AddTestCase( DeviceIdTests, L"GetProductName Chars",    L"GetPN.Chars",  ValidateCharacters, NULL, CleanUpTestContext, &mTest9);

    AddTestCase( DeviceIdTests, L"GetManufacturer NULL",    L"GetMfg.NULL",  ValidateNull,       NULL, CleanUpTestContext, &mTest10);
    AddTestCase( DeviceIdTests, L"GetManufacturer Size",    L"GetMfg.Size",  ValidateSize,       NULL, CleanUpTestContext, &mTest11);
    AddTestCase( DeviceIdTests, L"GetSerialNumber NULL P2", L"GetSN.NULL",   ValidateNullP2,     NULL, CleanUpTestContext, &mTest12);
    AddTestCase( DeviceIdTests, L"GetManufacturer Chars",   L"GetMfg.Chars", ValidateCharacters, NULL, CleanUpTestContext, &mTest13);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);

EXIT:
    if (Fw) {
        FreeUnitTestFramework (Fw);
    }

    return Status;
}