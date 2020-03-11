/**
Unit Tests that verify functionality of XmlTreeQueryLib for element queries

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "XmlTreeQueryLibUnitTests.h"


UNIT_TEST_STATUS
EFIAPI
FindFirstFound(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode* Result = NULL;

  Result = FindFirstChildNodeByName(mNode, "AnotherGen1Node");
  UT_ASSERT_NOT_NULL(Result);

  UT_ASSERT_NOT_NULL(Result->Name);
  UT_ASSERT_EQUAL(AsciiStrLen("AnotherGen1Node"), AsciiStrLen(Result->Name));
  UT_ASSERT_MEM_EQUAL(Result->Name, "AnotherGen1Node", AsciiStrLen("AnotherGen1Node"));

  UT_ASSERT_NOT_NULL(Result->Value);
  UT_ASSERT_EQUAL(AsciiStrLen("Test Data 123"), AsciiStrLen(Result->Value));
  UT_ASSERT_MEM_EQUAL(Result->Value, "Test Data 123", AsciiStrLen("Test Data 123"));

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstNotFound(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode* Result = NULL;

  Result = FindFirstChildNodeByName(mNode, "NotGoingToFindMe");
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstNotFound2(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode* Result = NULL;

  //give a valid name but it is gen2 so it should not be found
  Result = FindFirstChildNodeByName(mNode, "Gen2Node");
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstNullParameters(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode* Result = NULL;

  Result = FindFirstChildNodeByName(NULL, "NotGoingToFindMe");
  UT_ASSERT_TRUE(Result == NULL);

  Result = FindFirstChildNodeByName(mNode, NULL);
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}


EFI_STATUS
EFIAPI
RegisterElementTests(
  IN UNIT_TEST_SUITE_HANDLE TestSuite)
{
  //Test find node
  AddTestCase(TestSuite, "Find 1st Child Node By Name Null Parameters", "FindFirstByName.Null", FindFirstNullParameters, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Child Node By Name Found", "FindFirstByName.Found", FindFirstFound, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Child Node By Name Not Found", "FindFirstByName.NotFound", FindFirstNotFound, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Child Node By Name Not Found 2nd Generation", "FindFirstByName.NotFound2", FindFirstNotFound2, PreReqNodeTreeIsValid, NULL, NULL);

  return EFI_SUCCESS;
}
