/** @file
  This module installs Crypto protocols used by Surface System Firmware

  Copyright (c) 2015, Microsoft Corporation. All rights reserved.<BR>

**/

#include <PiDxe.h>
#include <Guid/EventGroup.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include "DfciCryptoDxe.h"

//
// Module globals.
//
EFI_EVENT								mReadyToBootEvent;
EFI_EVENT               mEndOfDxeEvent;


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
  The module Entry Point of the Surface Crypto Dxe Driver.  

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SurfaceCryptoDxeEntry(
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
	  DEBUG((DEBUG_ERROR, "Surface Crypto Failed to register for End Of Dxe Event.  Status = %r\n", Status));
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
    DEBUG((DEBUG_ERROR, "Surface Crypto Failed to register for Ready To Boot Event.  Status = %r\n", Status));
  }



  return Status;
}
