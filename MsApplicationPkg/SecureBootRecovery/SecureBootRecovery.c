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

#include "RecoveryPayload.h"

// 10 seconds in microseconds
#define STALL_10_SECONDS  10000000

#define STATUS_SIZE         (sizeof(EFI_STATUS) * 2)
#define STATUS_STRING_SIZE  (STATUS_SIZE + sizeof(L"\0"))

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
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nAttempting to update the system's secureboot certificates\r\n");
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Learn more about this tool at https://aka.ms/securebootrecovery\r\n");

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
    // Likely this will continue to fail on reboot, the user will hopefully go to https://aka.ms/securebootrecovery to learn more
    //
    SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nFailed to update the system's secureboot keys\r\n");
    SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Error: 0x");
    SystemTable->ConOut->OutputString (SystemTable->ConOut, StatusToHexString (Status));
    SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\n");

    goto Reboot;
  }

  //
  // Otherwise the system took the update, so let's inform the user
  //
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\nSuccessfully updated the system's secureboot keys\r\n");

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
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Exiting unexpectedly!\r\n");
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"Error: 0x");
  SystemTable->ConOut->OutputString (SystemTable->ConOut, StatusToHexString (Status));
  SystemTable->ConOut->OutputString (SystemTable->ConOut, L"\r\n");

  //
  // Stall for 10 seconds to give the user a chance to read the error message
  //
  SystemTable->BootServices->Stall (STALL_10_SECONDS);

  return Status;
}
