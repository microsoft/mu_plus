/** @file
  Sets variable policy for Boot Manager load options that are not required to be supported per the UEFI Specification.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/GlobalVariable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/VariablePolicyHelperLib.h>

#define PLATFORM_RECOVERY_VARIABLE_NAME  L"PlatformRecovery"

typedef struct {
  CHAR16    *VariableName;
  UINT32    VariableAttributes;
} BM_LOAD_OPTION_VAR_INFO;

//
// Boot Manager load options that have variable policy applied by this driver.
//
GLOBAL_REMOVE_IF_UNREFERENCED BM_LOAD_OPTION_VAR_INFO  mBmLoadOptionInfo[] = {
  // Fixed-Name Order Variables
  {
    EFI_DRIVER_ORDER_VARIABLE_NAME,
    (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)
  },
  {
    EFI_SYS_PREP_ORDER_VARIABLE_NAME,
    (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)
  },

  // Wildcard Option Variables
  {
    L"Driver####",
    (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)
  },
  {
    L"PlatformRecovery####",
    (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)
  },
  {
    L"SysPrep####",
    (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)
  }
};

/**
  Gets the next UEFI variable name.

  This buffer manages the UEFI variable name buffer, performing memory reallocations as necessary.

  Note: The first time this function is called, VariableNameBufferSize must be 0 and
  the VariableName buffer pointer must point to NULL.

  @param[in,out] VariableNameBufferSize   On input, a pointer to a buffer that holds the current
                                          size of the VariableName buffer in bytes.
                                          On output, a pointer to a buffer that holds the updated
                                          size of the VariableName buffer in bytes.
  @param[in,out] VariableName             On input, a pointer to a pointer to a buffer that holds the
                                          current UEFI variable name.
                                          On output, a pointer to a pointer to a buffer that holds the
                                          next UEFI variable name.
  @param[in,out] VariableGuid             On input, a pointer to a buffer that holds the current UEFI
                                          variable GUID.
                                          On output, a pointer to a buffer that holds the next UEFI
                                          variable GUID.

  @retval    EFI_SUCCESS              The next UEFI variable name was found successfully.
  @retval    EFI_INVALID_PARAMETER    A pointer argument is NULL or initial input values are invalid.
  @retval    EFI_OUT_OF_RESOURCES     Insufficient memory resources to allocate a required buffer.
  @retval    Others                   Return status codes from the UEFI spec define GetNextVariableName() interface.

**/
EFI_STATUS
GetNextVariableNameWithDynamicReallocation (
  IN  OUT UINTN     *VariableNameBufferSize,
  IN  OUT CHAR16    **VariableName,
  IN  OUT EFI_GUID  *VariableGuid
  )
{
  EFI_STATUS  Status;
  UINTN       NextVariableNameBufferSize;

  if ((VariableNameBufferSize == NULL) || (VariableName == NULL) || (VariableGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*VariableNameBufferSize == 0) {
    if (*VariableName != NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Allocate a buffer to temporarily hold variable names. To reduce memory
    // allocations, the default buffer size is 256 characters. The buffer can
    // be reallocated if expansion is necessary (should be very rare).
    //
    *VariableNameBufferSize = sizeof (CHAR16) * 256;
    *VariableName           = AllocateZeroPool (*VariableNameBufferSize);
    if (*VariableName == NULL) {
      *VariableNameBufferSize = 0;
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem ((VOID *)VariableGuid, sizeof (EFI_GUID));
  }

  NextVariableNameBufferSize = *VariableNameBufferSize;
  Status                     = gRT->GetNextVariableName (
                                      &NextVariableNameBufferSize,
                                      *VariableName,
                                      VariableGuid
                                      );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *VariableName = ReallocatePool (
                      *VariableNameBufferSize,
                      NextVariableNameBufferSize,
                      *VariableName
                      );
    if (*VariableName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *VariableNameBufferSize = NextVariableNameBufferSize;

    Status = gRT->GetNextVariableName (
                    &NextVariableNameBufferSize,
                    *VariableName,
                    VariableGuid
                    );
    ASSERT (Status != EFI_BUFFER_TOO_SMALL);
  }

  return Status;
}

/**
  Check if a Unicode character is a hexadecimal character.

  Valid hexadecimal characters are L'0' to L'9', L'a' to L'f', or L'A' to L'F'.

  @param[in]  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecimal character.
  @retval FALSE If the Char is not a hexadecimal character.

**/
BOOLEAN
EFIAPI
IsHexaDecimalDigitCharacter (
  IN      CHAR16  Char
  )
{
  return (BOOLEAN)((Char >= L'0' && Char <= L'9') || (Char >= L'A' && Char <= L'F') || (Char >= L'a' && Char <= L'f'));
}

/**
  Removes pre-existing variables that may have been written during this boot before the load option
  policy enforced by this driver is active.

**/
VOID
RemovePreExistingVariables (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  GetNextVariableStatus;
  UINT32      Attributes;
  UINTN       DataSize;
  UINTN       Index;
  UINTN       VariableNameBufferSize;
  UINTN       VariableNameLength;
  UINTN       VariablePendingDeleteNameBufferSize;
  EFI_GUID    VariableGuid;
  BOOLEAN     VariableDeleted;
  CHAR16      *VariableName;
  CHAR16      *VariablePendingDeleteName;

  Status                              = EFI_SUCCESS;
  VariableName                        = NULL;
  VariablePendingDeleteName           = NULL;
  VariableDeleted                     = FALSE;
  VariableNameBufferSize              = 0;
  VariablePendingDeleteNameBufferSize = 0;

  do {
    if (!VariableDeleted) {
      GetNextVariableStatus = GetNextVariableNameWithDynamicReallocation (
                                &VariableNameBufferSize,
                                &VariableName,
                                &VariableGuid
                                );
    } else {
      VariableDeleted = FALSE;
    }

    if (!EFI_ERROR (GetNextVariableStatus) && CompareGuid (&VariableGuid, &gEfiGlobalVariableGuid)) {
      VariableNameLength = StrLen (VariableName);

      //
      // Check that the variable root name is a candidate for a platform recovery load option
      //
      if ((VariableNameLength <= 4) || (VariableNameLength - 4 != StrLen (PLATFORM_RECOVERY_VARIABLE_NAME))) {
        continue;
      }

      //
      // Check that the variable root name is a platform recovery load option
      //
      if (StrnCmp (VariableName, PLATFORM_RECOVERY_VARIABLE_NAME, VariableNameLength - 4) != 0) {
        continue;
      }

      //
      // Check that the last 4 digits are hex digits
      //
      for (Index = VariableNameLength - 4; Index < VariableNameLength; Index++) {
        if (!IsHexaDecimalDigitCharacter (VariableName[Index])) {
          // Only variables with hex digits are considered valid per UEFI Specification
          break;
        }
      }

      if (Index == VariableNameLength) {
        DEBUG ((
          DEBUG_WARN,
          "[%a] - The UEFI variable %s was written this boot. It is being deleted per load option policy.\n",
          __FUNCTION__,
          VariableName
          ));

        Attributes = 0;
        DataSize   = 0;
        Status     = gRT->GetVariable (
                            VariableName,
                            &gEfiGlobalVariableGuid,
                            &Attributes,
                            &DataSize,
                            NULL
                            );
        if (Status != EFI_BUFFER_TOO_SMALL) {
          // A zero size buffer should be too small for a present (enumerable) variable
          ASSERT (Status == EFI_BUFFER_TOO_SMALL);
          continue;
        }

        ASSERT (DataSize > 0);
        if (Attributes != (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) {
          // PlatformRecovery#### must have (RT|BS) per UEFI Spec
          // Assert to bring attention if this is not the case but proceed to delete regardless
          ASSERT_EFI_ERROR (EFI_SECURITY_VIOLATION);
        }

        // Prepare a backup name buffer to keep tracking GetNextVariableName() during deletion
        ASSERT ((VariablePendingDeleteNameBufferSize > 0) || (VariablePendingDeleteName == NULL));
        if (VariablePendingDeleteNameBufferSize < VariableNameBufferSize) {
          if (VariablePendingDeleteName != NULL) {
            FreePool (VariablePendingDeleteName);
          }

          VariablePendingDeleteName = AllocateZeroPool (VariableNameBufferSize);
          if (VariablePendingDeleteName == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            ASSERT_EFI_ERROR (Status);
            goto DeallocateAndExit;
          }

          VariablePendingDeleteNameBufferSize = VariableNameBufferSize;
        }

        StrnCpyS (
          VariablePendingDeleteName,
          VariableNameBufferSize / sizeof (CHAR16),
          VariableName,
          (VariableNameBufferSize / sizeof (CHAR16)) - 1
          );

        //
        // Skip to the next variable name since the current name will
        // be invalid after deletion.
        //
        GetNextVariableStatus = GetNextVariableNameWithDynamicReallocation (
                                  &VariableNameBufferSize,
                                  &VariableName,
                                  &VariableGuid
                                  );

        Status = gRT->SetVariable (
                        VariablePendingDeleteName,
                        &gEfiGlobalVariableGuid,
                        0,
                        0,
                        NULL
                        );
        ASSERT_EFI_ERROR (Status);
        VariableDeleted = TRUE;
      }
    }
  } while (!EFI_ERROR (GetNextVariableStatus));

DeallocateAndExit:
  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  if (VariablePendingDeleteName != NULL) {
    FreePool (VariablePendingDeleteName);
  }
}

/**
  Applies variable policies for the variables protected by this driver.

**/
VOID
SetVariablePolicy (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           Index;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to find the variable policy protocol.\n", __FUNCTION__));
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return;
  }

  for (Index = 0; Index < ARRAY_SIZE (mBmLoadOptionInfo); Index++) {
    Status = RegisterBasicVariablePolicy (
               VariablePolicy,
               &gEfiGlobalVariableGuid,
               mBmLoadOptionInfo[Index].VariableName,
               VARIABLE_POLICY_NO_MIN_SIZE,
               VARIABLE_POLICY_NO_MAX_SIZE,
               mBmLoadOptionInfo[Index].VariableAttributes,
               ~(mBmLoadOptionInfo[Index].VariableAttributes),
               VARIABLE_POLICY_TYPE_LOCK_NOW
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - RegisterBasicVariablePolicy() returned %r!\n", __FUNCTION__, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**
  Driver entry point that performs the main responsibilities of the driver.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   This constructor always returns success.

**/
EFI_STATUS
EFIAPI
LoadOptionVariablePolicyDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  DEBUG ((DEBUG_VERBOSE, "[%a] - Entry", __FUNCTION__));

  RemovePreExistingVariables ();
  SetVariablePolicy ();

  return EFI_SUCCESS;
}
