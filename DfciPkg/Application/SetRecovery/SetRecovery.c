/** @file
SetRecovery.c

This application will create a recovery packet.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/DfciRecoveryLib.h>
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
    { L"-f", TypeValue },  // -f Blob File Name
    { L"-c", TypeValue },  // -c Cert File Name
    { L"-s", TypeValue },  // -s Max String Size
    { NULL,  TypeMax }
};

BOOLEAN       gFlagVerbose         = FALSE;
CONST CHAR16 *gOutputFileName      = NULL;
CONST CHAR16 *gCertFileName        = NULL;
CONST CHAR16 *gMaxStringSize       = NULL;

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
    UINT8                          *LocalBuffer;

    if ((NULL == FileName) || (NULL == Buffer) || (NULL == BufferSize)) {
        AsciiPrint("Internal error in SetDfciVariable\n");
        return EFI_INVALID_PARAMETER;
    }

    if (gFlagVerbose) {
        AsciiPrint ("Opening %.\n", FileName);
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

    if (gFlagVerbose) {
        AsciiPrint ("Size of  %s id %d.\n", FileName, *BufferSize);
    }

    LocalBuffer = AllocatePool (*BufferSize);

    if (NULL == LocalBuffer) {
        AsciiPrint ("Unable to allocate buffer for %s\n", FileName);
        Status = EFI_OUT_OF_RESOURCES;
        ReadSize = 0;
    } else {
        if (gFlagVerbose) {
            AsciiPrint ("Reading %s into %p.\n", FileName, LocalBuffer);
        }
        ReadSize = *BufferSize;
        Status = ShellReadFile (FileHandle, &ReadSize, LocalBuffer);
        ShellCloseFile (&FileHandle);

        if (EFI_ERROR(Status)) {
            AsciiPrint ("Error reading file %s. Code = %r\n", FileName, Status);
        } else if (ReadSize != *BufferSize) {
            AsciiPrint ("File Read not complete reading file %s. Req=%d,Act=%dr\n", FileName, *BufferSize, ReadSize);
            Status = EFI_BUFFER_TOO_SMALL;
        }
    }

    if (gFlagVerbose) {
        AsciiPrint ("Finished Reading %s, size=%d. Code=%r\n", FileName, ReadSize, Status);
    }

    if (EFI_ERROR(Status)) {
        if (NULL != LocalBuffer) {
            FreePool (LocalBuffer);
            LocalBuffer = NULL;
        }
    }
    *Buffer = LocalBuffer;

    return Status;
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
SetRecoveryEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    ) {

    BOOLEAN                         FlagH;
    LIST_ENTRY                     *ParamPackage;
    CHAR16                         *ProblemParm = NULL;
    EFI_STATUS                      Status;
    CHAR8                          *Cert;
    UINT64                          CertSize;
    CHAR8                          *EData;
    UINT64                          EDataSize;
    DFCI_RECOVERY_CHALLENGE        *Challenge = NULL;
    UINTN                           ChallengeSize;
    DFCI_RECOVERY_CHALLENGE        *ChallengeV2;
    UINTN                           ChallengeV2Size;
    UINTN                           Sz;
    UINTN                           CurSz;
    CHAR8                          *SnString;
    CHAR8                          *MfgString;
    CHAR8                          *ModelString;
    UINTN                           MaxStringSize = DFCI_MULTI_STRING_MAX_SIZE;


    AsciiPrint ("SetRecovery V0.1\n");

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

    if (FlagH) {
        AsciiPrint ("SetRecovery -c CertFileName -f OutputFileName [-s Msize] [-?] [-h]\n");
        AsciiPrint ("   -c    CertFileName\n");
        AsciiPrint ("   -f    OutputFileName\n");
        AsciiPrint ("   -s    MaxStringSize\n");
        AsciiPrint ("   -h    Print this Help\n");
        AsciiPrint ("   -r    Print results\n");

        return 0;
    }

    gOutputFileName = ShellCommandLineGetValue(ParamPackage, L"-f");
    gCertFileName = ShellCommandLineGetValue(ParamPackage, L"-c");
    gMaxStringSize = ShellCommandLineGetValue(ParamPackage, L"-s");

    if ((NULL == gOutputFileName) || (NULL == gCertFileName)) {
        AsciiPrint ("BlobFileName and CertFileName are both required\n");
        return 8;
    }

    if (gMaxStringSize != NULL) {
        Status = StrDecimalToUintnS (gMaxStringSize, NULL, &MaxStringSize);
        if (EFI_ERROR(Status)) {
            AsciiPrint("Invalid conversion of MaxStringSize. Code=%r\n",Status);
        }
    }

    Status = ReadFileIntoMemory (gCertFileName, &Cert, &CertSize);

    if (EFI_ERROR(Status)) {
        return Status;
    }

    //Make the Challenge Packet
    Status = GetRecoveryChallenge(&Challenge, &ChallengeSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint("%a - Failed to get Recovery Challenge %r\n", __FUNCTION__, Status);
        return Status;
    }

    AsciiPrint ("RecoveryChallenge packet of %d bytes created\n", ChallengeSize);

    ChallengeV2 = (DFCI_RECOVERY_CHALLENGE * ) AllocatePool (512);
    if (ChallengeV2 == NULL) {
        AsciiPrint ("Unable to allocate memory for ChallengeV2\n" );
        return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (ChallengeV2, Challenge, sizeof(DFCI_RECOVERY_CHALLENGE));

    ChallengeV2->MultiString[0] = '\0';

    SnString = "SN47866398211-779581006";
    MfgString =  "-My Computer Corp the one in Outlandia";
    ModelString = "-My Computer Corp Model 1 supporing Outlandia, -Just some more data to fill up the maximum space with data to test truncation at the suspected maximum size";


    CurSz = 0;
    Sz = AsciiStrLen (SnString);
    AsciiPrint ("SnString - Len=%d, CurSz=%d\n", Sz, CurSz);
    if (Sz > (MaxStringSize - 1) ) {
        Sz = MaxStringSize - 1;
    }
    CurSz += Sz;
    AsciiPrint ("SnString - Len=%d, CurSz=%d\n", Sz, CurSz);

    AsciiStrnCatS (ChallengeV2->MultiString, MaxStringSize, SnString, Sz);

    Sz = AsciiStrLen (MfgString);
    AsciiPrint ("MfgString - Len=%d, CurSz=%d\n", Sz, CurSz);
    if (Sz > (MaxStringSize - CurSz - 1) ) {
        Sz = MaxStringSize - CurSz - 1;
    }
    CurSz += Sz;
    AsciiPrint ("MfgString - Len=%d, CurSz=%d\n", Sz, CurSz);

    if (Sz > 0) {
        AsciiStrnCatS(ChallengeV2->MultiString, MaxStringSize, MfgString, Sz);
    }

    Sz = AsciiStrLen (ModelString);
    AsciiPrint ("ModelString - Len=%d, CurSz=%d\n", Sz, CurSz);
    if (Sz > (MaxStringSize - CurSz - 1) ) {
        Sz = MaxStringSize - CurSz - 1;
    }
    CurSz += Sz;
    AsciiPrint ("ModelString - Len=%d, CurSz=%d\n", Sz, CurSz);

    if (Sz > 0) {
        AsciiStrnCatS (ChallengeV2->MultiString, MaxStringSize, ModelString, Sz);
    }

    AsciiPrint ("Size checks:\n");
    AsciiPrint ("  Total characters is string is %d\n", AsciiStrLen(SnString) + AsciiStrLen(MfgString) + AsciiStrLen(ModelString) );
    AsciiPrint ("  Computed size of MultiString is %d\n", CurSz + 1);
    AsciiPrint ("  Actual size of MultiString is %d\n", AsciiStrSize(ChallengeV2->MultiString));

    ChallengeV2Size = ChallengeSize + AsciiStrSize (ChallengeV2->MultiString);

    AsciiPrint ("The multistring size is %d for a total size of %d\n", AsciiStrSize (ChallengeV2->MultiString), ChallengeV2Size);

    //Encrypt Challenge
    Status = EncryptRecoveryChallenge(Challenge, ChallengeSize, Cert, CertSize, &EData, &EDataSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint("%a - Failed to Encrypt Recovery Challenge %r\n", __FUNCTION__, Status);
        return Status;
    }

    AsciiPrint ("File of %d bytes would have been created for the original challenge\n", EDataSize);

    //Encrypt ChallengeV2
    Status = EncryptRecoveryChallenge(ChallengeV2, ChallengeV2Size, Cert, CertSize, &EData, &EDataSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint("%a - Failed to Encrypt Recovery Challenge %r\n", __FUNCTION__, Status);
        return Status;
    }

    AsciiPrint ("File of %d bytes would have been created for the new challenge\n", EDataSize);


    return 0;
}