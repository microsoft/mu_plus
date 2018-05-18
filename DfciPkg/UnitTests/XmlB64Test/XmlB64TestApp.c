/** @file -- Xmlb64TestApp.c
This is an EFI Shell application to test the b64 conversion routines.


Copyright (c) 2018, Microsoft Corporation.

RFC 4648  https://tools.ietf.org/html/rfc4648 test vectors

   BASE64("") = ""

   BASE64("f") = "Zg=="

   BASE64("fo") = "Zm8="

   BASE64("foo") = "Zm9v"

   BASE64("foob") = "Zm9vYg=="

   BASE64("fooba") = "Zm9vYmE="

   BASE64("foobar") = "Zm9vYmFy"


**/
#include <Uefi.h>
#include <UnitTestTypes.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciBaseStringLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME        L"b64 conversion test cases"
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
#define B64_ERROR_9  "Zm9vymFy="

// Invalid characters in the string
#define B64_ERROR_10  "Zm$vymFy"

// Too many '=' characters
#define B64_ERROR_11 "Z==="

// Poorly placed '='
#define B64_ERROR_12 "Zm=vYmFy"

#define MAX_TEST_STRING_SIZE (200)


typedef struct {
  VOID  *BufferToFree;
} BASE64_TEST_CONTEXT;

static BASE64_TEST_CONTEXT  mBase64TestContext = {NULL};

///================================================================================================
///================================================================================================
///
/// HELPER FUNCTIONS
///
///================================================================================================
///================================================================================================
static

UNIT_TEST_STATUS
EFIAPI
CleanUpTestContext (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {
    BASE64_TEST_CONTEXT *Base64Context = (BASE64_TEST_CONTEXT * ) Context;

    if (Base64Context != NULL) {
        if (Base64Context->BufferToFree != NULL) {
            FreePool(Base64Context->BufferToFree);
            Base64Context->BufferToFree = NULL;
        }
    }
    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
RfcEncodeTest(
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context,
    IN CHAR8                      *binString,   // Binary data to be encoded
    IN CHAR8                      *b64String    // Expected b64 data
    ) {

    EFI_STATUS Status;
    UINT8     *BinData;
    UINTN      BinSize;
    CHAR8     *b64WorkString;
    UINTN      b64StringSize;
    UINTN      ReturnSize;
    INTN       CompareStatus;
    UINTN      indx;
    BASE64_TEST_CONTEXT *b64Context;

    b64Context = (BASE64_TEST_CONTEXT *) Context;
    //
    //  Only testing the the trasnslate functionality, so
    //  pre-allocate the proper string buffer

    BinSize = AsciiStrnLenS(binString, MAX_TEST_STRING_SIZE);
    BinData = (UINT8 *)  binString;

    b64StringSize = AsciiStrnSizeS(b64String, MAX_TEST_STRING_SIZE);
    b64WorkString = (CHAR8 *) AllocatePool(b64StringSize);
    b64Context->BufferToFree = b64WorkString;
    ReturnSize = b64StringSize;
    UT_ASSERT_NOT_NULL(b64WorkString);

    Status = Base64_Encode(binString, BinSize, b64WorkString, &ReturnSize);

    UT_ASSERT_NOT_EFI_ERROR(Status);

    UT_ASSERT_EQUAL(b64StringSize, ReturnSize);

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

    FreePool (b64WorkString);
    b64Context->BufferToFree = NULL;
    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
RfcDecodeTest(
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context,
    IN CHAR8                      *b64String,   // b64 string to be decoded into binary
    IN CHAR8                      *binString    // Expected binary data
    ) {

    EFI_STATUS Status;
    UINTN      b64StringLen;
    UINTN      ReturnSize;
    UINT8     *BinData;
    UINTN      BinSize;
    INTN       CompareStatus;
    UINTN      indx;
    BASE64_TEST_CONTEXT *b64Context;

    b64Context = (BASE64_TEST_CONTEXT *) Context;

    //
    //  Only testing the the trasnslate functionality
    //


    BinSize = AsciiStrnLenS(binString, MAX_TEST_STRING_SIZE);
    BinData = AllocatePool (BinSize);
    b64Context->BufferToFree = BinData;

    b64StringLen = AsciiStrnLenS(b64String, MAX_TEST_STRING_SIZE);

    ReturnSize = BinSize;
    Status = Base64_Decode(b64String, b64StringLen, BinData, &ReturnSize);

    UT_ASSERT_NOT_EFI_ERROR(Status);

    UT_ASSERT_EQUAL(BinSize, ReturnSize);

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

    FreePool (BinData);
    b64Context->BufferToFree = NULL;
    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
RfcDecodeErrorTest(
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context,
    IN CHAR8                      *b64String,    // b64 string with an error
    IN UINT8                      *binString,
    IN EFI_STATUS                  ExpectedStatus
    ) {

    EFI_STATUS Status;
    UINTN      b64StringLen;
    UINTN      ReturnSize;
    UINT8     *BinData;
    UINTN      BinSize;
    BASE64_TEST_CONTEXT *b64Context;

    b64Context = (BASE64_TEST_CONTEXT *) Context;

    //
    //  Only testing the the trasnslate functionality
    //

    BinSize = AsciiStrnLenS(binString, MAX_TEST_STRING_SIZE);
    BinData = AllocatePool (BinSize);
    b64Context->BufferToFree = BinData;

    b64StringLen = AsciiStrnLenS(b64String, MAX_TEST_STRING_SIZE);

    ReturnSize = BinSize;
    Status = Base64_Decode(b64String, b64StringLen, BinData, &ReturnSize);
    UT_ASSERT_STATUS_EQUAL(Status, ExpectedStatus);

    FreePool (BinData);
    b64Context->BufferToFree = NULL;
    return UNIT_TEST_PASSED;
}

///================================================================================================
///================================================================================================
///
/// TEST CASES
///
///================================================================================================
///================================================================================================

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test1 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_1,
                         B64_TEST_1);
} // RFC4686_b64Encode_Test1()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test1 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_1,
                       BIN_TEST_1);
} // RFC4686_b64Decode_Test1()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test2 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_2,
                         B64_TEST_2);
} // RFC4686_b64Encode_Test2()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test2 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_2,
                       BIN_TEST_2);
} // RFC4686_b64Decode_Test2()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test3 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_3,
                         B64_TEST_3);
} // RFC4686_b64Encode_Test3()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test3 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_3,
                       BIN_TEST_3);
} // RFC4686_b64Decode_Test3()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test4 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_4,
                         B64_TEST_4);
} // RFC4686_b64Encode_Test1()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test4 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_4,
                       BIN_TEST_4);
} // RFC4686_b64Decode_Test4()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test5 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_5,
                         B64_TEST_5);
} // RFC4686_b64Encode_Test5()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test5 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_5,
                       BIN_TEST_5);
} // RFC4686_b64Decode_Test5()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test6 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_6,
                         B64_TEST_6);
} // RFC4686_b64Encode_Test6()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test6 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_6,
                       BIN_TEST_6);
} // RFC4686_b64Decode_Test6()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test7 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_7,
                         B64_TEST_7);
} // RFC4686_b64Encode_Test7()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test7 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_7,
                       BIN_TEST_7);
} // RFC4686_b64Decode_Test7()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Encode_Test8 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

    return RfcEncodeTest(Framework,
                         Context,
                         BIN_TEST_8,
                         B64_TEST_8_OUT);
} // RFC4686_b64Encode_Test8()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test8 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeTest(Framework,
                       Context,
                       B64_TEST_8_IN,
                       BIN_TEST_8);
} // RFC4686_b64Decode_Test8()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test9 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeErrorTest(Framework,
                            Context,
                            B64_ERROR_9,
                            B64_ERROR_9,
                            EFI_INVALID_PARAMETER);
} // RFC4686_b64Decode_Test9()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test10 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeErrorTest(Framework,
                            Context,
                            B64_ERROR_10,
                            B64_ERROR_10,
                            EFI_INVALID_PARAMETER);
} // RFC4686_b64Decode_Test10()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test11 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeErrorTest(Framework,
                            Context,
                            B64_ERROR_11,
                            B64_ERROR_11,
                            EFI_INVALID_PARAMETER);
} // RFC4686_b64Decode_Test11()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test12 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeErrorTest(Framework,
                            Context,
                            B64_ERROR_12,
                            B64_ERROR_12,
                            EFI_INVALID_PARAMETER);
} // RFC4686_b64Decode_Test12()

UNIT_TEST_STATUS
EFIAPI
RFC4648_b64Decode_Test13 (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{

  return RfcDecodeErrorTest(Framework,
                            Context,
                            B64_TEST_7,
                            BIN_TEST_1,    // Output smaller than required
                            EFI_BUFFER_TOO_SMALL);
} // RFC4686_b64Decode_Test13()


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
Xmlb64UnitTestApp (
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
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the SimpleMathTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &b64EncodeTests, Fw, L"b64 Encode binary to Ascii string", L"b64Encode.tests", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for b64EncodeTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test1", RFC4648_b64Encode_Test1, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test2", RFC4648_b64Encode_Test2, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test3", RFC4648_b64Encode_Test3, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test4", RFC4648_b64Encode_Test4, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test5", RFC4648_b64Encode_Test5, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test6", RFC4648_b64Encode_Test6, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase( b64EncodeTests, L"RFC 4686 Test Vector", L"b64Encode.Test7", RFC4648_b64Encode_Test7, NULL, CleanUpTestContext, &mBase64TestContext );

  //
  // Populate the GlobalVarTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite( &b64DecodeTests, Fw, L"b64 Decode Ascii string to binary", L"b64Decode.tests", NULL, NULL );
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for b64Decode Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test1", RFC4648_b64Decode_Test1, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test2", RFC4648_b64Decode_Test2, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test3", RFC4648_b64Decode_Test3, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test4", RFC4648_b64Decode_Test4, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test5", RFC4648_b64Decode_Test5, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test6", RFC4648_b64Decode_Test6, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test7", RFC4648_b64Decode_Test7, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test8", RFC4648_b64Decode_Test8, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test9", RFC4648_b64Decode_Test9, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test10", RFC4648_b64Decode_Test10, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test11", RFC4648_b64Decode_Test11, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test12", RFC4648_b64Decode_Test12, NULL, CleanUpTestContext, &mBase64TestContext );
  AddTestCase(b64DecodeTests, L"RFC 4686 Test Vector", L"b64Decode.Test13", RFC4648_b64Decode_Test13, NULL, CleanUpTestContext, &mBase64TestContext );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites( Fw );

EXIT:
  if (Fw)
  {
    FreeUnitTestFramework( Fw );
  }

  return Status;
}