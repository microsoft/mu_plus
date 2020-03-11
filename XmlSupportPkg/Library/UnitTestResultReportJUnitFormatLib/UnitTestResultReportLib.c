/**
Implement UnitTestResultReportLib instance that writes to an JUnit compliant XML file on the same filesystem where the test efi resides

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestResultReportLib.h>
#include <Library/ShellLib.h>
#include <Library/MemoryAllocationLib.h>

#include "JUnitXmlSupport.h"

struct _UNIT_TEST_FAILURE_TYPE_STRING
{
  FAILURE_TYPE  Type;
  CHAR8         *String;
};

struct _UNIT_TEST_FAILURE_TYPE_STRING mFailureTypeStrings[] =
{
  { FAILURETYPE_NOFAILURE, "NO FAILURE" },
  { FAILURETYPE_OTHER, "OTHER FAILURE" },
  { FAILURETYPE_ASSERTTRUE, "ASSERT_TRUE FAILURE" },
  { FAILURETYPE_ASSERTFALSE, "ASSERT_FALSE FAILURE" },
  { FAILURETYPE_ASSERTEQUAL, "ASSERT_EQUAL FAILURE" },
  { FAILURETYPE_ASSERTNOTEQUAL, "ASSERT_NOTEQUAL FAILURE" },
  { FAILURETYPE_ASSERTNOTEFIERROR, "ASSERT_NOTEFIERROR FAILURE" },
  { FAILURETYPE_ASSERTSTATUSEQUAL, "ASSERT_STATUSEQUAL FAILURE" },
  { FAILURETYPE_ASSERTNOTNULL , "ASSERT_NOTNULL FAILURE" }
};
UINTN mFailureTypeStringsCount = sizeof(mFailureTypeStrings) / sizeof(mFailureTypeStrings[0]);
CHAR8 *mUnknownFailureType = "*UNKNOWN* Failure";

STATIC
CONST CHAR8*
GetStringForFailureType(
  IN FAILURE_TYPE   Failure
)
{
  UINTN   Index;
  CHAR8   *Result;

  //special case for No failure so that failure node is not created.
  if (Failure == FAILURETYPE_NOFAILURE)
  {
    return NULL;
  }

  Result = mUnknownFailureType;
  for (Index = 0; Index < mFailureTypeStringsCount; Index++)
  {
    if (mFailureTypeStrings[Index].Type == Failure)
    {
      Result = mFailureTypeStrings[Index].String;
      break;
    }
  }
  if (Result == mUnknownFailureType)
  {
    DEBUG((DEBUG_INFO, "%a Failure Type does not have string defined 0x%X\n", __FUNCTION__, (UINT32)Failure));
  }
  return Result;
}

STATIC
EFI_STATUS
WriteXmlNodeToLogFile(
  IN UNIT_TEST_FRAMEWORK  *Framework,
  IN XmlNode *Doc
)
{
  EFI_STATUS                     Status;
  UINTN                          FileNameLen = 0;
  CHAR16                         *LogFileName = NULL;
  CHAR16                         *LogFileNameSuffix = L"_JUNIT.XML";
  SHELL_FILE_HANDLE              FileHandle;
  UINTN                          StringSize = 0;
  CHAR8*                         XmlString = NULL;

  if (Framework == NULL)
  {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((DEBUG_ERROR, "Framework is NULL.\n"));
    goto Exit;
  }

  if (Doc == NULL)
  {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((DEBUG_ERROR, "XML Doc Node List is NULL.\n"));
    goto Exit;
  }

  //Get the file name based on framework name
  FileNameLen = AsciiStrLen(Framework->ShortTitle);
  FileNameLen += StrLen(LogFileNameSuffix);
  FileNameLen += 1;  //NULL term
  LogFileName = AllocateZeroPool(FileNameLen * sizeof(CHAR16));
  if (LogFileName == NULL)
  {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG((DEBUG_ERROR, "failed to allocate memory for log file name.\n"));
    goto Exit;
  }

  AsciiStrToUnicodeStrS(Framework->ShortTitle, LogFileName, FileNameLen);
  StrnCatS(LogFileName, FileNameLen, LogFileNameSuffix, FileNameLen - 1);


  //Write XML
  Status = XmlTreeToString(Doc, TRUE, &StringSize, &XmlString);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "XmlTreeToString failed.  %r\n", Status));
    goto Exit;
  }

  if (StringSize == 0)
  {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG((DEBUG_ERROR, "StringSize equal 0.\n"));
    goto Exit;
  }

  //
  // subtract 1 from string size to avoid writing the NULL terminator
  //
  StringSize--;

  //
  //First lets open the file if it exists so we can delete it...This is the work around for truncation
  //
  Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
  if (!EFI_ERROR(Status))
  {
    //if file handle above was opened it will be closed by the delete.
    Status = ShellDeleteFile(&FileHandle);
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a failed to delete file %r\n", __FUNCTION__, Status));
    }
  }


  Status = ShellOpenFileByName(LogFileName, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to open %s file for create. Status = %r\n", LogFileName, Status));
    goto Exit;
  }
  else
  {
    ShellPrintEx(-1, -1, L"Writing XML to file %s\n", LogFileName);
    ShellWriteFile(FileHandle, &StringSize, XmlString);
    ShellCloseFile(&FileHandle);
  }

  //success
  Status = EFI_SUCCESS;

Exit:
  if (LogFileName != NULL) {FreePool(LogFileName); }
  if (XmlString != NULL) { FreePool(XmlString); }
  return Status;
}



/*
Method to print the Unit Test run results

@retval Success
*/
EFI_STATUS
EFIAPI
OutputUnitTestFrameworkReport(
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UNIT_TEST_SUITE_LIST_ENTRY *Suite = NULL;
  XmlNode *Doc = NULL;
  UINTN Id = 0;
  UNIT_TEST_FRAMEWORK *Framework = NULL;

  Framework = (UNIT_TEST_FRAMEWORK *) FrameworkHandle;

  if (Framework == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a Failed. Framework is NULL\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  Doc = New_JUnitXmlDocNodeList();
  if (Doc == NULL)
  {
    DEBUG((DEBUG_ERROR, "%a Failed to create new xml doc\n", __FUNCTION__));
    Status = EFI_DEVICE_ERROR;
    goto EXIT;
  }

  //
  // Iterate all suites
  //
  for (Suite = (UNIT_TEST_SUITE_LIST_ENTRY*)GetFirstNode(&Framework->TestSuiteList);
    (LIST_ENTRY*)Suite != &Framework->TestSuiteList;
    Suite = (UNIT_TEST_SUITE_LIST_ENTRY*)GetNextNode(&Framework->TestSuiteList, (LIST_ENTRY*)Suite), Id++)
  {
    UNIT_TEST_LIST_ENTRY *Test = NULL;
    XmlNode* SuiteNode = NULL;
    CHAR8 *SuiteName = NULL;
    CHAR8 *SuitePackage = NULL;
    UINTN TotalTests = 0;
    UINTN TotalFailures = 0;
    UINTN TotalSkips = 0;
    UINTN TotalErrors = 0;
    SuiteName = Suite->UTS.Title;
    SuitePackage = Suite->UTS.Name;
    if (SuiteName == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a SuiteName is NULL \n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }
    if (SuitePackage == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a SuitePackage is NULL \n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }

    SuiteNode = New_TestSuiteNodeInList(Doc,SuiteName,SuitePackage, Id);
    FreePool(SuiteName);
    FreePool(SuitePackage);
    if (SuiteNode == NULL)
    {
      DEBUG((DEBUG_ERROR, "%a Failed to create new test suite\n", __FUNCTION__));
      Status = EFI_DEVICE_ERROR;
      goto EXIT;
    }

    //
    // Iterate all tests within the suite
    //
    for (Test = (UNIT_TEST_LIST_ENTRY*)GetFirstNode(&(Suite->UTS.TestCaseList));
      (LIST_ENTRY*)Test != &(Suite->UTS.TestCaseList);
      Test = (UNIT_TEST_LIST_ENTRY*)GetNextNode(&(Suite->UTS.TestCaseList), (LIST_ENTRY*)Test))
    {
      CHAR8  *Name = NULL;
      CHAR8  *ClassName = NULL;
      CHAR8  *Log = NULL;
      CHAR8  *FailureMsg = NULL;
      BOOLEAN Skipped = FALSE;

      TotalTests++;

      if (Test->UT.Result == UNIT_TEST_ERROR_PREREQUISITE_NOT_MET)
      {
        Skipped = TRUE;
        TotalSkips++;
      }
      else
      {
        //only count failures if there is a failuretype and no skip
        if (Test->UT.FailureType != FAILURETYPE_NOFAILURE)
        {
          TotalFailures++;
        }
      }

      Name = Test->UT.Description;
      ClassName = Test->UT.Name;
      Log = Test->UT.Log;
      FailureMsg = Test->UT.FailureMessage;


      //TODO:  need to handle timing.  Right now its hard coded to 1 second.
      New_TestCaseInSuite(SuiteNode, Name, ClassName, 1, Log, FailureMsg, GetStringForFailureType(Test->UT.FailureType), Skipped);

      if (Name != NULL) { FreePool(Name); }
      if (Log != NULL) { FreePool(Log); }
      if (ClassName != NULL) { FreePool(ClassName); }

    } //End Test iteration
    Status = AddTestSuiteStats(SuiteNode, TotalTests, TotalFailures, TotalSkips, TotalErrors);
    if(EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a Failed in AddTestSuiteStats.  Status = %r\n", __FUNCTION__, Status));
      goto EXIT;
    }
  }//End Suite iteration

  //write junit xml to file
  Status = WriteXmlNodeToLogFile(Framework, Doc);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "%a Failed to Write Xml Node To LogFile.  Status = %r\n", __FUNCTION__, Status));
    goto EXIT;
  }

  //done - clean up

EXIT:
  if(Doc != NULL) { FreeXmlTree(&Doc); }
  return Status;
}