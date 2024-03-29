/** @file UsbMouseHid.h
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

#ifndef _USB_HID_MOUSE_H_
#define _USB_HID_MOUSE_H_

#include <Uefi.h>

#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HidPointerProtocol.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiUsbLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Usb.h>

#define CLASS_HID       3
#define SUBCLASS_BOOT   1
#define PROTOCOL_MOUSE  2

#define BOOT_PROTOCOL    0
#define REPORT_PROTOCOL  1

#define USB_MOUSE_HID_DEV_SIGNATURE  SIGNATURE_32 ('u', 'm', 'h', 'd')
//
// A common header for usb standard descriptor.
// Each standard descriptor has a length and type.
//
#pragma pack(1)
typedef struct {
  UINT8    Len;
  UINT8    Type;
} USB_DESC_HEAD;
#pragma pack()

///
/// Device instance of USB mouse.
///
typedef struct {
  UINTN                           Signature;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  EFI_EVENT                       DelayedRecoveryEvent;
  EFI_USB_IO_PROTOCOL             *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     IntEndpointDescriptor;
  EFI_UNICODE_STRING_TABLE        *ControllerNameTable;
  HID_POINTER_PROTOCOL            HidPointerProtocol;
  POINTER_HID_REPORT_CALLBACK     MouseReportCallback;
  VOID                            *MouseReportCallbackContext;
} USB_MOUSE_HID_DEV;

#define USB_MOUSE_HID_DEV_FROM_HID_POINTER_PROTOCOL(a) \
    CR(a, USB_MOUSE_HID_DEV, HidPointerProtocol, USB_MOUSE_HID_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbMouseHidDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbMouseHidComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbMouseHidComponentName2;

//
// Functions of Driver Binding Protocol
//

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
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

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
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

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
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
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
UsbMouseHidComponentNameGetDriverName (
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
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
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
UsbMouseHidComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

//
// HID Pointer Protocol functions
//

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
  HID_POINTER_PROTOCOL         *This,
  POINTER_HID_REPORT_CALLBACK  PointerReportCallback,
  VOID                         *Context
  );

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
  HID_POINTER_PROTOCOL  *This
  );

//
// Internal worker functions
//

/**
  Uses USB I/O to check whether the device is a USB mouse device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB mouse device.
  @retval FALSE    Device is a not USB mouse device.

**/
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  );

/**
  Initialize the USB mouse device.

  @param  UsbMouseAbsolutePointerDev   Device instance to be initialized.

  @retval EFI_SUCCESS                  USB mouse device successfully initialized.
  @retval EFI_UNSUPPORTED              HID descriptor type is not report descriptor.
  @retval Other                        USB mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeUsbMouseDevice (
  IN  USB_MOUSE_HID_DEV  *UsbMouseAbsolutePointerDev
  );

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
  IN  VOID    *Data,
  IN  UINTN   DataLength,
  IN  VOID    *Context,
  IN  UINT32  Result
  );

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
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  );

#endif
