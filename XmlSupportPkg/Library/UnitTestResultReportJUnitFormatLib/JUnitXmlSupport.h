/** @file
  Internal header file for XML helper routines for JUNIT support


  Copyright (c) 2017, Microsoft Corporation. All rights reserved.<BR>

**/

#ifndef JUNIT_XML_SUPPORT_H
#define JUNIT_XML_SUPPORT_H

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <XmlTypes.h>
#include <Library/XmlTreeLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
//add once we do real compute function #include <Library/MsXmlNodeQueryLib.h>


/**

// Note: JUnit schema definition doesn't seem to be completely clear.  Sounds like since no real "owner" different projects have changed and adapted
// for their usage.  Here is one schema found online that seems reasonable.  Need to confirm our tools/usage (visual studio) parse this correctly 
// and take full advantage of the format. 

http://help.catchsoftware.com/display/ET/JUnit+Format


<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
  <testsuite name=""      <!-- Full (class) name of the test for non-aggregated testsuite documents.
                               Class name without the package for aggregated testsuites documents. Required -->
	     tests=""     <!-- The total number of tests in the suite, required. -->
	     disabled=""  <!-- the total number of disabled tests in the suite. optional -->
             errors=""    <!-- The total number of tests in the suite that errored. An errored test is one that had an unanticipated problem,
                               for example an unchecked throwable; or a problem with the implementation of the test. optional -->
             failures=""  <!-- The total number of tests in the suite that failed. A failure is a test which the code has explicitly failed
                               by using the mechanisms for that purpose. e.g., via an assertEquals. optional -->
             hostname=""  <!-- Host on which the tests were executed. 'localhost' should be used if the hostname cannot be determined. optional -->
	     id=""        <!-- Starts at 0 for the first testsuite and is incremented by 1 for each following testsuite -->
	     package=""   <!-- Derived from testsuite/@name in the non-aggregated documents. optional -->
	     skipped=""   <!-- The total number of skipped tests. optional -->
	     time=""      <!-- Time taken (in seconds) to execute the tests in the suite. optional -->
	     timestamp="" <!-- when the test was executed in ISO 8601 format (2014-01-21T16:17:18). Timezone may not be specified. optional -->
	     >
         <!-- testcase can appear multiple times, see /testsuites/testsuite@tests -->
         <testcase name=""       <!-- Name of the test method, required. -->
         assertions="" <!-- number of assertions in the test case. optional -->
         classname=""  <!-- Full class name for the class the test method is in. required -->
         status=""
         time=""       <!-- Time taken (in seconds) to execute the test. optional -->
         >

         <!-- If the test was not executed or failed, you can specify one
         the skipped, error or failure elements. -->

         <!-- skipped can appear 0 or once. optional -->
         <skipped/>

         <!-- Indicates that the test errored. An errored test is one
         that had an unanticipated problem. For example an unchecked
         throwable or a problem with the implementation of the
         test. Contains as a text node relevant data for the error,
         for example a stack trace. optional -->
         <error message="" <!-- The error message. e.g., if a java exception is thrown, the return value of getMessage() -->
         type=""    <!-- The type of error that occured. e.g., if a java execption is thrown the full class name of the exception. -->
         ></error>

         <!-- Indicates that the test failed. A failure is a test which
         the code has explicitly failed by using the mechanisms for
         that purpose. For example via an assertEquals. Contains as
         a text node relevant data for the failure, e.g., a stack
         trace. optional -->
         <failure message="" <!-- The message specified in the assert. -->
         type=""    <!-- The type of the assert. -->
         ></failure>

         <!-- Data that was written to standard out while the test was executed. optional -->
         <system-out></system-out>

         <!-- Data that was written to standard error while the test was executed. optional -->
         <system-err></system-err>
         </testcase>

       <!-- Data that was written to standard out while the test suite was executed. optional -->
       <system-out></system-out>
       <!-- Data that was written to standard error while the test suite was executed. optional -->
       <system-err></system-err>
       </testsuite>
</testsuites>
**/

#define TESTSUITE_LIST_ELEMENT_NAME "testsuites"
#define TESTSUITE_ELEMENT_NAME "testsuite"
#define TESTCASE_ELEMENT_NAME  "testcase"
#define TESTCASE_FAILURE_ELEMENT_NAME "failure"
#define TESTCASE_LOG_ELEMENT_NAME "system-out"
#define TESTCASE_SKIPPED_ELEMENT_NAME "skipped"


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
New_JUnitXmlDocNodeList();


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
);


/**
Creates a new MsXmlNode for a Test case and adds it to the list

Return NULL if error occurs. Otherwise return a pointer to the Testcase Node.

return pointer will be the newly created test case element node
**/
XmlNode *
EFIAPI
New_TestCaseInSuite(
  IN CONST XmlNode *TestSuite,
  IN CONST  CHAR8*    Name,
  IN CONST  CHAR8*    ClassName,
  UINTN              TimeInSeconds,
  IN CONST  CHAR8*    Log  OPTIONAL,
  IN CONST  CHAR8*    FailureMsg   OPTIONAL, 
  IN CONST  CHAR8*    FailureType  OPTIONAL,
  IN        BOOLEAN   Skipped
);

/**
Creates a new MsXmlNode for a Test case failure and add it to the test case

Return NULL if error occurs or no failure node needed. Otherwise return a pointer to the failure Node.

return pointer will be the newly created failure element node
**/
XmlNode *
EFIAPI
New_FailureForTestCase(
  IN CONST  XmlNode*  TestCase,
  IN CONST  CHAR8*      Msg  OPTIONAL,
  IN CONST  CHAR8*      Type OPTIONAL
);

EFI_STATUS
EFIAPI
AddTestSuiteStats(
  IN    XmlNode*  TestSuite,
  UINTN TotalTests,
  UINTN TotalFailures,
  UINTN TotalSkips,
  UINTN TotalErrors
);



#endif