/** @file
  TPM Replay PEI - Main Module File

  Defines the entry point and main execution flow.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include "TpmReplayPei.h"
#include "TpmReplayPeiPlatformFvExclusion.h"

#include <Library/DebugLib.h>

/**
  The PEIM entry point.

  @param[in]  FileHandle          Handle of the file being invoked.
  @param[in]  PeiServices         Pointer to PEI Services table.

  @retval EFI_SUCCESS             This entry point always returns EFI_SUCCESS.
  @retval EFI_INVALID_PARAMETER   A pointer argument passed is NULL.

**/
EFI_STATUS
EFIAPI
TpmReplayPeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  // This should never happen but check since it's actually being used.
  if ((FileHandle == NULL) || (PeiServices == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Ensure Shadow Registration
  Status = (**PeiServices).RegisterForShadow (FileHandle);

  switch (Status) {
    case EFI_SUCCESS:
      // Pre-Memory Only Actions

      // Do not allow the platform to make real FV blob measurements
      Status = InstallPlatformFvExclusions ();
      ASSERT_EFI_ERROR (Status);

      // Perform Pre-Memory TCG Related Actions
      Status = TpmReplayPreMemActions ();
      ASSERT_EFI_ERROR (Status);

      break;

    case EFI_ALREADY_STARTED:
      // Post-Memory Only Actions

      // Nothing right now

      break;

    default:
      DEBUG ((DEBUG_ERROR, "[%a] - Unexpected status error code in shadow registration.\n", __FUNCTION__));
      DEBUG ((
        DEBUG_ERROR,
        "[%a] - PEIM [%a]-{%g}. Aborting.",
        __FUNCTION__,
        gEfiCallerBaseName,
        &gEfiCallerIdGuid
        ));

      ASSERT_EFI_ERROR (Status);

      return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}
