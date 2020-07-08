/* @file HidMouseAbsolutePointer.h

 HID Mouse Driver that manages HID Mouse device and produces Absolute Pointer Protocol.

 This Mouse Driver consumes the HID Mouse Protocol and produces
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

#ifndef _HID_MOUSE_ABSOLUTE_POINTER_H_
#define _HID_MOUSE_ABSOLUTE_POINTER_H_


#include <Uefi.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HidPointerProtocol.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

//
// Private structs
//
typedef struct {
  UINTN                           Signature;
  HID_POINTER_PROTOCOL            *HidMouseProtocol;
  EFI_ABSOLUTE_POINTER_PROTOCOL   AbsolutePointerProtocol;
  EFI_ABSOLUTE_POINTER_STATE      State;
  EFI_ABSOLUTE_POINTER_MODE       Mode;
  BOOLEAN                         StateChanged;
  EFI_UNICODE_STRING_TABLE        *ControllerNameTable;
} HID_MOUSE_ABSOLUTE_POINTER_DEV;

#define HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE SIGNATURE_32 ('H', 'I', 'D', 'M')
#define HID_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL(a) \
    CR(a, HID_MOUSE_ABSOLUTE_POINTER_DEV, AbsolutePointerProtocol, HID_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE)


//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   mHidMouseAbsolutePointerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   mHidMouseAbsolutePointerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  mHidMouseAbsolutePointerComponentName2;

//
// Functions of Driver Binding Protocol
//

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
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Starts the mouse device with this driver.

  This function consumes HID Mouse Protocol, initializes HID mouse device,
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
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

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
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
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
HidMouseAbsolutePointerComponentNameGetDriverName (
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
HidMouseAbsolutePointerComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

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
  IN  HID_MOUSE_ABSOLUTE_POINTER_DEV           *HidMouseDev
  );


/**
  Start the HID mouse device receiving reports.

  This function installs the callbacks for lower HID layers to
  start producing input reports.

  @param  HidMouseDev       Device instance to be started.

  @retval EFI_SUCCESS       HID mouse device successfully started.
  @retval Other             Error starting HID mouse device.

**/
EFI_STATUS
StartMouseDevice (
  IN  HID_MOUSE_ABSOLUTE_POINTER_DEV           *HidMouseDev
  );

/**
  Stop the HID mouse device receiving reports.

  This function uninstalls the callbacks for lower HID layers to
  stop producing input reports.

  @param  HidMouseDev       Device instance to be stopped.

  @retval EFI_SUCCESS       HID mouse device successfully stopped.
  @retval Other             HID mouse device was not successfully stopped.

**/
EFI_STATUS
StopMouseDevice (
  IN  HID_MOUSE_ABSOLUTE_POINTER_DEV           *HidMouseDev
  );

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
  );

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
  );

/**
  Event notification function for EFI_ABSOLUTE_POINTER_PROTOCOL.WaitForInput event.

  @param  Event        Event to be signaled when there's input from mouse.
  @param  Context      Points to HID_MOUSE_ABSOLUTE_POINTER_DEV instance.

**/
VOID
EFIAPI
HidMouseAbsolutePointerWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

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
  IN HID_POINTER_INTERFACE Interface,
  IN UINT8                 *HidInputReportBuffer,
  IN UINTN                 HidInputReportBufferSize,
  IN VOID                  *Context
  );
#endif
