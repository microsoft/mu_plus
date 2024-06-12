/** @file

  The boot services environment configuration library for the Adv Logger OS Connector PRM module.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/PcdLib.h>
#include <Protocol/PrmConfig.h>
#include <Protocol/AdvancedLogger.h>
#include <Guid/EventGroup.h>
#include <AdvancedLoggerInternal.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <PrmContextBuffer.h>
#include <PrmDataBuffer.h>

#include "../../AdvLoggerOsConnectorPrm.h"

// we need to have a module global of the static data buffer so that we can update the LoggerInfo pointer
// on the virtual address change event
STATIC PRM_DATA_BUFFER  *mStaticDataBuffer       = NULL;
STATIC EFI_HANDLE       mPrmConfigProtocolHandle = NULL;
EFI_EVENT               mVirtualAddressChangeEvent;

/**
  Convert internal pointer addresses to virtual addresses.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
AdvLoggerOsConnectorPrmVirtualAddressCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                  Status;
  ADV_LOGGER_PRM_DATA_BUFFER  *DataBuf;

  if (mStaticDataBuffer != NULL) {
    DataBuf = (ADV_LOGGER_PRM_DATA_BUFFER *)mStaticDataBuffer->Data;
    Status  = EfiConvertPointer (0, (VOID **)&(DataBuf->LoggerInfo));
    if (EFI_ERROR (Status)) {
      // if we failed to convert the pointer, we need to nullify the data as the PRM won't know this isn't
      // a virtual pointer and could read random kernel memory or crash the kernel if the physical address
      // is invalid in virtual space. We don't free the old StaticDataBuffer as we are past ExitBootServices
      // at this point
      DataBuf->LoggerInfo         = NULL;
      DataBuf->ExpectedHeaderSize = 0;
      DataBuf->ExpectedLogSize    = 0;
    }
  }
}

/**
    CheckAddress

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.
    LogBufferOffset, LogCurrentOffset, and LogBufferSize, could be written to by untrusted code.  Here, we check that
    the offsets are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          DataBuf     PRM static data buffer containing LoggerInfo pointer

    @return         BOOLEAN     TRUE - LoggerInfo passes security checks
    @return         BOOLEAN     FALSE- LoggerInfo failed security checks

**/
STATIC
BOOLEAN
PrmConfigLibValidateInfoBlock (
  ADV_LOGGER_PRM_DATA_BUFFER  *DataBuf
  )
{
  if ((DataBuf == NULL) || (DataBuf->LoggerInfo == NULL)) {
    return FALSE;
  }

  if (DataBuf->LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (DataBuf->LoggerInfo->LogBufferOffset != sizeof (*(DataBuf->LoggerInfo))) {
    return FALSE;
  }

  if ((DataBuf->LoggerInfo->LogCurrentOffset > TOTAL_LOG_SIZE_WITH_ALI (DataBuf->LoggerInfo)) ||
      (DataBuf->LoggerInfo->LogCurrentOffset < DataBuf->LoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  // ensure that the passed in sizes match what we see in the advanced logger structure
  if ((DataBuf->ExpectedLogSize != DataBuf->LoggerInfo->LogBufferSize) ||
      (DataBuf->ExpectedHeaderSize != DataBuf->LoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Constructor of the PRM configuration library.

  @param[in] ImageHandle        The image handle of the driver.
  @param[in] SystemTable        The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed successfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
AdvLoggerOsConnectorPrmConfigLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  PRM_CONFIG_PROTOCOL         *PrmConfigProtocol = NULL;
  ADVANCED_LOGGER_PROTOCOL    *LoggerProtocol;
  ADV_LOGGER_PRM_DATA_BUFFER  *DataBuf          = NULL;
  PRM_CONTEXT_BUFFER          *PrmContextBuffer = NULL;
  UINTN                       DataBufferLength;

  //
  // Before we do anything, let's make sure the PCD was set correctly for the size. If we have
  // a log buffer size < sizeof (AdvancedLoggerInfo), we need to fail as that is a misconfiguration.
  // This is the one place in this lib where we want to assert, as this is a dangerous configuration
  // that will cause buffer overflow issues
  //
  if (FixedPcdGet32 (PcdAdvancedLoggerPages) < sizeof (ADVANCED_LOGGER_INFO)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a PcdAdvancedLoggerPages is < sizeof (ADVANCED_LOGGER_INFO)! This is a misconfiguration.",
      __func__
      ));
    ASSERT (FixedPcdGet32 (PcdAdvancedLoggerPages) >= sizeof (ADVANCED_LOGGER_INFO));
    Status = EFI_BAD_BUFFER_SIZE;
    goto Done;
  }

  //
  // Length of the data buffer = Buffer Header Size + Size of LoggerInfo pointer
  //
  DataBufferLength = sizeof (PRM_DATA_BUFFER_HEADER) + sizeof (ADV_LOGGER_PRM_DATA_BUFFER);

  mStaticDataBuffer = AllocateRuntimeZeroPool (DataBufferLength);
  if (mStaticDataBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a Failed to allocate static buffer\n", __func__));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Locate the Logger Information block.
  //
  Status = gBS->LocateProtocol (
                  &gAdvancedLoggerProtocolGuid,
                  NULL,
                  (VOID **)&LoggerProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a Failed to find Advanced Logger Protocol\n", __func__));
    goto Done;
  }

  //
  // To ensure we only read the size of the log buffer in the PRM, read the log buffer size
  // directly from the PCD. We need to share the header size we are using, which is recorded as
  // the log buffer offset
  //
  DataBuf                     = (ADV_LOGGER_PRM_DATA_BUFFER *)mStaticDataBuffer->Data;
  DataBuf->LoggerInfo         = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);
  DataBuf->ExpectedLogSize    = EFI_PAGES_TO_SIZE (FixedPcdGet32 (PcdAdvancedLoggerPages)) - sizeof (ADVANCED_LOGGER_INFO);
  DataBuf->ExpectedHeaderSize = EXPECTED_LOG_BUFFER_OFFSET (DataBuf->LoggerInfo);
  if (!PrmConfigLibValidateInfoBlock (DataBuf)) {
    DEBUG ((DEBUG_ERROR, "AdvLoggerOsConnectorPrmConfigLib Failed to validate AdvLogger region\n"));
    Status = EFI_COMPROMISED_DATA;
    goto Done;
  }

  //
  // Initialize the data buffer header
  //
  mStaticDataBuffer->Header.Signature = PRM_DATA_BUFFER_HEADER_SIGNATURE;
  mStaticDataBuffer->Header.Length    = (UINT32)DataBufferLength;

  //
  // Allocate and populate the context buffer
  //

  //
  // This context buffer is not actually used by PRM handler at OS runtime. The OS will allocate
  // the actual context buffer passed to the PRM handler.
  //
  // This context buffer is used internally in the firmware to associate a PRM handler with a
  // a static data buffer and a runtime MMIO ranges array so those can be placed into the
  // PRM_HANDLER_INFORMATION_STRUCT and PRM_MODULE_INFORMATION_STRUCT respectively for the PRM handler.
  //
  PrmContextBuffer = AllocateZeroPool (sizeof (*PrmContextBuffer));
  if (PrmContextBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyGuid (&PrmContextBuffer->HandlerGuid, &mAdvLoggerOsConnectorPrmHandlerGuid);
  PrmContextBuffer->Signature = PRM_CONTEXT_BUFFER_SIGNATURE;
  PrmContextBuffer->Version   = PRM_CONTEXT_BUFFER_INTERFACE_VERSION;

  PrmConfigProtocol = AllocateZeroPool (sizeof (*PrmConfigProtocol));
  if (PrmConfigProtocol == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  CopyGuid (&PrmConfigProtocol->ModuleContextBuffers.ModuleGuid, &mPrmModuleGuid);
  PrmConfigProtocol->ModuleContextBuffers.BufferCount = 1;
  PrmConfigProtocol->ModuleContextBuffers.Buffer      = PrmContextBuffer;
  PrmContextBuffer->StaticDataBuffer                  = mStaticDataBuffer;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  AdvLoggerOsConnectorPrmVirtualAddressCallback,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );

  // if we couldn't virtualize the address, we need to not publish the config protocol, as a physical
  // address could point anywhere in kernel virtual space and we would give that to the caller or
  // crash the system if it is unaccessible
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed to register for virtual address callback Status %r\n", __func__, Status));
    goto Done;
  }

  //
  // Install the PRM Configuration Protocol for this module. This indicates the configuration
  // library has completed resource initialization for the PRM module.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mPrmConfigProtocolHandle,
                  &gPrmConfigProtocolGuid,
                  (VOID *)PrmConfigProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a failed to install config protocol\n", __func__));
    // if we fail to close the event, that's ok because in the event callback
    // we check to see if mStaticDataBuffer is NULL, which in the error path
    // we reset
    gBS->CloseEvent (mVirtualAddressChangeEvent);
    goto Done;
  }

Done:
  if (EFI_ERROR (Status)) {
    if ((DataBuf != NULL) && (DataBuf->LoggerInfo != NULL)) {
      // if we have failed out, even though we are freeing the memory and should not
      // have produced the config protocol, let's be sure not to pass the log pointer
      // to ensure we don't have an invalid address that could be read
      DataBuf->LoggerInfo = NULL;
    }

    if (mStaticDataBuffer != NULL) {
      FreePool (mStaticDataBuffer);
      mStaticDataBuffer = NULL;
    }

    if (PrmContextBuffer != NULL) {
      FreePool (PrmContextBuffer);
    }

    if (PrmConfigProtocol != NULL) {
      FreePool (PrmConfigProtocol);
    }
  }

  // if we failed to setup the PRM, we should still boot as this is a diagnostic fetching mechanism
  // we've logged (the log we won't be able to fetch) and that's all we can do
  // however, we've freed the context buffer, so we won't be passing an invalid LoggerInfo pointer to the PRM module
  return EFI_SUCCESS;
}
