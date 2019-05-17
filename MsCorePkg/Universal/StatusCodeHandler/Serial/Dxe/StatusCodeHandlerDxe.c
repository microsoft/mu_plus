/** @file
  Simple SerialLib based Status Code handler

  Copyright (c) 2018, Microsoft Corporation

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <PiDxe.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Guid/EventGroup.h>
#include <Guid/StatusCodeDataTypeDebug.h>
#include <Protocol/ReportStatusCodeHandler.h>
#include "../Common/SerialStatusCodeHandler.h"

EFI_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol = NULL;
EFI_EVENT                 mExitBootServicesEvent = NULL;
EFI_EVENT                 mRscRegisterEvent = NULL;



/**

Unregister status code callback functions only available at boot time from
report status code router when exiting boot services.

@param  Event         Event whose notification function is being invoked.
@param  Context       Pointer to the notification function's context, which is
                      always zero in current implementation.

**/
VOID
EFIAPI
UnregisterBootTimeHandlers (
  IN EFI_EVENT        Event,
  IN VOID             *Context
)
{
  mRscHandlerProtocol->Unregister(SerialStatusCode);
}

/**
  Status Code DXE Entry point.
  
  Register this handler with the DXE Router

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
DxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS                Status;
  EFI_HANDLE                Handle;

  Status = gBS->LocateProtocol (
                  &gEfiRscHandlerProtocolGuid,
                  NULL,
                  (VOID **) &mRscHandlerProtocol
                  );
  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
    return Status;
  }

  Status = SerialPortInitialize();
  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
    return Status;
  }

  mRscHandlerProtocol->Register(SerialStatusCode, TPL_HIGH_LEVEL);

  //
  // This callback should be invoked AFTER the ExitBootServices callback
  // in DxeDebugLibRouter is completed to provide better print coverage.
  // So once this callback is triggered, all protocol based debug prints
  // could be routed to serial ports. TPL is used to guarantee sequence
  //
  Status = gBS->CreateEventEx (
              EVT_NOTIFY_SIGNAL,
              TPL_CALLBACK,
              UnregisterBootTimeHandlers,
              NULL,
              &gEfiEventExitBootServicesGuid,
              &mExitBootServicesEvent
              );

  //
  // Installation of this protocol notifies the DxeCore DebugLib that
  // it can switch to using RSC.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface(
             &Handle,
             &gMsSerialStatusCodeHandlerDxeProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL);

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": failed to install DXE serial status code handler protocol (%r)\n", Status));
    ASSERT_EFI_ERROR(Status);
  }

  return Status;
}


/**
  This routine writes a status code string to the correct place.
  
  @retval Buffer        Supplies a buffer holding the string to output.
  @retval NumberOfBytes Supplies a value that indicates how many bytes are in
                        the buffer.

**/
VOID
EFIAPI
WriteStatusCode (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  SerialPortWrite (Buffer, NumberOfBytes);
}
