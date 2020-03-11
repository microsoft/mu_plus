/**
Unit Tests that verify functionality of XmlTreeQueryLib for attribute queries


Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "XmlTreeQueryLibUnitTests.h"

//
// Attributes Tests
//
UNIT_TEST_STATUS
EFIAPI
FindFirstAttFound(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlAttribute* Result = NULL;
  XmlNode* NodeWithAtt = NULL;

  //2nd child of root has attributes
  NodeWithAtt = (XmlNode*)((mNode->ChildrenListHead.ForwardLink)->ForwardLink);

  Result = FindFirstAttributeByName(NodeWithAtt, "attribute1");
  UT_ASSERT_NOT_NULL(Result);

  UT_ASSERT_NOT_NULL(Result->Name);
  UT_ASSERT_EQUAL(AsciiStrLen("attribute1"), AsciiStrLen(Result->Name));
  UT_ASSERT_MEM_EQUAL(Result->Name, "attribute1", AsciiStrLen("attribute1"));

  UT_ASSERT_NOT_NULL(Result->Value);
  UT_ASSERT_EQUAL(AsciiStrLen("value1"), AsciiStrLen(Result->Value));
  UT_ASSERT_MEM_EQUAL(Result->Value, "value1", AsciiStrLen("value1"));

  //Make sure parent and parent link are equal
  UT_ASSERT_EQUAL(Result->Parent, NodeWithAtt);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstAttFound2(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlAttribute* Result = NULL;
  XmlNode* NodeWithAtt = NULL;

  //2nd child of root has attributes
  NodeWithAtt = (XmlNode*)((mNode->ChildrenListHead.ForwardLink)->ForwardLink);

  Result = FindFirstAttributeByName(NodeWithAtt, "attribute2");
  UT_ASSERT_NOT_NULL(Result);

  UT_ASSERT_NOT_NULL(Result->Name);
  UT_ASSERT_EQUAL(AsciiStrLen("attribute2"), AsciiStrLen(Result->Name));
  UT_ASSERT_MEM_EQUAL(Result->Name, "attribute2", AsciiStrLen("attribute2"));

  UT_ASSERT_NOT_NULL(Result->Value);  //make sure value pointer is good
  UT_ASSERT_EQUAL(AsciiStrLen("value2"), AsciiStrLen(Result->Value)); //Make sure string length is the same
  UT_ASSERT_MEM_EQUAL(Result->Value, "value2", AsciiStrLen("value2")); //Make sure contents are the same

  //Make sure parent and parent link are equal
  UT_ASSERT_EQUAL(Result->Parent, NodeWithAtt);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstAttNotFound(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlAttribute* Result = NULL;
  XmlNode* NodeWithAtt = NULL;

  //2nd child of root has attributes
  NodeWithAtt = (XmlNode*)((mNode->ChildrenListHead.ForwardLink)->ForwardLink);

  Result = FindFirstAttributeByName(NodeWithAtt, "NotGoingToFindMe");
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}

//give a valid attribute name in the tree but not on this element
UNIT_TEST_STATUS
EFIAPI
FindFirstAttNotFound2(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlAttribute* Result = NULL;
  XmlNode* NodeWithAtt = NULL;

  //2nd child of root has attributes
  NodeWithAtt = (XmlNode*)((mNode->ChildrenListHead.ForwardLink)->ForwardLink);

  Result = FindFirstAttributeByName(NodeWithAtt, "attribute2.1");
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}

UNIT_TEST_STATUS
EFIAPI
FindFirstAttNullParameters(
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlAttribute* Result = NULL;
  XmlNode* NodeWithAtt = NULL;

  Result = FindFirstAttributeByName(NULL, "NotGoingToFindMe");
  UT_ASSERT_TRUE(Result == NULL);

  //2nd child of root has attributes
  NodeWithAtt = (XmlNode*)((mNode->ChildrenListHead.ForwardLink)->ForwardLink);

  Result = FindFirstAttributeByName(NodeWithAtt, NULL);
  UT_ASSERT_TRUE(Result == NULL);

  return UNIT_TEST_PASSED;
}


EFI_STATUS
EFIAPI
RegisterAttributeTests(UNIT_TEST_SUITE_HANDLE           TestSuite)
{

  AddTestCase(TestSuite, "Find 1st Attribute By Name Null Parameters", "FindFirstAttribute", FindFirstAttNullParameters, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Attribute By Name Found", "FindFirstAttribute", FindFirstAttFound, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Attribute By Name Found 2nd Attribute", "FindFirstAttribute", FindFirstAttFound2, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st Attribute By Name Not Existing Not Found ", "FindFirstAttribute", FindFirstAttNotFound, PreReqNodeTreeIsValid, NULL, NULL);
  AddTestCase(TestSuite, "Find 1st AttributeBy Name Not Found Different Node", "FindFirstAttribute", FindFirstAttNotFound2, PreReqNodeTreeIsValid, NULL, NULL);

  return EFI_SUCCESS;
}
