
/** @file
  This application will request new DFCI configuration data from server.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/ImageAuthentication.h>
#include <Guid/DfciSettingsGuid.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "Compress.h"

#define MAX_URL_FILE_SIZE   512
#define MAX_CERT_FILE_SIZE 8000

//
// Parameters
//
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
    { L"-v", TypeFlag },   // -v   Verbose Mode
    { L"-h", TypeFlag },   // -h   Help
    { L"-?", TypeFlag },   // -h   Help
    { L"-z", TypeFlag },   // -z   Compress Certificate
    { L"-u", TypeValue },  // -u URL input file
    { L"-c", TypeValue },  // -c Cert File
    { NULL,  TypeMax }
};

// Characters allowed in a URL
CONST CHAR8 *AllowedChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.~-%!*'();:@&=+$,/?#[]";

//
// Possible certificate file suffixes, end with NULL pointer.
//
CHAR16* mValidCertSuffix[] = {
  L".cer",
  L".der",
  L".crt",
  L".pem",
  NULL
};

/**
 * ValidateFileExtension - validates if file extension is supported
 *
 *
 * @param FileSuffix  - Suffix of current file name
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateFileExtension (CONST CHAR16 *FileSuffix) {

    UINTN     Index;

    for (Index = 0; mValidCertSuffix[Index] != NULL; Index++) {
      if (StrCmp (FileSuffix, mValidCertSuffix[Index]) == 0) {
        return TRUE;
      }
    }
    return FALSE;
}

/**
 * EnrollGetFileSize
 *
 * @param FileName
 *
 * @return UINT64
 */
UINT64
EnrollGetFileSize (IN CONST CHAR16 *FileName) {

    SHELL_FILE_HANDLE               FileHandle;
    UINT64                          FileSize;
    EFI_STATUS                      Status;

    AsciiPrint ("Getting file size 1 for %s\n",FileName);

    Status = ShellOpenFileByName(FileName,
                                &FileHandle,
                                 EFI_FILE_MODE_READ,
                                 0);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to open %s file. Status = %r\n", FileName, Status);
        return 0;
    }

    AsciiPrint ("Getting file size 2 for %s\n",FileName);
    FileSize = 0;
    Status = ShellGetFileSize (FileHandle,
                               &FileSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to get filesize of %s. Status = %r\n", FileName, Status);
    }
    AsciiPrint ("Getting file size 3 for %s\n",FileName);

    ShellCloseFile (&FileHandle);

    return FileSize;
}

/**
 * ReadFileIntoMemory
 *
 *
 * @param FileName
 * @param Buffer
 * @param BufferSize
 *
 * @return EFI_STATUS
 */
EFI_STATUS
ReadFileIntoMemory (IN CONST CHAR16  *FileName,
                    IN       VOID    *Buffer,
                    IN       UINT64   BufferSize) {

    SHELL_FILE_HANDLE               FileHandle;
    UINT64                          FileSize;
    UINT64                          ReadSize;
    EFI_STATUS                      Status;

    Status = ShellOpenFileByName (FileName,
                                 &FileHandle,
                                  EFI_FILE_MODE_READ,
                                  0);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to open %s file. Status = %r\n", FileName, Status);
        return Status;
    }

    Status = ShellGetFileSize (FileHandle,
                               &FileSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Failed to get filesize of %s. Status = %r\n", FileName, Status);
        return Status;
    }

    if (BufferSize != FileSize) {
        AsciiPrint ("File contents have unexpected size. Size=%d)\n", FileSize);
        return EFI_INVALID_PARAMETER;
    }

    ReadSize = FileSize;
    Status = ShellReadFile (FileHandle, &ReadSize, Buffer);
    ShellCloseFile (&FileHandle);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Error reading file %s. Code = %r\n", FileName, Status);
        return Status;
    }

    if (ReadSize != FileSize) {
        AsciiPrint ("File Read not complete reading file %s. Req=%d,Act=%dr\n", FileName, FileSize, ReadSize);
        return EFI_BUFFER_TOO_SMALL;
    }

    return EFI_SUCCESS;
}

/**
 * ReadUrlFileIntoMemory
 *
 *
 * @param FileName   - File name to read
 * @param BufferSize - On INPUT, maximum size, on OUTPUT, current size;
 *
 * @return CHAR16*
 */
CHAR8 *
ReadUrlFileIntoMemory (IN CONST CHAR16   *FileName,
                       OUT      UINT64   *UrlSize) {

    UINT64                          AllocateSize;
    UINT64                          FileSize;
    UINTN                           i;
    UINTN                           j;
    EFI_STATUS                      Status;
    CHAR8                          *Url;
    BOOLEAN                         UrlValid;

    FileSize = EnrollGetFileSize (FileName);

    if ((0 == FileSize) || (FileSize > MAX_URL_FILE_SIZE)) {
        AsciiPrint ("Invalid URL Length. Size=%d)\n", FileSize);
        return NULL;
    }

    AllocateSize = FileSize + 1;
    Url = AllocatePool(AllocateSize);  // Allocate room for NULL
    if (NULL == Url) {
        AsciiPrint ("Unable to allocate buffer for %s\n",FileName);
        return NULL;
    }

    Url[FileSize] = '\0';
    Status = ReadFileIntoMemory (FileName, Url, FileSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Unable to read %s. Status = %r\n", FileName, Status);
        FreePool (Url);
        return NULL;
    }

    // Make sure the URL is valid
    UrlValid = TRUE;
    for (i = 0; (i<FileSize && UrlValid); i++) {
        UrlValid = FALSE;
        for (j = 0; j < AsciiStrLen(AllowedChars); j++) {
            if (Url[i]==AllowedChars[j]) {
                UrlValid = TRUE;
                break;
            } else if ((Url[i] == '\r') || (Url[i] == '\n')) {
                CopyMem (&Url[i],&Url[i+1],FileSize - i);
                i--;            // Restart with this character
                FileSize--;     // Account for removal here
                AllocateSize--; //  and here
            }
        }
    }

    if (!UrlValid) {
        AsciiPrint ("Invalid characters in the URL near location %d\n", i);
        FreePool (Url);
        return NULL;
    }

    *UrlSize = AllocateSize;
    return Url;
}

/**
 * ReadCertFileIntoMemory
 *
 *
 * @param FileName   - File name to read
 * @param BufferSize - OUTPUT, current size;
 *
 * @return CHAR8*
 */
CHAR8 *
ReadCertFileIntoMemory (IN CONST CHAR16  *FileName,
                        OUT      UINT64  *BufferSize) {

    UINT64                          AllocateSize;
    CHAR8                          *Buffer;
    EFI_SIGNATURE_LIST             *CACert;
    EFI_SIGNATURE_DATA             *CACertData;
    UINT64                          FileSize;
    EFI_STATUS                      Status;

    FileSize = EnrollGetFileSize (FileName);

    if ((0 == FileSize) || (FileSize > MAX_CERT_FILE_SIZE)) {
        AsciiPrint ("Invalid CERT Length. Size=%d)\n", FileSize);
        return NULL;
    }

    AllocateSize = FileSize + sizeof(EFI_SIGNATURE_LIST) + sizeof(EFI_SIGNATURE_DATA) - 1;

    AsciiPrint ("Filesize=%d, AllocateSize=%d\n",FileSize,AllocateSize);

    Buffer = AllocatePool(AllocateSize);

    if (NULL == Buffer) {
        AsciiPrint ("Unable to allocate buffer for %s\n",FileName);
        return NULL;
    }

    //
    // Fill Certificate Database parameters.
    //
    CACert = (EFI_SIGNATURE_LIST*) Buffer;
    CACert->SignatureListSize   = (UINT32) AllocateSize;
    CACert->SignatureHeaderSize = 0;
    CACert->SignatureSize = (UINT32) (sizeof(EFI_SIGNATURE_DATA) - 1 + FileSize);
    CopyGuid (&CACert->SignatureType, &gEfiCertX509Guid);

    CACertData = (EFI_SIGNATURE_DATA*) ((UINT8* ) CACert + sizeof (EFI_SIGNATURE_LIST));
    CopyGuid (&CACertData->SignatureOwner, &gDfciSettingsGuid);

    AsciiPrint ("Buffer=%p, ReadP=%p\n",Buffer, &CACertData->SignatureData[0]);
    Status = ReadFileIntoMemory (FileName, &CACertData->SignatureData[0], FileSize);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Unable to read %s. Status = %r\n", FileName, Status);
        FreePool (Buffer);
        return NULL;
    }

    AsciiPrint ("Returning BufferSize=%d\n",*BufferSize);
    *BufferSize = AllocateSize;
    return Buffer;
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
EnrollInDfciEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    ) {

    CHAR8                          *CertBuffer;
    CONST CHAR16                   *CertFileName;
    UINT64                          CertFileSize;
    CHAR8                          *CompressedBuffer;
    UINT64                          CompressedSize;
    BOOLEAN                         FlagH;
    BOOLEAN                         FlagZ;
    LIST_ENTRY                     *ParamPackage;
    CHAR16                         *ProblemParm = NULL;
    EFI_STATUS                      Status;
    CHAR8                          *UrlBuffer;
    CONST CHAR16                   *UrlFileName;
    UINT64                          UrlFileSize;

    AsciiPrint ("EnrollInDfci V0.1\n");

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
    FlagZ   = ShellCommandLineGetFlag(ParamPackage, L"-z");

    if (FlagH) {
        AsciiPrint ("EnrollInDfci -c CertFileName -u UrlFileName [-z] [-h] [-?] \n");
        AsciiPrint ("   -h    Print this Help\n");
        AsciiPrint ("   -l    Print this help\n");
        AsciiPrint ("   -z    Compress Certificate\n");
        AsciiPrint ("   -c    Certificate File Name - Certificate for HTTPS\n");
        AsciiPrint ("   -u    UrlFIleName - ASCII Encoded file with base URL\"\n");

        return 0;
    }

    CertFileName = ShellCommandLineGetValue(ParamPackage, L"-c");
    if (NULL == CertFileName) {
        AsciiPrint ("Certificate file is required\n");
        return 8;
    }
    UrlFileName = ShellCommandLineGetValue(ParamPackage, L"-u");
    if (NULL == UrlFileName) {
        AsciiPrint ("Url file is required\n");
        return 8;
    }
    AsciiPrint ("URL Checking FileName\n");

    if (!ValidateFileExtension (&CertFileName[StrLen(CertFileName) - 4])) {
        AsciiPrint ("Cert file name must be one of .cer, .der, .crt, or .pem\n");
        return 8;
    }
    AsciiPrint ("Processing Cert file\n");
    CertBuffer = ReadCertFileIntoMemory (CertFileName, &CertFileSize);

    if (NULL == CertBuffer) {
        return 8;
    }

    AsciiPrint ("Processing URL file\n");
    UrlBuffer = ReadUrlFileIntoMemory (UrlFileName, &UrlFileSize);

    if (NULL == UrlBuffer) {
        FreePool (UrlBuffer);
        return 8;
    }

    AsciiPrint ("Saving URL file\n");
    Status = gRT->SetVariable (DFCI_SETTINGS_RECOVERY_URL_NAME,
                              &gDfciSettingsGuid,
                               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                               UrlFileSize,
                               UrlBuffer);
    FreePool (UrlBuffer);
    if (EFI_ERROR(Status)) {
        FreePool (CertBuffer);
        AsciiPrint ("Error saving URL setting. Code=%r\n",Status);
        return 8;
    }

    if (FlagZ) {
        CompressedSize = 0;
        Status = Compress(CertBuffer, CertFileSize, NULL, &CompressedSize);
        if (EFI_BUFFER_TOO_SMALL != Status) {
            FreePool (CertBuffer);
            AsciiPrint ("Error determining compressed size. Code=%r\n",Status);
            return 8;
        }
        CompressedBuffer = AllocatePool (CompressedSize);
        if (NULL == CompressedBuffer) {
            FreePool (CertBuffer);
            AsciiPrint ("Error allocating compressed buffer. Size = %d\n", CompressedSize);
            return 8;
        }
        Status = Compress(CertBuffer, CertFileSize, CompressedBuffer, &CompressedSize);
        if (EFI_ERROR(Status)) {
            FreePool (CompressedBuffer);
            FreePool (CertBuffer);
            AsciiPrint ("Error compressing Cert. Code=%r\n",Status);
            return 8;
        }

        AsciiPrint ("Cert compressed. Size=%d, CompressedSize=%d\n", CertFileSize, CompressedSize);

        FreePool (CertBuffer);
        CertBuffer = CompressedBuffer;
        CertFileSize = CompressedSize;
    }

    AsciiPrint ("Saving CERT file\n");
    Status = gRT->SetVariable (DFCI_SETTINGS_HTTPS_CERT_NAME,
                               &gDfciSettingsGuid,
                               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                               CertFileSize,
                               CertBuffer);
    FreePool (CertBuffer);
    if (EFI_ERROR(Status)) {
        AsciiPrint ("Error saving certificate. Code=%r\n",Status);
        return 8;
    }

    return 0;
}

