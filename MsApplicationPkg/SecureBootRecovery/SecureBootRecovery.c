/** @file
  This application will attempt to append the 'Windows UEFI CA 2023' and then reboot the system.
  On success, this application will allow the system to boot into 2023 signed Windows

  Copyright (C) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/VariableFormat.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>

#include "RecoveryPayload.h"

//
// 10 seconds in microseconds
//
#define STALL_10_SECONDS  10000000

//
// The size of an EFI_STATUS in bytes
//
#define STATUS_SIZE         (sizeof(EFI_STATUS) * 2)
#define STATUS_STRING_SIZE  (STATUS_SIZE + sizeof(L"\0"))

/**
  Converts an EFI_STATUS to a hex string
  @param[in] Status The EFI_STATUS to convert

  @return A pointer to a static buffer containing the hex string
  @note The caller must not free the returned pointer
  @note The returned pointer is only valid until the next call to this function
*/
CHAR16 *
StatusToHexString (
  EFI_STATUS  Status
  )
{
  STATIC CHAR16  StatusString[STATUS_STRING_SIZE] = { 0 };
  CONST CHAR16   HexChars[]                       = L"0123456789ABCDEF";
  UINT32         Shift;
  UINT32         Index;
  UINT32         i;

  for (i = 0; i < STATUS_SIZE; i++) {
    Shift           = ((STATUS_SIZE - 1) - i) * 4;
    Index           = (Status >> Shift) & 0xF;
    StatusString[i] = HexChars[Index];
  }

  return StatusString;
}

/**
  Prints an error message to the console

  @param[in] SystemTable  A pointer to the EFI System Table
  @param[in] Message     The message to print
  @param[in] Status      The EFI_STATUS to print
*/
VOID
PrintError (
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN CHAR16            *Message,
  IN EFI_STATUS        Status
  )
{
  if (Message == NULL) {
    return;
  }

  SystemTable->ConOut->OutputString (SystemTable->ConOut, Message);
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Error: 0x");
  SystemTable->ConOut->OutputString (SystemTable->ConOut, StatusToHexString (Status));
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\n");
}

/**
  This function checks if a specified certificate is present in the EFI image security database (db).

  @param[in]  SystemTable      A pointer to the EFI System Table.
  @param[in]  Certificate      A pointer to the CERTIFICATE_ENTRY structure that contains the certificate to search for.
  @param[out] IsFound          A pointer to a BOOLEAN that will be set to TRUE if the certificate is found in the DB, FALSE otherwise.

  @retval EFI_SUCCESS            The certificate was found in the DB.
  @retval EFI_UNSUPPORTED        The DB does not have the expected attributes.
  @retval EFI_INVALID_PARAMETER  The input parameters are invalid.
  @retval others                 An error occurred while attempting to read the DB. See GetVariable or AllocatePool for more details.

**/
EFI_STATUS
IsCertificateInDB (
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN  UINT8            *TargetCert,
  IN  UINTN            TargetCertSize,
  OUT BOOLEAN          *IsFound
  )
{
  EFI_STATUS  Status;
  UINT32      Attributes;
  UINTN       DbSize;
  UINTN       Index;
  UINTN       CertSize;
  UINT8       *Cert;
  UINTN       CertCount;
  UINT8       *Db;

  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *CertData;

  Db         = NULL;
  DbSize     = 0;
  Attributes = VARIABLE_ATTRIBUTE_NV_BS_RT_AT | EFI_VARIABLE_APPEND_WRITE;

  if ((TargetCert == NULL) || (IsFound == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  *IsFound = FALSE;

  //
  // Read the size and attributes of the DB
  //
  Status = SystemTable->RuntimeServices->GetVariable (
                                           L"db",
                                           &gEfiImageSecurityDatabaseGuid,
                                           &Attributes,
                                           &DbSize,
                                           NULL
                                           );

  //
  // Confirm we got the expected error that the buffer was too small
  // We need to know the size of the DB to allocate a buffer to read it
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Likely this will continue to fail on reboot
    //
    if (Status == EFI_SUCCESS) {
      //
      // While unlikely, it is possible that the DB is empty
      // In that case, we should fail out gracefully because there is nothing we can do
      //
      Status = EFI_UNSUPPORTED;
    }

    goto Exit;
  }

  //
  // Check that the DB has the expected attributes
  //
  if ((Attributes & VARIABLE_ATTRIBUTE_NV_BS_RT_AT) != VARIABLE_ATTRIBUTE_NV_BS_RT_AT) {
    Status = EFI_UNSUPPORTED;
    //
    // Likely this will continue to fail on reboot
    //
    goto Exit;
  }

  Status = SystemTable->BootServices->AllocatePool (
                                        EfiBootServicesData,
                                        DbSize,
                                        &Db
                                        );
  if ((Db == NULL) || EFI_ERROR (Status)) {
    //
    // Likely this will continue to fail on reboot
    //
    goto Exit;
  }

  //
  // Grab the DB
  //
  Status = SystemTable->RuntimeServices->GetVariable (
                                           L"db",
                                           &gEfiImageSecurityDatabaseGuid,
                                           &Attributes,
                                           &DbSize,
                                           Db
                                           );
  if (EFI_ERROR (Status)) {
    //
    // Likely this will continue to fail on reboot
    // In that case, we should fail out gracefully because there is nothing we can do
    //
    goto Exit;
  }

  //
  // Confirm the DB is at least as large as the EFI_SIGNATURE_LIST header
  //
  if (DbSize < sizeof (EFI_SIGNATURE_LIST)) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  //
  // The DB is a list of EFI_SIGNATURE_LISTs but we only care about certificates
  // they are not gauranteed to be in any particular order
  //
  CertList = (EFI_SIGNATURE_LIST *)Db;

  //
  // Iterate through the DB entries
  // The DB is a list of EFI_SIGNATURE_LISTs
  //
  while ((DbSize > 0) && (DbSize >= CertList->SignatureListSize)) {
    //
    // If the signature type is not X509, skip it
    //
    if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
      //
      // Otherwise, we have a certificate list and now we need to iterate through the certificates
      //
      CertData  = (EFI_SIGNATURE_DATA *)((UINT8 *)CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
      CertCount = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;

      //
      // Iterate over each certificate in the list
      //
      for (Index = 0; Index < CertCount; Index++) {
        //
        // grab the certificate and its size
        //
        Cert     = CertData->SignatureData;
        CertSize = CertList->SignatureSize - sizeof (EFI_GUID);

        //
        // If the certificate is the same size as the target certificate and the contents are the same, we found it
        //
        if ((TargetCertSize == CertSize) && (CompareMem (Cert, TargetCert, CertSize) == 0)) {
          Status   = EFI_SUCCESS;
          *IsFound = TRUE;
          goto Exit;
        }
      }
    }

    //
    // Regardless of the signature type, we need to move to the next signature list
    // Subtract the size of the current signature list from the total size of the DB
    // Then move the pointer to the next signature list
    //
    DbSize  -= CertList->SignatureListSize;
    CertList = (EFI_SIGNATURE_LIST *)((UINT8 *)CertList + CertList->SignatureListSize);
  }

  //
  // If we get here, we did not find the certificate in the DB, however, we did not encounter any errors
  //
  Status = EFI_SUCCESS;

Exit:

  //
  // Regardless of the outcome, free the DB buffer
  // We only needed it to check if the certificate was in the DB
  //
  if (Db != NULL) {
    SystemTable->BootServices->FreePool (Db);
  }

  return Status;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle        The firmware allocated handle for the EFI image.
  @param[in] SystemTable        A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval EFI_INVALID_PARAMETER SystemTable provided was not valid.
  @retval other                 Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINT32      Attributes;
  UINTN       DbSize;
  BOOLEAN     IsFound;

  IsFound    = FALSE;
  DbSize     = 0;
  Attributes = VARIABLE_ATTRIBUTE_NV_BS_RT_AT | EFI_VARIABLE_APPEND_WRITE;

  //
  // Start checking that the system is in a state we can safely use
  //
  if (SystemTable == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemTable->ConOut == NULL) || (SystemTable->ConOut->OutputString == NULL) || (SystemTable->ConOut->ClearScreen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemTable->BootServices == NULL) || (SystemTable->BootServices->Stall == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // After this point, we should be able to print and stall but nothing else has been verified
  //
  if ((SystemTable->RuntimeServices == NULL) || (SystemTable->RuntimeServices->ResetSystem == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Start informing the user of what is happening
  //
  SystemTable->ConOut->ClearScreen (SystemTable->ConOut);
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nAttempting to update the system's secureboot certificates!\r\n");
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Learn more about this tool at https://aka.ms/securebootrecovery\r\n");

  //
  // Determine if the system is in a state we can safely use or if the system is already up to date
  //
  Status = IsCertificateInDB (SystemTable, mTargetCertificate, sizeof (mTargetCertificate), &IsFound);
  if ((Status == EFI_SUCCESS) && IsFound) {
    //
    // If the 2023 certificate is already in the DB, inform the user and reboot
    // This is likely someone running this tool multiple times so let's do nothing
    //
    SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nThe system's secureboot keys are already up to date!\r\n");
    goto Reboot;
  } else if (EFI_ERROR (Status) && IsFound) {
    //
    // Should be impossible to get here, but just in case, let's inform the user and reboot
    //
    PrintError (SystemTable, L"\r\nFailed to read the system's secure boot keys and something went wrong!\r\n", Status);
    goto Reboot;
  } else if (EFI_ERROR (Status) && !IsFound) {
    //
    // Likely this will continue to fail on reboot, the user will hopefully go to https://aka.ms/securebootrecovery to learn more
    //
    PrintError (SystemTable, L"\r\nFailed to read the system's secureboot keys!\r\n", Status);
    goto Reboot;
  }

  //
  // If we get here, the system is in a state we can safely use
  // The 2023 certificate is not in the DB, and we have not encountered any errors
  //

  //
  // Perform the append operation
  //
  Status = SystemTable->RuntimeServices->SetVariable (
                                           L"db",
                                           &gEfiImageSecurityDatabaseGuid,
                                           Attributes,
                                           sizeof (mDbUpdate),
                                           mDbUpdate
                                           );
  if (EFI_ERROR (Status)) {
    //
    // On failure, inform the user and reboot
    // This means that the payload being applied is not signed by a key in the KEK
    // Likely this will continue to fail on reboot, the user will hopefully go to https://aka.ms/securebootrecovery to learn more
    //
    PrintError (SystemTable, L"\r\nFailed to update the system to the 2023 Secure Boot Certificate!\r\n", Status);
    goto Reboot;
  }

  //
  // Otherwise the system took the update, so let's inform the user
  //
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nSuccessfully updated the system's secureboot keys!\r\n");

Reboot:
  //
  //  Stall for 10 seconds to give the user a chance to read the message
  //
  SystemTable->BootServices->Stall (STALL_10_SECONDS);

  //
  // Reset the system
  //
  SystemTable->RuntimeServices->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

Exit:
  //
  // If we get here, something really bad happened and we don't have a means to recover
  //

  //
  // let's atleast try to print an error to the console
  //
  PrintError (SystemTable, L"Exiting unexpectedly!\r\n", Status);

  //
  // Stall for 10 seconds to give the user a chance to read the error message
  //
  SystemTable->BootServices->Stall (STALL_10_SECONDS);

  return Status;
}
