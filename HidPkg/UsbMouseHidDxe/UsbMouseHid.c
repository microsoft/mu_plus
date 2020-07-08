/** @file UsbMouseHid.c
 * USB Mouse Driver that manages USB mouse and produces HID Mouse Protocol.
 *
 * USB Mouse Driver consumes USB I/O Protocol and Device Path Protocol, and produces
 * HID Mouse Protocol on USB mouse devices.
 * It manages the USB mouse device via Asynchronous Interrupt Transfer of USB I/O Protocol,
 * and parses the data according to USB HID Specification.
 * This module refers to following specifications:
 * 1. Universal Serial Bus HID Firmware Specification, ver 1.11
 * 2. UEFI Specification, v2.1
 *
 * Copyright (C) Microsoft Corporation. All rights reserved.
 * Derived from MdeModulePkg\Bus\Usb\UsbMouseAbsolutePointerDxe, which is:
 * Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 */
#include "UsbMouseHid.h"

EFI_DRIVER_BINDING_PROTOCOL gUsbMouseHidDriverBinding = {
  UsbMouseHidDriverBindingSupported,
  UsbMouseHidDriverBindingStart,
  UsbMouseHidDriverBindingStop,
  0x1,
  NULL,
  NULL
};

/**
  Entrypoint of USB Mouse HID Driver.

  This function is the entrypoint of USB Mouse Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UsbMouseHidDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbMouseHidDriverBinding,
             ImageHandle,
             &gUsbMouseHidComponentName,
             &gUsbMouseHidComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


/**
  Check whether USB Mouse Absolute Pointer Driver supports this device.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
UsbMouseHidDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a mouse device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;
  if (!IsUsbMouse (UsbIo)) {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiUsbIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}


/**
  Starts the mouse device with this driver.

  This function consumes USB I/O Protocol, initializes USB mouse device,
  installs Absolute Pointer Protocol, and submits Asynchronous Interrupt
  Transfer to manage the USB mouse device.

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
UsbMouseHidDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_USB_IO_PROTOCOL            *UsbIo;
  USB_MOUSE_HID_DEV              *UsbMouseHidDevice;
  UINT8                          EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR    EndpointDescriptor;
  UINT8                          Index;
  UINT8                          EndpointAddr;
  UINT8                          PollingInterval;
  UINT8                          PacketSize;
  BOOLEAN                        Found;
  EFI_TPL                        OldTpl;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  //
  // Open USB I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExitNoUsbIo;
  }

  UsbMouseHidDevice = AllocateZeroPool (sizeof (USB_MOUSE_HID_DEV));
  ASSERT (UsbMouseHidDevice != NULL);
  if (UsbMouseHidDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }


  UsbMouseHidDevice->UsbIo     = UsbIo;
  UsbMouseHidDevice->Signature = USB_MOUSE_HID_DEV_SIGNATURE;

  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbMouseHidDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Report Status Code here since USB mouse will be detected next.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_PRESENCE_DETECT),
    UsbMouseHidDevice->DevicePath
    );

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbMouseHidDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbMouseHidDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find interrupt endpoint IN
  //
  Found = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
             UsbIo,
             Index,
             &EndpointDescriptor
             );

    if (((EndpointDescriptor.Attributes & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT) &&
        ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN) != 0)) {
      //
      // We only care interrupt endpoint here
      //
      CopyMem (&UsbMouseHidDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    //
    // Report Status Code to indicate that there is no USB mouse
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_NOT_DETECTED)
      );
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }

  //
  // Report Status Code here since USB mouse has be detected.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DETECTED),
    UsbMouseHidDevice->DevicePath
    );

  Status = InitializeUsbMouseDevice (UsbMouseHidDevice);
  if (EFI_ERROR (Status)) {
    //
    // Fail to initialize USB mouse device.
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INTERFACE_ERROR),
      UsbMouseHidDevice->DevicePath
      );

    goto ErrorExit;
  }


  //
  // The next step would be submitting Asynchronous Interrupt Transfer on this mouse device.
  // After that we will be able to get key data from it. Thus this is deemed as
  // the enable action of the mouse, so report status code accordingly.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_ENABLE),
    UsbMouseHidDevice->DevicePath
    );

  //
  // Submit Asynchronous Interrupt Transfer to manage this device.
  //
  EndpointAddr    = UsbMouseHidDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbMouseHidDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8) (UsbMouseHidDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    OnMouseInterruptComplete,
                    UsbMouseHidDevice
                    );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Initialize and install HID Pointer Protocol.
  //
  UsbMouseHidDevice->HidPointerProtocol.RegisterPointerReportCallback = RegisterPointerReportCallback;
  UsbMouseHidDevice->HidPointerProtocol.UnRegisterPointerReportCallback = UnRegisterPointerReportCallback;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gHidPointerProtocolGuid,
                  &UsbMouseHidDevice->HidPointerProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbMouseHidDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbMouseHidComponentName.SupportedLanguages,
    &UsbMouseHidDevice->ControllerNameTable,
    L"Generic Usb Mouse Absolute Pointer",
      TRUE
      );
  AddUnicodeString2 (
    "en",
    gUsbMouseHidComponentName2.SupportedLanguages,
    &UsbMouseHidDevice->ControllerNameTable,
    L"Generic Usb Mouse Absolute Pointer",
    FALSE
    );

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

//
// Error handler
//
ErrorExit:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          Controller,
          &gEfiUsbIoProtocolGuid,
          This->DriverBindingHandle,
          Controller
          );

    if (UsbMouseHidDevice != NULL) {
      FreePool (UsbMouseHidDevice);
      UsbMouseHidDevice = NULL;
    }
  }

ErrorExitNoUsbIo:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Stop the USB mouse device handled by this driver.

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
UsbMouseHidDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  USB_MOUSE_HID_DEV     *UsbMouseHidDevice;
  HID_POINTER_PROTOCOL  *HidPointerProtocol;
  EFI_USB_IO_PROTOCOL   *UsbIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gHidPointerProtocolGuid,
                  (VOID **) &HidPointerProtocol,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbMouseHidDevice = USB_MOUSE_HID_DEV_FROM_HID_POINTER_PROTOCOL (HidPointerProtocol);

  UsbIo = UsbMouseHidDevice->UsbIo;

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_MOUSE | EFI_P_PC_DISABLE),
    UsbMouseHidDevice->DevicePath
    );

  //
  // Delete the Asynchronous Interrupt Transfer from this device
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseHidDevice->IntEndpointDescriptor.EndpointAddress,
           FALSE,
           UsbMouseHidDevice->IntEndpointDescriptor.Interval,
           0,
           NULL,
           NULL
           );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gHidPointerProtocolGuid,
                  &UsbMouseHidDevice->HidPointerProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status); //Proceed on error in non-debug case.

  //Close the recovery event if one exists
  if (UsbMouseHidDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseHidDevice->DelayedRecoveryEvent);
    UsbMouseHidDevice->DelayedRecoveryEvent = NULL;
  }

  Status = gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  ASSERT_EFI_ERROR (Status); //Proceed on error in non-debug case.

  //
  // Free all resources.
  //

  if (UsbMouseHidDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbMouseHidDevice->ControllerNameTable);
  }

  FreePool (UsbMouseHidDevice);

  return Status;
}

/**
  This function registers a callback function to occur whenever there is a HID Mouse Report
  packet available.

  @param  This                      points to this driver's protocol
  @param  PointerReportCallback     Pointer Report Callback function
  @param  Context                   points to the mouse device context to be used when making the callback

  @retval EFI_SUCCESS         - Successfully registered the callback.
  @retval EFI_ALREADY_STARTED - Callback is already registered.
  @retval other               - there was an implementation specific failure registering the callback.

**/
EFI_STATUS
EFIAPI
RegisterPointerReportCallback (
  HID_POINTER_PROTOCOL        *This,
  POINTER_HID_REPORT_CALLBACK PointerReportCallback,
  VOID                        *Context
  )
{
  USB_MOUSE_HID_DEV  *UsbMouseHidDevice;

  if ((This == NULL) || (PointerReportCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  UsbMouseHidDevice = USB_MOUSE_HID_DEV_FROM_HID_POINTER_PROTOCOL (This);

  if (UsbMouseHidDevice->MouseReportCallback != NULL) {
    return EFI_ALREADY_STARTED;
  }

  UsbMouseHidDevice->MouseReportCallback = PointerReportCallback;
  UsbMouseHidDevice->MouseReportCallbackContext = Context;

  return EFI_SUCCESS;
}

/**
  This function unregisters a pointer report callback function that has been previously registered.

  @param  This            points to this driver's protocol

  @retval EFI_SUCCESS   - Successfully unregistered the callback.
  @retval EFI_NOT_FOUND - No callback installed.
  @retval Other         - there was an implementation specific failure unregistering the callback.
**/
EFI_STATUS
EFIAPI
UnRegisterPointerReportCallback (
  HID_POINTER_PROTOCOL *This
  )
{
  USB_MOUSE_HID_DEV  *UsbMouseHidDevice;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UsbMouseHidDevice = USB_MOUSE_HID_DEV_FROM_HID_POINTER_PROTOCOL (This);

  if (UsbMouseHidDevice->MouseReportCallback == NULL) {
    return EFI_NOT_FOUND;
  }

  UsbMouseHidDevice->MouseReportCallback = NULL;
  UsbMouseHidDevice->MouseReportCallbackContext = NULL;

  return EFI_SUCCESS;
}

/**
  Uses USB I/O to check whether the device is a USB mouse device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB mouse device.
  @retval FALSE    Device is a not USB mouse device.

**/
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;

  //
  // Get the default interface descriptor
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (
                    UsbIo,
                    &InterfaceDescriptor
                    );

  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  if ((InterfaceDescriptor.InterfaceClass == CLASS_HID) &&
      (InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT) &&
      (InterfaceDescriptor.InterfaceProtocol == PROTOCOL_MOUSE)
      ) {
    return TRUE;
  }

  return FALSE;
}


/**
  Initialize the USB mouse device.

  @param  UsbMouseHidDev   Device instance to be initialized.

  @retval EFI_SUCCESS                  USB mouse device successfully initialized.
  @retval EFI_UNSUPPORTED              HID descriptor type is not report descriptor.
  @retval Other                        USB mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_HID_DEV   *UsbMouseHidDev
  )
{
  EFI_USB_IO_PROTOCOL       *UsbIo;
  UINT8                     Protocol;
  EFI_STATUS                Status;

  UsbIo = UsbMouseHidDev->UsbIo;

  //
  // Set boot protocol for the USB mouse.
  // This driver only supports boot protocol.
  //
  Status = UsbGetProtocolRequest (
    UsbIo,
    UsbMouseHidDev->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (Protocol != BOOT_PROTOCOL) {
    Status = UsbSetProtocolRequest (
               UsbIo,
               UsbMouseHidDev->InterfaceDescriptor.InterfaceNumber,
               BOOT_PROTOCOL
               );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbMouseHidDev->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbMouseHidDev->DelayedRecoveryEvent);
    UsbMouseHidDev->DelayedRecoveryEvent = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         UsbMouseHidRecoveryHandler,
         UsbMouseHidDev,
         &UsbMouseHidDev->DelayedRecoveryEvent
         );

  return EFI_SUCCESS;
}

/**
  Handler function for USB mouse's asynchronous interrupt transfer.

  This function is the handler function for USB mouse's asynchronous interrupt transfer
  to manage the mouse. It parses data returned from asynchronous interrupt transfer, and
  get button and movement state.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  )
{
  USB_MOUSE_HID_DEV     *UsbMouseHidDevice;
  EFI_USB_IO_PROTOCOL   *UsbIo;
  UINT8                 EndpointAddr;
  UINT32                UsbResult;

  UsbMouseHidDevice  = (USB_MOUSE_HID_DEV *) Context;
  UsbIo              = UsbMouseHidDevice->UsbIo;

  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_MOUSE | EFI_P_EC_INPUT_ERROR),
      UsbMouseHidDevice->DevicePath
      );

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      EndpointAddr = UsbMouseHidDevice->IntEndpointDescriptor.EndpointAddress;

      UsbClearEndpointHalt (
        UsbIo,
        EndpointAddr,
        &UsbResult
        );
    }

    //
    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt.
    //
    UsbIo->UsbAsyncInterruptTransfer (
             UsbIo,
             UsbMouseHidDevice->IntEndpointDescriptor.EndpointAddress,
             FALSE,
             0,
             0,
             NULL,
             NULL
             );
    //
    // EFI_USB_INTERRUPT_DELAY is defined in USB standard for error handling.
    //
    gBS->SetTimer (
           UsbMouseHidDevice->DelayedRecoveryEvent,
           TimerRelative,
           EFI_USB_INTERRUPT_DELAY
           );
    return EFI_DEVICE_ERROR;
  }

  //
  // If no error and no data, just return EFI_SUCCESS.
  //
  if (DataLength == 0 || Data == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Send report to the HID layer.
  //
  if (UsbMouseHidDevice->MouseReportCallback != NULL) {
    UsbMouseHidDevice->MouseReportCallback (
                         BootMouse,
                         (UINT8 *)Data,
                         DataLength,
                         UsbMouseHidDevice->MouseReportCallbackContext
                         );
  }
  return EFI_SUCCESS;
}

/**
  Handler for Delayed Recovery event.

  This function is the handler for Delayed Recovery event triggered
  by timer.
  After a device error occurs, the event would be triggered
  with interval of EFI_USB_INTERRUPT_DELAY. EFI_USB_INTERRUPT_DELAY
  is defined in USB standard for error handling.

  @param  Event                 The Delayed Recovery event.
  @param  Context               Points to the USB_MOUSE_ABSOLUTE_POINTER_DEV instance.

**/
VOID
EFIAPI
UsbMouseHidRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_MOUSE_HID_DEV       *UsbMouseHidDev;
  EFI_USB_IO_PROTOCOL                  *UsbIo;

  UsbMouseHidDev = (USB_MOUSE_HID_DEV *) Context;

  UsbIo       = UsbMouseHidDev->UsbIo;

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbMouseHidDev->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbMouseHidDev->IntEndpointDescriptor.Interval,
           UsbMouseHidDev->IntEndpointDescriptor.MaxPacketSize,
           OnMouseInterruptComplete,
           UsbMouseHidDev
           );
}
