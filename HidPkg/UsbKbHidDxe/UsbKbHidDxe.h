/** @file UsbKbHidDxe
  Header file for USB HID Keyboard Driver's Data Structures.

Copyright (C) Microsoft Corporation. All rights reserved.
Portions derived from MdeModulePkg\Bus\Usb\UsbKbDxe which is:
Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef _EFI_USB_HID_KB_H_
#define _EFI_USB_HID_KB_H_

#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/HidKeyboardProtocol.h>
#include <Protocol/UsbIo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiUsbLib.h>

#include <IndustryStandard/Usb.h>

#define USB_HID_KEYBOARD_DRIVER_VERSION 0x10

#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

#define BOOT_PROTOCOL       0
#define REPORT_PROTOCOL     1

#define USB_HID_KB_DEV_SIGNATURE  SIGNATURE_32 ('u', 'k', 'h', 'd')

///
/// Structure to describe USB keyboard device
///
typedef struct {
  UINTN                             Signature;
  EFI_HANDLE                        ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_UNICODE_STRING_TABLE          *ControllerNameTable;
  EFI_EVENT                         DelayedRecoveryEvent;
  EFI_USB_IO_PROTOCOL               *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR      InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR       IntEndpointDescriptor;
  HID_KEYBOARD_PROTOCOL             HidKeyboard;
  KEYBOARD_HID_REPORT_CALLBACK      KeyReportCallback;
  VOID                              *KeyReportCallbackContext;
} USB_KB_HID_DEV;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbKbHidDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbKbHidComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbKbHidComponentName2;

#define USB_KB_HID_DEV_FROM_THIS(a) \
    CR(a, USB_KB_HID_DEV, HidKeyboard, USB_HID_KB_DEV_SIGNATURE)

//
// Functions of Driver Binding Protocol
//
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
  );

/**
  Starts the HID keyboard device with this driver.

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
  );

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
  );

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbKbHidComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.
  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbKbHidComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

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
  );

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
  );

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
  );

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
  );

/**
  Initialize USB keyboard device and all private data structures.

  @param  UsbKeyboardDevice  The USB_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitUsbKeyboard (
  IN OUT USB_KB_HID_DEV   *UsbKeyboardDevice
  );

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
  );

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
  );

#endif

