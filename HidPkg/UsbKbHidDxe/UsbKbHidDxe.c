/* @file UsbKbHid.c
 * USB HID Keyboard Driver that manages USB keyboard and produces HID Keyboard Protocol.
 *
 * USB Keyboard Driver consumes USB I/O Protocol and Device Path Protocol, and produces
 * HID Keyboard Protocol on USB keyboard devices.
 * This module refers to following specifications:
 * 1. Universal Serial Bus HID Firmware Specification, ver 1.11
 * 2. Universal Serial Bus HID Usage Tables, ver 1.12
 * 3. UEFI Specification, v2.1
 *
 * Copyright (C) Microsoft Corporation. All rights reserved.
 * Portions derived from MdeModulePkg\Bus\Usb\UsbKbDxe which is:
 * Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "UsbKbHidDxe.h"

//
// USB Keyboard Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gUsbKbHidDriverBinding = {
  UsbKbHidDriverBindingSupported,
  UsbKbHidDriverBindingStart,
  UsbKbHidDriverBindingStop,
  USB_HID_KEYBOARD_DRIVER_VERSION,
  NULL,
  NULL
};

/**
  Entrypoint of USB HID Keyboard Driver.

  This function is the entrypoint of the USB HID Keyboard Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UsbKbHidDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbKbHidDriverBinding,
             ImageHandle,
             &gUsbKbHidComponentName,
             &gUsbKbHidComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether USB HID keyboard driver supports this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
UsbKbHidDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;

  //
  // Check if USB I/O Protocol is attached on the controller handle.
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
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a keyboard device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUSBKeyboard (UsbIo)) {
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
  Starts the keyboard device with this driver.

  This function produces Simple Text Input Protocol and Simple Text Input Ex Protocol,
  initializes the keyboard device, and submit Asynchronous Interrupt Transfer to manage
  this keyboard device.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @retval EFI_UNSUPPORTED        No interrupt endpoint can be found.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
UsbKbHidDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                    Status;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  USB_KB_HID_DEV                *UsbKeyboardDevice;
  UINT8                         EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  UINT8                         Index;
  UINT8                         EndpointAddr;
  UINT8                         PollingInterval;
  UINT8                         PacketSize;
  BOOLEAN                       Found;
  EFI_TPL                       OldTpl;

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

  UsbKeyboardDevice = AllocateZeroPool (sizeof (USB_KB_HID_DEV));
  ASSERT (UsbKeyboardDevice != NULL);
  if (UsbKeyboardDevice == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }
  //
  // Get the Device Path Protocol on Controller's handle
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &UsbKeyboardDevice->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }
  //
  // Report that the USB keyboard is being enabled
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_ENABLE),
    UsbKeyboardDevice->DevicePath
    );

  //
  // This is pretty close to keyboard detection, so log progress
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_PRESENCE_DETECT),
    UsbKeyboardDevice->DevicePath
    );

  UsbKeyboardDevice->UsbIo = UsbIo;

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbKeyboardDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbKeyboardDevice->InterfaceDescriptor.NumEndpoints;

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
      CopyMem(&UsbKeyboardDevice->IntEndpointDescriptor, &EndpointDescriptor, sizeof(EndpointDescriptor));
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    //
    // Report Status Code to indicate that there is no USB keyboard
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_DETECTED)
      );
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    DEBUG ((DEBUG_ERROR, "[%a] - failed to locate keyboard interrupt endpoint: %r.\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DETECTED),
    UsbKeyboardDevice->DevicePath
    );

  UsbKeyboardDevice->Signature = USB_HID_KB_DEV_SIGNATURE;
  UsbKeyboardDevice->HidKeyboard.RegisterKeyboardHidReportCallback = RegisterKeyboardHidReportCallback;
  UsbKeyboardDevice->HidKeyboard.UnRegisterKeyboardHidReportCallback = UnRegisterKeyboardHidReportCallback;
  UsbKeyboardDevice->HidKeyboard.SetOutputReport = SetOutputReport;

  UsbKeyboardDevice->ControllerHandle = Controller;

  Status = InitUsbKeyboard(UsbKeyboardDevice);
  if (EFI_ERROR (Status)) {
    //
    // Report Status Code to indicate keyboard init failure
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_NOT_CONFIGURED)
      );
    //
    DEBUG ((DEBUG_ERROR, "[%a] - failed to initialize keyboard: %r.\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  //
  // Submit Asynchronous Interrupt Transfer to manage this device.
  //
  EndpointAddr    = UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress;
  PollingInterval = UsbKeyboardDevice->IntEndpointDescriptor.Interval;
  PacketSize      = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  Status = UsbIo->UsbAsyncInterruptTransfer (
                    UsbIo,
                    EndpointAddr,
                    TRUE,
                    PollingInterval,
                    PacketSize,
                    KeyboardHandler,
                    UsbKeyboardDevice
                    );
  if (EFI_ERROR (Status)) {
    //
    // Report Status Code to indicate keyboard init failure
    //
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_CONTROLLER_ERROR)
      );
    DEBUG ((DEBUG_ERROR, "[%a] - failed to initialize keyboard interrupt handler: %r.\n", __FUNCTION__, Status));
    goto ErrorExit;
  }

  //
  // Install HID USB keyboard device.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gHidKeyboardProtocolGuid,
                  &UsbKeyboardDevice->HidKeyboard,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  UsbKeyboardDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbKbHidComponentName.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic USB HID Keyboard",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUsbKbHidComponentName2.SupportedLanguages,
    &UsbKeyboardDevice->ControllerNameTable,
    L"Generic USB HID Keyboard",
    FALSE
    );

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

//
// Error handler
//
ErrorExit:
  if (UsbKeyboardDevice != NULL) {
    FreePool (UsbKeyboardDevice);
    UsbKeyboardDevice = NULL;
  }
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

ErrorExitNoUsbIo:
  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Stop the USB keyboard device handled by this driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Text In Protocol or Simple Text In Ex Protocol
                                 is not installed on Controller.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
UsbKbHidDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  HID_KEYBOARD_PROTOCOL *HidKeyboard;
  USB_KB_HID_DEV        *UsbKeyboardDevice;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gHidKeyboardProtocolGuid,
                  (VOID **) &HidKeyboard,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UsbKeyboardDevice = USB_KB_HID_DEV_FROM_THIS (HidKeyboard);

  //
  // The key data input from this device will be disabled.
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DISABLE),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Delete the Asynchronous Interrupt Transfer from this device
  //
  UsbKeyboardDevice->UsbIo->UsbAsyncInterruptTransfer (
                              UsbKeyboardDevice->UsbIo,
                              UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
                              FALSE,
                              UsbKeyboardDevice->IntEndpointDescriptor.Interval,
                              0,
                              NULL,
                              NULL
                              );
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gHidKeyboardProtocolGuid,
                  &UsbKeyboardDevice->HidKeyboard,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status); //Proceed on error in non-debug case.

  // Close the recovery event, if one exists.
  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    UsbKeyboardDevice->DelayedRecoveryEvent = NULL;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  ASSERT_EFI_ERROR (Status); //Proceed on error in non-debug case.

  //
  // Free all resources.
  //
  if (UsbKeyboardDevice->ControllerNameTable != NULL) {
    FreeUnicodeStringTable (UsbKeyboardDevice->ControllerNameTable);
  }

  FreePool (UsbKeyboardDevice);

  return Status;
}

//
// HID Keyboard Protocol functions
//
/**
  This function registers a callback function to occur whenever there is a HID Keyboard Report packet available.
  Note: Only one callback registration is permitted.

  @param  This                    - pointer to the current driver instance that this callback is being registered with.
  @param  KeyboardReportCallback  - Keyboard HID Report Callback function (see: KEYBOARD_HID_REPORT_CALLBACK)
  @param  Context                 - pointer to context (if any) that should be included when the callback is invoked.

  @retval EFI_SUCCESS         - Successfully registered the callback.
  @retval EFI_ALREADY_STARTED - Another callback is already registered.
  @retval other               - there was an implementation specific failure registering the callback.
**/
EFI_STATUS
EFIAPI
RegisterKeyboardHidReportCallback (
  IN HID_KEYBOARD_PROTOCOL         *This,
  IN KEYBOARD_HID_REPORT_CALLBACK  KeyboardReportCallback,
  IN VOID                          *Context
  )
{
  USB_KB_HID_DEV  *HidKeyboard;

  if ((This == NULL) || (KeyboardReportCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HidKeyboard = USB_KB_HID_DEV_FROM_THIS (This);

  if (HidKeyboard->KeyReportCallback != NULL) {
    return EFI_ALREADY_STARTED;
  }

  HidKeyboard->KeyReportCallback = KeyboardReportCallback;
  HidKeyboard->KeyReportCallbackContext = Context;

  return EFI_SUCCESS;
}

/**
  This function unregisters a previously registered keyboard HID report callback function.
  Note: Only one callback registration is permitted.

  @param  This  - pointer to the current driver instance that this callback is being unregistered from.

  @retval EFI_SUCCESS     - Successfully unregistered the callback.
  @retval EFI_NOT_FOUND   - No previously registered callback.
  @retval other           - there was an implementation specific failure unregistering the callback.
**/
EFI_STATUS
EFIAPI
UnRegisterKeyboardHidReportCallback (
  IN HID_KEYBOARD_PROTOCOL *This
  )
{
  USB_KB_HID_DEV  *HidKeyboard;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HidKeyboard = USB_KB_HID_DEV_FROM_THIS (This);

  if (HidKeyboard->KeyReportCallback == NULL) {
    return EFI_NOT_FOUND;
  }

  HidKeyboard->KeyReportCallback = NULL;
  HidKeyboard->KeyReportCallbackContext = NULL;

  return EFI_SUCCESS;
}

/**
  This function sends an Ouput Report HID packet to the hardware layer.

  @param  This                      - pointer to the current driver instance.
  @param  Interface                 - defines the format of the HID report.
  @param  HidOuputReportBuffer      - pointer to the HID ouput report buffer.
  @param  HidOuputReportBufferSize  - Size of the HidOutputReportBuffer

  @retval EFI_SUCCESS     - Output Report successfully transmitted to the hardware.
  @retval other           - there was an implementation specific failure transmitting the output report.
**/
EFI_STATUS
EFIAPI
SetOutputReport (
  IN HID_KEYBOARD_PROTOCOL  *This,
  IN KEYBOARD_HID_INTERFACE Interface,
  IN UINT8                  *HidOutputReportBuffer,
  IN UINTN                  HidOutputReportBufferSize
  )
{
  EFI_STATUS      Status;
  UINT8           ReportId;
  USB_KB_HID_DEV  *HidKeyboard;

  if (Interface != BootKeyboard) {
    DEBUG ((DEBUG_ERROR, "[%a] - Unsupported HID report interface %d\n", __FUNCTION__, Interface));
    return EFI_UNSUPPORTED;
  }

  if (HidOutputReportBufferSize != 1) {
    return EFI_UNSUPPORTED;
  }

  HidKeyboard = USB_KB_HID_DEV_FROM_THIS (This);

  ReportId = 1;

  //
  // Call Set_Report Request to lighten the LED.
  //
  Status = UsbSetReportRequest (
             HidKeyboard->UsbIo,
             HidKeyboard->InterfaceDescriptor.InterfaceNumber,
             ReportId,
             HID_OUTPUT_REPORT,
             1,
             HidOutputReportBuffer
             );

  return Status;
}

//
// Module-global utility functions
//
/**
  Uses USB I/O to check whether the device is a USB keyboard device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB keyboard device.
  @retval FALSE    Device is a not USB keyboard device.

**/
BOOLEAN
IsUSBKeyboard (
  IN  EFI_USB_IO_PROTOCOL       *UsbIo
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

  if (InterfaceDescriptor.InterfaceClass == CLASS_HID &&
      InterfaceDescriptor.InterfaceSubClass == SUBCLASS_BOOT &&
      InterfaceDescriptor.InterfaceProtocol == PROTOCOL_KEYBOARD
      ) {
    return TRUE;
  }

  return FALSE;
}

/**
  Initialize USB keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitUsbKeyboard (
  IN OUT USB_KB_HID_DEV   *UsbKeyboardDevice
  )
{
  UINT16              ConfigValue;
  UINT8               Protocol;
  EFI_STATUS          Status;
  UINT32              TransferResult;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_SELF_TEST),
    UsbKeyboardDevice->DevicePath
    );

  //
  // Use the config out of the descriptor
  // Assumed the first config is the correct one and this is not always the case
  //
  Status = UsbGetConfiguration (
             UsbKeyboardDevice->UsbIo,
             &ConfigValue,
             &TransferResult
             );
  if (EFI_ERROR (Status)) {
    ConfigValue = 0x01;
    //
    // Uses default configuration to configure the USB Keyboard device.
    //
    Status = UsbSetConfiguration (
               UsbKeyboardDevice->UsbIo,
               ConfigValue,
               &TransferResult
               );
    if (EFI_ERROR (Status)) {
      //
      // If configuration could not be set here, it means
      // the keyboard interface has some errors and could
      // not be initialized
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_ERROR_CODE | EFI_ERROR_MINOR,
        (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INTERFACE_ERROR),
        UsbKeyboardDevice->DevicePath
        );

      return EFI_DEVICE_ERROR;
    }
  }

  UsbGetProtocolRequest (
    UsbKeyboardDevice->UsbIo,
    UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
    &Protocol
    );
  //
  // Set boot protocol for the USB Keyboard.
  // This driver only supports boot protocol.
  //
  if (Protocol != BOOT_PROTOCOL) {
    UsbSetProtocolRequest (
      UsbKeyboardDevice->UsbIo,
      UsbKeyboardDevice->InterfaceDescriptor.InterfaceNumber,
      BOOT_PROTOCOL
      );
  }

  //
  // Create event for delayed recovery, which deals with device error.
  //
  if (UsbKeyboardDevice->DelayedRecoveryEvent != NULL) {
    gBS->CloseEvent (UsbKeyboardDevice->DelayedRecoveryEvent);
    UsbKeyboardDevice->DelayedRecoveryEvent = NULL;
  }

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_NOTIFY,
         UsbKbHidRecoveryHandler,
         UsbKeyboardDevice,
         &UsbKeyboardDevice->DelayedRecoveryEvent
         );

  return EFI_SUCCESS;
}


/**
  Handler function for USB keyboard's asynchronous interrupt transfer.

  This function is the handler function for USB keyboard's asynchronous interrupt transfer
  to manage the keyboard. It parses the USB keyboard input report, and inserts data to
  keyboard buffer according to state of modifier keys and normal keys. Timer for repeat key
  is also set accordingly.

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
KeyboardHandler (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  )
{
  USB_KB_HID_DEV      *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT32              UsbStatus;
  UINT8               EmptyKeyPacket[8];

  ASSERT (Context != NULL);

  UsbKeyboardDevice = (USB_KB_HID_DEV *) Context;
  UsbIo             = UsbKeyboardDevice->UsbIo;

  //
  // Analyzes Result and performs corresponding action.
  //
  if (Result != EFI_USB_NOERROR) {
    //
    // Some errors happen during the process
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_ERROR_CODE | EFI_ERROR_MINOR,
      (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR),
      UsbKeyboardDevice->DevicePath
      );

    //send a HID packet with no keys pressed so that
    //the HID layer will cancel repeat.
    ZeroMem(EmptyKeyPacket, sizeof (EmptyKeyPacket));
    if (UsbKeyboardDevice->KeyReportCallback != NULL) {
      UsbKeyboardDevice->KeyReportCallback (
        BootKeyboard,
        EmptyKeyPacket,
        sizeof (EmptyKeyPacket),
        UsbKeyboardDevice->KeyReportCallbackContext
      );
    }

    if ((Result & EFI_USB_ERR_STALL) == EFI_USB_ERR_STALL) {
      UsbClearEndpointHalt (
        UsbIo,
        UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
        &UsbStatus
        );
    }

    //
    // Delete & Submit this interrupt again
    // Handler of DelayedRecoveryEvent triggered by timer will re-submit the interrupt.
    //
    UsbIo->UsbAsyncInterruptTransfer (
             UsbIo,
             UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
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
           UsbKeyboardDevice->DelayedRecoveryEvent,
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
  // Send the data up to the HID layer via means of the KeyReportCallback.
  //
  if (UsbKeyboardDevice->KeyReportCallback != NULL) {
    UsbKeyboardDevice->KeyReportCallback (
        BootKeyboard,
        (UINT8 *)Data,
        DataLength,
        UsbKeyboardDevice->KeyReportCallbackContext
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

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_KB_HID_DEV instance.

**/
VOID
EFIAPI
UsbKbHidRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  )
{
  USB_KB_HID_DEV      *UsbKeyboardDevice;
  EFI_USB_IO_PROTOCOL *UsbIo;
  UINT8               PacketSize;

  UsbKeyboardDevice = (USB_KB_HID_DEV *) Context;

  ASSERT (Context != NULL);

  UsbIo             = UsbKeyboardDevice->UsbIo;

  PacketSize        = (UINT8) (UsbKeyboardDevice->IntEndpointDescriptor.MaxPacketSize);

  //
  // Re-submit Asynchronous Interrupt Transfer for recovery.
  //
  UsbIo->UsbAsyncInterruptTransfer (
           UsbIo,
           UsbKeyboardDevice->IntEndpointDescriptor.EndpointAddress,
           TRUE,
           UsbKeyboardDevice->IntEndpointDescriptor.Interval,
           PacketSize,
           KeyboardHandler,
           UsbKeyboardDevice
           );
}