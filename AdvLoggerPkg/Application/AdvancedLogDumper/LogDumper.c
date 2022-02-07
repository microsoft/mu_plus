/** @file
LogDumper.c

This application will dump the AdvancedLog to a file.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <LogDumper.h>

//
// Parameters
//
STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-h", TypeFlag  },    // -h   Help
  { L"-?", TypeFlag  },    // -?   Help
  { L"-v", TypeFlag  },    // -v Verbose
  { L"-o", TypeValue },    // -o output file
  { NULL,  TypeMax   }
};

BOOLEAN  mFlagVerbose = FALSE;

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
DumpToFile (
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
  Index      = 0;
  Status     = EFI_SUCCESS;
  Buffer     = NULL;
  BufferSize = 0;
  // Since we don't know how many advanced logger entries there are, we need
  // to loop until we get a not found return code
  // Getting not found on the first variable means advanced logger didn't
  // create a log, which is an error
  while (!EFI_ERROR (Status)) {
    // Write the name of the block we want to access
    UnicodeSPrint (VarName, sizeof (VarName), L"V%d", Index);
    if (Verbose) {
      AsciiPrint ("Requesting Block %s\n", VarName);
    }

    // Get the advanced logger through the variable
    Status = GetVariable3 (
               VarName,
               &gAdvLoggerAccessGuid,
               (VOID **)&Buffer,
               &BufferSize,
               &Attributes
               );
    if (Verbose) {
      AsciiPrint ("Got %d bytes. Status = %r\n", BufferSize, Status);
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

    // Since GetVariable3 allocates new memory each time, we need to free it on every loop
    if (Buffer != NULL) {
      FreePool (Buffer);
      Buffer = NULL;
    }
  }

Exit:
  return Status;
}

/**
  The user Entry Point for LogDumper Application.
  It starts with this function as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
EntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BOOLEAN            FlagH;
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
  FlagH       |= ShellCommandLineGetFlag (ParamPackage, L"-?");
  mFlagVerbose = ShellCommandLineGetFlag (ParamPackage, L"-v");

  OutputFileName = ShellCommandLineGetValue (ParamPackage, L"-o");

  if (NULL == OutputFileName) {
    AsciiPrint ("Please specify an output file.\n");
    FlagH |= TRUE;
  }

  if (FlagH) {
    AsciiPrint ("%a [-o OutputFileName] [-?] [-h] [-d]\n", gEfiCallerBaseName);
    AsciiPrint ("   -h    Print this Help\n");

    return 0;
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

  Status = DumpToFile (FileHandle, mFlagVerbose);
  if (EFI_ERROR (Status)) {
    AsciiPrint ("ERROR: Failed to read in Advanced Log = %r\n", Status);
  }

  ShellCloseFile (&FileHandle);

  return Status;   // EFI_SUCCESS is zero
}
