/*++ @file HidKeyboardProtocol.h

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  HidKeyboardProtocol.h

Abstract:

  This header defines an interface for transmitting HID data between hardware and Keyboard HID processing driver.

Environment:

  UEFI pre-boot Driver Execution Environment (DXE).

Spec:
  Refer to USB Device Class Definition for Human Interface Devices (HID) version 1.11 Appendix B.1

--*/

#ifndef __HID_KEYBOARD_PROTOCOL_H__
#define __HID_KEYBOARD_PROTOCOL_H__

typedef struct _HID_KEYBOARD_PROTOCOL HID_KEYBOARD_PROTOCOL;

// Define the supported HID interfaces.
// Currently only Boot Keyboard as defined in HID 1.11 B.1 is supported.
typedef enum {
  BootKeyboard
} KEYBOARD_HID_INTERFACE;

// Structures for BootKeyboard interface
#pragma pack(1)
// Note: HID spec defines only 6 keycodes by default, but we extend that here to
// allow for an arbitrary number of additional keycodes. If there more or fewer
// than 6, then the HID report may actually be larger or smaller than
// sizeof(KEYBOARD_HID_INPUT_BUFFER).
typedef struct {
  UINT8    ModifierKeys;
  UINT8    Reserved;
  UINT8    KeyCode[6];
} KEYBOARD_HID_INPUT_BUFFER;

typedef struct {
  UINT8    NumLock    : 1;
  UINT8    CapsLock   : 1;
  UINT8    ScrollLock : 1;
  UINT8    Compose    : 1;
  UINT8    Kana       : 1;
  UINT8    Constant   : 3;
} KEYBOARD_HID_OUTPUT_BUFFER;
#pragma pack()

/**
  The HID Keyboard Driver registers a callback function with this signature to receive Keyboard HID reports from the
  hardware.

  @param  Interface                - defines the format of the HID report.
  @param  HidInputReportBuffer     - points to the keyboard HID report buffer.
  @param  HidInputReportBufferSize - indicates the size of the keyboard HID report buffer.
  @param  Context                  - pointer to context (if any) provided when registering the callback.

  @retval None
**/
typedef
VOID
(EFIAPI *KEYBOARD_HID_REPORT_CALLBACK)(
  IN KEYBOARD_HID_INTERFACE Interface,
  IN UINT8                  *HidInputReportBuffer,
  IN UINTN                  HidInputReportBufferSize,
  IN VOID                   *Context
  );

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
typedef
EFI_STATUS
(EFIAPI *REGISTER_KEYBOARD_HID_REPORT_CALLBACK)(
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
typedef
EFI_STATUS
(EFIAPI *UNREGISTER_KEYBOARD_HID_REPORT_CALLBACK)(
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
typedef
EFI_STATUS
(EFIAPI *SET_OUTPUT_REPORT)(
  IN HID_KEYBOARD_PROTOCOL  *This,
  IN KEYBOARD_HID_INTERFACE Interface,
  IN UINT8                  *HidOutputReportBuffer,
  IN UINTN                  HidOutputReportBufferSize
  );

struct _HID_KEYBOARD_PROTOCOL {
  REGISTER_KEYBOARD_HID_REPORT_CALLBACK      RegisterKeyboardHidReportCallback;
  UNREGISTER_KEYBOARD_HID_REPORT_CALLBACK    UnRegisterKeyboardHidReportCallback;
  SET_OUTPUT_REPORT                          SetOutputReport;
};

extern EFI_GUID  gHidKeyboardProtocolGuid;

#endif //__HID_KEYBOARD_PROTOCOL_H__
