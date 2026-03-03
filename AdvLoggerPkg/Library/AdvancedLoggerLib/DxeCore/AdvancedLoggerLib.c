/** @file
  DXE_CORE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Protocol/AdvancedLogger.h>
#include <Guid/AdvancedLoggerPreDxeLogs.h>
#include <Protocol/VariablePolicy.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <Library/AdvancedLoggerHdwPortLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/TimerLib.h>
#include <Library/VariablePolicyHelperLib.h>

#include "../AdvancedLoggerCommon.h"

//
// Protocol interface that connects the DXE library instances with the AdvancedLogger
//
STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo  = NULL;
STATIC UINT32                mBufferSize   = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress   = 0;
STATIC BOOLEAN               mInitialized  = FALSE;
STATIC EFI_SYSTEM_TABLE      *mSystemTable = NULL;
STATIC EFI_HANDLE            mImageHandle  = NULL;

VOID
EFIAPI
AdvancedLoggerWriteProtocol (
  IN  ADVANCED_LOGGER_PROTOCOL  *This,
  IN  UINTN                     ErrorLevel,
  IN  CONST CHAR8               *Buffer,
  IN  UINTN                     NumberOfBytes
  );

STATIC ADVANCED_LOGGER_PROTOCOL_CONTAINER  mAdvLoggerProtocol = {
  .AdvLoggerProtocol             = {
    .Signature                   = ADVANCED_LOGGER_PROTOCOL_SIGNATURE,
    .Version                     = ADVANCED_LOGGER_PROTOCOL_VERSION,
    .AdvancedLoggerWriteProtocol = AdvancedLoggerWriteProtocol
  },
  .LoggerInfo                    = NULL
};

/**
  AdvancedLoggerWriteProtocol

  @param  This            Pointer to Protocol,
  @param  ErrorLevel      The error level of the debug message.
  @param  Buffer          The debug message to log.
  @param  NumberOfBytes   Number of bytes in the debug message.

**/
VOID
EFIAPI
AdvancedLoggerWriteProtocol (
  IN        ADVANCED_LOGGER_PROTOCOL  *This,
  IN        UINTN                     ErrorLevel,
  IN  CONST CHAR8                     *Buffer,
  IN        UINTN                     NumberOfBytes
  )
{
  AdvancedLoggerWrite (ErrorLevel, Buffer, NumberOfBytes);
}

/**
    ValidateInfoBlock

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.
    LogBufferOffset, LogCurrentOffset, and LogBufferSize could be written to by untrusted code.  Here, we check that
    the offsets are within the allocated LoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          NONE

    @return         BOOLEAN     TRUE - mInforBlock passes security checks
    @return         BOOLEAN     FALSE- mInforBlock failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  VOID
  )
{
  if (mLoggerInfo == NULL) {
    return FALSE;
  }

  if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (mLoggerInfo->LogBufferOffset != EXPECTED_LOG_BUFFER_OFFSET (mLoggerInfo)) {
    return FALSE;
  }

  if ((PA_FROM_PTR (LOG_CURRENT_FROM_ALI (mLoggerInfo)) > mMaxAddress) ||
      (mLoggerInfo->LogCurrentOffset < mLoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  if (mLoggerInfo->LogBufferSize != mBufferSize) {
    return FALSE;
  }

  return TRUE;
}

/**
  Get the Logger Information block

 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  )
{
  EFI_HOB_GUID_TYPE    *GuidHob;
  ADVANCED_LOGGER_PTR  *LogPtr;

  if (!mInitialized) {
    mInitialized = TRUE;

    //
    // Locate the Logger Information block.
    //

    if (FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
      mLoggerInfo = (ADVANCED_LOGGER_INFO *)(UINTN)FixedPcdGet64 (PcdAdvancedLoggerBase);
    } else {
      GuidHob = GetFirstGuidHob (&gAdvancedLoggerHobGuid);
      if (GuidHob != NULL) {
        LogPtr      = (ADVANCED_LOGGER_PTR *)GET_GUID_HOB_DATA (GuidHob);
        mLoggerInfo = ALI_FROM_PA (LogPtr->LogBuffer);
        if (!mLoggerInfo->HdwPortInitialized) {
          AdvancedLoggerHdwPortInitialize ();
          mLoggerInfo->HdwPortInitialized = TRUE;
        }
      }
    }

    if (mLoggerInfo != NULL) {
      mMaxAddress = LOG_MAX_ADDRESS (mLoggerInfo);
      mBufferSize = mLoggerInfo->LogBufferSize;
    }
  }

  if (((mLoggerInfo) != NULL) && !ValidateInfoBlock ()) {
    mLoggerInfo = NULL;
  } else if ((mLoggerInfo != NULL) && AdvancedLoggerCheckForNewerLogger (&mLoggerInfo, &mMaxAddress, &mBufferSize)) {
    DEBUG ((DEBUG_INFO, "DxeCore %a: Logger Update. LoggerInfo=%p\n", __func__, mLoggerInfo));
  }

  return mLoggerInfo;
}

/**
  Helper function to return the log phase for each message.

  This function is intended to be used to distinguish between
  various types of modules.

  @return       Phase of current advanced logger instance.
**/
UINT16
EFIAPI
AdvancedLoggerGetPhase (
  VOID
  )
{
  return ADVANCED_LOGGER_PHASE_DXE;
}

/**
    OnRuntimeArchNotification

    Collect the UEFI system time.

  **/
STATIC
VOID
EFIAPI
OnRealTimeClockArchNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS        Status;
  EFI_SYSTEM_TABLE  *SystemTable;

  SystemTable = (EFI_SYSTEM_TABLE *)Context;

  DEBUG ((DEBUG_INFO, "%a: getting real time\n", __func__));

  SystemTable->BootServices->CloseEvent (Event);

  if (mLoggerInfo != NULL) {
    Status = SystemTable->RuntimeServices->GetTime ((EFI_TIME *)&mLoggerInfo->Time, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: error getting real time. Code=%r\n", __func__, Status));
    } else {
      mLoggerInfo->TicksAtTime = GetPerformanceCounter ();
    }
  }

  return;
}

/**
    OnVariableWriteNotification

    Writes the log locator variable.

  **/
STATIC
VOID
EFIAPI
OnVariableWriteNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  IN EFI_SYSTEM_TABLE  *SystemTable;

  SystemTable = (EFI_SYSTEM_TABLE *)Context;

  DEBUG ((DEBUG_INFO, "%a: writing locator variable\n", __func__));

  SystemTable->RuntimeServices->SetVariable (
                                  ADVANCED_LOGGER_LOCATOR_NAME,
                                  &gAdvancedLoggerHobGuid,
                                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                                  sizeof (mLoggerInfo),
                                  (VOID *)&mLoggerInfo
                                  );

  SystemTable->BootServices->CloseEvent (Event);

  return;
}

/**
    OnVariablePolicyProtocolNotification

    Sets the AdvancedLogger Locator variable policy.

  **/
STATIC
VOID
EFIAPI
OnVariablePolicyProtocolNotification (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy = NULL;
  EFI_SYSTEM_TABLE                *SystemTable;
  EFI_STATUS                      Status;

  SystemTable = (EFI_SYSTEM_TABLE *)Context;

  DEBUG ((DEBUG_INFO, "%a: writing locator variable policy\n", __func__));

  Status = SystemTable->BootServices->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: - Locating Variable Policy failed - Code=%r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  Status = RegisterBasicVariablePolicy (
             VariablePolicy,
             &gAdvancedLoggerHobGuid,
             ADVANCED_LOGGER_LOCATOR_NAME,
             sizeof (mLoggerInfo),
             sizeof (mLoggerInfo),
             EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             (UINT32) ~(EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS),
             VARIABLE_POLICY_TYPE_LOCK_ON_CREATE               // Will act as LOCK now if already created
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: - Error registering AdvancedLoggerLocator - Code=%r\n", __func__, Status));
    ASSERT_EFI_ERROR (Status);
  }

  return;
}

/**
    ProcessProtocolRegistration

    This function registers for Variable Write being available.

    @param       VOID

    @retval      EFI_SUCCESS     Variable Write protocol registration successful
    @retval      error code      Something went wrong.

 **/
EFI_STATUS
ProcessProtocolRegistration (
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN EFI_GUID          *ProtocolGuid,
  IN EFI_EVENT_NOTIFY  NotifyFunction
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ProtocolEvent;
  VOID        *ProtocolRegistration;

  //
  // Register for protocol notification.
  //
  DEBUG ((DEBUG_INFO, "%a: Registering for %g\n", __func__, ProtocolGuid));
  Status = SystemTable->BootServices->CreateEvent (
                                        EVT_NOTIFY_SIGNAL,
                                        TPL_CALLBACK,
                                        NotifyFunction,
                                        SystemTable,
                                        &ProtocolEvent
                                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create notification callback event (%r)\n", __func__, Status));
    goto Cleanup;
  }

  Status = SystemTable->BootServices->RegisterProtocolNotify (
                                        ProtocolGuid,
                                        ProtocolEvent,
                                        &ProtocolRegistration
                                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to register for notification (%r)\n", __func__, Status));
    SystemTable->BootServices->CloseEvent (ProtocolEvent);
    goto Cleanup;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**
  Initialize a logger info structure with basic values.

  Sets up the signature, version, buffer offsets, sizes, and hardware print level.
  Does not initialize hardware port - caller must do that separately if needed.

  @param[in,out] LoggerInfo  Pointer to logger info structure to initialize.

**/
STATIC
VOID
InitializeLoggerInfoStructure (
  IN OUT ADVANCED_LOGGER_INFO  *LoggerInfo
  )
{
  if (LoggerInfo == NULL) {
    return;
  }

  ZeroMem ((VOID *)LoggerInfo, sizeof (ADVANCED_LOGGER_INFO));
  LoggerInfo->Signature        = ADVANCED_LOGGER_SIGNATURE;
  LoggerInfo->Version          = ADVANCED_LOGGER_VERSION;
  LoggerInfo->LogBufferOffset  = EXPECTED_LOG_BUFFER_OFFSET (LoggerInfo);
  LoggerInfo->LogBufferSize    = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof (ADVANCED_LOGGER_INFO);
  LoggerInfo->LogCurrentOffset = LoggerInfo->LogBufferOffset;
  LoggerInfo->HwPrintLevel     = FixedPcdGet32 (PcdAdvancedLoggerHdwPortDebugPrintErrorLevel);
}

/**
  Process logs in the pre-DXE logs HOB.

  Retrieves the pre-DXE logs HOB and writes the log data to the logger buffer.

**/
STATIC
VOID
ProcessPreDxeLogs (
  VOID
  )
{
  ADVANCED_LOGGER_PRE_DXE_LOGS_HOB  *PreDxeLogs;
  EFI_HOB_GUID_TYPE                 *PreDxeLogsHobEntry;

  PreDxeLogsHobEntry = GetFirstGuidHob (&gAdvancedLoggerPreDxeLogsGuid);
  if (PreDxeLogsHobEntry != NULL) {
    PreDxeLogs = (ADVANCED_LOGGER_PRE_DXE_LOGS_HOB *)GET_GUID_HOB_DATA (PreDxeLogsHobEntry);
    if (PreDxeLogs->Signature != ADVANCED_LOGGER_PRE_DXE_LOGS_SIGNATURE) {
      ASSERT (PreDxeLogs->Signature == ADVANCED_LOGGER_PRE_DXE_LOGS_SIGNATURE);
    } else {
      AdvancedLoggerMemoryLoggerWrite (DEBUG_INFO, (CONST CHAR8 *)(UINTN)PreDxeLogs->BaseAddress, PreDxeLogs->LengthInBytes);
    }
  }
}

/**
  Initialize the timer frequency for the logger.

**/
STATIC
VOID
InitializeTimerFrequency (
  VOID
  )
{
  if (mLoggerInfo != NULL) {
    mLoggerInfo->TimerFrequency = GetPerformanceCounterProperties (NULL, NULL);
  }
}

/**
  Migrate the PEI logger buffer to a new DXE reserved buffer.

  Called at End of DXE to ensure all MM services are available to unblock the new logger buffer.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context.
**/
STATIC
VOID
EFIAPI
OnEndOfDxe (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;
  ADVANCED_LOGGER_INFO  *ExistingLoggerInfo;
  ADVANCED_LOGGER_INFO  *NewLoggerInfo;
  EFI_BOOT_SERVICES     *BootServices;

  DEBUG ((DEBUG_INFO, "%a: Migrating logger buffer\n", __func__));

  BootServices = (EFI_BOOT_SERVICES *)Context;
  if (BootServices == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Boot services context is null, the logger buffer will not be migrated.\n", __func__));
    return;
  }

  ExistingLoggerInfo = AdvancedLoggerGetLoggerInfo ();

  //
  // Allocate new reserved buffer for the logger
  //
  NewLoggerInfo = (ADVANCED_LOGGER_INFO *)AllocateReservedPages (FixedPcdGet32 (PcdAdvancedLoggerPages));
  if (NewLoggerInfo == NULL) {
    ASSERT (NewLoggerInfo != NULL);
    return;
  }

  //
  // Unblock the new buffer for MM access
  // If this is not needed on a platform, the null instance of MmUnblockMemoryLib can be used.
  //
  Status = MmUnblockMemoryRequest (
             (EFI_PHYSICAL_ADDRESS)(UINTN)NewLoggerInfo,
             FixedPcdGet32 (PcdAdvancedLoggerPages)
             );
  if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to unblock advanced logger buffer for MM access - %r\n", __func__, Status));
  }

  //
  // Prevent other notifications at TPL NOTIFY or lower from interrupting the overall migration flow.
  // First, check that raising to TPL_NOTIFY is successful.
  //
  OldTpl = BootServices->RaiseTPL (TPL_NOTIFY);

  //
  // Raise to TPL_HIGH_LEVEL during the main copy operation so interrupts are disabled.
  // No need to store the current TPL since was just set to TPL_NOTIFY.
  //
  BootServices->RaiseTPL (TPL_HIGH_LEVEL);

  InitializeLoggerInfoStructure (NewLoggerInfo);

  //
  // If a pre-existing buffer was provided, copy its contents to the new buffer
  //
  if (ExistingLoggerInfo != NULL) {
    NewLoggerInfo->TimerFrequency = ExistingLoggerInfo->TimerFrequency;
    NewLoggerInfo->TicksAtTime    = ExistingLoggerInfo->TicksAtTime;
    CopyMem ((VOID *)&NewLoggerInfo->Time, (VOID *)&ExistingLoggerInfo->Time, sizeof (NewLoggerInfo->Time));

    if (ExistingLoggerInfo->LogCurrentOffset > ExistingLoggerInfo->LogBufferOffset) {
      CopyMem (
        LOG_BUFFER_FROM_ALI (NewLoggerInfo),
        LOG_BUFFER_FROM_ALI (ExistingLoggerInfo),
        USED_LOG_SIZE (ExistingLoggerInfo)
        );
      NewLoggerInfo->LogCurrentOffset = NewLoggerInfo->LogBufferOffset + USED_LOG_SIZE (ExistingLoggerInfo);
    }

    NewLoggerInfo->DiscardedSize = ExistingLoggerInfo->DiscardedSize;

    //
    // Set the old logger info's NewLoggerInfoAddress to redirect to the new buffer
    //
    ExistingLoggerInfo->NewLoggerInfoAddress = PA_FROM_PTR (NewLoggerInfo);
  }

  //
  // Update module state to use the new buffer
  //
  mMaxAddress                   = LOG_MAX_ADDRESS (NewLoggerInfo);
  mBufferSize                   = NewLoggerInfo->LogBufferSize;
  mLoggerInfo                   = NewLoggerInfo;
  mAdvLoggerProtocol.LoggerInfo = NewLoggerInfo;

  //
  // Restore back to TPL_NOTIFY before calling functions with level restrictions.
  //
  BootServices->RestoreTPL (TPL_NOTIFY);

  //
  // Initialize the hardware port if not already done
  //
  if (!NewLoggerInfo->HdwPortInitialized) {
    AdvancedLoggerHdwPortInitialize ();
    NewLoggerInfo->HdwPortInitialized = TRUE;
  }

  //
  // Reinstall protocol with the new buffer information.
  // This updates any existing protocol installation to point to the migrated buffer.
  //
  Status = mSystemTable->BootServices->ReinstallProtocolInterface (
                                         mImageHandle,
                                         &gAdvancedLoggerProtocolGuid,
                                         &mAdvLoggerProtocol.AdvLoggerProtocol,
                                         &mAdvLoggerProtocol.AdvLoggerProtocol
                                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error reinstalling advanced logger protocol - %r\n", __func__, Status));
  }

  BootServices->RestoreTPL (OldTpl);

  //
  // Close the event as we only need to migrate once
  //
  mSystemTable->BootServices->CloseEvent (Event);
}

/**
  DxeCore Advanced Logger initialization.
**/
EFI_STATUS
EFIAPI
DxeCoreAdvancedLoggerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_EVENT             EndOfDxeEvent;
  ADVANCED_LOGGER_INFO  *LoggerInfo;

  mSystemTable = SystemTable;
  mImageHandle = ImageHandle;

  LoggerInfo = AdvancedLoggerGetLoggerInfo ();

  //
  // For an implementation of the AdvancedLogger with a pre-DXE implementation, there will be a
  // Logger Information block published and available.
  //
  if (LoggerInfo == NULL) {
    LoggerInfo = (ADVANCED_LOGGER_INFO *)AllocateReservedPages (FixedPcdGet32 (PcdAdvancedLoggerPages));
    if (LoggerInfo != NULL) {
      InitializeLoggerInfoStructure (LoggerInfo);

      mMaxAddress = LOG_MAX_ADDRESS (LoggerInfo);
      mBufferSize = LoggerInfo->LogBufferSize;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Error allocating Advanced Logger Buffer\n", __func__));
    }
  }

  mLoggerInfo = LoggerInfo;
  if (LoggerInfo != NULL) {
    //
    // Initialize hardware port if not already done
    //
    if (!LoggerInfo->HdwPortInitialized) {
      AdvancedLoggerHdwPortInitialize ();
      LoggerInfo->HdwPortInitialized = TRUE;
    }

    mMaxAddress                   = LOG_MAX_ADDRESS (LoggerInfo);
    mBufferSize                   = LoggerInfo->LogBufferSize;
    mAdvLoggerProtocol.LoggerInfo = LoggerInfo;

    InitializeTimerFrequency ();
    ProcessPreDxeLogs ();

    //
    // Install protocol
    //
    Status = SystemTable->BootServices->InstallProtocolInterface (
                                          &ImageHandle,
                                          &gAdvancedLoggerProtocolGuid,
                                          EFI_NATIVE_INTERFACE,
                                          &mAdvLoggerProtocol.AdvLoggerProtocol
                                          );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error installing protocol - %r\n", __func__, Status));
      // If the protocol doesn't install, don't fail.
    }

    //
    // Only allocate a new reserved buffer and migrate when the address was not designated
    // to be fixed in RAM.
    //
    if (!FeaturePcdGet (PcdAdvancedLoggerFixedInRAM)) {
      //
      // Migrate the advanced logger buffer at End of DXE.
      // This allows the migrated buffer to be unblocked for MM access.
      //
      Status = SystemTable->BootServices->CreateEventEx (
                                            EVT_NOTIFY_SIGNAL,
                                            TPL_CALLBACK,
                                            OnEndOfDxe,
                                            SystemTable->BootServices,
                                            &gEfiEndOfDxeEventGroupGuid,
                                            &EndOfDxeEvent
                                            );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to create End of DXE event - %r\n", __func__, Status));
      }
    }
  }

  DEBUG ((DEBUG_INFO, "%a Initialized. mLoggerInfo = %p, Container=%p\n", __func__, mLoggerInfo, &mAdvLoggerProtocol));

  ProcessProtocolRegistration (
    SystemTable,
    &gEfiRealTimeClockArchProtocolGuid,
    OnRealTimeClockArchNotification
    );

  if (FeaturePcdGet (PcdAdvancedLoggerLocator)) {
    ProcessProtocolRegistration (
      SystemTable,
      &gEfiVariableWriteArchProtocolGuid,
      OnVariableWriteNotification
      );
    ProcessProtocolRegistration (
      SystemTable,
      &gEdkiiVariablePolicyProtocolGuid,
      OnVariablePolicyProtocolNotification
      );
  }

  return EFI_SUCCESS;
}
