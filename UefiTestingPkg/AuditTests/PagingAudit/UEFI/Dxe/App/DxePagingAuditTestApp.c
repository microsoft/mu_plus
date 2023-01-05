/** @file -- DxePagingAuditTestApp.c
This Shell App tests the page table or writes page table and
memory map information to SFS

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/ShellParameters.h>
#include <Protocol/Shell.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/FileHandleLib.h>

#include "DxePagingAuditTestApp.h"

#define UNIT_TEST_APP_NAME     "Paging Audit Test"
#define UNIT_TEST_APP_VERSION  "1"
#define MAX_CHARS_TO_READ      3

CHAR8  *mMemoryInfoDatabaseBuffer   = NULL;
UINTN  mMemoryInfoDatabaseSize      = 0;
UINTN  mMemoryInfoDatabaseAllocSize = 0;

/**

 Locates and opens the SFS volume containing the application and, if successful, returns an
 FS handle to the opened volume.

  @param    mFs_Handle       Handle to the opened volume.

  @retval   EFI_SUCCESS     The FS volume was opened successfully.
  @retval   Others          The operation failed.

**/
STATIC
EFI_STATUS
OpenAppSFS (
  OUT EFI_FILE  **Fs_Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  BOOLEAN                          Found;
  EFI_HANDLE                       Handle;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            Index;
  UINTN                            NumHandles;
  EFI_STRING                       PathNameStr;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SfProtocol;
  EFI_STATUS                       Status;
  EFI_FILE_PROTOCOL                *FileHandle;
  EFI_FILE_PROTOCOL                *FileHandle2;

  Status       = EFI_SUCCESS;
  SfProtocol   = NULL;
  NumHandles   = 0;
  HandleBuffer = NULL;

  //
  // Locate all handles that are using the SFS protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiSimpleFileSystemProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );

  if (EFI_ERROR (Status) != FALSE) {
    DEBUG ((DEBUG_ERROR, "%a: failed to locate all handles using the Simple FS protocol (%r)\n", __FUNCTION__, Status));
    goto CleanUp;
  }

  //
  // Search the handles to find one that is on a GPT partition on a hard drive.
  //
  Found = FALSE;
  for (Index = 0; (Index < NumHandles) && (Found == FALSE); Index += 1) {
    DevicePath = DevicePathFromHandle (HandleBuffer[Index]);
    if (DevicePath == NULL) {
      continue;
    }

    //
    // Convert the device path to a string to print it.
    //
    PathNameStr = ConvertDevicePathToText (DevicePath, TRUE, TRUE);
    DEBUG ((DEBUG_ERROR, "%a: device path %d -> %s\n", __FUNCTION__, Index, PathNameStr));

    //
    // Check if this is a block IO device path. If it is not, keep searching.
    // This changes our locate device path variable, so we'll have to restore
    // it afterwards.
    //
    Status = gBS->LocateDevicePath (
                    &gEfiBlockIoProtocolGuid,
                    &DevicePath,
                    &Handle
                    );

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: not a block IO device path\n", __FUNCTION__));
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID **)&SfProtocol
                    );

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to locate Simple FS protocol using the handle to fs0: %r \n", __FUNCTION__, Status));
      goto CleanUp;
    }

    //
    // Open the volume/partition.
    //
    Status = SfProtocol->OpenVolume (SfProtocol, &FileHandle);
    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to open Simple FS volume fs0: %r \n", __FUNCTION__, Status));
      goto CleanUp;
    }

    //
    // Ensure the PktName file is present
    //
    Status = FileHandle->Open (FileHandle, &FileHandle2, L"DxePagingAuditTestApp.efi", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: Unable to locate %s. Status: %r\n", __FUNCTION__, L"DxePagingAuditTestApp.efi", Status));
      Status = FileHandleClose (FileHandle);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Error closing Vol Handle. Code = %r\n", __FUNCTION__, Status));
      }

      Status = EFI_NOT_FOUND;
      continue;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Located app device path\n", __FUNCTION__));
      Status     = FileHandleClose (FileHandle2);
      *Fs_Handle = (EFI_FILE *)FileHandle;
      break;
    }
  }

CleanUp:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Status;
}

/**
  DxePagingAuditTestAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
DxePagingAuditTestAppEntryPoint (
  IN     EFI_HANDLE        ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  UNIT_TEST_FRAMEWORK_HANDLE     Fw       = NULL;
  UNIT_TEST_SUITE_HANDLE         Misc     = NULL;
  BOOLEAN                        RunTests = TRUE;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParams;
  EFI_FILE                       *Fs_Handle;

  DEBUG ((DEBUG_ERROR, "%a()\n", __FUNCTION__));

  DEBUG ((DEBUG_ERROR, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParams
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a Could not retrieve command line args!\n", __FUNCTION__));
    return EFI_PROTOCOL_ERROR;
  }

  if (ShellParams->Argc > 1) {
    RunTests = FALSE;
    if (StrnCmp (ShellParams->Argv[1], L"-r", 4) == 0) {
      RunTests = TRUE;
    } else if (StrnCmp (ShellParams->Argv[1], L"-d", 4) == 0) {
      Status = OpenAppSFS (&Fs_Handle);

      if (!EFI_ERROR ((Status))) {
        DumpPagingInfo (Fs_Handle);
      } else {
        DumpPagingInfo (NULL);
      }
    } else {
      if (StrnCmp (ShellParams->Argv[1], L"-h", 4) != 0) {
        DEBUG ((DEBUG_INFO, "Invalid argument!\n"));
      }

      DEBUG ((DEBUG_INFO, "-h : Print available flags\n"));
      DEBUG ((DEBUG_INFO, "-d : Dump the page table files to the EFI partition\n"));
      DEBUG ((DEBUG_INFO, "-r : Run the application tests\n"));
      DEBUG ((DEBUG_INFO, "NOTE: Combined flags (i.e. -rd) is not supported\n"));
    }
  }

  if (RunTests) {
    //
    // Start setting up the test framework for running the tests.
    //
    Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
    }

    //
    // Create test suite
    //
    CreateUnitTestSuite (&Misc, Fw, "Miscellaneous tests", "Security.Misc", NULL, NULL);

    if (Misc == NULL) {
      DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for TestSuite\n"));
      goto EXIT;
    }

    AddTestCase (Misc, "No pages can be read,write,execute", "Security.Misc.NoReadWriteExecute", NoReadWriteExecute, NULL, NULL, NULL);

    //
    // Execute the tests.
    //
    Status = RunAllTestSuites (Fw);
EXIT:

    if (Fw) {
      FreeUnitTestFramework (Fw);
    }
  }

  return EFI_SUCCESS;
}   // DxePagingAuditTestAppEntryPoint()
