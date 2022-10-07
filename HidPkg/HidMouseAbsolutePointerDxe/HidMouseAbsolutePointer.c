/* @file HidMouseAbsolutePointer.c

 HID Mouse Driver that manages HID Mouse device and produces Absolute Pointer Protocol.

 This Mouse Driver consumes the HID Mouse Protocol and Device Path Protocol, and produces
 Absolute Pointer Protocol on HID Mouse mouse devices.

 It manages the HID mouse device via the HID mouse protocol abstraction,
 and parses the data according to USB HID Specification.
 This module refers to following specifications:
 1. Universal Serial Bus HID Firmware Specification, ver 1.11
 2. UEFI Specification, v2.1

 Copyright (c) Microsoft Corporation.

 Most of this driver derived from USB Mouse driver in UDK, which is:
Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

*/

#include "HidMouseAbsolutePointer.h"

EFI_DRIVER_BINDING_PROTOCOL  mHidMouseAbsolutePointerDriverBinding = {
  HidMouseAbsolutePointerDriverBindingSupported,
  HidMouseAbsolutePointerDriverBindingStart,
  HidMouseAbsolutePointerDriverBindingStop,
  0x1,
  NULL,
  NULL
};

/**
  Entrypoint of HID Mouse Absolute Pointer Driver.

  This function is the entrypoint of HID Mouse Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerDriverBindingEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &mHidMouseAbsolutePointerDriverBinding,
             ImageHandle,
             &mHidMouseAbsolutePointerComponentName,
             &mHidMouseAbsolutePointerComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether HID Mouse Absolute Pointer Driver supports this device.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  HID_POINTER_PROTOCOL  *HidMouseProtocol;

  // Check to see if controller has unbound MS Mouse Protocol installed.
  Status = gBS->OpenProtocol (
                  Controller,
                  &gHidPointerProtocolGuid,
                  (VOID **)(HID_POINTER_PROTOCOL **)&HidMouseProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gHidPointerProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

/**
  Starts the mouse device with this driver.

  This function consumes Mouse Protocol, initializes HID mouse device,
  and installs Absolute Pointer Protocol.

  @param  This                  The driver binding instance.
  @param  Controller            Handle of device to bind driver to.
  @param  RemainingDevicePath   Optional parameter use to pick a specific child
                                device to start.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
  @retval EFI_OUT_OF_RESOURCES  Can't allocate memory resources.
  @retval EFI_ALREADY_STARTED   This driver has been started.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                      Status;
  HID_POINTER_PROTOCOL            *HidMouseProtocol;
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;

  HidMouseDev = NULL;

  DEBUG ((DEBUG_VERBOSE, "[%a]\n", __FUNCTION__));

  // Get our HID mouse abstraction
  Status = gBS->OpenProtocol (
                  Controller,
                  &gHidPointerProtocolGuid,
                  (VOID **)(HID_POINTER_PROTOCOL **)&HidMouseProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  HidMouseDev = AllocateZeroPool (sizeof (HID_MOUSE_ABSOLUTE_POINTER_DEV));
  if (HidMouseDev == NULL) {
    ASSERT (HidMouseDev != NULL);
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  HidMouseDev->Signature        = HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE;
  HidMouseDev->HidMouseProtocol = HidMouseProtocol;

  // Initialize the mouse device.
  Status = InitializeMouseDevice (HidMouseDev);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // Initialize and install EFI Absolute Pointer Protocol.
  HidMouseDev->AbsolutePointerProtocol.GetState = GetMouseAbsolutePointerState;
  HidMouseDev->AbsolutePointerProtocol.Reset    = HidMouseAbsolutePointerReset;
  HidMouseDev->AbsolutePointerProtocol.Mode     = &HidMouseDev->Mode;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_WAIT,
                  TPL_NOTIFY,
                  HidMouseAbsolutePointerWaitForInput,
                  HidMouseDev,
                  &((HidMouseDev->AbsolutePointerProtocol).WaitForInput)
                  );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HidMouseDev->AbsolutePointerProtocol
                  );

  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // Register for Async Mouse HID reports from HID layer.
  Status = HidMouseDev->HidMouseProtocol->RegisterPointerReportCallback (
                                            HidMouseDev->HidMouseProtocol,
                                            OnMouseReport,
                                            HidMouseDev
                                            );
  if (EFI_ERROR (Status)) {
    // If failure on start, uninstall the protocol interface before exiting.
    gBS->UninstallProtocolInterface (
           Controller,
           &gEfiAbsolutePointerProtocolGuid,
           &HidMouseDev->AbsolutePointerProtocol
           );
    goto Cleanup;
  }

  // Set up Controller name support.
  HidMouseDev->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    mHidMouseAbsolutePointerComponentName.SupportedLanguages,
    &HidMouseDev->ControllerNameTable,
    L"HID Mouse Absolute Pointer",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    mHidMouseAbsolutePointerComponentName2.SupportedLanguages,
    &HidMouseDev->ControllerNameTable,
    L"HID Mouse Absolute Pointer",
    FALSE
    );

Cleanup:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "[%a] - Error Status = %r\n", __FUNCTION__, Status));
    gBS->CloseProtocol (
           Controller,
           &gHidPointerProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (HidMouseDev != NULL) {
      if ((HidMouseDev->AbsolutePointerProtocol).WaitForInput != NULL) {
        gBS->CloseEvent ((HidMouseDev->AbsolutePointerProtocol).WaitForInput);
      }

      FreePool (HidMouseDev);
    }
  }

  return Status;
}

/**
  Stop the HID mouse device handled by this driver.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Absolute Pointer Protocol is not installed on Controller.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;
  EFI_ABSOLUTE_POINTER_PROTOCOL   *AbsolutePointerProtocol;

  // Get the Absolute Pointer instance from this controller and use it to retrieve context.
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  (VOID **)&AbsolutePointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_UNSUPPORTED;
  }

  HidMouseDev = HID_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (AbsolutePointerProtocol);

  // Unregister Mouse HID report callback.
  Status = HidMouseDev->HidMouseProtocol->UnRegisterPointerReportCallback (HidMouseDev->HidMouseProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Error stopping mouse device. Status=%r\n", __FUNCTION__, Status));
    // Continue tear-down on error.
  }

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiAbsolutePointerProtocolGuid,
                  &HidMouseDev->AbsolutePointerProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    // This is unexpected and should never happen, but we can't really proceed with teardown if we can't uninstall the protocol.
    // We don't want to free resources the protocol points to or might be using.
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gHidPointerProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Free all resources.
  //
  gBS->CloseEvent (HidMouseDev->AbsolutePointerProtocol.WaitForInput);

  if (HidMouseDev->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (HidMouseDev->ControllerNameTable);
  }

  FreePool (HidMouseDev);

  return EFI_SUCCESS;
}

/**
  Initialize the HID mouse device.

  This function retrieves and parses HID report descriptor, and
  initializes state of HID_MOUSE_ABSOLUTE_POINTER_DEV.

  @param  HidMouseDev       Device instance to be initialized.

  @retval EFI_SUCCESS       HID mouse device successfully initialized.
  @retval EFI_UNSUPPORTED   HID descriptor type is not report descriptor.
  @retval Other             HID mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeMouseDevice (
  IN  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev
  )
{
  // Future improvement - we could actually read and parse the descriptor and
  // set up the Absolute Pointer mode attributes accordingly. We could also change the
  // protocol to something other than boot protocol for mouse.
  // For now, we only support boot protocol for mouse, so we just hard
  // code the attributes.

  HidMouseDev->Mode.AbsoluteMaxX = 1024;
  HidMouseDev->Mode.AbsoluteMaxY = 1024;
  HidMouseDev->Mode.AbsoluteMaxZ = 0;
  HidMouseDev->Mode.AbsoluteMinX = 0;
  HidMouseDev->Mode.AbsoluteMinY = 0;
  HidMouseDev->Mode.AbsoluteMinZ = 0;
  HidMouseDev->Mode.Attributes   = 0x3;

  //
  // Set the cursor's starting position to the center of the screen.
  //
  HidMouseDev->State.CurrentX =
    DivU64x32 (HidMouseDev->Mode.AbsoluteMaxX + HidMouseDev->Mode.AbsoluteMinX, 2);
  HidMouseDev->State.CurrentY =
    DivU64x32 (HidMouseDev->Mode.AbsoluteMaxY + HidMouseDev->Mode.AbsoluteMinY, 2);

  return EFI_SUCCESS;
}

/**
  Retrieves the current state of a pointer device.

  @param  This                  A pointer to the EFI_ABSOLUTE_POINTER_PROTOCOL instance.
  @param  MouseState            A pointer to the state information on the pointer device.

  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.
  @retval EFI_INVALID_PARAMETER State is NULL.

**/
EFI_STATUS
EFIAPI
GetMouseAbsolutePointerState (
  IN   EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT  EFI_ABSOLUTE_POINTER_STATE     *State
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HidMouseDev = HID_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  if (!HidMouseDev->StateChanged) {
    return EFI_NOT_READY;
  }

  //
  // Retrieve mouse state from HID_MOUSE_ABSOLUTE_POINTER_DEV,
  // which was filled by OnMouseReport()
  //
  CopyMem (
    State,
    &HidMouseDev->State,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );

  HidMouseDev->StateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Resets the pointer device hardware.

  @param  This                  A pointer to the EFI_ABSOLUTE_POINTER_PROTOCOL instance.
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
HidMouseAbsolutePointerReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                        ExtendedVerification
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;

  HidMouseDev = HID_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL (This);

  //
  // Clear mouse state.
  //
  ZeroMem (
    &HidMouseDev->State,
    sizeof (EFI_ABSOLUTE_POINTER_STATE)
    );

  //
  // Set the cursor's starting position to the center of the screen.
  //
  HidMouseDev->State.CurrentX =
    DivU64x32 (HidMouseDev->Mode.AbsoluteMaxX + HidMouseDev->Mode.AbsoluteMinX, 2);
  HidMouseDev->State.CurrentY =
    DivU64x32 (HidMouseDev->Mode.AbsoluteMaxY + HidMouseDev->Mode.AbsoluteMinY, 2);

  HidMouseDev->StateChanged = FALSE;

  return EFI_SUCCESS;
}

/**
  Event notification function for EFI_ABSOLUTE_POINTER_PROTOCOL.WaitForInput event.

  @param  Event        Event to be signaled when there's input from mouse.
  @param  Context      Points to HID_MOUSE_ABSOLUTE_POINTER_DEV instance.

**/
VOID
EFIAPI
HidMouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;

  HidMouseDev = (HID_MOUSE_ABSOLUTE_POINTER_DEV *)Context;

  //
  // If there's input from mouse, signal the event.
  //
  if (HidMouseDev->StateChanged) {
    gBS->SignalEvent (Event);
  }
}

/**
  Handler function for HID mouse's asynchronous HID report.

  This function is the handler function for HID mouse's asynchronous HID report.
  It parses data returned from the report to get button and movement state.

  @param  Interface                - defines the format of the HID report.
  @param  HidInputReportBuffer     - points to the pointer HID report buffer.
  @param  HidInputReportBufferSize - indicates the size of the pointer HID report buffer.
  @param  Context                  - pointer to context (if any) provided when registering the callback.

  @retval None       - Error occurred parsing/handling the HID report.

**/
VOID
EFIAPI
OnMouseReport (
  IN HID_POINTER_INTERFACE  Interface,
  IN UINT8                  *HidInputReportBuffer,
  IN UINTN                  HidInputReportBufferSize,
  IN VOID                   *Context
  )
{
  HID_MOUSE_ABSOLUTE_POINTER_DEV  *HidMouseDev;
  SINGLETOUCH_HID_INPUT_BUFFER    *SingleTouchInput;
  MOUSE_HID_INPUT_BUFFER          *MouseInput;

  HidMouseDev = (HID_MOUSE_ABSOLUTE_POINTER_DEV *)Context;

  if (HidMouseDev == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Invalid Context pointer: Null.\n", __FUNCTION__));
    ASSERT (HidMouseDev != NULL);
    return;
  }

  // Since this is called by external module the function should do basic
  // check on Context parameter.
  if (HidMouseDev->Signature != HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "[%a] - Invalid context pointer: Signature match failure.\n", __FUNCTION__));
    ASSERT (HidMouseDev->Signature == HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE);
    return;
  }

  if (HidInputReportBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a] - Invalid input HidInputReportBuffer pointer.\n", __FUNCTION__));
    ASSERT (HidInputReportBuffer != NULL);
    return;
  }

  switch (Interface) {
    case SingleTouch:
      //
      // Byte    Bits    Description
      // 0       0 to 7  HID Report Id (Vendor defined)
      // 1       0       Touch
      //         1 to 7  Ignored
      // 2       0 to 7  X coordinate LSB
      // 3       0 to 7  X coordinate MSB
      // 4       0 to 7  Y coordinate LSB
      // 5       0 to 7  Y coordinate MSB
      //
      if (HidInputReportBufferSize != sizeof (SINGLETOUCH_HID_INPUT_BUFFER)) {
        DEBUG ((DEBUG_ERROR, "[%a] - invalid SingleTouch HID report size\n", __FUNCTION__));
        ASSERT (HidInputReportBufferSize == sizeof (SINGLETOUCH_HID_INPUT_BUFFER));
        return;
      }

      SingleTouchInput = (SINGLETOUCH_HID_INPUT_BUFFER *)HidInputReportBuffer;

      // Check values against known good range before copy.
      if ((SingleTouchInput->CurrentX < HidMouseDev->Mode.AbsoluteMinX) ||
          (SingleTouchInput->CurrentX > HidMouseDev->Mode.AbsoluteMaxX) ||
          (SingleTouchInput->CurrentY < HidMouseDev->Mode.AbsoluteMinY) ||
          (SingleTouchInput->CurrentY > HidMouseDev->Mode.AbsoluteMaxY))
      {
        DEBUG ((DEBUG_ERROR, "[%a] - invalid SingleTouch Coordinates [%d, %d]\n", __FUNCTION__, SingleTouchInput->CurrentX, SingleTouchInput->CurrentY));
        return;
      }

      HidMouseDev->State.ActiveButtons = SingleTouchInput->Touch;
      HidMouseDev->State.CurrentX      = SingleTouchInput->CurrentX;
      HidMouseDev->State.CurrentY      = SingleTouchInput->CurrentY;
      break;
    case BootMouse:
      //
      // Check mouse Data
      // USB HID Specification specifies following data format:
      // Byte    Bits    Description
      // 0       0 to 7  HID Report Id (Ignored, gMsMouseProtocol will only forward us Mouse HID packets).
      // 1       0       Button 1
      //         1       Button 2
      //         2       Button 3
      //         4 to 7  Device-specific
      // 2       0 to 7  X displacement
      // 3       0 to 7  Y displacement
      // 4 to n  0 to 7  Device specific (optional)
      //
      // Check the size. Note that Z displacement is optional, so don't include it in the check.
      if (HidInputReportBufferSize < (sizeof (MOUSE_HID_INPUT_BUFFER) - sizeof (INT8))) {
        DEBUG ((DEBUG_ERROR, "[%a] - invalid mouse report size\n", __FUNCTION__));
        ASSERT (HidInputReportBufferSize >= (sizeof (MOUSE_HID_INPUT_BUFFER) - sizeof (INT8)));
        return;
      }

      MouseInput = (MOUSE_HID_INPUT_BUFFER *)HidInputReportBuffer;
      // copy first byte with button state straight from report buffer - it's already formatted correctly.
      HidMouseDev->State.ActiveButtons = HidInputReportBuffer[0];

      HidMouseDev->State.CurrentX =
        MIN (
          MAX (
            (INT64)HidMouseDev->State.CurrentX + MouseInput->XDisplacement,
            (INT64)HidMouseDev->Mode.AbsoluteMinX
            ),
          (INT64)HidMouseDev->Mode.AbsoluteMaxX
          );
      HidMouseDev->State.CurrentY =
        MIN (
          MAX (
            (INT64)HidMouseDev->State.CurrentY + MouseInput->YDisplacement,
            (INT64)HidMouseDev->Mode.AbsoluteMinY
            ),
          (INT64)HidMouseDev->Mode.AbsoluteMaxY
          );
      // only use Z if optional byte is included (as indicated by the report size)
      if (HidInputReportBufferSize >= sizeof (MOUSE_HID_INPUT_BUFFER)) {
        HidMouseDev->State.CurrentZ =
          MIN (
            MAX (
              (INT64)HidMouseDev->State.CurrentZ + MouseInput->ZDisplacement,
              (INT64)HidMouseDev->Mode.AbsoluteMinZ
              ),
            (INT64)HidMouseDev->Mode.AbsoluteMaxZ
            );
      }

      break;
    default:
      DEBUG ((DEBUG_ERROR, "[%a] - unrecognized HID report type.\n", __FUNCTION__));
      ASSERT (FALSE);
      return;
  }

  HidMouseDev->StateChanged = TRUE;

  return;
}
