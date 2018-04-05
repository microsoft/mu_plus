
/** @file
  This application will request new DFCI configuration data from server.


  Copyright (c) 2017, Microsoft Corporation. All rights reserved.<BR>

**/


#include <Uefi.h>

#include <Guid/DfciSettingsGuid.h>
#include <Guid/DfciIdentityAndAuthManagerVariables.h>
#include <Guid/DfciPermissionManagerVariables.h>
#include <Guid/DfciSettingsManagerVariables.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Parameters
//
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
    { L"-h", TypeFlag },   // -h   Help
    { L"-?", TypeFlag },   // -?   Help
    { L"-v", TypeFlag },   // -v Verbose
    { L"-r", TypeFlag },   // -r Display Results
    { L"-c", TypeFlag },   // -r Display Current Settings
    { L"-i", TypeValue },  // -i Identity Packet
    { L"-p", TypeValue },  // -p Permission Packet
    { L"-s", TypeValue },  // -s Settings Packete
    { NULL,  TypeMax }
};

BOOLEAN       gFlagVerbose         = FALSE;
BOOLEAN       gFlagResults         = FALSE;
BOOLEAN       gFlagCurrent         = FALSE;
CONST CHAR16 *gIdentityFileName    = NULL;
CONST CHAR16 *gPermissionsFileName = NULL;
CONST CHAR16 *gSettingsFileName    = NULL;

/**
 * ReadFileIntoMemory
 *
 *
 * @param FileName  - Pointer to file name
 * @param Buffer    - Pointer to buffer allocated by this routine
 * @param BufferSize- Pointer to file size
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ReadFileIntoMemory (IN  CONST CHAR16  *FileName,
                    OUT       UINT8  **Buffer,
                    OUT       UINT64  *BufferSize) {

    SHELL_FILE_HANDLE               FileHandle;
    UINT64                          ReadSize;
    EFI_STATUS                      Status;

    if ((NULL == FileName) || (NULL == Buffer) || (NULL == BufferSize)) {
        AsciiPrint("Internal error in SetDfciVariable\n");
        return EFI_INVALID_PARAMETER;
    }

    if (gFlagVerbose) {
        AsciiPrint ("Openning %.\n", FileName);
    }

    Status = ShellOpenFileByName(FileName,
                                 &FileHandle,
                                  EFI_FILE_MODE_READ,
                                  0);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to open %s file. Status = %r\n", FileName, Status);
        return Status;
    }

    Status = ShellGetFileSize (FileHandle,
                               BufferSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to get filesize of %s. Status = %r\n", FileName, Status);
        return Status;
    }

    *Buffer = AllocatePool (*BufferSize);

    if (gFlagVerbose) {
        AsciiPrint ("Reading %.\n", FileName);
    }

    if (NULL == *Buffer) {
        AsciiPrint ("Unable to allocate buffer for %s\n", FileName);
        Status = EFI_OUT_OF_RESOURCES;
    } else {
        ReadSize = *BufferSize;
        Status = ShellReadFile (FileHandle, &ReadSize, Buffer);
        ShellCloseFile (&FileHandle);

        if (EFI_ERROR(Status)) {
            AsciiPrint ("Error reading file %s. Code = %r\n", FileName, Status);
        } else if (ReadSize != *BufferSize) {
            AsciiPrint ("File Read not complete reading file %s. Req=%d,Act=%dr\n", FileName, *BufferSize, ReadSize);
            Status = EFI_BUFFER_TOO_SMALL;
        }
    }

    if (gFlagVerbose) {
        AsciiPrint ("Finished Reading %. Code=%r\n", FileName, Status);
    }

    if (EFI_ERROR(Status)) {
        if (NULL != *Buffer) {
            FreePool (*Buffer);
            *Buffer = NULL;
        }
    }

    return Status;
}

/**
 * SetDfciVariable - set the mailbox variable
 *
 * @param FileName
 * @param VariableName
 *
 * @return EFI_STATUS
 */
EFI_STATUS
SetDfciVariable (
    IN CONST CHAR16 *FileName,
    IN CONST CHAR16 *VariableName) {

    EFI_STATUS  Status;
    CHAR8      *Buffer;
    UINT64      FileSize;

    if (gFlagVerbose) {
        AsciiPrint("Processing file %s\n",FileName);
    }

    if ((NULL == FileName) || NULL == VariableName) {
        AsciiPrint("Internal error in SetDfciVariable\n");
        return EFI_INVALID_PARAMETER;
    }

    Status = ReadFileIntoMemory (FileName, &Buffer, &FileSize);

    if (EFI_ERROR(Status)) {
        AsciiPrint ("Error reading file %s. COde=%r\n", FileName, Status);
        return Status;
    }

    if (gFlagVerbose) {
        AsciiPrint("Saving file %s to %s\n",FileName,VariableName);
    }

    Status = gRT->SetVariable ((CHAR16 *)VariableName,
                              &gDfciSettingsGuid,
                               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                               FileSize,
                               Buffer);

    if (EFI_ERROR(Status)) {
        AsciiPrint ("Error setting variable %s. Code=%r\n", VariableName, Status);
    } else if (gFlagVerbose) {
        AsciiPrint ("Finished Setting %s. Code=%r\n", VariableName, Status);
    }

    FreePool (Buffer);

    return Status;
}

/**
 * Print Results
 *
 * @param VariableName
 *
 * @return EFI_STATUS
 */
EFI_STATUS
PrintResults (IN CONST CHAR16 *VariableName) {

    if (gFlagVerbose) {
        AsciiPrint ("Processing results for %s\n",VariableName);
    }
    AsciiPrint ("Not implenented yet for %s\n",VariableName);
    return EFI_SUCCESS;
}

/**
 * Print Current
 *
 * @param VariableName
 *
 * @return EFI_STATUS
 */
EFI_STATUS
PrintCurrent (IN CONST CHAR16 *VariableName) {

    if (gFlagVerbose) {
        AsciiPrint ("Processing current settings for %s\n",VariableName);
    }
    AsciiPrint ("Not implenented yet for %s\n",VariableName);
    return EFI_SUCCESS;
}

/**
  The user Entry Point for DfciRequest test Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DfciApplyEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    ) {

    BOOLEAN                         FlagH;
    LIST_ENTRY                     *ParamPackage;
    CHAR16                         *ProblemParm = NULL;
    EFI_STATUS                      Status;

    AsciiPrint ("DfciApply V0.1\n");

    Status = ShellCommandLineParseEx(ParamList, &ParamPackage, &ProblemParm, FALSE, TRUE);
    if (EFI_ERROR(Status)) {
        if (ProblemParm != NULL) {
           AsciiPrint ("Invalid parameter %s\n",ProblemParm);
           FreePool (ProblemParm);
        } else {
            AsciiPrint ("Unable to parse command line. Code=%r",Status);
        }
        return SHELL_INVALID_PARAMETER;
    }
    FlagH   = ShellCommandLineGetFlag(ParamPackage, L"-h");
    FlagH  |= ShellCommandLineGetFlag(ParamPackage, L"-?");
    gFlagVerbose = ShellCommandLineGetFlag(ParamPackage, L"-v");
    gFlagResults = ShellCommandLineGetFlag(ParamPackage, L"-r");
    gFlagCurrent = ShellCommandLineGetFlag(ParamPackage, L"-c");

    if (FlagH) {
        AsciiPrint ("DfciApply [-i IdentityFileName] [-p PermissionFileName] [-s SettingsFileName] [-?] [-h] [-r]\n");
        AsciiPrint ("   -h    Print this Help\n");
        AsciiPrint ("   -r    Print results\n");

        return 0;
    }

    gIdentityFileName = ShellCommandLineGetValue(ParamPackage, L"-i");
    gPermissionsFileName = ShellCommandLineGetValue(ParamPackage, L"-p");
    gSettingsFileName = ShellCommandLineGetValue(ParamPackage, L"-s");

    if ((NULL == gIdentityFileName) && (NULL == gPermissionsFileName) && (NULL == gSettingsFileName) &&
        (!gFlagResults) && (!gFlagCurrent)) {
        gFlagResults = TRUE;              // If no options set, set -r
        if (gFlagVerbose) {
            AsciiPrint ("Defaulting to -r\n");
        }
    }

    if (gFlagResults) {
        PrintResults (DFCI_IDENTITY_AUTH_PROVISION_SIGNER_RESULT_VAR_NAME);
        PrintResults (DFCI_PERMISSION_POLICY_RESULT_VAR_NAME);
        PrintResults (XML_SETTINGS_APPLY_OUTPUT_VAR_NAME);
    }

    if (gFlagCurrent) {
        PrintCurrent (XML_SETTINGS_CURRENT_OUTPUT_VAR_NAME);
    }

    if (gIdentityFileName) {
        SetDfciVariable (gIdentityFileName, DFCI_IDENTITY_AUTH_PROVISION_SIGNER_VAR_NAME);
    }
    if (gPermissionsFileName) {
        SetDfciVariable (gPermissionsFileName, DFCI_PERMISSION_POLICY_APPLY_VAR_NAME);
    }
    if (gSettingsFileName) {
        SetDfciVariable (gSettingsFileName, XML_SETTINGS_APPLY_INPUT_VAR_NAME);
    }

    return 0;
}

