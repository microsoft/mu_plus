/*@file HidKbDxe.h

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

Portions derived from UsbKbDxe:
  Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  HidKbDxe.h

Abstract:

  This module defines the functions for HID Keyboard.

Environment:

  UEFI pre-boot Driver Execution Environment (DXE).

--*/
#ifndef _EFI_HID_KB_DXE_H_
#define _EFI_HID_KB_DXE_H_


#include <Uefi.h>

#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Guid/HiiKeyBoardLayout.h>
#include <Guid/HidKeyBoardLayout.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Protocol/HidKeyboardProtocol.h>
#include <Library/HiiLib.h>

#define KEYBOARD_TIMER_INTERVAL         200000  // 0.02s

#define MAX_KEY_ALLOWED                 32

#define HZ                              1000 * 1000 * 10
#define HIDKBD_REPEAT_DELAY             ((HZ) / 2)
#define HIDKBD_REPEAT_RATE              ((HZ) / 50)

#define HID_KEYBOARD_DRIVER_VERSION     0x10

#define INPUT_REPORT_HEADER_SIZE        (OFFSET_OF(KEYBOARD_HID_INPUT_BUFFER, KeyCode))

typedef struct {
  BOOLEAN  Down;
  UINT8    KeyCode;
} HID_KEY;

typedef struct {
  VOID          *Buffer[MAX_KEY_ALLOWED + 1];
  UINTN         Head;
  UINTN         Tail;
  UINTN         ItemSize;
} HID_SIMPLE_QUEUE;

#define HID_KB_DEV_SIGNATURE  SIGNATURE_32 ('h', 'k', 'b', 'd')
#define HID_KB_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNATURE_32 ('h', 'k', 'b', 'x')

typedef struct _KEYBOARD_CONSOLE_IN_EX_NOTIFY {
  UINTN                                 Signature;
  EFI_KEY_DATA                          KeyData;
  EFI_KEY_NOTIFY_FUNCTION               KeyNotificationFn;
  LIST_ENTRY                            NotifyEntry;
} KEYBOARD_CONSOLE_IN_EX_NOTIFY;

#define HID_NS_KEY_SIGNATURE  SIGNATURE_32 ('h', 'n', 's', 'k')

typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;

  //
  // The number of EFI_NS_KEY_MODIFIER children definitions
  //
  UINTN                         KeyCount;

  //
  // NsKey[0] : Non-spacing key
  // NsKey[1] ~ NsKey[KeyCount] : Physical keys
  //
  EFI_KEY_DESCRIPTOR            *NsKey;
} HID_NS_KEY;

#define HID_NS_KEY_FORM_FROM_LINK(a)  CR (a, HID_NS_KEY, Link, HID_NS_KEY_SIGNATURE)

///
/// Structure to describe HID keyboard device
///
typedef struct {
  UINTN                                 Signature;
  EFI_HANDLE                            ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_UNICODE_STRING_TABLE              *ControllerNameTable;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL        SimpleInput;
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL     SimpleInputEx;
  HID_KEYBOARD_PROTOCOL                 *KeyboardProtocol;

  HID_SIMPLE_QUEUE                      HidKeyQueue;
  HID_SIMPLE_QUEUE                      EfiKeyQueue;
  HID_SIMPLE_QUEUE                      EfiKeyQueueForNotify;

  KEYBOARD_HID_INPUT_BUFFER             *LastReport;
  UINTN                                 LastReportSize;
  UINT8                                 CurKeyCode;

  UINT8                                 RepeatKey;
  EFI_EVENT                             RepeatTimer;

  BOOLEAN                               CtrlOn;
  BOOLEAN                               AltOn;
  BOOLEAN                               ShiftOn;
  BOOLEAN                               NumLockOn;
  BOOLEAN                               CapsOn;
  BOOLEAN                               ScrollOn;
  BOOLEAN                               LeftCtrlOn;
  BOOLEAN                               LeftAltOn;
  BOOLEAN                               LeftShiftOn;
  BOOLEAN                               LeftLogoOn;
  BOOLEAN                               RightCtrlOn;
  BOOLEAN                               RightAltOn;
  BOOLEAN                               RightShiftOn;
  BOOLEAN                               RightLogoOn;
  BOOLEAN                               MenuKeyOn;
  BOOLEAN                               SysReqOn;
  BOOLEAN                               AltGrOn;

  BOOLEAN                               IsSupportPartialKey;

  EFI_KEY_STATE                         KeyState;

  //
  // Notification function list
  //
  LIST_ENTRY                            NotifyList;
  EFI_EVENT                             KeyNotifyProcessEvent;

  //
  // Non-spacing key list
  //
  LIST_ENTRY                            NsKeyList;
  HID_NS_KEY                            *CurrentNsKey;
  EFI_KEY_DESCRIPTOR                    *KeyConvertionTable;
  EFI_EVENT                             KeyboardLayoutEvent;
} HID_KB_DEV;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   mHidKeyboardDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   mHidKeyboardComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  mHidKeyboardComponentName2;

#define HID_KB_DEV_FROM_THIS(a) \
    CR(a, HID_KB_DEV, SimpleInput, HID_KB_DEV_SIGNATURE)
#define TEXT_INPUT_EX_HID_KB_DEV_FROM_THIS(a) \
    CR(a, HID_KB_DEV, SimpleInputEx, HID_KB_DEV_SIGNATURE)

//
// According to Universal Serial Bus HID Usage Tables document ver 1.12,
// a Boot Keyboard should support the keycode range from 0x0 to 0x65 and 0xE0 to 0xE7.
// 0xE0 to 0xE7 are for modifier keys, and 0x0 to 0x3 are reserved for typical
// keyboard status or keyboard errors.
// So the number of valid non-modifier HID keycodes is 0x62, and the number of
// valid keycodes is 0x6A.
//
#define NUMBER_OF_VALID_NON_MODIFIER_HID_KEYCODE      0x62
#define NUMBER_OF_VALID_HID_KEYCODE                   0x6A
//
// 0x0 to 0x3 are reserved for typical keyboard status or keyboard errors.
//
#define HIDKBD_VALID_KEYCODE(Key) ((UINT8) (Key) > 3)

typedef struct {
  UINT8 NumLock : 1;
  UINT8 CapsLock : 1;
  UINT8 ScrollLock : 1;
  UINT8 Resrvd : 5;
} LED_MAP;

//
// Functions of Driver Binding Protocol
//
/**
  Check whether HID keyboard driver supports this device.

  @param  This                   The HID keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
HIDKeyboardDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Starts the HID Keyboard device with this driver.

    This function produces Simple Text Input Protocol and Simple Text Input Ex Protocol,
  initializes the keyboard device, and registers the Key Stroke
  Callback function on the HidKeyboardProtocol so that lower layers
  will call this callback when ever a key stroke event happens.


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
HIDKeyboardDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop the HID keyboard device handled by this driver.

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
HIDKeyboardDriverBindingStop (
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
HidKeyboardComponentNameGetDriverName (
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
HidKeyboardComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

//
// Functions of Simple Text Input Protocol
//
/**
  Reset the input device and optionally run diagnostics

  There are 2 types of reset for HID keyboard.
  For non-exhaustive reset, only keyboard buffer is cleared.
  For exhaustive reset, in addition to clearance of keyboard buffer, the hardware status
  is also re-initialized.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
EFI_STATUS
EFIAPI
HIDKeyboardReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                          ExtendedVerification
  );

/**
  Reads the next keystroke from the input device.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          The keystroke information was returned.
  @retval EFI_NOT_READY        There was no keystroke data availiable.
  @retval EFI_DEVICE_ERROR     The keydtroke information was not returned due to
                               hardware errors.

**/
EFI_STATUS
EFIAPI
HIDKeyboardReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                    *Key
  );

//
// Simple Text Input Ex protocol functions
//
/**
  Resets the input device hardware.

  The Reset() function resets the input device hardware. As part
  of initialization process, the firmware/device will make a quick
  but reasonable attempt to verify that the device is functioning.
  If the ExtendedVerification flag is TRUE the firmware may take
  an extended amount of time to verify the device is operating on
  reset. Otherwise the reset operation is to occur as quickly as
  possible. The hardware verification process is not defined by
  this specification and is left up to the platform firmware or
  driver to implement.

  @param This                 A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param ExtendedVerification Indicates that the driver may perform a more exhaustive
                              verification operation of the device during reset.

  @retval EFI_SUCCESS         The device was reset.
  @retval EFI_DEVICE_ERROR    The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
HIDKeyboardResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Reads the next keystroke from the input device.

  @param  This                   Protocol instance pointer.
  @param  KeyData                A pointer to a buffer that is filled in with the keystroke
                                 state data for the key that was pressed.

  @retval EFI_SUCCESS            The keystroke information was returned.
  @retval EFI_NOT_READY          There was no keystroke data available.
  @retval EFI_DEVICE_ERROR       The keystroke information was not returned due to
                                 hardware errors.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.

**/
EFI_STATUS
EFIAPI
HIDKeyboardReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyToggleState          A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                  state for the input device.

  @retval EFI_SUCCESS             The device state was set appropriately.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly and could
                                  not have the setting adjusted.
  @retval EFI_UNSUPPORTED         The device does not support the ability to have its state set.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
HIDKeyboardSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with
                                      the keystroke information for the key that was
                                      pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                      and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                      keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData. This notification function
                                      should be called at <=TPL_CALLBACK.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necessary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle or KeyNotificationFunction is NULL.

**/
EFI_STATUS
EFIAPI
HIDKeyboardRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid
  @retval EFI_NOT_FOUND            Cannot find the matching entry in database.

**/
EFI_STATUS
EFIAPI
HIDKeyboardUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
  Event notification function registered for EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL.WaitForKeyEx
  and EFI_SIMPLE_TEXT_INPUT_PROTOCOL.WaitForKey.

  @param  Event        Event to be signaled when a key is pressed.
  @param  Context      Points to HID_KB_DEV instance.

**/
VOID
EFIAPI
HIDKeyboardWaitForKey (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

/**
  Free keyboard notify list.

  @param  NotifyList              The keyboard notify list to free.

  @retval EFI_SUCCESS             Free the notify list successfully.
  @retval EFI_INVALID_PARAMETER   NotifyList is NULL.

**/
EFI_STATUS
KbdFreeNotifyList (
  IN OUT LIST_ENTRY           *NotifyList
  );

/**
  Check whether the pressed key matches a registered key or not.

  @param  RegisteredData    A pointer to keystroke data for the key that was registered.
  @param  InputData         A pointer to keystroke data for the key that was pressed.

  @retval TRUE              Key pressed matches a registered key.
  @retval FALSE             Key pressed does not matche a registered key.

**/
BOOLEAN
IsKeyRegistered (
  IN EFI_KEY_DATA  *RegisteredData,
  IN EFI_KEY_DATA  *InputData
  );

/**
  Process key notify.

  @param  Event                 Indicates the event that invoke this function.
  @param  Context               Indicates the calling context.
**/
VOID
EFIAPI
KeyNotifyProcessHandler (
  IN  EFI_EVENT                 Event,
  IN  VOID                      *Context
  );

#endif

