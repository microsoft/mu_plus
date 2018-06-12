/** @file
  Support using XML as the File format for var report data


  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>

**/


#include "JUnitXmlSupport.h"
#define DOC_XML_TEMPLATE "<?xml version=\"1.0\" encoding=\"utf-8\"?><testsuites />"




/**
Creates a new MsXmlNode list following the List
format.  This is the root document that test suite objects can be
added to.

Return NULL if error occurs. Otherwise return a pointer to xml doc root element
of a testsuite.

List must be freed using FreeXmlTree

**/
XmlNode *
EFIAPI
New_JUnitXmlDocNodeList()
{
  EFI_STATUS Status;
  XmlNode *Root = NULL;

  Status = CreateXmlTree(DOC_XML_TEMPLATE, sizeof(DOC_XML_TEMPLATE) - 1, &Root);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR;
  }

  //Return the root
  return Root;

ERROR:
  if (Root)
  {
    FreeXmlTree(&Root);
    //root will be NULL and null returned
  }
  return Root;
}

/**
Creates a new MsXmlNode for a Test Suite and adds it to the list

Return NULL if error occurs.

return pointer will be the newly created test suite element node
**/
XmlNode *
EFIAPI
New_TestSuiteNodeInList(
  IN CONST  XmlNode*  RootNode,
  IN CONST  CHAR8*      Name,
  IN CONST  CHAR8*      Package,
  UINTN                 Id
)
{
  XmlNode* NewSuiteNode = NULL;
  EFI_STATUS Status;
  CHAR8  IntString[30];

  //1 - confirm good root node
  if (RootNode == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (RootNode->XmlDeclaration.Declaration == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not the root node\n", __FUNCTION__));
    ASSERT(RootNode->XmlDeclaration.Declaration != NULL);
    return NULL;
  }

  if (AsciiStrnCmp(RootNode->Name, TESTSUITE_LIST_ELEMENT_NAME, sizeof(TESTSUITE_LIST_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - RootNode is not testsuites List\n", __FUNCTION__));
    return NULL;
  }
  //RootNode is good. 


  //Create the testsuite node with no parent
  Status = AddNode(NULL, TESTSUITE_ELEMENT_NAME, NULL, &NewSuiteNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for test suite Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the Id attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, Id, 0);
  Status = AddAttributeToNode(NewSuiteNode, "id", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for id Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the name attribute
  Status = AddAttributeToNode(NewSuiteNode, "name", Name);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for Name Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the package attribute
  Status = AddAttributeToNode(NewSuiteNode, "package", Package);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for Package Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Add the testsuite to the end of the RootNode children
  Status = AddChildTree((XmlNode*)RootNode, NewSuiteNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Can't add new var to list.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewSuiteNode;

ERROR_EXIT:
  if (NewSuiteNode != NULL) { FreeXmlTree(&NewSuiteNode); }
  return NULL;
}

/**
Creates a new MsXmlNode for a Test case and adds it to the list

Return NULL if error occurs. Otherwise return a pointer to the Testcase Node.

return pointer will be the newly created test case element node
**/
XmlNode *
EFIAPI
New_TestCaseInSuite(
  IN CONST  XmlNode *TestSuite,
  IN CONST  CHAR8*    Name,
  IN CONST  CHAR8*    ClassName,
  UINTN              TimeInSeconds,
  IN CONST  CHAR8*    Log OPTIONAL,
  IN CONST  CHAR8*    FailureMsg   OPTIONAL,
  IN CONST  CHAR8*    FailureType  OPTIONAL,
  IN        BOOLEAN   Skipped
)
{
  XmlNode* NewTestNode = NULL;
  EFI_STATUS Status;
  CHAR8  IntString[30];

  //1 - confirm good suite node
  if (TestSuite == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - TesteSuite is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (AsciiStrnCmp(TestSuite->Name, TESTSUITE_ELEMENT_NAME, sizeof(TESTSUITE_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - TestSuite is not a testsuite\n", __FUNCTION__));
    return NULL;
  }
  //TestSuite Node is good. 


  //Create the tesetCase node with no parent
  Status = AddNode(NULL, TESTCASE_ELEMENT_NAME, NULL, &NewTestNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for test case Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }
  
  //Create the classname attribute
  Status = AddAttributeToNode(NewTestNode, "classname", ClassName);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for ClassName Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the name attribute
  Status = AddAttributeToNode(NewTestNode, "name", Name);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for Name Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the time attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, TimeInSeconds, 0);
  Status = AddAttributeToNode(NewTestNode, "time", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for Time Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Handle skipped node
  if (Skipped == TRUE) {
    Status = AddNode(NewTestNode, TESTCASE_SKIPPED_ELEMENT_NAME, NULL, NULL);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a - AddNode for skipped element failed.  Status = %r\n", __FUNCTION__, Status));
      //do not exit...just keep going.  
    }
  }

  //Handle the failure element
  New_FailureForTestCase(NewTestNode, FailureMsg, FailureType);  //if failuremsg/type are NULL then it will just return


  //the log element
  Status = AddNode(NewTestNode, TESTCASE_LOG_ELEMENT_NAME, Log, NULL);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for log element failed.  Status = %r\n", __FUNCTION__, Status));
    //do not exit...just keep going.  
  }

  //Add the testcase to the end of the testsuite children
  Status = AddChildTree((XmlNode*)TestSuite, NewTestNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Can't add new test class to test suite.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewTestNode;

ERROR_EXIT:
  if (NewTestNode != NULL) { FreeXmlTree(&NewTestNode); }
  return NULL;
}


/**
Creates a new XmlNode for a Test case failure and add it to the test case

Return NULL if error occurs or no failure node needed. Otherwise return a pointer to the failure Node.

return pointer will be the newly created failure element node
**/
XmlNode *
EFIAPI
New_FailureForTestCase(
  IN CONST  XmlNode*  TestCase,
  IN CONST  CHAR8*      Msg  OPTIONAL,
  IN CONST  CHAR8*      Type OPTIONAL
)
{
  XmlNode* NewFailureNode = NULL;
  EFI_STATUS Status;

  //1 - do we need to create a failure node??
  if (Msg == NULL)
  {
    DEBUG((DEBUG_VERBOSE, __FUNCTION__ " - Msg is NULL\n"));
    //This isn't a bug...easier code if we just allow this function to get called unconditionally and return if parameters are null
    return NULL;
  }
  if (Type == NULL)
  {
    DEBUG((DEBUG_VERBOSE, __FUNCTION__ " - Type is NULL\n"));
    //This isn't a bug...easier code if we just allow this function to get called unconditionally and return if parameters are null
    return NULL;
  }

  //Have a failure - lets create the node and add it to the test case

  //2 - confirm good TestCase node
  if (TestCase == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - TestCase is NULL\n", __FUNCTION__));
    return NULL;
  }

  if (AsciiStrnCmp(TestCase->Name, TESTCASE_ELEMENT_NAME, sizeof(TESTCASE_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - TestCase is not a testcase\n", __FUNCTION__));
    return NULL;
  }
  //TestCase Node is good. 


  //Create the failure node with no parent
  Status = AddNode(NULL, TESTCASE_FAILURE_ELEMENT_NAME, NULL, &NewFailureNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddNode for failure node Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the message attribute
  Status = AddAttributeToNode(NewFailureNode, "message", Msg);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for message Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  //Create the type attribute
  Status = AddAttributeToNode(NewFailureNode, "type", Type);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for type Failed.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  // Add the failure to the end of the testcase children
  Status = AddChildTree((XmlNode*)TestCase, NewFailureNode);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - Can't add new failure to test case.  Status %r\n", __FUNCTION__, Status));
    goto ERROR_EXIT;
  }

  return NewFailureNode;

ERROR_EXIT:
  if (NewFailureNode != NULL) { FreeXmlTree(&NewFailureNode); }
  return NULL;
}



EFI_STATUS
EFIAPI
AddTestSuiteStats(
  IN    XmlNode*  TestSuite,
  UINTN TotalTests,
  UINTN TotalFailures,
  UINTN TotalSkips,
  UINTN TotalErrors
)
{
  EFI_STATUS Status;
  CHAR8  IntString[30];

  //1 - confirm good suite node
  if (TestSuite == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a - TesteSuite is NULL\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrnCmp(TestSuite->Name, TESTSUITE_ELEMENT_NAME, sizeof(TESTSUITE_ELEMENT_NAME)) != 0)
  {
    DEBUG((DEBUG_ERROR, "%a - TestSuite is not a testsuite\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }
  //TestSuite Node is good. 

  //Create the total errors attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, TotalErrors, 0);
  Status = AddAttributeToNode(TestSuite, "errors", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for errors Failed.  Status %r\n", __FUNCTION__, Status));
    return Status;
  }

  //Create the total tests attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, TotalTests, 0);
  Status = AddAttributeToNode(TestSuite, "tests", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for tests Failed.  Status %r\n", __FUNCTION__, Status));
    return Status;
  }

  //Create the total failures attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, TotalFailures, 0);
  Status = AddAttributeToNode(TestSuite, "failures", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for failures Failed.  Status %r\n", __FUNCTION__, Status));
    return Status;
  }

  //Create the total skipped attribute
  //Convert int to string
  AsciiValueToString(IntString, 0, TotalSkips, 0);
  Status = AddAttributeToNode(TestSuite, "skipped", IntString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a - AddAttribute for skipped Failed.  Status %r\n", __FUNCTION__, Status));
    return Status;
  }
 
  return EFI_SUCCESS;
}
