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

//
// The certificate we are looking for
//
#define CERT_ORGANIZATION         "Microsoft Corporation"
#define CERT_ORGANIZATION_OFFSET  0xF6
#define CERT_COMMON_NAME          "Windows UEFI CA 2023"
#define CERT_COMMON_NAME_OFFSET   0x116
#define CERT_SIGNATURE            { 0x9f, 0xc9, 0xb6, 0xff, 0x6e, 0xe1, 0x9c, 0x3b, 0x55, 0xf6, 0xfe, 0x8b, 0x39, 0xdd, 0x61, 0x04 }
#define CERT_SIGNATURE_OFFSET     0x3AE

//
// The system table pointers
//
EFI_SYSTEM_TABLE      *mST = NULL;
EFI_RUNTIME_SERVICES  *mRT = NULL;
EFI_BOOT_SERVICES     *mBS = NULL;

typedef struct {
  UINT8    *Organization;      // The organization name in the certificate
  UINTN    OrganizationLength; // The length of the organization name in the certificate
  UINTN    OrganizationOffset; // The offset of the organization name in the certificate
  UINT8    *CommonName;        // The common name in the certificate
  UINTN    CommonNameLength;   // The length of the common name in the certificate
  UINTN    CommonNameOffset;   // The offset of the common name in the certificate
  UINT8    Signature[16];      // The signature of the certificate
  UINTN    SignatureOffset;    // The offset of the signature in the certificate
} CERTIFICATE_ENTRY;

//
// This is Certificate entry for the Windows UEFI CA 2023 certificate
// This will be used to check if the certificate is already in the DB
//
CERTIFICATE_ENTRY  mWindows2023ProductionCA = {
  .Organization       = CERT_ORGANIZATION,
  .OrganizationLength = sizeof (CERT_ORGANIZATION) - 1,
  .OrganizationOffset = CERT_ORGANIZATION_OFFSET,
  .CommonName         = CERT_COMMON_NAME,
  .CommonNameLength   = sizeof (CERT_COMMON_NAME) - 1,
  .CommonNameOffset   = CERT_COMMON_NAME_OFFSET,
  .Signature          = CERT_SIGNATURE,
  .SignatureOffset    = CERT_SIGNATURE_OFFSET
};

/**
 * Converts an EFI_STATUS to a hex string
 * @param[in] Status The EFI_STATUS to convert
 *
 * @return A pointer to a static buffer containing the hex string
 * @note The caller must not free the returned pointer
 * @note The returned pointer is only valid until the next call to this function
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
 * Prints an error message to the console
 * @param[in] Message     The message to print
 * @param[in] Status      The EFI_STATUS to print
*/
VOID
PrintError (
  CHAR16      *Message,
  EFI_STATUS  Status
  )
{
  mST->ConOut->OutputString (mST->ConOut, Message);
  mST->ConOut->OutputString (mST->ConOut, L"Error: 0x");
  mST->ConOut->OutputString (mST->ConOut, StatusToHexString (Status));
  mST->ConOut->OutputString (mST->ConOut, L"\r\n");
}

/**
  This function checks if a specified certificate is present in the EFI image security database (db).

  @param  Certificate      A pointer to the CERTIFICATE_ENTRY structure that contains the certificate to search for.
  @param  OptionalStatus   If not NULL, a pointer to a variable that receives the status of the operation.

  @retval TRUE             The certificate was found in the database.
  @retval FALSE            The certificate was not found in the database.
                           If OptionalStatus is not NULL, it contains an error code indicating the reason for the failure.

**/
BOOLEAN
IsCertificateInDB (
  IN  CERTIFICATE_ENTRY  *Certificate,
  OUT EFI_STATUS         *OptionalStatus
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
  BOOLEAN     Found;

  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *CertData;

  Found      = FALSE;
  Db         = NULL;
  DbSize     = 0;
  Attributes = VARIABLE_ATTRIBUTE_NV_BS_RT_AT | EFI_VARIABLE_APPEND_WRITE;

  if ((mST == NULL) || (Certificate == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  //
  // Read the size and attributes of the DB
  //
  Status = mRT->GetVariable (
                  L"db",
                  &gEfiImageSecurityDatabaseGuid,
                  &Attributes,
                  &DbSize,
                  NULL
                  );

  //
  // Confirm we got the expected error that the buffer was too small
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Likely this will continue to fail on reboot
    //
    return FALSE;
  }

  //
  // Check that the DB has the expected attributes
  //
  if ((Attributes & VARIABLE_ATTRIBUTE_NV_BS_RT_AT) != VARIABLE_ATTRIBUTE_NV_BS_RT_AT) {
    Status = EFI_UNSUPPORTED;
    //
    // Likely this will continue to fail on reboot
    //
    return FALSE;
  }

  Status = mBS->AllocatePool (
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
  Status = mRT->GetVariable (
                  L"db",
                  &gEfiImageSecurityDatabaseGuid,
                  &Attributes,
                  &DbSize,
                  Db
                  );
  if (EFI_ERROR (Status)) {
    //
    // Likely this will continue to fail on reboot
    // GetVariable could return NOT_FOUND if the DB variable does not exist
    // In that case, we should fail out gracefully because there is nothing we can do
    //
    goto Exit;
  }

  // Confirm the DB is at least as large as the EFI_SIGNATURE_LIST header
  if (DbSize < sizeof (EFI_SIGNATURE_LIST)) {
    Status = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  //
  // The DB is a list of EFI_SIGNATURE_LISTs but we only care about certificates
  // they are not garunteed to be in any particular order
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
        // First lets ensure that the size of the certificate is at least as large as the certificate we are looking for
        // Each offset in the certficate entry plus the size of the string must be less than the size of the certificate
        //
        if (((Certificate->OrganizationOffset + sizeof (Certificate->Organization)) <= CertSize) &&
            ((Certificate->CommonNameOffset + sizeof (Certificate->CommonName)) <= CertSize) &&
            ((Certificate->SignatureOffset + sizeof (Certificate->Signature)) <= CertSize))
        {
          //
          // If the certificate is the one we are looking for, return EFI_SUCCESS
          //
          if ((CompareMem (Cert + Certificate->OrganizationOffset, Certificate->Organization, Certificate->OrganizationLength) == 0) &&
              (CompareMem (Cert + Certificate->CommonNameOffset, Certificate->CommonName, Certificate->CommonNameLength) == 0) &&
              (CompareMem (Cert + Certificate->SignatureOffset, Certificate->Signature, sizeof (Certificate->Signature)) == 0))
          {
            Status = EFI_SUCCESS;
            Found  = TRUE;
            goto Exit;
          }
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

  if (OptionalStatus != NULL) {
    *OptionalStatus = Status;
  }

  return Found;
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

  // Save off the system table pointers
  mST = SystemTable;
  mBS = SystemTable->BootServices;
  mRT = SystemTable->RuntimeServices;

  //
  // Start informing the user of what is happening
  //
  mST->ConOut->ClearScreen (mST->ConOut);
  mST->ConOut->OutputString (mST->ConOut, L"\r\nAttempting to update the system's secureboot certificates!\r\n");
  mST->ConOut->OutputString (mST->ConOut, L"Learn more about this tool at https://aka.ms/securebootrecovery\r\n");

  //
  // Determine if the system is in a state we can safely use or if the system is already up to date
  //
  IsFound = IsCertificateInDB (&mWindows2023ProductionCA, &Status);
  if (IsFound && (Status == EFI_SUCCESS)) {
    //
    // If the 2023 certificate is already in the DB, inform the user and reboot
    // This is likely someone running this tool multiple times so let's do nothing
    //
    mST->ConOut->OutputString (mST->ConOut, L"\r\nThe system's secureboot keys are already up to date!\r\n");
    goto Reboot;
  } else if (IsFound && (EFI_ERROR (Status))) {
    //
    // Should be impossible to get here, but just in case, let's inform the user and reboot
    //
    PrintError (L"\r\nFailed to read the system's secure boot keys and something went wrong!\r\n", Status);
    goto Reboot;
  } else if (!IsFound) {
    if (EFI_ERROR (Status)) {
      //
      // Likely this will continue to fail on reboot, the user will hopefully go to https://aka.ms/securebootrecovery to learn more
      //
      PrintError (L"\r\nFailed to read the system's secureboot keys!\r\n", Status);
    } else {
      //
      // The DB is empty, so let's assume the system has secureboot disabled, inform the user and reboot
      //
      PrintError (L"\r\nFailed to read the system's secureboot keys! Is Secureboot enabled?\r\n", Status);
    }

    goto Reboot;
  }

  //
  // If we get here, the system is in a state we can safely use
  // The 2023 certificate is not in the DB, and we have not encountered any errors
  //

  //
  // Perform the append operation
  //
  Status = mRT->SetVariable (
                  L"db",
                  &gEfiImageSecurityDatabaseGuid,
                  Attributes,
                  sizeof (mDbUpdate),
                  mDbUpdate
                  );
  if (EFI_ERROR (Status)) {
    //
    // On failure, inform the user and reboot
    // Likely this will continue to fail on reboot, the user will hopefully go to https://aka.ms/securebootrecovery to learn more
    //
    PrintError (L"\r\nFailed to update the system to the 2023 Secure Boot Certificate!\r\n", Status);
    goto Reboot;
  }

  //
  // Otherwise the system took the update, so let's inform the user
  //
  mST->ConOut->OutputString (mST->ConOut, L"\r\nSuccessfully updated the system's secureboot keys!\r\n");

Reboot:

  //
  //  Stall for 10 seconds to give the user a chance to read the message
  //
  mBS->Stall (STALL_10_SECONDS);

  //
  // Reset the system
  //
  mRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

Exit:
  //
  // If we get here, something really bad happened and we don't have a means to recover
  //

  //
  // let's atleast try to print an error to the console
  //
  PrintError (L"Exiting unexpectedly!\r\n", Status);

  //
  // Stall for 10 seconds to give the user a chance to read the error message
  //
  mBS->Stall (STALL_10_SECONDS);

  return Status;
}
