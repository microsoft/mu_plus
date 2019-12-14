/** @file -- HwErrorBert.c

At EBS, driver will generate BERT table with all HwErrRec from flash storage.
At ReadyToBoot, driver will delete HwErrRec from flash.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Guid/Cper.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/CheckHwErrRecHeaderLib.h>
#include <Library/PrintLib.h>
#include <IndustryStandard/Acpi.h>
#include <Uefi.h>
#include "BertHelper.h"

STATIC EFI_EVENT                      mExitBootServicesEvent  = NULL;
STATIC EFI_EVENT                      mReadyToBootEvent  = NULL;
UINT16                                mVarNameListCount = 0;
CHAR16                               *mVarNameList = NULL;

/**

Create and publish Boot Error Runtime Table.
Gather variables in mVarNameList and add them
to the BootErrorRegion of the BERT table.

**/
VOID
EFIAPI
SetupBert() {
  UINTN             Size = 0;
  CHAR16           *NamePtr = NULL;
  VOID             *Buffer = NULL;
  EFI_STATUS        Status;
  BERT_CONTEXT      Context;


  if (!mVarNameList || mVarNameListCount == 0) {
    DEBUG((DEBUG_WARN, "%a: leaving because list of entries to catalogue was empty.\n", __FUNCTION__));
    return;
  }

  DEBUG((DEBUG_VERBOSE, "%a - %x CPER entries to publish to BERT\n", __FUNCTION__, mVarNameListCount));

  // Create & publish BERT Header
  BertHeaderCreator(&Context, BOOT_ERROR_REGION_SIZE);
  BertErrorBlockInitial(Context.Block, EFI_ACPI_6_2_ERROR_SEVERITY_CORRECTED);
  Status = BertSetAcpiTable(&Context);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Publishing BERT ACPI table failed\n"));
    return;
  }

  // Iterate through the list of variable names
  for (UINTN Index = 0; Index < mVarNameListCount; Index ++) {
    Size = 0;
    Buffer = NULL;
    NamePtr = &mVarNameList[Index * EFI_HW_ERR_REC_VAR_NAME_LEN];
    DEBUG((DEBUG_VERBOSE, "%a - Publishing %s\n", __FUNCTION__, NamePtr));

    //
    // Call, get variable size; call again, get variable
    //
    Status = gRT->GetVariable(NamePtr,
                              &gEfiHardwareErrorVariableGuid,
                              NULL,
                              &Size,
                              NULL);

    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG((DEBUG_ERROR, "%a - %s 0 size GetVariable returned %r\n", __FUNCTION__, NamePtr, Status));
      ASSERT(FALSE);
      continue;
    }

    Buffer = AllocateZeroPool(Size);
    if (Buffer == NULL) {
      DEBUG((DEBUG_VERBOSE, "%a - out of memory", __FUNCTION__));
      FreePool(Buffer);
      return;
    }

    Status = gRT->GetVariable(NamePtr,
                             &gEfiHardwareErrorVariableGuid,
                              NULL,
                             &Size,
                              Buffer);

    if (!EFI_ERROR (Status) && ValidateCperHeader((EFI_COMMON_ERROR_RECORD_HEADER *)Buffer, Size)) {
      // We got a CPER, time to add it to BERT!
      if (!BertAddAllCperSections(Context.BertHeader, Buffer)) {
        DEBUG((DEBUG_ERROR, "Ran out of space in BERT boot error region\n"));
        FreePool(Buffer);
        return;
      }
    } else {
      DEBUG((DEBUG_ERROR, "%a: Variable failed or CPER was deemed unsafe - %r\n", __FUNCTION__, Status));
      FreePool(Buffer);
      return;
    }
    if (Buffer) {
      FreePool(Buffer);
    }
  }
}


/**

Go through variables on flash and find
variables with GUID gEfiHardwareErrorVariableGuid.

**/
VOID
EFIAPI
GenerateVariableList() {
  CHAR16               *Name = NULL;
  UINTN                 NameSize;
  UINTN                 NewNameSize;
  EFI_GUID              Guid;
  EFI_STATUS            Status = EFI_SUCCESS;

  DEBUG((DEBUG_VERBOSE, "%a enter\n", __FUNCTION__));

  NameSize = sizeof(CHAR16);
  Name = AllocateZeroPool(NameSize);

  // Go through all the variables on flash
  while (TRUE) {
    // Get ready to receive the next name
    NewNameSize = NameSize;
    Status = gRT->GetNextVariableName(&NewNameSize, Name, &Guid);

    // Make room for the next name if necessary
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool(NameSize, NewNameSize, Name);
      if (Name == NULL) {
        DEBUG((DEBUG_ERROR, "%a - ReallocatePool failed, out of memory\n", __FUNCTION__));
        Status = EFI_OUT_OF_RESOURCES;
        goto cleanup;
      }
      Status = gRT->GetNextVariableName(&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      // All done!
      break;
    }

    ASSERT_EFI_ERROR(Status);

    // If the GUID doesn't match, we don't care about it!
    if (EFI_ERROR(Status)  || !CompareGuid(&Guid, &gEfiHardwareErrorVariableGuid)) {
      continue;
    }

    // Add this variable to the array
    DEBUG((DEBUG_ERROR, "%a - found %s\n", __FUNCTION__, Name));
    mVarNameList = ReallocatePool( mVarNameListCount * EFI_HW_ERR_REC_VAR_NAME_LEN * sizeof(CHAR16),
                                  (mVarNameListCount + 1) * EFI_HW_ERR_REC_VAR_NAME_LEN * sizeof(CHAR16),
                                   mVarNameList);
    if (mVarNameList == NULL) {
      DEBUG((DEBUG_ERROR, "%a - %d\n", __FUNCTION__, __LINE__));
      Status = EFI_OUT_OF_RESOURCES;
      goto cleanup;
    }
    StrCpyS(&mVarNameList[mVarNameListCount * EFI_HW_ERR_REC_VAR_NAME_LEN], StrLen(Name) + 1, Name);
    mVarNameListCount++;
  }

  // We succeeded! Let's not say otherwise.
  Status = EFI_SUCCESS;

cleanup:
  if (Name) {
    FreePool(Name);
  }
  if (EFI_ERROR(Status)) {
    // Shouldn't be happening... but we can cleanup anyways
    ASSERT(FALSE);
    mVarNameListCount = 0;
    mVarNameList = NULL;
  }
  DEBUG((DEBUG_INFO, "%a found %x variables for the BERT table - %r\n", __FUNCTION__, mVarNameListCount, Status));
}

/**

Go through the variables on our list and delete them from flash.

**/
VOID
EFIAPI
ClearVariables() {
  UINT32                Index = 0;
  EFI_STATUS            Status = EFI_SUCCESS;
  CHAR16               *NamePtr = NULL;

  DEBUG((DEBUG_VERBOSE, "%a enter: number of elements to clear = %x\n", __FUNCTION__, mVarNameListCount));

  if (mVarNameList == NULL) {
    DEBUG((DEBUG_WARN, "%a mVarNameList is null!\n", __FUNCTION__));
    return;
  }

  for (Index = 0; Index < mVarNameListCount; Index ++) {
    NamePtr = &mVarNameList[Index * EFI_HW_ERR_REC_VAR_NAME_LEN];
    Status = gRT->SetVariable(NamePtr, &gEfiHardwareErrorVariableGuid, 0, 0, NULL);

    // Can't really do much if this fails
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Clearing variable %s failed with %r \n", NamePtr, Status));
      ASSERT_EFI_ERROR(Status);
    }

    DEBUG((DEBUG_VERBOSE, "%a - Removed %s\n", __FUNCTION__, NamePtr));
  }
}


/**
  Exit Boot Services Callback Handler

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event contxt.

  @retval None.

**/
VOID
EFIAPI
ExitBootServicesHandlerCallback(
  IN  EFI_EVENT    Event,
  IN  VOID    *Context
) {
  EFI_STATUS Status;
  if(mExitBootServicesEvent != NULL) {
    Status = gBS->CloseEvent(mExitBootServicesEvent);
    ASSERT_EFI_ERROR (Status);
  }

  // Delete all variables in the array because they have been published
  ClearVariables();

  if (mVarNameList) {
    FreePool(mVarNameList);
  }
  return;
}


/**

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event contxt.

  @retval None.

**/
VOID
EFIAPI
ReadyToBootHandlerCallback(
  IN  EFI_EVENT    Event,
  IN  VOID    *Context
) {
  EFI_STATUS Status;

  if(mReadyToBootEvent != NULL) {
    Status = gBS->CloseEvent(mReadyToBootEvent);
    ASSERT_EFI_ERROR (Status);
  }
  // Go through the variables in flash and note that have the right GUID
  GenerateVariableList();
  // Go through the variables in our list and publish them to the BERT table
  SetupBert();
  return;
}


/**
Entry to

@param[in] ImageHandle                The image handle.
@param[in] SystemTable                The system table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
HwErrorBertEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_VERBOSE, "%a\n", __FUNCTION__));

  // Register for the ready to boot event to publish BERT table.
  Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                TPL_CALLBACK,
                ReadyToBootHandlerCallback,
                NULL,
                &gEfiEventReadyToBootGuid,
                &mReadyToBootEvent);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a - Error creating mReadyToBootEvent\n", __FUNCTION__));
    goto cleanup;
  }

  // Register for the exit boot services event to clear variables!
  // This is separate from publishing BERT table in case we never boot to OS.
  // In that case, we would be able to try to publish the variables again.
  Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                              TPL_CALLBACK,
                              ExitBootServicesHandlerCallback,
                              NULL,
                              &gEfiEventExitBootServicesGuid,
                              &mExitBootServicesEvent);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a - Error creating mExitBootServicesEvent\n", __FUNCTION__));
    goto cleanup;
  }

cleanup:
  // If anything goes wrong, we will need to close the events.
  ASSERT_EFI_ERROR(Status);
  if (EFI_ERROR(Status)) {
    if(mReadyToBootEvent != NULL) {
      Status = gBS->CloseEvent(mReadyToBootEvent);
    }

    if(mExitBootServicesEvent != NULL) {
      Status = gBS->CloseEvent(mExitBootServicesEvent);
    }
  }

  return Status;
}