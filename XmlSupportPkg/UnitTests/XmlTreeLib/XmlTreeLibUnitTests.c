/**
@file
UEFI Shell based application for unit testing the XmlTreeLib.  


Copyright (c) 2016, Microsoft Corporation

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
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <UnitTestTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/UnitTestAssertLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include "TestData.h"


#define UNIT_TEST_APP_NAME        L"XML Lib Unit Test Application"
#define UNIT_TEST_APP_SHORT_NAME  L"XML_Lib_Unit_Test_App"
#define UNIT_TEST_APP_VERSION     L"0.1"


/**
Simple clean up method to make sure string parsing tests clean up even if interrupted and fail in the middle.
**/
UNIT_TEST_STATUS
EFIAPI
CleanUpXmlStringParseContext (
IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
IN UNIT_TEST_CONTEXT           Context
)
{
XmlStringParseContext *XmlContext = (XmlStringParseContext*)Context;

if (XmlContext != NULL)
{
  //free string if set
  if (XmlContext->String != NULL)
  {
    FreePool(XmlContext->String);
    XmlContext->String = NULL;
  }
}
return UNIT_TEST_PASSED;
}

/**
Simple clean up method to make sure tests clean up even if interrupted and fail in the middle.
**/
UNIT_TEST_STATUS
EFIAPI
CleanUpXmlTestContext(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlTestContext *XmlContext = (XmlTestContext*)Context;

  if (XmlContext != NULL)
  {
    //free string if set
    if (XmlContext->ToFreeXmlString != NULL)
    {
      FreePool(XmlContext->ToFreeXmlString);
      XmlContext->ToFreeXmlString = NULL;
    }

    //free xml node tree
    if (XmlContext->Node != NULL)
    {
      FreeXmlTree(&(XmlContext->Node));
    }
  }
  return UNIT_TEST_PASSED;
}

/**
Simple Test method that can be used to validate lots of different XML strings.
**/
UNIT_TEST_STATUS
EFIAPI
ParseValidXml(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode *ResultData = NULL;
  XmlTestContext *XmlContext = (XmlTestContext*)Context;
  EFI_STATUS Status;
  UINTN ResultTotalElements, ResultTotalAttributes, ResultMaxDepth, ResultMaxAttributes, StringSize;
  ResultTotalElements = 0;
  ResultTotalAttributes = 0;
  ResultMaxDepth = 0;
  ResultMaxAttributes = 0;

  if (XmlContext == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Context is NULL.  Test is not valid\n"));
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  if (XmlContext->InputXmlString == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " InputXmlString in the context struct is NULL.  Test is not valid!\n"));
    return UNIT_TEST_ERROR_TEST_FAILED;
  }


  Status = CreateXmlTree(XmlContext->InputXmlString, AsciiStrLen(XmlContext->InputXmlString), &ResultData);
  XmlContext->Node = ResultData;  //set dynamic memory pointer in struct so memory gets cleanup in cleanup function
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " Failed to create xml tree node\n"));
  }
  
  UT_ASSERT_NOT_NULL(ResultData);

  //Check all the things we can check to make sure it was parsed correctly
  Status = XmlTreeNumberOfNodes(ResultData, &ResultTotalElements);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->TotalElements, ResultTotalElements);

  Status = XmlTreeNumberOfAttributes(ResultData, &ResultTotalAttributes);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->TotalAttributes, ResultTotalAttributes);

  Status = XmlTreeMaxDepth(ResultData, &ResultMaxDepth);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->MaxDepth, ResultMaxDepth);

  Status = XmlTreeMaxAttributes(ResultData, &ResultMaxAttributes);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->MaxAttributes, ResultMaxAttributes);

  //write to string
  Status = XmlTreeToString(ResultData, TRUE, &StringSize, &XmlContext->ToFreeXmlString);
  UT_ASSERT_NOT_EFI_ERROR(Status);

  DebugPrintXmlTree(ResultData, 0);

  //free the old xml tree
  FreeXmlTree(&ResultData);
  XmlContext->Node = NULL;

  //read from the new string
  Status = CreateXmlTree(XmlContext->ToFreeXmlString, AsciiStrLen(XmlContext->ToFreeXmlString), &ResultData);
  XmlContext->Node = ResultData; 
  UT_ASSERT_NOT_EFI_ERROR(Status);

  ResultTotalElements = 0;
  ResultTotalAttributes = 0;
  ResultMaxDepth = 0;
  ResultMaxAttributes = 0;

  //Check all the things we can check to make sure it was parsed correctly
  Status = XmlTreeNumberOfNodes(ResultData, &ResultTotalElements);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->TotalElements, ResultTotalElements);

  Status = XmlTreeNumberOfAttributes(ResultData, &ResultTotalAttributes);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->TotalAttributes, ResultTotalAttributes);

  Status = XmlTreeMaxDepth(ResultData, &ResultMaxDepth);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->MaxDepth, ResultMaxDepth);

  Status = XmlTreeMaxAttributes(ResultData, &ResultMaxAttributes);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(XmlContext->MaxAttributes, ResultMaxAttributes);

  DebugPrintXmlTree(ResultData, 0);

  return UNIT_TEST_PASSED;
}


/**
Parse strings
**/
UNIT_TEST_STATUS
EFIAPI
TestStringParsing(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  UINTN Length = 0;
  XmlStringParseContext *XmlContext = (XmlStringParseContext*)Context;
  EFI_STATUS Status;

  //Test the data input - these are test errors not failures
  if (XmlContext->StringEscaped == NULL)
  {
    DEBUG((DEBUG_ERROR, "Test Data Error - Context invalid (StringEscaped == NULL)\n"));
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }
  Length = AsciiStrLen(XmlContext->StringEscaped);
  if (Length != XmlContext->EscapedLength)
  {
    DEBUG((DEBUG_ERROR, "Test Data Error - Context invalid (EscapedLength Incorrect %d Test Data EscapedLength %d)\n", Length, XmlContext->EscapedLength));
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }

  if (XmlContext->StringNotEscaped == NULL)
  {
    DEBUG((DEBUG_ERROR, "Test Data Error - Context invalid (StringNotEscaped == NULL)\n"));
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }
  Length = AsciiStrLen(XmlContext->StringNotEscaped);
  if (Length != XmlContext->NotEscapedLength)
  {
    DEBUG((DEBUG_ERROR, "Test Data Error - Context invalid (NotEscapedLength Incorrect %d Test Data NotEscapedLength %d)\n", Length, XmlContext->NotEscapedLength));
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }

  if (XmlContext->String != NULL)
  {
    DEBUG((DEBUG_ERROR, "Test Data Error - Context invalid (String Not NULL Before Test)\n"));
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }

  //Now start actual tests

  //Test Escape Function
  Status = XmlEscape(XmlContext->StringNotEscaped, XmlContext->NotEscapedLength + 1, &(XmlContext->String));
  UT_ASSERT_NOT_EFI_ERROR(Status);
  Length = AsciiStrLen(XmlContext->String);
  UT_ASSERT_EQUAL(Length, XmlContext->EscapedLength);
  UT_ASSERT_MEM_EQUAL(XmlContext->String, XmlContext->StringEscaped, Length+1);

  FreePool(XmlContext->String);
  XmlContext->String = NULL;

  //Test UnEscape Function
  Status = XmlUnEscape(XmlContext->StringEscaped, XmlContext->EscapedLength + 1, &(XmlContext->String));
  UT_ASSERT_NOT_EFI_ERROR(Status);
  Length = AsciiStrLen(XmlContext->String);
  UT_ASSERT_EQUAL(Length, XmlContext->NotEscapedLength);
  UT_ASSERT_MEM_EQUAL(XmlContext->String, XmlContext->StringNotEscaped, Length + 1);

  FreePool(XmlContext->String);
  XmlContext->String = NULL;

  return UNIT_TEST_PASSED;
}

/**
Test that invalid Xml is not parsed and error is reported correctly
**/
UNIT_TEST_STATUS
EFIAPI
ParseInValidXml1(
	IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
	IN UNIT_TEST_CONTEXT           Context
)
{
	XmlNode *ResultData = NULL;
	EFI_STATUS Status;
	CHAR8 BadString[] = "This is not valid xml"; //Not even close
	
	Status = CreateXmlTree(BadString, AsciiStrLen(BadString), &ResultData);
	if (ResultData != NULL)
	{
		FreePool(ResultData);
		UT_ASSERT_TRUE(ResultData == NULL);  //cause it to fail here after memory clean up
	}
	UT_ASSERT_TRUE(EFI_ERROR(Status));

	return UNIT_TEST_PASSED;
}

/**
Test that invalid Xml is not parsed and error is reported correctly
**/
UNIT_TEST_STATUS
EFIAPI
ParseInValidXml2(
	IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
	IN UNIT_TEST_CONTEXT           Context
)
{
	XmlNode *ResultData = NULL;
	EFI_STATUS Status;
	CHAR8 BadString[] = "<Node1><Node2></Node1>";  //malformed XML

	Status = CreateXmlTree(BadString, AsciiStrLen(BadString), &ResultData);
	if (ResultData != NULL)
	{
		FreePool(ResultData);
		UT_ASSERT_TRUE(ResultData == NULL);  //cause it to fail here after memory clean up
	}
	UT_ASSERT_TRUE(EFI_ERROR(Status));

	return UNIT_TEST_PASSED;
}

/**
Test that invalid Xml is not parsed and error is reported correctly
**/
UNIT_TEST_STATUS
EFIAPI
ParseInValidXml3(
	IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
	IN UNIT_TEST_CONTEXT           Context
)
{
	XmlNode *ResultData = NULL;
	EFI_STATUS Status;
	CHAR8 BadString[] = "<Node1><Node2><Node3 /></Node1>";  //malformed xml - Missing Node 2 closure

	Status = CreateXmlTree(BadString, AsciiStrLen(BadString), &ResultData);
	if (ResultData != NULL)
	{
		FreePool(ResultData);
		UT_ASSERT_TRUE(ResultData == NULL);  //cause it to fail here after memory clean up
	}
	UT_ASSERT_TRUE(EFI_ERROR(Status));

	return UNIT_TEST_PASSED;
}



/**
Test Node count function on known count
**/
UNIT_TEST_STATUS
EFIAPI
TestNodeCount(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode *ResultData = NULL;
  UINTN TotalElements = 0;
  EFI_STATUS Status;
  CHAR8 MyString[] = "<Node1><Node2><Node3 /><Node4 /></Node2> </Node1>";  //4 nodes with depth of 3

  Status = CreateXmlTree(MyString, AsciiStrLen(MyString), &ResultData);
  UT_ASSERT_NOT_NULL(ResultData); 

  Status = XmlTreeNumberOfNodes(ResultData, &TotalElements);

  //free our memory 
  FreeXmlTree(&ResultData);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(4, TotalElements);

  return UNIT_TEST_PASSED;
}


/**
Test Node max depth function on known value
**/
UNIT_TEST_STATUS
EFIAPI
TestNodeMaxDepth(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode *ResultData = NULL;
  UINTN MaxDepth = 0;
  EFI_STATUS Status;
  CHAR8 MyString[] = "<Node1><Node2><Node3 /><Node4 /></Node2><Node5><Node6><Node7 /></Node6></Node5></Node1>";  //4 nodes with depth of 4

  Status = CreateXmlTree(MyString, AsciiStrLen(MyString), &ResultData);
  UT_ASSERT_NOT_NULL(ResultData);

  Status = XmlTreeMaxDepth(ResultData, &MaxDepth);

  //free our memory 
  FreeXmlTree(&ResultData);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(4, MaxDepth);

  return UNIT_TEST_PASSED;
}


/**
Test attribute count function with known value
**/
UNIT_TEST_STATUS
EFIAPI
TestAttributeCount(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode *ResultData = NULL;
  UINTN Count = 0;
  EFI_STATUS Status;
  CHAR8 MyString[] = "<Node1 att1='test1'><Node2 att2='test2'><Node3 att3='test3' att4='test4'  /></Node2></Node1>";  //4 attributes total

  Status = CreateXmlTree(MyString, AsciiStrLen(MyString), &ResultData);
  UT_ASSERT_NOT_NULL(ResultData);

  Status = XmlTreeNumberOfAttributes(ResultData, &Count);

  //free our memory 
  FreeXmlTree(&ResultData);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(4, Count);

  return UNIT_TEST_PASSED;
}

/**
Test max attribute function on known value
**/
UNIT_TEST_STATUS
EFIAPI
TestAttributeMax(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  XmlNode *ResultData = NULL;
  UINTN Count = 0;
  EFI_STATUS Status;
  CHAR8 MyString[] = "<Node1 att1='test1'><Node2 att2='test2'><Node3 att3='test3' att4='test4'  /></Node2></Node1>";  //4 attributes total

  Status = CreateXmlTree(MyString, AsciiStrLen(MyString), &ResultData);
  UT_ASSERT_NOT_NULL(ResultData);

  Status = XmlTreeMaxAttributes(ResultData, &Count);

  //free our memory 
  FreeXmlTree(&ResultData);
  UT_ASSERT_NOT_EFI_ERROR(Status);
  UT_ASSERT_EQUAL(2, Count);

  return UNIT_TEST_PASSED;
}



/**
  
  Main fuction sets up the unit test environment


**/
EFI_STATUS
EFIAPI
UefiMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable)
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *InputTestSuite;
  UNIT_TEST_SUITE           *ProcessEscapedInputTestSuite;
  UNIT_TEST_SUITE           *BasicMetricsTestSuite;

  DEBUG((DEBUG_INFO, "%s v%s\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework(&Fw, UNIT_TEST_APP_NAME, UNIT_TEST_APP_SHORT_NAME, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  ///
  // Test the Escape / Unescape functions for special characters in the XML strings
  //
  Status = CreateUnitTestSuite(&ProcessEscapedInputTestSuite, Fw, L"XML Escape Strings Test Suite ", L"Common.Xml.ParseEscape", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for XML Escape Input String Parsing Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with no escape characters", L"Common.Xml.ParseEscapeNone", TestStringParsing, NULL, CleanUpXmlStringParseContext, &Context1);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with three Less Than escape characters", L"Common.Xml.ParseEscapeLessThan", TestStringParsing, NULL, CleanUpXmlStringParseContext, &ContextLT);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with three Greater Than escape characters", L"Common.Xml.ParseEscapeGreaterThan", TestStringParsing, NULL, CleanUpXmlStringParseContext, &ContextGT);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with three Quote escape characters", L"Common.Xml.ParseEscapeQuote", TestStringParsing, NULL, CleanUpXmlStringParseContext, &ContextQuote);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with three Apostrophe escape characters", L"Common.Xml.ParseEscapeApostrophe", TestStringParsing, NULL, CleanUpXmlStringParseContext, &ContextApostrophe);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with three Ampersand escape characters", L"Common.Xml.ParseEscapeAmpersand", TestStringParsing, NULL, CleanUpXmlStringParseContext, &ContextAmp);
  AddTestCase(ProcessEscapedInputTestSuite, L"Parse string with all escape characters and other inputs", L"Common.Xml.ParseEscapeMany", TestStringParsing, NULL, CleanUpXmlStringParseContext, &Context7Esc);


  //
  // The the metric functions - Max depth, node count, max attributes, attribute count 
  //
  Status = CreateUnitTestSuite(&BasicMetricsTestSuite, Fw, L"XML Node Tree Metrics", L"Common.Xml.TreeMetrics", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for XML Node Tree Metrics Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase(BasicMetricsTestSuite, L"Test Node Count Function", L"Common.Xml.TreeMetric.NodeCount", TestNodeCount, NULL, NULL, NULL);
  AddTestCase(BasicMetricsTestSuite, L"Test Max Node Depth Function", L"Common.Xml.TreeMetic.MaxDepth", TestNodeMaxDepth, NULL, NULL, NULL);
  AddTestCase(BasicMetricsTestSuite, L"Test Attribute Count Function", L"Common.Xml.TreeMetric.AttributeCount", TestAttributeCount, NULL, NULL, NULL);
  AddTestCase(BasicMetricsTestSuite, L"Test Max Node Depth Function", L"Common.Xml.TreeMetic.AttributeMax", TestAttributeMax, NULL, NULL, NULL);


  //
  // Test the conversion of string to tree and back to string
  //
  Status = CreateUnitTestSuite(&InputTestSuite, Fw, L"XML Input Parsing Test Suite ", L"Common.Xml.Parse", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for XML Input Parsing Test Suite\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase(InputTestSuite, L"Parse Valid XML with simple elements 3 layers", L"Common.Xml.ParseValidElements", ParseValidXml, NULL, CleanUpXmlTestContext, &SimpleElementsOnlyContext);
  AddTestCase(InputTestSuite, L"Parse Valid XML with 2 elements and 2 attributes", L"Common.Xml.ParseValidElementsAndAttributes", ParseValidXml, NULL, CleanUpXmlTestContext, &SimpleElementsAttributesContext);
  AddTestCase(InputTestSuite, L"Parse Invalid XML string containing an attribute with invalid xml chars", L"Common.Xml.NonXmlEncodedAttribute", ParseValidXml, NULL, CleanUpXmlTestContext, &NonEncodedXmlAttribute1Context);
  AddTestCase(InputTestSuite, L"Parse valid encoded XML string containing an attribute with encoded xml chars", L"Common.Xml.XmlEncodedAttribute", ParseValidXml, NULL, CleanUpXmlTestContext, &EncodedXmlAttribute1Context);

  AddTestCase(InputTestSuite, L"Fail parsing string with no XML Tags", L"Common.Xml.ParseInvalidString", ParseInValidXml1, NULL, NULL, NULL);
  AddTestCase(InputTestSuite, L"Fail parsing string missing closing element", L"Common.Xml.ParseInvalidString", ParseInValidXml2, NULL, NULL, NULL);
  AddTestCase(InputTestSuite, L"Fail parsing string missing nested closing element", L"Common.Xml.ParseInvalidString", ParseInValidXml3, NULL, NULL, NULL);
  //
  // Execute the tests.
  //
  Status = RunAllTestSuites(Fw);

EXIT:
  if (Fw)
  {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}
