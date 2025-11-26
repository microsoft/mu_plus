/** @file
  DXE implementation of Advanced Logger Library.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Guid/EventGroup.h>

#include <Protocol/AdvancedLogger.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

#include "../AdvancedLoggerCommon.h"

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo                 = NULL;
STATIC UINT32                mBufferSize                  = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress                  = 0;
STATIC EFI_BOOT_SERVICES     *mBS                         = NULL;
STATIC EFI_EVENT             mExitBootServicesEvent       = NULL;
STATIC EFI_EVENT             mAdvancedLoggerProtocolEvent = NULL;
STATIC VOID                  *mAdvancedLoggerProtocolReg  = NULL;

/**
    CheckAddress

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.
    LogBufferOffset and LogCurrentOffset, and LogBufferSize could be written to by untrusted code.  Here, we check that
    the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
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

  if (mBufferSize == 0) {
    mBufferSize = mLoggerInfo->LogBufferSize;
  } else {
    if (mLoggerInfo->LogBufferSize != mBufferSize) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Update cached logger info from the first Advanced Logger Protocol instance found.

  @retval EFI_SUCCESS           Logger info successfully updated.
  @retval EFI_NOT_FOUND         The advanced logger protocol was not found.
  @retval EFI_INVALID_PARAMETER Invalid protocol signature or version.

**/
STATIC
EFI_STATUS
UpdateLoggerInfoFromProtocol (
  VOID
  )
{
  ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol;
  EFI_STATUS                Status;

  if (mBS == NULL) {
    return EFI_NOT_READY;
  }

  Status = mBS->LocateProtocol (
                  &gAdvancedLoggerProtocolGuid,
                  NULL,
                  (VOID **)&LoggerProtocol
                  );
  if (EFI_ERROR (Status) || (LoggerProtocol == NULL)) {
    return EFI_NOT_FOUND;
  }

  ASSERT (LoggerProtocol->Signature == ADVANCED_LOGGER_PROTOCOL_SIGNATURE);
  ASSERT (LoggerProtocol->Version == ADVANCED_LOGGER_PROTOCOL_VERSION);

  if ((LoggerProtocol->Signature != ADVANCED_LOGGER_PROTOCOL_SIGNATURE) ||
      (LoggerProtocol->Version != ADVANCED_LOGGER_PROTOCOL_VERSION))
  {
    return EFI_INVALID_PARAMETER;
  }

  mLoggerInfo = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);

  if (mLoggerInfo != NULL) {
    mMaxAddress = LOG_MAX_ADDRESS (mLoggerInfo);
    // Force the buffer size to be reevaluated
    mBufferSize = 0;
  }

  return EFI_SUCCESS;
}

/**
    Get the Logger Information block

    @return   A pointer to the active advanced logger info structure or NULL if the structure is
              invalid.
 **/
ADVANCED_LOGGER_INFO *
EFIAPI
AdvancedLoggerGetLoggerInfo (
  VOID
  )
{
  if (mLoggerInfo == NULL) {
    UpdateLoggerInfoFromProtocol ();
  }

  if (!ValidateInfoBlock ()) {
    mLoggerInfo = NULL;
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
  return ADVANCED_LOGGER_PHASE_RUNTIME;
}

/**
  Notification function for Advanced Logger Protocol installation.

  This function is called when the Advanced Logger Protocol is installed or reinstalled
  (e.g., during buffer migration). It updates the globally cached logger to the new logger buffer.

  @param[in]  Event    Event whose notification function is being invoked.
  @param[in]  Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnAdvancedLoggerProtocolNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UpdateLoggerInfoFromProtocol ();
}

/**
    Inform all instances of Advanced Logger that ExitBoot Services has occurred.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none
 **/
VOID
EFIAPI
OnExitBootServicesNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  //
  // Runtime logging is currently not supported, so clear mLoggerInfo.
  //
  mLoggerInfo = NULL;
  mBS         = NULL;
}

/**
    The constructor registers for the ExitBootServices event.

    @param  ImageHandle   The firmware allocated handle for the EFI image.
    @param  SystemTable   A pointer to the EFI System Table.

    @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeRuntimeAdvancedLoggerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Cache BootServices pointer as AdvLogger calls may be made before
  // the constructor runs.
  //
  mBS = SystemTable->BootServices;
  AdvancedLoggerGetLoggerInfo ();

  //
  // Register a notification function for Advanced Logger Protocol installation.
  //
  Status = mBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnAdvancedLoggerProtocolNotification,
                  NULL,
                  &mAdvancedLoggerProtocolEvent
                  );

  if (!EFI_ERROR (Status)) {
    Status = mBS->RegisterProtocolNotify (
                    &gAdvancedLoggerProtocolGuid,
                    mAdvancedLoggerProtocolEvent,
                    &mAdvancedLoggerProtocolReg
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Register Protocol Notify failed. Code = %r\n", __func__, Status));
      mBS->CloseEvent (mAdvancedLoggerProtocolEvent);
      mAdvancedLoggerProtocolEvent = NULL;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a - Create Event for Protocol Notify failed. Code = %r\n", __func__, Status));
  }

  if (mLoggerInfo != NULL) {
    //
    // Register notify function for ExitBootServices.
    //
    Status = mBS->CreateEvent (
                    EVT_SIGNAL_EXIT_BOOT_SERVICES,
                    TPL_CALLBACK,
                    OnExitBootServicesNotification,
                    NULL,
                    &mExitBootServicesEvent
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Create Event for Exit Boot Services failed. Code = %r\n", __FUNCTION__, Status));
    }
  }

  return EFI_SUCCESS;
}

/**
    The destructor closes the registered event in case of driver failure.

    @param  ImageHandle   The firmware allocated handle for the EFI image.
    @param  SystemTable   A pointer to the EFI System Table.

    @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeRuntimeAdvancedLoggerLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mExitBootServicesEvent != NULL) {
    mBS->CloseEvent (mExitBootServicesEvent);
  }

  if (mAdvancedLoggerProtocolEvent != NULL) {
    mBS->CloseEvent (mAdvancedLoggerProtocolEvent);
  }

  return EFI_SUCCESS;
}
