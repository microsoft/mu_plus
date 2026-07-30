/** @file

   Graphics (GOP) Override implementation - UEFI Driver Model

   This driver uses a protocol notification to install the GopOverride protocol
   on the first GraphicsOutputProtocol handle found. It also implements the UEFI
   Driver Binding Protocol so that Stop() can uninstall GopOverride and restore
   the original GraphicsOutputProtocol, and Start() can re-bind.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GraphicsOutput.h>

//
// ****** Global variables ******
//

EFI_GUID                        *mMsGopOverrideProtocolGuid;
EFI_EVENT                       mGopRegisterEvent;
VOID                            *mGopRegistration;
EFI_HANDLE                      mBoundHandle;
EFI_DRIVER_BINDING_PROTOCOL     gGopOverrideDriverBinding;

typedef struct {
  EFI_HANDLE                    ControllerHandle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *OriginalGop;
} GOP_OVERRIDE_BINDING_MARKER;

//
// Forward declarations
//

EFI_STATUS
EFIAPI
GopOverrideDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
GopOverrideDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
GopOverrideDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

//
// Driver Binding Protocol instance
//
EFI_DRIVER_BINDING_PROTOCOL  gGopOverrideDriverBinding = {
  GopOverrideDriverBindingSupported,
  GopOverrideDriverBindingStart,
  GopOverrideDriverBindingStop,
  0x10,   // Version
  NULL,   // ImageHandle - filled in at entry
  NULL    // DriverBindingHandle - filled in at entry
};

/**
  Install GopOverride on the given handle, uninstalling the original GOP.

  @param[in] This      Driver Binding Protocol instance.
  @param[in] Handle    Handle with GraphicsOutputProtocol installed.

  @retval EFI_SUCCESS  Override installed successfully.
**/
STATIC
EFI_STATUS
InstallGopOverride (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Handle
  )
{
  EFI_STATUS                   Status;
  GOP_OVERRIDE_BINDING_MARKER  *BindingMarker;

  BindingMarker = NULL;

  BindingMarker = AllocateZeroPool (sizeof (*BindingMarker));
  if (BindingMarker == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  BindingMarker->ControllerHandle = Handle;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&BindingMarker->OriginalGop
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to get GOP protocol - code=%r\n", Status));
    goto FreeBindingMarker;
  }

  //
  // Install GopOverride protocol on this handle.
  //
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  mMsGopOverrideProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)BindingMarker->OriginalGop
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to install GopOverride protocol - code=%r\n", Status));
    goto FreeBindingMarker;
  }

  //
  // Install and open the private binding marker to establish driver-model
  // ownership before removing the consumed GOP from the controller.
  //
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  BindingMarker
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to install binding marker - code=%r\n", Status));
    goto UninstallOverride;
  }

  Status = gBS->OpenProtocol (
                  Handle,
                  &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                  (VOID **)&BindingMarker,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to open binding marker - code=%r\n", Status));
    goto UninstallMarker;
  }

  //
  // Uninstall the original GraphicsOutputProtocol on this handle.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID *)BindingMarker->OriginalGop,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to uninstall GOP protocol - code=%r\n", Status));
    goto CloseMarker;
  }

  //
  // Save state for Stop() to restore.
  //
  mBoundHandle = Handle;

  DEBUG ((DEBUG_INFO, "INFO [GOP]: GopOverride installed on handle %p\n", Handle));
  goto Exit;

CloseMarker:
  gBS->CloseProtocol (
         Handle,
         &gMsGopOverrideDriverBindingMarkerProtocolGuid,
         This->DriverBindingHandle,
         Handle
         );

UninstallMarker:
  gBS->UninstallProtocolInterface (
         Handle,
         &gMsGopOverrideDriverBindingMarkerProtocolGuid,
         BindingMarker
         );

UninstallOverride:
  gBS->UninstallProtocolInterface (
         Handle,
         mMsGopOverrideProtocolGuid,
         BindingMarker->OriginalGop
         );

FreeBindingMarker:
  FreePool (BindingMarker);

Exit:
  return Status;
}

/**
  GOP registration notification callback.

  Installs the GopOverride on the first GOP handle found, then closes the event.

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event context.
**/
VOID
EFIAPI
GopRegisteredCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       HandleCount;

  HandleCount = 0;

  //
  // Find handles with GraphicsOutputProtocol installed.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to locate GOP handle - code=%r - HandleCount=%d\n", Status, HandleCount));
    goto Exit;
  }

  // In case multiple GOPs are installed prior to dispatching the callback, just use the first handle. Generally HandleCount should be 1.
  Status = gBS->ConnectController (Handles[0], NULL, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "ERROR [GOP]: Unable to connect controller - code=%r\n", Status));
    goto Exit;
  }

  //
  // Close the registration event on success.
  //
  if (mGopRegisterEvent != NULL) {
    Status = gBS->CloseEvent (mGopRegisterEvent);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to close GOP event - code=%r\n", Status));
    }

    mGopRegisterEvent = NULL;
  }

Exit:
  if (Handles != NULL) {
    FreePool (Handles);
  }
}

/**
  Tests whether this driver supports a given controller.

  Returns EFI_ALREADY_STARTED if the GopOverride protocol is already installed
  on the handle. Returns EFI_UNSUPPORTED if the handle does not have GOP or
  already has GopOverride.

  @param[in] This                Protocol instance pointer.
  @param[in] ControllerHandle    Handle of device to test.
  @param[in] RemainingDevicePath Optional parameter use is not checked.

  @retval EFI_SUCCESS            This driver supports this device.
  @retval EFI_ALREADY_STARTED    GopOverride already installed on this handle.
  @retval EFI_UNSUPPORTED        This driver does not support this device.
**/
EFI_STATUS
EFIAPI
GopOverrideDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  //
  // Check if GraphicsOutputProtocol is present on this handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Interface,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Check if GopOverride is already installed on any handle.
  //
  Status = gBS->LocateProtocol (
                  mMsGopOverrideProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  return EFI_SUCCESS;
}

/**
  Starts the driver on the given controller handle.

  Installs the GopOverride protocol and uninstalls GraphicsOutputProtocol.

  @param[in] This                Protocol instance pointer.
  @param[in] ControllerHandle    Handle of device to bind.
  @param[in] RemainingDevicePath Optional parameter use is not checked.

  @retval EFI_SUCCESS            Driver started on this device.
  @retval other                  Driver failed to start on this device.
**/
EFI_STATUS
EFIAPI
GopOverrideDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "INFO [GOP]: DriverBindingStart on handle %p\n", ControllerHandle));

  Status = InstallGopOverride (This, ControllerHandle);

  return Status;
}

/**
  Stops the driver on the given controller handle.

  Uninstalls the GopOverride protocol and reinstalls the original
  GraphicsOutputProtocol on the same handle.

  @param[in] This                Protocol instance pointer.
  @param[in] ControllerHandle    Handle of device to stop.
  @param[in] NumberOfChildren    Number of child handle in ChildHandleBuffer.
  @param[in] ChildHandleBuffer   Array of child handles.

  @retval EFI_SUCCESS            Driver stopped successfully.
  @retval EFI_DEVICE_ERROR       Could not restore the original GOP.
**/
EFI_STATUS
EFIAPI
GopOverrideDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    RollbackStatus;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GopOverrideInterface;
  GOP_OVERRIDE_BINDING_MARKER   *BindingMarker;

  BindingMarker = NULL;

  DEBUG ((DEBUG_INFO, "INFO [GOP]: DriverBindingStop on handle %p\n", ControllerHandle));

  //
  // Get the GopOverride interface currently installed on this handle.
  //
  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  mMsGopOverrideProtocolGuid,
                  (VOID **)&GopOverrideInterface
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: GopOverride not found on handle - code=%r\n", Status));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                  (VOID **)&BindingMarker
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Binding marker not found on handle - code=%r\n", Status));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (BindingMarker->ControllerHandle != ControllerHandle) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Binding marker controller mismatch\n"));
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  //
  // Uninstall the GopOverride protocol. Must be uninstalled first to initiate a Stop() on the SimpleRenderingEngine driver.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  mMsGopOverrideProtocolGuid,
                  (VOID *)GopOverrideInterface,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to uninstall GopOverride - code=%r\n", Status));
    goto Exit;
  }

  //
  // Reinstall the original GraphicsOutputProtocol on the same handle.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)BindingMarker->OriginalGop
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to reinstall GOP - code=%r\n", Status));
    Status = EFI_DEVICE_ERROR;
    goto RestoreOverride;
  }

  //
  // Close and remove the binding marker after all produced protocols have
  // been removed and the original GOP has been restored.
  //
  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to close binding marker - code=%r\n", Status));
    Status = EFI_DEVICE_ERROR;
    goto RemoveRestoredGop;
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                  BindingMarker,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to uninstall binding marker - code=%r\n", Status));
    goto ReopenMarker;
  }

  if (ControllerHandle == mBoundHandle) {
    mBoundHandle = NULL;
  }

  FreePool (BindingMarker);

  DEBUG ((DEBUG_INFO, "INFO [GOP]: Original GOP restored on handle %p\n", ControllerHandle));
  goto Exit;

ReopenMarker:
  RollbackStatus = gBS->OpenProtocol (
                          ControllerHandle,
                          &gMsGopOverrideDriverBindingMarkerProtocolGuid,
                          (VOID **)&BindingMarker,
                          This->DriverBindingHandle,
                          ControllerHandle,
                          EFI_OPEN_PROTOCOL_BY_DRIVER
                          );
  if (EFI_ERROR (RollbackStatus)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to reopen binding marker during rollback - code=%r\n", RollbackStatus));
  }

RemoveRestoredGop:
  RollbackStatus = gBS->UninstallProtocolInterface (
                          ControllerHandle,
                          &gEfiGraphicsOutputProtocolGuid,
                          BindingMarker->OriginalGop
                          );
  if (EFI_ERROR (RollbackStatus)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to remove restored GOP during rollback - code=%r\n", RollbackStatus));
    goto Exit;
  }

RestoreOverride:
  RollbackStatus = gBS->InstallProtocolInterface (
                          &ControllerHandle,
                          mMsGopOverrideProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          BindingMarker->OriginalGop
                          );
  if (EFI_ERROR (RollbackStatus)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Unable to restore GopOverride during rollback - code=%r\n", RollbackStatus));
  }

Exit:
  return Status;
}

/**
  Main entry point for this driver.

  Installs the Driver Binding Protocol and sets up a protocol notification
  for GraphicsOutputProtocol to perform the initial override.

  @param[in] ImageHandle    Image handle this driver.
  @param[in] SystemTable    Pointer to SystemTable.

  @retval EFI_SUCCESS           This function always completes successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to initialize.
**/
EFI_STATUS
EFIAPI
DriverInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       HandleCount;

  HandleCount = 0;

  mMsGopOverrideProtocolGuid = PcdGetPtr (PcdMsGopOverrideProtocolGuid);

  //
  // Install the Driver Binding Protocol.
  //
  gGopOverrideDriverBinding.ImageHandle         = ImageHandle;
  gGopOverrideDriverBinding.DriverBindingHandle = ImageHandle;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gGopOverrideDriverBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Failed to install DriverBinding (%r)\n", Status));
    return Status;
  }

  //
  // Check if GOP is already available. If so, perform the override immediately.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &Handles
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "INFO [GOP]: GOP already present, overriding immediately\n"));
    GopRegisteredCallback (NULL, NULL);
    if (Handles != NULL) {
      FreePool (Handles);
    }
  } else {
    //
    // GOP not available yet. Register for protocol notification.
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_NOTIFY,
                    GopRegisteredCallback,
                    NULL,
                    &mGopRegisterEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Failed to create GOP registration event (%r)\n", Status));
      return Status;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEfiGraphicsOutputProtocolGuid,
                    mGopRegisterEvent,
                    &mGopRegistration
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR [GOP]: Failed to register for GOP notifications (%r)\n", Status));
      gBS->CloseEvent (mGopRegisterEvent);
      mGopRegisterEvent = NULL;
      return Status;
    }
  }

  DEBUG ((DEBUG_INFO, "INFO [GOP]: DriverInit exit - code=%r\n", Status));
  return EFI_SUCCESS;
}

/**
  Driver unload handler.

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS       This function always completes successfully.
**/
EFI_STATUS
EFIAPI
DriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                          Status;

  //
  // Close the registration event if still active.
  //
  if (mGopRegisterEvent != NULL) {
    gBS->CloseEvent (mGopRegisterEvent);
    mGopRegisterEvent = NULL;
  }

  //
  // If we have a bound handle, restore it via Stop.
  //
  if (mBoundHandle != NULL) {
    GopOverrideDriverBindingStop (&gGopOverrideDriverBinding, mBoundHandle, 0, NULL);
  }

  //
  // Uninstall Driver Binding Protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gGopOverrideDriverBinding,
                  NULL
                  );

  return Status;
}
