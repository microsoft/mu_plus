/** @file
Xmlb64TestApp.c

This is an EFI Shell application to test the b64 conversion routines.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <Uefi.h>
#include <UnitTestTypes.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME        L"Base64 Unit Test Application"
#define UNIT_TEST_APP_VERSION     L"1.0"

/**
RFC 4648  https://tools.ietf.org/html/rfc4648 test vectors

   BASE64("") = ""
   BASE64("f") = "Zg=="
   BASE64("fo") = "Zm8="
   BASE64("foo") = "Zm9v"
   BASE64("foob") = "Zm9vYg=="
   BASE64("fooba") = "Zm9vYmE="
   BASE64("foobar") = "Zm9vYmFy"

   The test vectors are using ascii strings for the binary data
 */

typedef struct {
    CHAR8      *TestInput;
    CHAR8      *TestOutput;
    EFI_STATUS  ExpectedStatus;
    VOID       *BufferToFree;
    UINTN       ExpectedSize;
} BASIC_TEST_CONTEXT;


#define B64_TEST_1     ""
#define BIN_TEST_1     ""

#define B64_TEST_2     "Zg=="
#define BIN_TEST_2     "f"

#define B64_TEST_3     "Zm8="
#define BIN_TEST_3     "fo"

#define B64_TEST_4     "Zm9v"
#define BIN_TEST_4     "foo"

#define B64_TEST_5     "Zm9vYg=="
#define BIN_TEST_5     "foob"

#define B64_TEST_6     "Zm9vYmE="
#define BIN_TEST_6     "fooba"

#define B64_TEST_7     "Zm9vYmFy"
#define BIN_TEST_7     "foobar"

// Adds white space - also ends the last quantum with only spaces afterwards
#define B64_TEST_8_IN   "   Zm9\r\nvYmFy   "
#define B64_TEST_8_OUT  "Zm9vYmFy"
#define BIN_TEST_8      "foobar"

// Not a quantum multiple of 4
#define B64_ERROR_1  "Zm9vymFy="

// Invalid characters in the string
#define B64_ERROR_2  "Zm$vymFy"

// Too many '=' characters
#define B64_ERROR_3 "Z==="

// Poorly placed '='
#define B64_ERROR_4 "Zm=vYmFy"

#define MAX_TEST_STRING_SIZE (200)
// ------------------------------------------------ Input----------Output-----------Result-------Free--Expected Output Size
static BASIC_TEST_CONTEXT    mBasicEncodeTest1  = {BIN_TEST_1,     B64_TEST_1,      EFI_SUCCESS, NULL, sizeof(B64_TEST_1)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest2  = {BIN_TEST_2,     B64_TEST_2,      EFI_SUCCESS, NULL, sizeof(B64_TEST_2)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest3  = {BIN_TEST_3,     B64_TEST_3,      EFI_SUCCESS, NULL, sizeof(B64_TEST_3)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest4  = {BIN_TEST_4,     B64_TEST_4,      EFI_SUCCESS, NULL, sizeof(B64_TEST_4)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest5  = {BIN_TEST_5,     B64_TEST_5,      EFI_SUCCESS, NULL, sizeof(B64_TEST_5)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest6  = {BIN_TEST_6,     B64_TEST_6,      EFI_SUCCESS, NULL, sizeof(B64_TEST_6)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest7  = {BIN_TEST_7,     B64_TEST_7,      EFI_SUCCESS, NULL, sizeof(B64_TEST_7)};
static BASIC_TEST_CONTEXT    mBasicEncodeTest8  = {BIN_TEST_8,     B64_TEST_8_OUT,  EFI_SUCCESS, NULL, sizeof(B64_TEST_8_OUT)};

static BASIC_TEST_CONTEXT    mBasicDecodeTest1  = {B64_TEST_1,     BIN_TEST_1,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_1)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest2  = {B64_TEST_2,     BIN_TEST_2,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_2)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest3  = {B64_TEST_3,     BIN_TEST_3,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_3)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest4  = {B64_TEST_4,     BIN_TEST_4,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_4)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest5  = {B64_TEST_5,     BIN_TEST_5,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_5)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest6  = {B64_TEST_6,     BIN_TEST_6,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_6)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest7  = {B64_TEST_7,     BIN_TEST_7,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_7)-1};
static BASIC_TEST_CONTEXT    mBasicDecodeTest8  = {B64_TEST_8_IN,  BIN_TEST_8,      EFI_SUCCESS, NULL, sizeof(BIN_TEST_8)-1};

static BASIC_TEST_CONTEXT    mBasicDecodeError1 = {B64_ERROR_1,    B64_ERROR_1,     EFI_INVALID_PARAMETER, NULL, 0};
static BASIC_TEST_CONTEXT    mBasicDecodeError2 = {B64_ERROR_2,    B64_ERROR_2,     EFI_INVALID_PARAMETER, NULL, 0};
static BASIC_TEST_CONTEXT    mBasicDecodeError3 = {B64_ERROR_3,    B64_ERROR_3,     EFI_INVALID_PARAMETER, NULL, 0};
static BASIC_TEST_CONTEXT    mBasicDecodeError4 = {B64_ERROR_4,    B64_ERROR_4,     EFI_INVALID_PARAMETER, NULL, 0};
static BASIC_TEST_CONTEXT    mBasicDecodeError5 = {B64_TEST_7,     BIN_TEST_1,      EFI_BUFFER_TOO_SMALL,  NULL, sizeof(BIN_TEST_7)-1};

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================

/**
Simple clean up method to make sure tests clean up even if interrupted and fail in the middle.
**/
static
UNIT_TEST_STATUS
EFIAPI
CleanUpB64TestContext (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;


    Btc = (BASIC_TEST_CONTEXT *) Context;
    if (Btc != NULL) {
      //free string if set
      if (Btc->BufferToFree != NULL)
      {
        FreePool(Btc->BufferToFree);
        Btc->BufferToFree = NULL;
      }
    }
    return UNIT_TEST_PASSED;
}

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================
static
UNIT_TEST_STATUS
EFIAPI
RfcEncodeTest(
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
    ) {

    BASIC_TEST_CONTEXT *Btc;
    CHAR8              *b64String;
    CHAR8              *binString;
    UINTN               b64StringSize;
    EFI_STATUS          Status;
    UINT8              *BinData;
    UINTN               BinSize;
    CHAR8              *b64WorkString;
    UINTN               ReturnSize;
    INTN                CompareStatus;
    UINTN               indx;


    Btc = (BASIC_TEST_CONTEXT *) Context;
    binString = Btc->TestInput;
    b64String = Btc->TestOutput;

    //
    //  Only testing the the translate functionality, so
    //  preallocate the proper string buffer

    b64StringSize = AsciiStrnSizeS(b64String, MAX_TEST_STRING_SIZE);
    BinSize = AsciiStrnLenS(binString, MAX_TEST_STRING_SIZE);
    BinData = (UINT8 *)  binString;

    // Only allocate the expected buffer, and count on memory
    // protections to catch buffer overruns
    b64WorkString = (CHAR8 *) AllocatePool(Btc->ExpectedSize);
    UT_ASSERT_NOT_NULL(b64WorkString);

    Btc->BufferToFree = b64WorkString;
    ReturnSize = b64StringSize;

    Status = Base64Encode(binString, BinSize, b64WorkString, &ReturnSize);

    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);

    UT_ASSERT_EQUAL(ReturnSize, Btc->ExpectedSize);

    if (ReturnSize != 0) {
        CompareStatus = AsciiStrnCmp(b64String, b64WorkString, ReturnSize);
        if (CompareStatus != 0) {
            UT_LOG_ERROR("b64 string compare error - size=%d\n",ReturnSize);
            for (indx = 0; indx < ReturnSize; indx++) {
                UT_LOG_ERROR(" %2.2x", 0xff & b64String[indx]);
            }
            UT_LOG_ERROR("\n b64 work string:\n");
            for (indx = 0; indx < ReturnSize; indx++) {
                UT_LOG_ERROR(" %2.2x", 0xff & b64WorkString[indx]);
            }
            UT_LOG_ERROR("\n");
        }
        UT_ASSERT_EQUAL(CompareStatus, 0);
    }

    Btc->BufferToFree = NULL;
    FreePool (b64WorkString);
    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
EFIAPI
RfcDecodeTest(
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
    ) {

    BASIC_TEST_CONTEXT *Btc;
    CHAR8              *b64String;
    CHAR8              *binString;
    EFI_STATUS          Status;
    UINTN               b64StringLen;
    UINTN               ReturnSize;
    UINT8              *BinData;
    UINTN               BinSize;
    INTN                CompareStatus;
    UINTN               indx;


    Btc = (BASIC_TEST_CONTEXT *) Context;
    b64String = Btc->TestInput;
    binString = Btc->TestOutput;

    //
    //  Only testing the the translate functionality
    //

    b64StringLen = AsciiStrnLenS(b64String, MAX_TEST_STRING_SIZE);
    BinSize = AsciiStrnLenS(binString, MAX_TEST_STRING_SIZE);

    // Only allocate the expected buffer, and count on memory
    // protections to catch buffer overruns
    BinData = AllocatePool (Btc->ExpectedSize);
    Btc->BufferToFree = BinData;

    ReturnSize = BinSize;
    Status = Base64Decode(b64String, b64StringLen, BinData, &ReturnSize);

    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);

    // If an error is not expected, check the results
    if (EFI_ERROR(Btc->ExpectedStatus)) {
        if (Btc->ExpectedStatus == EFI_BUFFER_TOO_SMALL) {
            UT_ASSERT_EQUAL(ReturnSize, Btc->ExpectedSize);
        }
    } else {
        UT_ASSERT_EQUAL(ReturnSize, Btc->ExpectedSize);
        if (ReturnSize != 0) {
            CompareStatus = CompareMem(binString, BinData, ReturnSize);
            if (CompareStatus != 0) {
                UT_LOG_ERROR("bin string compare error - size=%d\n", ReturnSize);
                for (indx = 0; indx < ReturnSize; indx++) {
                    UT_LOG_ERROR(" %2.2x", 0xff & binString[indx]);
                }
                UT_LOG_ERROR("\nBinData:\n");
                for (indx = 0; indx < ReturnSize; indx++) {
                    UT_LOG_ERROR(" %2.2x", 0xff & BinData[indx]);
                }
                UT_LOG_ERROR("\n");
            }
            UT_ASSERT_EQUAL(CompareStatus, 0);
        }
    }

    Btc->BufferToFree = NULL;
    FreePool (BinData);
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
  b64UnitTestApp

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
XmlBase64UnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS                 Status;
    UNIT_TEST_FRAMEWORK       *Fw = NULL;
    UNIT_TEST_SUITE           *b64EncodeTests;
    UNIT_TEST_SUITE           *b64DecodeTests;
    CHAR16                     ShortName[100];


    ShortName[0] = L'\0';
    UnicodeSPrint(&ShortName[0], sizeof(ShortName), L"%a", gEfiCallerBaseName);
    DEBUG(( DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework( &Fw, UNIT_TEST_APP_NAME, ShortName, UNIT_TEST_APP_VERSION );
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
        goto EXIT;
    }

    //
    // Populate the B64 Encode Unit Test Suite.
    //
    Status = CreateUnitTestSuite( &b64EncodeTests, Fw, L"b64 Encode binary to Ascii string", L"b64Encode.tests", NULL, NULL );
    if (EFI_ERROR( Status )){
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for b64EncodeTests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

// --------------Suite-----------Description--------------Class Name----------Function--------Pre---Post-------------------Context-----------
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test1", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest1);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test2", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest2);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test3", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest3);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test4", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest4);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test5", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest5);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test6", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest6);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test7", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest7);
    AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test8", RfcEncodeTest, NULL, CleanUpB64TestContext, &mBasicEncodeTest7);

    //
    // Populate the B64 Decode Unit Test Suite.
    //
    Status = CreateUnitTestSuite( &b64DecodeTests, Fw, L"b64 Decode Ascii string to binary", L"b64Decode.tests", NULL, NULL );
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for b64Decode Tests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test1",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest1 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test2",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest2 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test3",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest3 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test4",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest4 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test5",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest5 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test6",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest6 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test7",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest7 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test8",  RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeTest8 );

    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Error1", RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeError1 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Error2", RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeError2 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Error3", RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeError3 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Error4", RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeError4 );
    AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Error5", RfcDecodeTest, NULL, CleanUpB64TestContext, &mBasicDecodeError5 );

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites( Fw );

EXIT:
    if (Fw) {
      FreeUnitTestFramework( Fw );
    }

    return Status;
}