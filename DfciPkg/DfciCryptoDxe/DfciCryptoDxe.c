/** @file
DfciCrypto.c

This module installs Crypto protocols used by Dfci.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include <PiDxe.h>
#include <Guid/EventGroup.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include "DfciCryptoDxe.h"

//
// Module globals.
//
EFI_EVENT           mReadyToBootEvent;
EFI_EVENT           mEndOfDxeEvent;


/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. 

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
ReadyToBootEventNotify (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  DEBUG((DEBUG_INFO, "%a \n", __FUNCTION__));
}

/**
Notify function for event group EFI_END_OF_DXE_EVENT_GROUP_GUID.

@param[in]  Event   The Event that is being processed.
@param[in]  Context The Event Context.

**/
VOID
EFIAPI
EndOfDxeEventNotify(
IN EFI_EVENT        Event,
IN VOID             *Context
)
{
  DEBUG((DEBUG_INFO, "%a \n", __FUNCTION__));
}

/**
  The module Entry Point of the Dfci Crypto Dxe Driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DfciCryptoDxeEntry(
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS  Status;


  InstallPkcs7Support(ImageHandle);

  InstallPkcs5Support(ImageHandle);

 

  //
  // Register notify function to uninstall protocols at EndOfDxe
  //
  Status = gBS->CreateEventEx(
      EVT_NOTIFY_SIGNAL,
      TPL_CALLBACK,
    EndOfDxeEventNotify,
      NULL,
    &gEfiEndOfDxeEventGroupGuid,
      &mEndOfDxeEvent
      );

  if (EFI_ERROR(Status))
  {
      DEBUG((DEBUG_ERROR, "Dfci Crypto Failed to register for End Of Dxe Event.  Status = %r\n", Status));
  }

  //
  // Register notify function to uninstall protocols at ReadyToBoot
  //
  Status = gBS->CreateEventEx(
    EVT_NOTIFY_SIGNAL,
    TPL_CALLBACK,
    ReadyToBootEventNotify,
    NULL,
    &gEfiEventReadyToBootGuid,
    &mReadyToBootEvent
    );

  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Dfci Crypto Failed to register for Ready To Boot Event.  Status = %r\n", Status));
  }



  return Status;
}
