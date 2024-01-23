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
// The system table pointers
//
EFI_SYSTEM_TABLE      *mST = NULL;
EFI_RUNTIME_SERVICES  *mRT = NULL;
EFI_BOOT_SERVICES     *mBS = NULL;

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
  mST->ConOut->OutputString (mST->ConOut, L"\r\nAttempting to update the system's secureboot certificates\r\n");
  mST->ConOut->OutputString (mST->ConOut, L"Learn more about this tool at https://aka.ms/securebootrecovery\r\n");

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
    PrintError (L"\r\nFailed to update the system to the 2023 Secure Boot Certificate\r\n", Status);
    goto Reboot;
  }

  //
  // Otherwise the system took the update, so let's inform the user
  //
  mST->ConOut->OutputString (mST->ConOut, L"\r\nSuccessfully updated the system's secureboot keys\r\n");

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
