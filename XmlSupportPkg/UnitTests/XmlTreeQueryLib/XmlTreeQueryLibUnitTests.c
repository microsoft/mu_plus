/**
Unit-tests UEFI shell app for XmlTreeQueryLib.


Copyright (c) 2017, Microsoft Corporation

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
#include "XmlTreeQueryLibUnitTests.h"

CONST CHAR8 XmlString[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<RootNode>"
"  <Gen1Node>Gen1Node1 contents</Gen1Node>"
"  <Gen1Node attribute1='value1' attribute2='value2'>Gen1Node2 contents"
"    <Gen2Node>Gen2Node1 contents</Gen2Node>"
"  </Gen1Node>"
"  <Gen1Node>Gen1Node3 contents "
"    <Gen2Node>Gen2Node1 contents"
"      <Gen3Node>Gen3Node1 contents</Gen3Node>"
"      <Gen3Node>Gen2Node2 contents</Gen3Node>"
"    </Gen2Node>"
"    <Gen2Node attribute2.1='value2.1' attribute2.2='value2.2'>Gen2Node2 Long Contents Here Long Contents Here Long Contents Here</Gen2Node>"
"  </Gen1Node>"
"  <AnotherGen1Node>Test Data 123</AnotherGen1Node>"
"</RootNode>";


XmlNode *mNode = NULL; //This is a node tree ptr used for all testing

/**
Simple clean up method to make sure string parsing tests clean up even if interrupted and fail in the middle.
**/
UNIT_TEST_STATUS
EFIAPI
PreReqNodeTreeIsValid(
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
)
{
  if (mNode == NULL)
  {
    return UNIT_TEST_ERROR_PREREQ_NOT_MET;
  }
  return UNIT_TEST_PASSED;
}


//----------------------------------------------------
// UEFI main
//----------------------------------------------------
EFI_STATUS
EFIAPI
UefiMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable)
{
  EFI_STATUS                Status;
  UNIT_TEST_FRAMEWORK       *Fw = NULL;
  UNIT_TEST_SUITE           *TestSuite;

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

  Status = CreateUnitTestSuite(&TestSuite, Fw, L"XML Tree Query Test Suite ", L"Common.Xml.Query", NULL, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed in CreateUnitTestSuite for XML Tree Query Test Suite %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  RegisterElementTests(TestSuite);
  RegisterAttributeTests(TestSuite);

  //Create the Node Tree for query
  Status = CreateXmlTree(XmlString, AsciiStrLen(XmlString), &mNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " test setup error.  CreateXmlTree failed. %r\n", Status));
  }
  
  //Run Tests
  Status = RunAllTestSuites(Fw);

  
 
EXIT:
  //Clean up Node Tree for query
  if (mNode != NULL)
  {
    FreeXmlTree(&mNode);
  }

  if (Fw != NULL)
  {
    FreeUnitTestFramework(Fw);
  }

  return Status;
}