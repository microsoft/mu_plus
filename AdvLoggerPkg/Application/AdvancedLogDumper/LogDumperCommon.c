/** @file
LogDumperCommon.c

This application will dump the AdvancedLog to a file.

Copyright (C) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <LogDumper.h>

//
// Parameters
//
STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-h", TypeFlag  },    // -h Help
  { L"-r", TypeFlag  },    // -r Raw file
  { L"-v", TypeFlag  },    // -v Verbose
  { L"-o", TypeValue },    // -o output file
  { NULL,  TypeMax   }
};

BOOLEAN                                           mFlagVerbose = FALSE;
STATIC ADVANCED_LOGGER_ACCESS_MESSAGE_LINE_ENTRY  mAccessEntry = { 0 };
extern EFI_HII_HANDLE                             gAdvLogHiiHandle;

/**
 * Requests the Advanced Logger variables and dumps the raw file to a file
 * @param[in] FileHandle         The handle of the file we want to write to
 * @param[in] Verbose            Whether debugging statements should be written out to console
 * @retval EFI_SUCCESS           We were able to write to the file
 * @retval EFI_OUT_OF_RESOURCES  We failed to allocate a buffer
 * @retval EFI_NOT_FOUND         If AdvLogger didn't create a buffer or isn't installed
 * @retval EFI_INVALID_PARAMETER The FileHandle was bad or null
 * @retval Others                Errors passed from GetVariable or ShellWriteFile
 */
EFI_STATUS
EFIAPI
RawDumpToFile (
  IN SHELL_FILE_HANDLE  FileHandle,
  IN BOOLEAN            Verbose
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINT32      Attributes;
  UINTN       BufferSize;
  VOID        *Buffer;
  CHAR16      VarName[32];

  if (FileHandle == NULL) {
    AsciiPrint ("[%a] FileHandle is Null\n", __FUNCTION__);
    return EFI_INVALID_PARAMETER;
  }

  // Initialize the variables
  Index  = 0;
  Status = EFI_SUCCESS;

  //
  // When getting the AdvancedLogger log through the memory variable interface, you cannot
  // rely on EFI_BUFFER_TOO_SMALL.  The reason is that more log messages can occur between
  // the two calls to GetVariable, resulting in a second return of EFI_BUFFER_TOO_SMALL.
  // To solve this issue, LogDumper allocates one buffer to hold the largest size block
  // from variable services; the value of PcdMaxVariableSize.
  //
  // NOTE: The value of PcdMaxVariableSize must be the same or larger when building LogDumper
  //       than the value used by the variable services code.
  //
  Buffer = AllocatePool (PcdGet32 (PcdMaxVariableSize));
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Since we don't know how many advanced logger entries there are, we need to loop until
  // we get an EFI_NOT_FOUND return code.  Getting EFI_NOT_FOUND on the first variable means
  // Advanced logger didn't create a log, which is an error.
  //
  while (!EFI_ERROR (Status)) {
    // Write the name of the block we want to access
    UnicodeSPrint (VarName, sizeof (VarName), L"V%d", Index);
    if (Verbose) {
      AsciiPrint ("Requesting Block %s\n", VarName);
    }

    BufferSize = PcdGet32 (PcdMaxVariableSize);

    // Get the advanced logger through the variable
    Status = gRT->GetVariable (
                    VarName,
                    &gAdvLoggerAccessGuid,
                    &Attributes,
                    &BufferSize,
                    (VOID **)Buffer
                    );

    if (Verbose) {
      if (EFI_ERROR (Status)) {
        if (Status == EFI_BUFFER_TOO_SMALL) {
          AsciiPrint ("Need a buffer size of %d bytes. Status = %r\n", BufferSize, Status);
        } else {
          if ((Status == EFI_NOT_FOUND) && (Index != 0)) {
            AsciiPrint ("Error from GetVariable. Status = %r\n", Status);
          }
        }
      } else {
        AsciiPrint ("Read %d bytes. Status = %r\n", BufferSize, Status);
      }
    }

    // Check if the variable read was successful
    if (EFI_ERROR (Status)) {
      // If we don't find the variable and we're not at the first index then we are at the end of the log
      if ((Status == EFI_NOT_FOUND) && (Index != 0)) {
        Status = EFI_SUCCESS;
      }

      // otherwise, return EFI_NOT_FOUND
      goto Exit;
    }

    // Write the buffer out to the file
    Status = ShellWriteFile (FileHandle, &BufferSize, Buffer);
    Index += 1;     // Increment the log pointer
  }

  // Free the memory buffer
  if (Buffer != NULL) {
    FreePool (Buffer);
  }

Exit:
  return Status;
}

/**
  Dumps the Advanced Logger text to a test file.

  @param[in] FileHandle         The handle of the file we want to write to

  @retval EFI_SUCCESS           We were able to write to the file
  @retval EFI_NOT_FOUND         If AdvLogger didn't create a buffer or isn't installed
  @retval EFI_INVALID_PARAMETER The FileHandle was bad or null
  @retval Others                Errors passed from ShellWriteFile
 */
EFI_STATUS
EFIAPI
TextDumpToFile (
  IN SHELL_FILE_HANDLE  FileHandle,
  IN BOOLEAN            Verbose
  )
{
  UINTN       BufferSize;
  UINTN       LineCount;
  EFI_STATUS  Status;

  if (FileHandle == NULL) {
    AsciiPrint ("[%a] FileHandle is Null\n", __FUNCTION__);
    return EFI_INVALID_PARAMETER;
  }

  LineCount = 0;
  Status    = AdvancedLoggerAccessLibGetNextFormattedLine (&mAccessEntry);
  while (!EFI_ERROR (Status)) {
    BufferSize = mAccessEntry.MessageLen;
    if (BufferSize > 0) {
      // Only selected messages go to the serial port.
      // Write the buffer out to the file
      Status = ShellWriteFile (FileHandle, &BufferSize, (VOID *)mAccessEntry.Message);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to write log data to file: %r\n", __FUNCTION__, Status));
        break;
      }
    }

    LineCount++;

    Status = AdvancedLoggerAccessLibGetNextFormattedLine (&mAccessEntry);
  }

  AsciiPrint ("Copied %d lines to the output file\n", LineCount);

  if (Status == EFI_END_OF_FILE) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  The worker function for LogDumper Application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.


  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
AdvLogDumperInternalWorker (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BOOLEAN            FlagH;
  BOOLEAN            FlagR;
  EFI_STATUS         Status;
  LIST_ENTRY         *ParamPackage;
  CHAR16             *ProblemParm = NULL;
  SHELL_FILE_HANDLE  FileHandle;
  CONST CHAR16       *OutputFileName = NULL;

  AsciiPrint ("Dumping  Advanced Logger to file\n");

  Status = ShellCommandLineParseEx (ParamList, &ParamPackage, &ProblemParm, FALSE, TRUE);
  if (EFI_ERROR (Status)) {
    if (ProblemParm != NULL) {
      AsciiPrint ("Invalid parameter %s\n", ProblemParm);
      FreePool (ProblemParm);
    } else {
      AsciiPrint ("Unable to parse command line. Code=%r", Status);
    }

    return SHELL_INVALID_PARAMETER;
  }

  FlagH        = ShellCommandLineGetFlag (ParamPackage, L"-h");
  FlagR        = ShellCommandLineGetFlag (ParamPackage, L"-r");
  mFlagVerbose = ShellCommandLineGetFlag (ParamPackage, L"-v");

  OutputFileName = ShellCommandLineGetValue (ParamPackage, L"-o");

  if (NULL == OutputFileName) {
    AsciiPrint ("Please specify an output file.\n");
    FlagH |= TRUE;
  }

  if (FlagH) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_ADV_LOG_HELP), gAdvLogHiiHandle);
    return 0;
  }

  //
  // First lets open the file if it exists so we can delete it...This is the work around for truncation
  //
  Status = ShellOpenFileByName (
             OutputFileName,
             &FileHandle,
             EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ,
             0
             );

  if (!EFI_ERROR (Status)) {
    //
    // If file handle above was opened it will be closed by the delete.
    //
    Status = ShellDeleteFile (&FileHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a failed to delete file %r\n", __FUNCTION__, Status));
    }
  }

  Status = ShellOpenFileByName (
             OutputFileName,
             &FileHandle,
             EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ,
             0
             );

  if (EFI_ERROR (Status)) {
    AsciiPrint ("ERROR: Failed to open %s file. Status = %r\n", OutputFileName, Status);
    return Status;
  }

  if (FlagR) {
    Status = RawDumpToFile (FileHandle, mFlagVerbose);
  } else {
    Status = TextDumpToFile (FileHandle, mFlagVerbose);
  }

  if (EFI_ERROR (Status)) {
    AsciiPrint ("ERROR: Failed to dump the Advanced Logger file = %r\n", Status);
  }

  ShellCloseFile (&FileHandle);

  return Status;
}

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param ImageHandle            The image handle of the process.

  @return HII handle.
**/
EFI_HII_HANDLE
InitializeHiiPackage (
  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return HiiHandle;
}
