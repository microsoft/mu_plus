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
  UINT8           Checksum;

  *IsFhr        = FALSE;
  *FhrResetData = NULL;

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
  FhrData       = ALIGN_POINTER ((VOID *)(ResetGuid + 1), sizeof (UINT64));
  RemainingSize = DataSize - (UINTN)((UINT8 *)FhrData - (UINT8 *)ResetData);

  if (RemainingSize < sizeof (FHR_RESET_DATA)) {
    DEBUG ((DEBUG_ERROR, "Data too small for reset data structure!\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  if (FhrData->Signature != FHR_RESET_DATA_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "Incorrect signature (0x%lx)!\n", FhrData->Signature));
    return EFI_INVALID_PARAMETER;
  }

  if (RemainingSize < FhrData->Length) {
    DEBUG ((DEBUG_ERROR, "Data too small for self described length!\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  Checksum          = FhrData->Checksum;
  FhrData->Checksum = 0;
  if (Checksum != CalculateCheckSum8 ((UINT8 *)FhrData, FhrData->Length)) {
    DEBUG ((DEBUG_ERROR, "Checksum mismatched!\n"));
    return EFI_INVALID_PARAMETER;
  }

  FhrData->Checksum = Checksum;

  DEBUG ((
    DEBUG_INFO,
    "FHR reset data:\n"
    "    Length:            0x%x\n"
    "    Revision:          0x%x\n"
    "    OsEntry:           0x%llx\n"
    "    OsDataBase:        0x%llx\n"
    "    OsDataSize:        0x%llx\n"
    "    CompatabilityId:   0x%llx\n",
    FhrData->Length,
    FhrData->Revision,
    FhrData->OsEntry,
    FhrData->OsDataBase,
    FhrData->OsDataSize,
    FhrData->CompatabilityId
    ));

  *FhrResetData = FhrData;
  return EFI_SUCCESS;
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
