/** @file
JsonTestApp.c

This is a Unit Test for the Json Lite Parser library.

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
#include <Library/JsonLiteParser.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UnitTestAssertLib.h>
#include <Library/UnitTestLogLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME        L"Json Lite test cases"
#define UNIT_TEST_APP_VERSION     L"1.0"

/**

JsonLite parsing rules:

       +->-------------------------------->-+
|->-{--+->-+->---STRING---:---VALUE--->-+->-+->--}-|
           +-<------------,-----------<-+

   -        represents white space (' ', '\r', '\n', '\t')
   >        direction to the right
   <        direction to the left
   +        indicates a switch
   {}       required characters
   :        required to separate string from value
   ,        required to separate pairs of data
   STRING   string of in quotes - no escape characters
   VALUE    string in quotes - no escape characters

   No comments are allowed

Good examples:

   { "String" : "Value" , "String2" : "Value2" }
   {"String":"Value","String2":"Value2"}
   {"String1": null, "String2" : "Value2" }
   {"String2":12345, "String3" : null}

Bad examples:

  "String" : "Value"
  {"String"}
  {"String","String":"Value"}
  {"String1" : 123abc, "String2" : 12345}

 */
EFI_STATUS
EFIAPI
JsonProcessFunction (
    IN  JSON_REQUEST_ELEMENT *JsonElement,
    IN  VOID                 *Context
);
#define MAX_APPLY_ELEMENTS 64

static JSON_REQUEST_ELEMENT mApplyElements[MAX_APPLY_ELEMENTS];
static INTN                 mApplyElementCount;

typedef struct {
    CHAR8                *JsonString;
    UINTN                 JsonStringSize;
    EFI_STATUS            ExpectedStatus;
    JSON_REQUEST_ELEMENT *ExpectedResults;
    INTN                  ExpectedCount;
    CHAR8                *BufferToFree;
} BASIC_TEST_CONTEXT;

//*----------------------------------------------------------------------------------*
// Decode Test 1 = Simple test of two string:value pairs                             *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_1_JSON     "{ \"String\" : \"Value\" , \"String2\" : \"Value2\" }"
#define DEC_TEST_1_1_String "String"
#define DEC_TEST_1_1_Value  "Value"
#define DEC_TEST_1_2_String "String2"
#define DEC_TEST_1_2_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest1Elements[] = {
    {DEC_TEST_1_1_String, sizeof(DEC_TEST_1_1_String) - sizeof (CHAR8), DEC_TEST_1_1_Value, sizeof(DEC_TEST_1_1_Value) - sizeof (CHAR8)},
    {DEC_TEST_1_2_String, sizeof(DEC_TEST_1_2_String) - sizeof (CHAR8), DEC_TEST_1_2_Value, sizeof(DEC_TEST_1_2_Value) - sizeof (CHAR8)}
};
#define mParseTest1ElementCount (sizeof(mParseTest1Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*---------------------------------------------------------------------------------------*
// Decode Test 2 = Same as test 1 with extra spaces, tabs, newlines and carriage returns. *
//*---------------------------------------------------------------------------------------*
#define DEC_TEST_2_JSON     "  { \t \"String\" \r \n: \"Value\" ,\t  \"String2\" : \"Value2\"\n\r }"
#define DEC_TEST_2_1_String "String"
#define DEC_TEST_2_1_Value  "Value"
#define DEC_TEST_2_2_String "String2"
#define DEC_TEST_2_2_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest2Elements[] = {
    {DEC_TEST_2_1_String, sizeof(DEC_TEST_2_1_String) - sizeof (CHAR8), DEC_TEST_2_1_Value, sizeof(DEC_TEST_2_1_Value) - sizeof (CHAR8)},
    {DEC_TEST_2_2_String, sizeof(DEC_TEST_2_2_String) - sizeof (CHAR8), DEC_TEST_2_2_Value, sizeof(DEC_TEST_2_2_Value) - sizeof (CHAR8)}
};
#define mParseTest2ElementCount (sizeof(mParseTest2Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 3 = Missing {                                                         *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_3_JSON     "\"String\" : \"Value\" , \"String2\" : \"Value2\" }"
#define mParseTest3ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 4 = Missing first " in first tuple string                             *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_4_JSON     "{String\" : \"Value\" , \"String2\" : \"Value2\" }"
#define mParseTest4ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 5 = Missing second " in first tuple string                            *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_5_JSON     "{\"String : \"Value\" , \"String2\" : \"Value2\" }"
#define mParseTest5ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 6 = Missing  : in first tuple                                         *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_6_JSON     "{\"String\"  \"Value\" , \"String2\" : \"Value2\" }"
#define mParseTest6ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 7 = Missing first " in first tuple value                              *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_7_JSON     "{\"String\" : Value\" , \"String2\" : \"Value2\" }"
#define mParseTest7ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 8 = Missing second " in first tuple value - actually finds next quote *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_8_JSON     "{\"String\" : \"Value , \"String2\" : \"Value2\" }"
#define DEC_TEST_8_1_String "String"
#define DEC_TEST_8_1_Value  "Value , "

static JSON_REQUEST_ELEMENT mParseTest8Elements[] = {
    {DEC_TEST_8_1_String, sizeof(DEC_TEST_8_1_String) - sizeof (CHAR8), DEC_TEST_8_1_Value, sizeof(DEC_TEST_8_1_Value) - sizeof (CHAR8)},
};
#define mParseTest8ElementCount (sizeof(mParseTest8Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 9 = Missing  , between tuples                                         *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_9_JSON     "{\"String\" : \"Value\" ? \"String2\" : \"Value2\" }"
#define DEC_TEST_9_1_String "String"
#define DEC_TEST_9_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest9Elements[] = {
    {DEC_TEST_9_1_String, sizeof(DEC_TEST_9_1_String) - sizeof (CHAR8), DEC_TEST_9_1_Value, sizeof(DEC_TEST_9_1_Value) - sizeof (CHAR8)},
};
#define mParseTest9ElementCount (sizeof(mParseTest9Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 10 = Missing first " in second tuple string                           *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_10_JSON     "{\"String\" : \"Value\" , String2\" : \"Value2\" }"
#define DEC_TEST_10_1_String "String"
#define DEC_TEST_10_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest10Elements[] = {
    {DEC_TEST_10_1_String, sizeof(DEC_TEST_10_1_String) - sizeof (CHAR8), DEC_TEST_10_1_Value, sizeof(DEC_TEST_10_1_Value) - sizeof (CHAR8)},
};
#define mParseTest10ElementCount (sizeof(mParseTest10Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 11 = Missing second " in second tuple string                            *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_11_JSON     "{\"String\" : \"Value\" , \"String2 : \"Value2\" }"
#define DEC_TEST_11_1_String "String"
#define DEC_TEST_11_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest11Elements[] = {
    {DEC_TEST_11_1_String, sizeof(DEC_TEST_11_1_String) - sizeof (CHAR8), DEC_TEST_11_1_Value, sizeof(DEC_TEST_11_1_Value) - sizeof (CHAR8)},
};
#define mParseTest11ElementCount (sizeof(mParseTest11Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 12 = Missing  : in second tuple                                         *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_12_JSON     "{\"String\" : \"Value\" , \"String2\" ? \"Value2\" }"
#define DEC_TEST_12_1_String "String"
#define DEC_TEST_12_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest12Elements[] = {
    {DEC_TEST_12_1_String, sizeof(DEC_TEST_12_1_String) - sizeof (CHAR8), DEC_TEST_12_1_Value, sizeof(DEC_TEST_12_1_Value) - sizeof (CHAR8)},
};
#define mParseTest12ElementCount (sizeof(mParseTest12Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 13 = Missing first " in second tuple value                            *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_13_JSON     "{\"String\" : \"Value\" , \"String2\" : Value2\" }"
#define DEC_TEST_13_1_String "String"
#define DEC_TEST_13_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest13Elements[] = {
    {DEC_TEST_13_1_String, sizeof(DEC_TEST_13_1_String) - sizeof (CHAR8), DEC_TEST_13_1_Value, sizeof(DEC_TEST_13_1_Value) - sizeof (CHAR8)},
};
#define mParseTest13ElementCount (sizeof(mParseTest13Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 14 = Missing second " in second tuple value                           *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_14_JSON     "{\"String\" : \"Value\" , \"String2\" : \"Value2 }"
#define DEC_TEST_14_1_String "String"
#define DEC_TEST_14_1_Value  "Value"

static JSON_REQUEST_ELEMENT mParseTest14Elements[] = {
    {DEC_TEST_14_1_String, sizeof(DEC_TEST_14_1_String) - sizeof (CHAR8), DEC_TEST_14_1_Value, sizeof(DEC_TEST_14_1_Value) - sizeof (CHAR8)},
};
#define mParseTest14ElementCount (sizeof(mParseTest14Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 15 = Missing  } between tuples                                         *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_15_JSON     "{\"String\" : \"Value\" , \"String2\" : \"Value2\" "
#define DEC_TEST_15_1_String "String"
#define DEC_TEST_15_1_Value  "Value"
#define DEC_TEST_15_2_String "String2"
#define DEC_TEST_15_2_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest15Elements[] = {
    {DEC_TEST_15_1_String, sizeof(DEC_TEST_15_1_String) - sizeof (CHAR8), DEC_TEST_15_1_Value, sizeof(DEC_TEST_15_1_Value) - sizeof (CHAR8)},
    {DEC_TEST_15_2_String, sizeof(DEC_TEST_15_2_String) - sizeof (CHAR8), DEC_TEST_15_2_Value, sizeof(DEC_TEST_15_2_Value) - sizeof (CHAR8)}
};
#define mParseTest15ElementCount (sizeof(mParseTest15Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 16 = Skip tuples when value == "null"                                 *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_16_JSON     "{\"String1\" :null, \"String2\" : \"Value2\" "
#define DEC_TEST_16_1_String "String2"
#define DEC_TEST_16_1_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest16Elements[] = {
    {DEC_TEST_16_1_String, sizeof(DEC_TEST_16_1_String) - sizeof (CHAR8), DEC_TEST_16_1_Value, sizeof(DEC_TEST_16_1_Value) - sizeof (CHAR8)}
};
#define mParseTest16ElementCount (sizeof(mParseTest16Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 17 = Skip tuples when value == "null"                                 *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_17_JSON     "{\"String1\" : null ,\"String2\" : \"Value2\" "
#define DEC_TEST_17_1_String "String2"
#define DEC_TEST_17_1_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest17Elements[] = {
    {DEC_TEST_17_1_String, sizeof(DEC_TEST_17_1_String) - sizeof (CHAR8) - sizeof (CHAR8), DEC_TEST_17_1_Value, sizeof(DEC_TEST_17_1_Value) - sizeof (CHAR8)}
};
#define mParseTest17ElementCount (sizeof(mParseTest17Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 18  Skip tuples when value == "null"                                 *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_18_JSON     "{\"String1\" : null, \"String2\" : \"Value2\" "
#define DEC_TEST_18_1_String "String2"
#define DEC_TEST_18_1_Value  "Value2"

static JSON_REQUEST_ELEMENT mParseTest18Elements[] = {
    {DEC_TEST_18_1_String, sizeof(DEC_TEST_18_1_String) - sizeof (CHAR8), DEC_TEST_18_1_Value, sizeof(DEC_TEST_18_1_Value) - sizeof (CHAR8)}
};
#define mParseTest18ElementCount (sizeof(mParseTest18Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 19= NULL before second quote String                                   *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_19_JSON     "{\"String"
#define mParseTest19ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 20 = NULL before second quote Value                                   *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_20_JSON     "{\"String\" : \"Value"
#define mParseTest20ElementCount 0

//*----------------------------------------------------------------------------------*
// Decode Test 21  Validate decimal digits                                           *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_21_JSON     "{\"String1\" : 1234AFZ, \"String2\" : \"Value2\" "
#define DEC_TEST_21_1_String "String1"
#define DEC_TEST_21_1_Value  "1234"

static JSON_REQUEST_ELEMENT mParseTest21Elements[] = {
    {DEC_TEST_21_1_String, sizeof(DEC_TEST_21_1_String) - sizeof (CHAR8), DEC_TEST_21_1_Value, sizeof(DEC_TEST_21_1_Value) - sizeof (CHAR8)}
};
#define mParseTest21ElementCount (sizeof(mParseTest21Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Decode Test 22  Validate decimal digits                                           *
//*----------------------------------------------------------------------------------*
#define DEC_TEST_22_JSON     "{\"String1\" : 1234, \"String2\" : F56789"
#define DEC_TEST_22_1_String "String1"
#define DEC_TEST_22_1_Value  "1234"

static JSON_REQUEST_ELEMENT mParseTest22Elements[] = {
    {DEC_TEST_22_1_String, sizeof(DEC_TEST_22_1_String) - sizeof (CHAR8), DEC_TEST_22_1_Value, sizeof(DEC_TEST_22_1_Value) - sizeof (CHAR8)}
};
#define mParseTest22ElementCount (sizeof(mParseTest22Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------------------------------------*
// Encode Test 1 = Validate some data                                                                              *
//*----------------------------------------------------------------------------------------------------------------*
#define ENC_TEST_1_JSON     "{\"String\":\"Value\",\"String2\":\"Value2\",\"String3\":\"9123456780\"}"
#define ENC_TEST_1_1_String "String"
#define ENC_TEST_1_1_Value  "Value"
#define ENC_TEST_1_2_String "String2"
#define ENC_TEST_1_2_Value  "Value2"
#define ENC_TEST_1_3_String "String3"
#define ENC_TEST_1_3_Value  "9123456780"

static JSON_REQUEST_ELEMENT mEncodeTest1Elements[] = {
    {ENC_TEST_1_1_String, sizeof(ENC_TEST_1_1_String) - sizeof (CHAR8), ENC_TEST_1_1_Value, sizeof(ENC_TEST_1_1_Value) - sizeof (CHAR8)},
    {ENC_TEST_1_2_String, sizeof(ENC_TEST_1_2_String) - sizeof (CHAR8), ENC_TEST_1_2_Value, sizeof(ENC_TEST_1_2_Value) - sizeof (CHAR8)},
    {ENC_TEST_1_3_String, sizeof(ENC_TEST_1_3_String) - sizeof (CHAR8), ENC_TEST_1_3_Value, sizeof(ENC_TEST_1_3_Value) - sizeof (CHAR8)}
};
#define mEncodeTest1ElementCount (sizeof(mEncodeTest1Elements)/sizeof(JSON_REQUEST_ELEMENT))

//*----------------------------------------------------------------------------------*
// Encode Test 2 = Send in NULL for request array                                    *
//*----------------------------------------------------------------------------------*
#define mEncodeTest2ElementCount 0

//*----------------------------------------------------------------------------------*
// Encode Test 3 = Set the element array count to 0                                  *
//*----------------------------------------------------------------------------------*
static JSON_REQUEST_ELEMENT mEncodeTest3Elements[] = { NULL, 0 , NULL, 0 };
#define mEncodeTest3ElementCount 0

//*----------------------------------------------------------------------------------*
//* Test Contexts                                                                    *
//*----------------------------------------------------------------------------------*
static BASIC_TEST_CONTEXT mParseTest1  = {DEC_TEST_1_JSON,  sizeof(DEC_TEST_1_JSON),  EFI_SUCCESS,           mParseTest1Elements,  mParseTest1ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest2  = {DEC_TEST_2_JSON,  sizeof(DEC_TEST_2_JSON),  EFI_SUCCESS,           mParseTest2Elements,  mParseTest2ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest3  = {DEC_TEST_3_JSON,  sizeof(DEC_TEST_3_JSON),  EFI_INVALID_PARAMETER, NULL,                 mParseTest3ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest4  = {DEC_TEST_4_JSON,  sizeof(DEC_TEST_4_JSON),  EFI_INVALID_PARAMETER, NULL,                 mParseTest4ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest5  = {DEC_TEST_5_JSON,  sizeof(DEC_TEST_5_JSON),  EFI_INVALID_PARAMETER, NULL,                 mParseTest5ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest6  = {DEC_TEST_6_JSON,  sizeof(DEC_TEST_6_JSON),  EFI_INVALID_PARAMETER, NULL,                 mParseTest6ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest7  = {DEC_TEST_7_JSON,  sizeof(DEC_TEST_7_JSON),  EFI_INVALID_PARAMETER, NULL,                 mParseTest7ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest8  = {DEC_TEST_8_JSON,  sizeof(DEC_TEST_8_JSON),  EFI_INVALID_PARAMETER, mParseTest8Elements,  mParseTest8ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest9  = {DEC_TEST_9_JSON,  sizeof(DEC_TEST_9_JSON),  EFI_INVALID_PARAMETER, mParseTest9Elements,  mParseTest9ElementCount,  NULL};
static BASIC_TEST_CONTEXT mParseTest10 = {DEC_TEST_10_JSON, sizeof(DEC_TEST_10_JSON), EFI_INVALID_PARAMETER, mParseTest10Elements, mParseTest10ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest11 = {DEC_TEST_11_JSON, sizeof(DEC_TEST_11_JSON), EFI_INVALID_PARAMETER, mParseTest11Elements, mParseTest11ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest12 = {DEC_TEST_12_JSON, sizeof(DEC_TEST_12_JSON), EFI_INVALID_PARAMETER, mParseTest12Elements, mParseTest12ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest13 = {DEC_TEST_13_JSON, sizeof(DEC_TEST_13_JSON), EFI_INVALID_PARAMETER, mParseTest13Elements, mParseTest13ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest14 = {DEC_TEST_14_JSON, sizeof(DEC_TEST_14_JSON), EFI_INVALID_PARAMETER, mParseTest14Elements, mParseTest14ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest15 = {DEC_TEST_15_JSON, sizeof(DEC_TEST_15_JSON), EFI_INVALID_PARAMETER, mParseTest15Elements, mParseTest15ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest16 = {DEC_TEST_16_JSON, sizeof(DEC_TEST_16_JSON), EFI_INVALID_PARAMETER, mParseTest16Elements, mParseTest16ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest17 = {DEC_TEST_17_JSON, sizeof(DEC_TEST_17_JSON), EFI_INVALID_PARAMETER, mParseTest17Elements, mParseTest17ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest18 = {DEC_TEST_18_JSON, sizeof(DEC_TEST_18_JSON), EFI_INVALID_PARAMETER, mParseTest18Elements, mParseTest18ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest19 = {DEC_TEST_19_JSON, sizeof(DEC_TEST_19_JSON), EFI_INVALID_PARAMETER, NULL,                 mParseTest19ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest20 = {DEC_TEST_20_JSON, sizeof(DEC_TEST_20_JSON), EFI_INVALID_PARAMETER, NULL,                 mParseTest20ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest21 = {DEC_TEST_21_JSON, sizeof(DEC_TEST_21_JSON), EFI_INVALID_PARAMETER, mParseTest21Elements, mParseTest21ElementCount, NULL};
static BASIC_TEST_CONTEXT mParseTest22 = {DEC_TEST_22_JSON, sizeof(DEC_TEST_22_JSON), EFI_INVALID_PARAMETER, mParseTest22Elements, mParseTest22ElementCount, NULL};

static BASIC_TEST_CONTEXT mEncodeTest1 = {ENC_TEST_1_JSON,  sizeof(ENC_TEST_1_JSON),  EFI_SUCCESS,           mEncodeTest1Elements, mEncodeTest1ElementCount, NULL};

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


    BASIC_TEST_CONTEXT *Btc;
    Btc = (BASIC_TEST_CONTEXT *) Context;

    if (NULL != Btc->BufferToFree) {
        FreePool (Btc->BufferToFree);
        Btc->BufferToFree = NULL;
    }

    return UNIT_TEST_PASSED;
}


//
// Apply function. Copy the JSON element to the result array
//
EFI_STATUS
EFIAPI
JsonProcessFunction (
    IN  JSON_REQUEST_ELEMENT *JsonElement,
    IN  VOID                 *Context
  ) {


    if (mApplyElementCount == MAX_APPLY_ELEMENTS) {
        DEBUG((DEBUG_ERROR, "Too many calls to ApplyFunction\n"));
        return EFI_BUFFER_TOO_SMALL;
    }
    DEBUG((DEBUG_INFO,"Process %p index=%d\n", JsonElement, mApplyElementCount));
    CopyMem (&mApplyElements[mApplyElementCount], JsonElement, sizeof(JSON_REQUEST_ELEMENT));
    mApplyElementCount++;

    return EFI_SUCCESS;
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
JsonParseTest (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
    ) {

    BASIC_TEST_CONTEXT *Btc;
    INTN                i;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    mApplyElementCount = 0;

    DEBUG(( DEBUG_INFO, "Processing Test: %a\n",Btc->JsonString));
    if (((Btc->ExpectedResults == NULL) && (Btc->ExpectedCount != 0)) ||
        ((Btc->ExpectedResults != NULL) && (Btc->ExpectedCount == 0)) ) {
        UT_LOG_INFO ("JsonLibParse invalid test setup. Results = %p, Count=%d\n", Btc->ExpectedResults, Btc->ExpectedCount);
        UT_ASSERT_TRUE (FALSE);
    }

    Status = JsonLibParse (Btc->JsonString, Btc->JsonStringSize, JsonProcessFunction, NULL);

    UT_LOG_INFO ("JsonLibParse returned %r, expected %r\n", Status, Btc->ExpectedStatus);
    UT_LOG_INFO ("ApplyCount = %d, ExpectedCount = %d\n", mApplyElementCount, Btc->ExpectedCount);

    UT_ASSERT_TRUE (Status == Btc->ExpectedStatus);
    // The apply function may succeed a number of times before the expected error.  Check
    // the valid results before checking the Parse error code.

    UT_ASSERT_TRUE (mApplyElementCount <= Btc->ExpectedCount);
    if (!EFI_ERROR(Status)) {
      for (i = 0; i < mApplyElementCount; i++) {
          DEBUG((DEBUG_INFO, "Processing element %d\n", i));
          UT_LOG_INFO ("Processing element %d\n", i);

          UT_LOG_INFO ("Expected FieldLen = %d, Actual FieldLen = %d\n", Btc->ExpectedResults[i].FieldLen, mApplyElements[i].FieldLen);
          UT_LOG_INFO ("Actual Field   = %.*a\n",  mApplyElements[i].FieldLen, mApplyElements[i].FieldName);
          UT_LOG_INFO ("Expected Field = %.*a\n", Btc->ExpectedResults[i].FieldLen, Btc->ExpectedResults[i].FieldName);

          UT_LOG_INFO ("Expected ValueSize = %d, Actual ValueSize = %d\n", Btc->ExpectedResults[i].ValueLen, mApplyElements[i].ValueLen);
          UT_LOG_INFO ("Actual Value   = %.*a\n", mApplyElements[i].ValueLen, mApplyElements[i].Value);
          UT_LOG_INFO ("Expected Value = %.*a\n", Btc->ExpectedResults[i].ValueLen, Btc->ExpectedResults[i].Value);

          UT_ASSERT_TRUE (Btc->ExpectedResults[i].FieldLen == mApplyElements[i].FieldLen);
          UT_ASSERT_TRUE (0 == AsciiStrnCmp(Btc->ExpectedResults[i].FieldName, mApplyElements[i].FieldName, mApplyElements[i].FieldLen));

          UT_ASSERT_TRUE (Btc->ExpectedResults[i].ValueLen == mApplyElements[i].ValueLen);
          UT_ASSERT_TRUE (0 == AsciiStrnCmp(Btc->ExpectedResults[i].Value, mApplyElements[i].Value, mApplyElements[i].ValueLen));
      }
    }

    UT_ASSERT_TRUE (mApplyElementCount == Btc->ExpectedCount);

    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);

    DEBUG((DEBUG_INFO, "Test completed successfully\n"));

    return UNIT_TEST_PASSED;
}

static
UNIT_TEST_STATUS
JsonEncodeTest (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
    ) {

    BASIC_TEST_CONTEXT *Btc;
    UINTN               NewStringSize;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    DEBUG(( DEBUG_INFO, "Processing Encode test: %a\n", Btc->ExpectedResults));

    Status = JsonLibEncode (Btc->ExpectedResults, Btc->ExpectedCount, &Btc->BufferToFree, &NewStringSize);

    UT_LOG_INFO ("JsonLibEncode returned %r, expected %r\n", Status, Btc->ExpectedStatus);

    UT_ASSERT_STATUS_EQUAL(Status, Btc->ExpectedStatus);

    if (!EFI_ERROR(Status)) {
        UT_LOG_INFO ("NewStringSize = %d, ExpectedSize = %d\n", NewStringSize, Btc->JsonStringSize);
        UT_LOG_INFO ("NewString       = %a\n", Btc->BufferToFree);
        UT_LOG_INFO ("Expected String = %a\n", Btc->JsonString);

        UT_ASSERT_TRUE (NewStringSize == Btc->JsonStringSize);

        UT_ASSERT_TRUE (0 == AsciiStrnCmp (Btc->JsonString, Btc->BufferToFree, NewStringSize));
    }

    DEBUG((DEBUG_INFO, "Encode test completed successfully\n"));

    return UNIT_TEST_PASSED;
}

/**
  Decode NULL test P1

  This test uses a valid test context, but passes NULL for the first parameter

  */
static
UNIT_TEST_STATUS
JsonParseNullP1 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing NullP1\n"));

    Status = JsonLibParse (NULL, Btc->JsonStringSize, JsonProcessFunction, NULL);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Decode NULL test P2

  This test uses a valid test context, but passes 0 for the second parameter

  */
static
UNIT_TEST_STATUS
JsonParseNullP2 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;

    DEBUG(( DEBUG_INFO, "Processing NullP2\n"));

    Status = JsonLibParse (Btc->JsonString, 0, JsonProcessFunction, NULL);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Decode NULL test P3

  This test uses a valid test context, but passes NULL for the third parameter

  */
static
UNIT_TEST_STATUS
JsonParseNullP3 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing NullP3\n"));

    Status = JsonLibParse (Btc->JsonString, Btc->JsonStringSize, NULL, NULL);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Encode NULL test

  This test uses a valid test context, but passes NULL for the first parameter

  */
static
UNIT_TEST_STATUS
JsonEncodeNullP1 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    UINTN               NewStringSize;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing EncodeNullP1\n"));

    Status = JsonLibEncode (NULL, Btc->ExpectedCount, &Btc->BufferToFree, &NewStringSize);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Encode NULL test

  This test uses a valid test context, but passes NULL for the first parameter

  */
static
UNIT_TEST_STATUS
JsonEncodeNullP2 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    UINTN               NewStringSize;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing EncodeNullP2\n"));
    Status = JsonLibEncode (Btc->ExpectedResults, 0, &Btc->BufferToFree, &NewStringSize);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Encode NULL test

  This test uses a valid test context, but passes NULL for the first parameter

  */
static
UNIT_TEST_STATUS
JsonEncodeNullP3 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    UINTN               NewStringSize;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing EncodeNullP3\n"));
    Status = JsonLibEncode (Btc->ExpectedResults, Btc->ExpectedCount, NULL, &NewStringSize);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

    return UNIT_TEST_PASSED;
}

/**
  Encode NULL test

  This test uses a valid test context, but passes NULL for the first parameter

  */
static
UNIT_TEST_STATUS
JsonEncodeNullP4 (
    IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
    IN UNIT_TEST_CONTEXT           Context
  ) {

    BASIC_TEST_CONTEXT *Btc;
    EFI_STATUS          Status;

    Btc = (BASIC_TEST_CONTEXT *) Context;
    DEBUG(( DEBUG_INFO, "Processing EncodeNullP4\n"));
    Status = JsonLibEncode (Btc->ExpectedResults, Btc->ExpectedCount, &Btc->BufferToFree, NULL);
    UT_ASSERT_STATUS_EQUAL(Status, EFI_INVALID_PARAMETER);

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
  JsonTestAppEntry

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
JsonTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    UNIT_TEST_FRAMEWORK       *Fw = NULL;
    UNIT_TEST_SUITE           *JsonParseTests;
    UNIT_TEST_SUITE           *JsonEncodeTests;
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
    // Populate the Json Library Test Suite.
    //
    Status = CreateUnitTestSuite( &JsonParseTests, Fw, L"Parse Json to individual components", L"JSON.Parse", NULL, NULL);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Parse JsonTests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    //-----------Suite-----------Description------------Class-----------------Test Function--Pre---Clean-Context
    AddTestCase( JsonParseTests, L"Json Parse Test 1",  L"JSON.Parse.Test1",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest1);
    AddTestCase( JsonParseTests, L"Json Parse Test 2",  L"JSON.Parse.Test2",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest2);
    AddTestCase( JsonParseTests, L"Json Parse Test 3",  L"JSON.Parse.Test3",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest3);
    AddTestCase( JsonParseTests, L"Json Parse Test 4",  L"JSON.Parse.Test4",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest4);
    AddTestCase( JsonParseTests, L"Json Parse Test 5",  L"JSON.Parse.Test5",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest5);
    AddTestCase( JsonParseTests, L"Json Parse Test 6",  L"JSON.Parse.Test6",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest6);
    AddTestCase( JsonParseTests, L"Json Parse Test 7",  L"JSON.Parse.Test7",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest7);
    AddTestCase( JsonParseTests, L"Json Parse Test 8",  L"JSON.Parse.Test8",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest8);
    AddTestCase( JsonParseTests, L"Json Parse Test 9",  L"JSON.Parse.Test9",  JsonParseTest, NULL, CleanUpTestContext, &mParseTest9);
    AddTestCase( JsonParseTests, L"Json Parse Test 10", L"JSON.Parse.Test10", JsonParseTest, NULL, CleanUpTestContext, &mParseTest10);
    AddTestCase( JsonParseTests, L"Json Parse Test 11", L"JSON.Parse.Test11", JsonParseTest, NULL, CleanUpTestContext, &mParseTest11);
    AddTestCase( JsonParseTests, L"Json Parse Test 12", L"JSON.Parse.Test12", JsonParseTest, NULL, CleanUpTestContext, &mParseTest12);
    AddTestCase( JsonParseTests, L"Json Parse Test 13", L"JSON.Parse.Test13", JsonParseTest, NULL, CleanUpTestContext, &mParseTest13);
    AddTestCase( JsonParseTests, L"Json Parse Test 14", L"JSON.Parse.Test14", JsonParseTest, NULL, CleanUpTestContext, &mParseTest14);
    AddTestCase( JsonParseTests, L"Json Parse Test 15", L"JSON.Parse.Test15", JsonParseTest, NULL, CleanUpTestContext, &mParseTest15);
    AddTestCase( JsonParseTests, L"Json Parse Test 16", L"JSON.Parse.Test16", JsonParseTest, NULL, CleanUpTestContext, &mParseTest16);
    AddTestCase( JsonParseTests, L"Json Parse Test 17", L"JSON.Parse.Test17", JsonParseTest, NULL, CleanUpTestContext, &mParseTest17);
    AddTestCase( JsonParseTests, L"Json Parse Test 18", L"JSON.Parse.Test18", JsonParseTest, NULL, CleanUpTestContext, &mParseTest18);
    AddTestCase( JsonParseTests, L"Json Parse Test 19", L"JSON.Parse.Test19", JsonParseTest, NULL, CleanUpTestContext, &mParseTest19);
    AddTestCase( JsonParseTests, L"Json Parse Test 20", L"JSON.Parse.Test20", JsonParseTest, NULL, CleanUpTestContext, &mParseTest20);
    AddTestCase( JsonParseTests, L"Json Parse Test 21", L"JSON.Parse.Test21", JsonParseTest, NULL, CleanUpTestContext, &mParseTest21);
    AddTestCase( JsonParseTests, L"Json Parse Test 22", L"JSON.Parse.Test22", JsonParseTest, NULL, CleanUpTestContext, &mParseTest22);

    AddTestCase( JsonParseTests, L"Json Parse NULL Test 1", L"JSON.Parse.NullTest1", JsonParseNullP1, NULL, CleanUpTestContext, &mParseTest1);
    AddTestCase( JsonParseTests, L"Json Parse NULL Test 2", L"JSON.Parse.NullTest2", JsonParseNullP2, NULL, CleanUpTestContext, &mParseTest1);
    AddTestCase( JsonParseTests, L"Json Parse NULL Test 3", L"JSON.Parse.NullTest3", JsonParseNullP3, NULL, CleanUpTestContext, &mParseTest1);

    //
    // Populate the GlobalVarTests Unit Test Suite.
    //
    Status = CreateUnitTestSuite( &JsonEncodeTests, Fw, L"Encode elements into a Json string", L"JSON.Encode", NULL, NULL);
    if (EFI_ERROR( Status )) {
        DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for Encode Json Tests\n"));
        Status = EFI_OUT_OF_RESOURCES;
        goto EXIT;
    }

    AddTestCase(JsonEncodeTests, L"Json Encode Test 1", L"JSON.EncodeTest1", JsonEncodeTest,   NULL, CleanUpTestContext, &mEncodeTest1);
    AddTestCase(JsonEncodeTests, L"Json Encode Test 2", L"JSON.EncodeTest2", JsonEncodeNullP1, NULL, CleanUpTestContext, &mEncodeTest1);
    AddTestCase(JsonEncodeTests, L"Json Encode Test 3", L"JSON.EncodeTest3", JsonEncodeNullP2, NULL, CleanUpTestContext, &mEncodeTest1);
    AddTestCase(JsonEncodeTests, L"Json Encode Test 4", L"JSON.EncodeTest4", JsonEncodeNullP3, NULL, CleanUpTestContext, &mEncodeTest1);
    AddTestCase(JsonEncodeTests, L"Json Encode Test 5", L"JSON.EncodeTest5", JsonEncodeNullP4, NULL, CleanUpTestContext, &mEncodeTest1);

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