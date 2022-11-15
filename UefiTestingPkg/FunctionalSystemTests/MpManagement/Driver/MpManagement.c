/** @file
  TODO: Populate this.

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>
#include <Protocol/MpManagement.h>

#include "MpManagementInternal.h"

#define APFUNC_BUFFER_LEN  256

EFI_MP_SERVICES_PROTOCOL            *mMpServices    = NULL;
EFI_HANDLE                          mHandle         = NULL;
UINTN                               mNumCpus        = 0;
UINTN                               mBspIndex       = 0;
volatile MP_MANAGEMENT_METADATA     *mCommonBuffer  = NULL;

/**
  Fetches the number of processors and which processor is the BSP.

  @param Mp  MP Services Protocol.
  @param NumProcessors Number of processors.
  @param BspIndex      The index of the BSP.
**/
STATIC
EFI_STATUS
GetProcessorInformation (
  IN  EFI_MP_SERVICES_PROTOCOL  *Mp,
  OUT UINTN                     *NumProcessors,
  OUT UINTN                     *BspIndex
  )
{
  EFI_STATUS  Status;
  UINTN       NumEnabledProcessors;

  Status = Mp->GetNumberOfProcessors (Mp, NumProcessors, &NumEnabledProcessors);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mp->WhoAmI (Mp, BspIndex);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Initialize the common buffer for all APs.

  @param NumCpus            The number of CPUs in the system.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
InitializeApCommonBuffer (
  IN    UINTN                    NumCpus,
  OUT   MP_MANAGEMENT_METADATA   **CommonBuffer
  )
{
  UINTN       Index;
  EFI_STATUS  Status;

  if (CommonBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *CommonBuffer = AllocateZeroPool (sizeof (MP_MANAGEMENT_METADATA) * NumCpus);
  if (*CommonBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EFI_SUCCESS;
  for (Index = 0; Index < NumCpus; Index ++) {
    (*CommonBuffer)[Index].ApStatus     = AP_STATE_OFF;
    (*CommonBuffer)[Index].TargetStatus = AP_STATE_OFF;
    (*CommonBuffer)[Index].ApTask       = AP_TASK_IDLE;
    (*CommonBuffer)[Index].ApBufferSize = EFI_PAGE_SIZE;
    (*CommonBuffer)[Index].ApBuffer     = AllocatePool ((*CommonBuffer)[Index].ApBufferSize);
    if ((*CommonBuffer)[Index].ApBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    // Free any allocated pools if init failed.
    for (Index = 0; Index < NumCpus; Index ++) {
      if ((*CommonBuffer)[Index].ApBuffer != NULL) {
        FreePool ((*CommonBuffer)[Index].ApBuffer);
      }
    }
  }

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
MpMgmtApOn (
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  )
{
  EFI_STATUS  Status;
  UINTN       StartIndex;
  UINTN       EndIndex;
  UINTN       Index;

  if ((ProcessorNumber == mBspIndex) || (ProcessorNumber > mNumCpus && ProcessorNumber != OPERATION_FOR_ALL_APS)) {
    DEBUG ((DEBUG_ERROR, "%a The specified processor is not acceptable %d\n", __FUNCTION__, ProcessorNumber));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (mCommonBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a The common buffer is not set up\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (mMpServices == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Mp service is not ready\n", __FUNCTION__));
    Status = EFI_NOT_READY;
    goto Done;
  }

  if (ProcessorNumber == OPERATION_FOR_ALL_APS) {
    StartIndex = 0;
    EndIndex   = mNumCpus - 1;
  } else {
    StartIndex = ProcessorNumber;
    EndIndex   = ProcessorNumber;
  }

  Status = EFI_NOT_FOUND;
  for (Index = StartIndex; Index <= EndIndex; Index ++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (mCommonBuffer[Index].ApStatus != AP_STATE_OFF) {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is already started\n", __FUNCTION__, Index));
      Status = EFI_ALREADY_STARTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus  = AP_STATE_ON;
    ZeroMem (mCommonBuffer[Index].ApBuffer, mCommonBuffer[Index].ApBufferSize);

    // This is the flag to release the core.
    mCommonBuffer[Index].ApTask  = AP_TASK_ACTIVE;

    Status = mMpServices->StartupThisAP (
                 mMpServices,
                 ApFunction,
                 Index,
                 NULL,
                 1,
                 NULL,
                 NULL
                 );
    // TODO: This is not quite right. The protocol will only support blocking mode after RTB...
    if (Status != EFI_SUCCESS && Status != EFI_TIMEOUT) {
      DEBUG ((DEBUG_ERROR, "%a Failed to start processor %d: %r\n", __FUNCTION__, Index, Status));
      break;
    } else {
      Status = EFI_SUCCESS;
    }
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // If successful, print the hellow world (blocking) from the APs.
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {}
    DEBUG ((DEBUG_INFO, "Initial message from common buffer %a\n", (CHAR8*)mCommonBuffer[Index].ApBuffer));
  }

Done:
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
MpMgmtApOff (
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  )
{
  EFI_STATUS  Status;
  UINTN       StartIndex;
  UINTN       EndIndex;
  UINTN       Index;

  if ((ProcessorNumber == mBspIndex) || (ProcessorNumber > mNumCpus && ProcessorNumber != OPERATION_FOR_ALL_APS)) {
    DEBUG ((DEBUG_ERROR, "%a The specified processor is not acceptable %d\n", __FUNCTION__, ProcessorNumber));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (mCommonBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a The common buffer is not set up\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (ProcessorNumber == OPERATION_FOR_ALL_APS) {
    StartIndex = 0;
    EndIndex   = mNumCpus - 1;
  } else {
    StartIndex = ProcessorNumber;
    EndIndex   = ProcessorNumber;
  }

  Status = EFI_SUCCESS;
  for (Index = StartIndex; Index <= EndIndex; Index ++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (mCommonBuffer[Index].ApStatus != AP_STATE_ON) {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is not in ON state (%d)\n", __FUNCTION__, Index, mCommonBuffer[Index].ApStatus));
      Status = EFI_ALREADY_STARTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus  = AP_STATE_OFF;
    mCommonBuffer[Index].ApTask        = AP_TASK_ACTIVE;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // If successful, print the hellow world (blocking) from the APs.
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {}
    DEBUG ((DEBUG_INFO, "Initial message from common buffer %a\n", (CHAR8*)mCommonBuffer[Index].ApBuffer));
  }

  // TODO: This is not ideal, but we could have messed up with the AP status here, wait for a bit to let the timer do the cleanup
  gBS->Stall (50000);

Done:
  return Status;
}

MP_MANAGEMENT_PROTOCOL mMpManagement = {
  .ApOn   = MpMgmtApOn,
  .ApOff  = MpMgmtApOff
};

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
MpManagementEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                Status;

  WriteBackDataCacheRange ((VOID *)&ApFunction, 32);

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&mMpServices
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate EFI_MP_SERVICES_PROTOCOL (%r). Not installed on platform?\n", Status));
    goto Done;
  }

  Status = GetProcessorInformation (mMpServices, &mNumCpus, &mBspIndex);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to fetch processor information.\n"));
    goto Done;
  }

  Status = InitializeApCommonBuffer (mNumCpus, (MP_MANAGEMENT_METADATA **)&mCommonBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to initialize Ap common buffer - %r.\n", Status));
    goto Done;
  }

  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gMpManagementProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMpManagement
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to fetch processor information.\n"));
    goto Done;
  }

Done:
  return Status;
}
