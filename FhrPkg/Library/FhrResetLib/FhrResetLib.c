/**
  A library for FHR helper functions.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>

#include <Fhr.h>
#include <Library/FhrLib.h>

VOID       *mFhrIndicatorPage     = NULL;
EFI_EVENT  mFhrAddressChangeEvent = NULL;

EFI_STATUS
EFIAPI
FhrCheckResetData (
  OUT BOOLEAN         *IsFhr,
  OUT FHR_RESET_DATA  **FhrResetData,
  IN UINTN            DataSize,
  IN VOID             *ResetData
  )

{
  UINTN           Index;
  EFI_GUID        *ResetGuid;
  EFI_GUID        FhrGuid = FHR_RESET_TYPE_GUID;
  UINT16          *FriendlyString;
  FHR_RESET_DATA  *FhrData;
  UINTN           RemainingSize;
  UINT8           Sum;
  UINT64          FhrStatus;
  EFI_STATUS      Status;

  *IsFhr        = FALSE;
  *FhrResetData = NULL;
  FhrData       = NULL;
  FhrStatus     = 0;
  Status        = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "Checking for FHR reset data.\n"));
  if ((DataSize < sizeof (UINT16)) || (ResetData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Parse through the friendly string.
  //

  FriendlyString = (UINT16 *)ResetData;
  DEBUG ((DEBUG_INFO, "String: '%ls'\n", FriendlyString));
  for (Index = 0; Index < (DataSize / sizeof (UINT16)); Index++) {
    if (FriendlyString[Index] == L'\0') {
      break;
    }
  }

  RemainingSize = DataSize - (Index * sizeof (UINT16));
  Index        += 1;

  if (sizeof (EFI_GUID) > RemainingSize) {
    DEBUG ((DEBUG_ERROR, "Data too small for reset guid!\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  ResetGuid = (EFI_GUID *)&FriendlyString[Index];
  DEBUG ((DEBUG_INFO, "Guid: %g\n", ResetGuid));
  if (!CompareGuid (ResetGuid, &FhrGuid)) {
    DEBUG ((DEBUG_ERROR, "Unknown GUID!\n"));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "FHR guid found. Looking for reset data.\n"));
  *IsFhr        = TRUE;
  RemainingSize = DataSize - (UINTN)((UINT8 *)FhrData - (UINT8 *)ResetData);

  if (RemainingSize < sizeof (FHR_RESET_DATA)) {
    DEBUG ((DEBUG_ERROR, "Data too small for reset data structure!\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  FhrData = ALIGN_POINTER ((VOID *)(ResetGuid + 1), sizeof (UINT64));
  if (FhrData->Signature != FHR_RESET_DATA_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "Incorrect signature (0x%lx)!\n", FhrData->Signature));
    FhrStatus = FHR_ERROR_RESET_BAD_SIGNATURE;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (RemainingSize < FhrData->Length) {
    DEBUG ((DEBUG_ERROR, "Data too small for self described length!\n"));
    FhrStatus = FHR_ERROR_RESET_BUFFER_TOO_SMALL;
    Status    = EFI_BUFFER_TOO_SMALL;
    goto Exit;
  }

  //
  // The checksum should ensure the sum of the structure is 0.
  //

  Sum = CalculateSum8 ((UINT8 *)FhrData, FhrData->Length);
  if (Sum != 0) {
    DEBUG ((DEBUG_ERROR, "Bad checksum! Sum should be 0, but is actually 0x%x\n", Sum));
    FhrStatus = FHR_ERROR_RESET_BAD_CHECKSUM;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (FhrData->Revision != FHR_RESET_DATA_REVISION) {
    DEBUG ((
      DEBUG_ERROR,
      "Unsupported revision! Supported: 0x%x Found: 0x%x\n",
      FHR_RESET_DATA_REVISION,
      FhrData->Revision
      ));

    FhrStatus = FHR_ERROR_RESET_UNSUPPORTED_REVISION;
    Status    = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  DEBUG ((
    DEBUG_INFO,
    "FHR reset data:\n"
    "    Length:            0x%x\n"
    "    Revision:          0x%x\n"
    "    ResumeCodeBase:    0x%llx\n"
    "    ResumeCodeSize:    0x%llx\n"
    "    OsDataBase:        0x%llx\n"
    "    OsDataSize:        0x%llx\n"
    "    CompatabilityId:   0x%llx\n",
    FhrData->Length,
    FhrData->Revision,
    FhrData->ResumeCodeBase,
    FhrData->ResumeCodeSize,
    FhrData->OsDataBase,
    FhrData->OsDataSize,
    FhrData->CompatabilityId
    ));

  *FhrResetData = FhrData;

Exit:
  if ((FhrStatus != 0) && (FhrData != NULL) && (FhrData->StatusCode != 0)) {
    *((UINT64 *)(VOID *)FhrData->StatusCode) = FhrStatus;
  }

  return Status;
}

EFI_STATUS
EFIAPI
FhrSetIndicator (
  IN  FHR_RESET_DATA  *FhrResetData
  )
{
  FHR_INDICATOR  *Indicator;

  ASSERT (mFhrIndicatorPage != NULL);

  DEBUG ((DEBUG_INFO, "Setting FHR indicator: 0x%llx\n", mFhrIndicatorPage));
  Indicator = (FHR_INDICATOR *)(mFhrIndicatorPage);

  ZeroMem (Indicator, sizeof (FHR_INDICATOR));
  Indicator->FhrHob.IsFhrBoot       = TRUE;
  Indicator->FhrHob.FhrReservedBase = PcdGet64 (PcdFhrReservedBlockBase);
  Indicator->FhrHob.FhrReservedSize = PcdGet64 (PcdFhrReservedBlockLength);
  Indicator->FhrHob.ResetData       = *FhrResetData;
  Indicator->Signature              = FHR_INDICATOR_SIGNATURE;

  return EFI_SUCCESS;
}

VOID
EFIAPI
FhrAddressChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gRT->ConvertPointer (0, (VOID **)&mFhrIndicatorPage);
  DEBUG ((DEBUG_INFO, "[FHR] New indicator page address: 0x%llx\n", mFhrIndicatorPage));
}

EFI_STATUS
EFIAPI
FhrResetInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Create callback to virtualize the
  //
  EFI_STATUS  Status;

  mFhrIndicatorPage = (VOID *)PcdGet64 (PcdFhrIndicatorPage);
  if (mFhrIndicatorPage != NULL) {
    DEBUG ((DEBUG_INFO, "[FHR] Indicator page: 0x%llx\n", mFhrIndicatorPage));
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    FhrAddressChange,
                    NULL,
                    &gEfiEventVirtualAddressChangeGuid,
                    &mFhrAddressChangeEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to create callback to fix FHR indicator page! %r\n",
        __FUNCTION__,
        Status
        ));

      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  return EFI_SUCCESS;
}
