/** @file
  MP management driver that supports the power management of AP on/off and
  suspend/resume for all cores.

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
#include <Protocol/LoadedImage.h>
#include <Protocol/MpService.h>
#include <Protocol/MpManagement.h>

#include "MpManagementInternal.h"

EFI_MP_SERVICES_PROTOCOL         *mMpServices   = NULL;
EFI_HANDLE                       mHandle        = NULL;
UINTN                            mNumCpus       = 0;
UINTN                            mBspIndex      = 0;
volatile MP_MANAGEMENT_METADATA  *mCommonBuffer = NULL;

/**
  Fetches the number of processors and which processor is the BSP.

  @param Mp             MP Services Protocol.
  @param NumProcessors  Number of processors.
  @param BspIndex       The index of the BSP.

  @return EFI_SUCCESS   The routine completed successfully.
  @return Others        The routine failed due to some MP service calls.
**/
STATIC
EFI_STATUS
GetMpInformation (
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

/**
  Helper function to query if a processor is enabled.

  @param Mp             MP Services Protocol.
  @param CpuIndex       Index of processor that is of interest.

  @return TRUE   The processor is enabled.
  @return FALSE  The processor is disabled.
**/
STATIC
BOOLEAN
IsProcessorEnabled (
  IN  EFI_MP_SERVICES_PROTOCOL  *Mp,
  UINTN                         CpuIndex
  )
{
  EFI_PROCESSOR_INFORMATION  CpuInfo;
  EFI_STATUS                 Status;

  if (Mp == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Input protocol is NULL\n", __FUNCTION__));
    return FALSE;
  }

  Status = Mp->GetProcessorInfo (Mp, CPU_V2_EXTENDED_TOPOLOGY | CpuIndex, &CpuInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Cannot get information for specified processor (%d) - %r\n", __FUNCTION__, CpuIndex, Status));
    return FALSE;
  }

  return ((CpuInfo.StatusFlag & PROCESSOR_ENABLED_BIT) != 0);
}

/** Initialize the common buffer for all APs.

  @param NumCpus            The number of CPUs in the system.

  @return EFI_SUCCESS on success, or an error code.

**/
EFI_STATUS
InitializeApCommonBuffer (
  IN    UINTN                   NumCpus,
  OUT   MP_MANAGEMENT_METADATA  **CommonBuffer
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
  for (Index = 0; Index < NumCpus; Index++) {
    (*CommonBuffer)[Index].ApStatus     = AP_STATE_OFF;
    (*CommonBuffer)[Index].TargetStatus = AP_STATE_OFF;
    (*CommonBuffer)[Index].ApTask       = AP_TASK_IDLE;
    (*CommonBuffer)[Index].ApBufferSize = EFI_PAGE_SIZE;
    (*CommonBuffer)[Index].ApBuffer     = AllocatePool ((*CommonBuffer)[Index].ApBufferSize);
    if ((*CommonBuffer)[Index].ApBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
  }

  WriteBackDataCacheRange ((VOID *)(*CommonBuffer), sizeof (MP_MANAGEMENT_METADATA) * NumCpus);

Done:
  if (EFI_ERROR (Status)) {
    // Free any allocated pools if init failed.
    for (Index = 0; Index < NumCpus; Index++) {
      if ((*CommonBuffer)[Index].ApBuffer != NULL) {
        FreePool ((*CommonBuffer)[Index].ApBuffer);
      }
    }
  }

  return Status;
}

/**
  A BSP invoked function to perform self suspend. A timeout period needs
  to be provided by the called to invoke self-wakeup service.

  @param This                   MP Management Protocol.
  @param BspPowerState          The target power state the BSP should be
                                suspended to.
  @param TargetPowerLevel       The target power level of BSP when suspended,
                                certain architecture could require this value
                                to be paired with BspPowerState.
  @param TimeoutInMicroseconds  Time out in microseconds specified when the
                                timer should fire to wake up itself.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The input power level or state is not within range.
  @return Others                  Other failures from interrupt setup/restorations.
**/
EFI_STATUS
EFIAPI
MpMgmtBspSuspend (
  IN  MP_MANAGEMENT_PROTOCOL *This,
  IN  AP_POWER_STATE BspPowerState,
  IN  UINTN TargetPowerLevel, OPTIONAL
  IN  UINTN                   TimeoutInMicroseconds
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  if (BspPowerState >= AP_POWER_NUM) {
    DEBUG ((DEBUG_ERROR, "%a The power state is not supported %d\n", __FUNCTION__, BspPowerState));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  // set up timer and turn off others...
  Status = CpuArchDisableAllInterruptsButSetupTimer (&Handle, TimeoutInMicroseconds);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a The timer setup is failed %r\n", __FUNCTION__, Status));
    goto Done;
  }

  switch (BspPowerState) {
    case AP_POWER_C1:
      DEBUG ((DEBUG_INFO, "%a See you later.\n", __FUNCTION__));
      Status = CpuArchHalt ();
      if (EFI_ERROR (Status)) {
        // if we ever return from this power level, something is off.
        DEBUG ((DEBUG_INFO, "%a failed to clock gate - %r.\n", __FUNCTION__, Status));
      }

      break;
    case AP_POWER_C2:
      DEBUG ((DEBUG_INFO, "%a Siesta time.\n", __FUNCTION__));
      Status = CpuArchClockGate (TargetPowerLevel);
      if (EFI_ERROR (Status)) {
        // if we ever return from this power level, something is off.
        DEBUG ((DEBUG_INFO, "%a failed to enter stand by - %r.\n", __FUNCTION__, Status));
      }

      break;
    case AP_POWER_C3:
      DEBUG ((DEBUG_INFO, "%a Good night.\n", __FUNCTION__));
      // Setup a long jump buffer so that the cores can come back to the same place after resuming.
      if (SetJump ((BASE_LIBRARY_JUMP_BUFFER *)(&(mCommonBuffer[mBspIndex].JumpBuffer))) == 0) {
        // This allows the program to perform extra steps for sleep specific transitioning
        Status = CpuArchBspSleepPrep (TargetPowerLevel, TimeoutInMicroseconds);
        if (EFI_ERROR (Status)) {
          // failed to setup the bed, cannot sleep...
          DEBUG ((DEBUG_INFO, "%a failed to prepare for sleeping - %r.\n", __FUNCTION__, Status));
          break;
        }

        Status = CpuArchSleep (TargetPowerLevel);
        if (EFI_ERROR (Status)) {
          // if we ever return from this power level, something is off.
          DEBUG ((DEBUG_INFO, "%a failed to sleep - %r.\n", __FUNCTION__, Status));
        } else {
          // This does not make much sense, C3 sleep should not come back here.
          ASSERT (FALSE);
        }
      } else {
        // Got back from the C-states, do some more clean up for BSP.
      }

      break;
    default:
      ASSERT (FALSE);
      break;
  }

Done:
  Status = CpuArchRestoreAllInterrupts (Handle);
  return Status;
}

/**
  Function to perform AP power on.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered on.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in ON state.
  @return EFI_ABORTED             The target AP is in unexpected states.
  @return Others                  Other errors from MP services.
**/
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

  if ((ProcessorNumber == mBspIndex) || ((ProcessorNumber > mNumCpus) && (ProcessorNumber != OPERATION_FOR_ALL_APS))) {
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
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      DEBUG ((DEBUG_INFO, "%a Processor (%d) disabled, skipping processing\n", __FUNCTION__, Index));
      continue;
    }

    if (mCommonBuffer[Index].ApStatus == AP_STATE_ON) {
      DEBUG ((DEBUG_WARN, "%a The specified processor (%d) is already in ON\n", __FUNCTION__, Index));
      Status = EFI_ALREADY_STARTED;
      continue;
    }

    if (mCommonBuffer[Index].ApStatus != AP_STATE_OFF) {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is already started\n", __FUNCTION__, Index));
      Status = EFI_ABORTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus = AP_STATE_ON;
    ZeroMem (mCommonBuffer[Index].ApBuffer, mCommonBuffer[Index].ApBufferSize);

    // This is the flag to release the core.
    mCommonBuffer[Index].ApTask = AP_TASK_ACTIVE;

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
    if ((Status != EFI_SUCCESS) && (Status != EFI_TIMEOUT)) {
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

    if (!IsProcessorEnabled (mMpServices, Index)) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {
    }

    DEBUG ((DEBUG_INFO, "Initial message from common buffer: %a\n", (CHAR8 *)mCommonBuffer[Index].ApBuffer));
  }

Done:
  return Status;
}

/**
  Function to perform AP power off.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in OFF state.
  @return EFI_ABORTED             The target AP is in unexpected states.
  @return Others                  Other errors from MP services.
**/
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

  if ((ProcessorNumber == mBspIndex) || ((ProcessorNumber > mNumCpus) && (ProcessorNumber != OPERATION_FOR_ALL_APS))) {
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

  Status = EFI_NOT_FOUND;
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      DEBUG ((DEBUG_INFO, "%a Processor (%d) disabled, skipping processing\n", __FUNCTION__, Index));
      continue;
    }

    if (mCommonBuffer[Index].ApStatus == AP_STATE_OFF) {
      DEBUG ((DEBUG_WARN, "%a The specified processor (%d) is already in OFF state\n", __FUNCTION__, Index));
      Status = EFI_ALREADY_STARTED;
      continue;
    }

    if (mCommonBuffer[Index].ApStatus != AP_STATE_ON) {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is not in ON state (%d)\n", __FUNCTION__, Index, mCommonBuffer[Index].ApStatus));
      Status = EFI_ABORTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus = AP_STATE_OFF;
    mCommonBuffer[Index].ApTask       = AP_TASK_ACTIVE;

    // At least we are successful for this AP.
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // If successful, print the hellow world (blocking) from the APs.
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {
    }

    DEBUG ((DEBUG_INFO, "Last word from common buffer: %a\n", (CHAR8 *)mCommonBuffer[Index].ApBuffer));
  }

  // TODO: This is not ideal, but we could have messed up with the AP status here, wait for a bit to let the timer do the cleanup
  gBS->Stall (50000);

Done:
  return Status;
}

/**
  Function to perform AP execution suspend.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.
  @param ApPowerState     The intended power state the CPU should be suspended to.
                          The support input values are defined in AP_POWER_STATE.
  @param TargetPowerLevel The target power level of AP when suspended, certain
                          architecture could require this value to be paired with
                          ApPowerState.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range or power state is
                                  not setup properly.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in intended power state.
  @return EFI_ABORTED             The target AP is in unexpected states.
**/
STATIC
EFI_STATUS
EFIAPI
MpMgmtApSuspend (
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber,
  IN  AP_POWER_STATE          ApPowerState,
  IN  UINTN                   TargetPowerLevel  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       StartIndex;
  UINTN       EndIndex;
  UINTN       Index;
  UINTN       InternalApPowerState;

  if ((ProcessorNumber == mBspIndex) || ((ProcessorNumber > mNumCpus) && (ProcessorNumber != OPERATION_FOR_ALL_APS))) {
    DEBUG ((DEBUG_ERROR, "%a The specified processor is not acceptable %d\n", __FUNCTION__, ProcessorNumber));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (mCommonBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a The common buffer is not set up\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (ApPowerState >= AP_POWER_NUM) {
    DEBUG ((DEBUG_ERROR, "%a The power state is not supported %d\n", __FUNCTION__, ApPowerState));
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  // Translate it to the internal state
  switch (ApPowerState) {
    case AP_POWER_C1:
      InternalApPowerState = AP_STATE_SUSPEND_HALT;
      break;
    case AP_POWER_C2:
      InternalApPowerState = AP_STATE_SUSPEND_CLOCK_GATE;
      break;
    case AP_POWER_C3:
      InternalApPowerState = AP_STATE_SUSPEND_SLEEP;
      break;
    default:
      // Should not happen...
      ASSERT (FALSE);
      break;
  }

  if (ProcessorNumber == OPERATION_FOR_ALL_APS) {
    StartIndex = 0;
    EndIndex   = mNumCpus - 1;
  } else {
    StartIndex = ProcessorNumber;
    EndIndex   = ProcessorNumber;
  }

  Status = EFI_NOT_FOUND;
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      DEBUG ((DEBUG_INFO, "%a Processor (%d) disabled, skipping processing\n", __FUNCTION__, Index));
      continue;
    }

    if (mCommonBuffer[Index].ApStatus == InternalApPowerState) {
      DEBUG ((DEBUG_WARN, "%a The specified processor (%d) is already in expected state (%d)\n", __FUNCTION__, Index, InternalApPowerState));
      Status = EFI_ALREADY_STARTED;
      continue;
    }

    if (mCommonBuffer[Index].ApStatus != AP_STATE_ON) {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is not in ON state (%d)\n", __FUNCTION__, Index, mCommonBuffer[Index].ApStatus));
      Status = EFI_ABORTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus     = InternalApPowerState;
    mCommonBuffer[Index].TargetPowerState = TargetPowerLevel;
    mCommonBuffer[Index].ApTask           = AP_TASK_ACTIVE;

    // At least we are successful for this AP.
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // If successful, print the hellow world (blocking) from the APs.
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {
    }

    DEBUG ((DEBUG_INFO, "Suspend message from common buffer: %a\n", (CHAR8 *)mCommonBuffer[Index].ApBuffer));
  }

  // TODO: This is not ideal, but we could have messed up with the AP status here, wait for a bit to let the timer do the cleanup
  gBS->Stall (50000);

Done:
  return Status;
}

/**
  Function to perform AP execution resumption.

  @param This             MP Management Protocol.
  @param ProcessorNumber  The CPU index to be powered off.

  @return EFI_SUCCESS             The routine completed successfully.
  @return EFI_INVALID_PARAMETER   The CPU index is out of range or power state is
                                  not setup properly.
  @return EFI_NOT_READY           The MP service is not initialized.
  @return EFI_ALREADY_STARTED     The target AP is already in ON state.
  @return EFI_ABORTED             The target AP is in unexpected states.
**/
STATIC
EFI_STATUS
EFIAPI
MpMgmtApResume (
  IN  MP_MANAGEMENT_PROTOCOL  *This,
  IN  UINTN                   ProcessorNumber
  )
{
  EFI_STATUS  Status;
  UINTN       StartIndex;
  UINTN       EndIndex;
  UINTN       Index;

  if ((ProcessorNumber == mBspIndex) || ((ProcessorNumber > mNumCpus) && (ProcessorNumber != OPERATION_FOR_ALL_APS))) {
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

  Status = EFI_NOT_FOUND;
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      DEBUG ((DEBUG_INFO, "%a Processor (%d) disabled, skipping processing\n", __FUNCTION__, Index));
      continue;
    }

    if (mCommonBuffer[Index].ApStatus == AP_STATE_ON) {
      DEBUG ((DEBUG_WARN, "%a The specified processor (%d) is already fully up\n", __FUNCTION__, Index));
      Status = EFI_ALREADY_STARTED;
      continue;
    }

    if ((mCommonBuffer[Index].ApStatus != AP_STATE_SUSPEND_HALT) &&
        (mCommonBuffer[Index].ApStatus != AP_STATE_SUSPEND_CLOCK_GATE) &&
        (mCommonBuffer[Index].ApStatus != AP_STATE_SUSPEND_SLEEP))
    {
      DEBUG ((DEBUG_ERROR, "%a The specified processor (%d) is not in expected state (%d)\n", __FUNCTION__, Index, mCommonBuffer[Index].ApStatus));
      Status = EFI_ABORTED;
      break;
    }

    // Update the task flag to be active, AP will clear it once wake up.
    mCommonBuffer[Index].TargetStatus = AP_STATE_RESUME;
    mCommonBuffer[Index].ApTask       = AP_TASK_ACTIVE;

    // Abstracted call to allow arch specific method to wake up this CPU
    CpuArchWakeFromSleep (Index);

    // At least we are successful for this AP.
    Status = EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  // If successful, print the hellow world (blocking) from the APs.
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (Index == mBspIndex) {
      continue;
    }

    if (!IsProcessorEnabled (mMpServices, Index)) {
      continue;
    }

    // Loop till specified AP is up and running
    while (mCommonBuffer[Index].ApTask != AP_TASK_IDLE) {
    }

    DEBUG ((DEBUG_INFO, "Resume message from common buffer: %a\n", (CHAR8 *)mCommonBuffer[Index].ApBuffer));
  }

Done:
  return Status;
}

MP_MANAGEMENT_PROTOCOL  mMpManagement = {
  .BspSuspend = MpMgmtBspSuspend,
  .ApOn       = MpMgmtApOn,
  .ApOff      = MpMgmtApOff,
  .ApSuspend  = MpMgmtApSuspend,
  .ApResume   = MpMgmtApResume
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
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *Image;

  Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&Image
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Parts of the code in this driver may be executed by other cores running
  // with the MMU off so we need to ensure that everything is clean to the
  // point of coherency (PoC)
  //
  WriteBackDataCacheRange (Image->ImageBase, Image->ImageSize);

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&mMpServices
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate EFI_MP_SERVICES_PROTOCOL (%r). Not installed on platform?\n", Status));
    goto Done;
  }

  Status = GetMpInformation (mMpServices, &mNumCpus, &mBspIndex);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to fetch processor information.\n"));
    goto Done;
  }

  Status = InitializeApCommonBuffer (mNumCpus, (MP_MANAGEMENT_METADATA **)&mCommonBuffer);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: Failed to initialize Ap common buffer - %r.\n", Status));
    goto Done;
  }

  Status = CpuMpArchInit (mNumCpus);
  if (EFI_ERROR (Status)) {
    return Status;
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
