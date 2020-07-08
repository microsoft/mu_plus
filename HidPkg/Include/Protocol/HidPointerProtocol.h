/*++ @file HidPointerProtocol.h

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  HidPointerProtocol.h

Abstract:

  This header defines an interface for transmitting HID data between hardware and pointer HID processing driver.

Environment:

  UEFI pre-boot Driver Execution Environment (DXE).

Spec:
  Refer to USB Device Class Definition for Human Interface Devices (HID) version 1.11 Appendix B.2

--*/
#ifndef __HID_POINTER_PROTOCOL_H__
#define __HID_POINTER_PROTOCOL_H__

typedef struct _HID_POINTER_PROTOCOL  HID_POINTER_PROTOCOL;

//Define the supported HID interfaces.
//Currently supported interfaces:
//Boot Mouse as defined in HID 1.11 B.1
//Single Touch HID interface as defined below.
typedef enum {
  BootMouse,
  SingleTouch
} HID_POINTER_INTERFACE;

//Structures for BootMouse interface
#pragma pack(1)
typedef struct {
  UINT8 Button1:1;
  UINT8 Button2:1;
  UINT8 Button3:1;
  UINT8 Reserved:5;
  INT8 XDisplacement;  //X displacement since last report: -127 to 127
  INT8 YDisplacement;  //Y displacement since last report: -127 to 127
  INT8 ZDisplacement;  //Z displacement since last report: -127 to 127. Optional: may not be present.
} MOUSE_HID_INPUT_BUFFER;
#pragma pack()

//Structures for SingleTouch interface
#pragma pack(1)
typedef struct {
  UINT8   Touch:1;
  UINT8   Reserved:7;
  UINT16  CurrentX; //Absolute X: 0 to 1024
  UINT16  CurrentY; //Absolute Y: 0 to 1024
} SINGLETOUCH_HID_INPUT_BUFFER;
#pragma pack()

/**
  The HID Pointer Driver registers a callback function with this signature to receive Pointer HID reports from the
  hardware.

  @param  Interface                - defines the format of the HID report.
  @param  HidInputReportBuffer     - points to the pointer HID report buffer.
  @param  HidInputReportBufferSize - indicates the size of the pointer HID report buffer.
  @param  Context                  - pointer to context (if any) provided when registering the callback.

  @retval None
**/
typedef
VOID
(EFIAPI *POINTER_HID_REPORT_CALLBACK) (
  IN HID_POINTER_INTERFACE Interface,
  IN UINT8                 *HidInputReportBuffer,
  IN UINTN                 HidInputReportBufferSize,
  IN VOID                  *Context
  );

/**
  This function registers a callback function to occur whenever there is a HID Pointer Report packet available.
  Note: Only one callback registration is permitted.

  @param  This                    - pointer to the current driver instance that this callback is being registered with.
  @param  PointerReportCallback   - Pointer HID Report Callback function (see: POINTER_HID_REPORT_CALLBACK)
  @param  Context                 - pointer to context (if any) that should be included when the callback is invoked.

  @retval EFI_SUCCESS         - Successfully registered the callback.
  @retval EFI_ALREADY_STARTED - Another callback is already registered.
  @retval other               - there was an implementation specific failure registering the callback.
**/
typedef
EFI_STATUS
(EFIAPI *REGISTER_POINTER_HID_REPORT_CALLBACK)(
  IN HID_POINTER_PROTOCOL         *This,
  IN POINTER_HID_REPORT_CALLBACK  PointerReportCallback,
  IN VOID                         *Context
  );

/**
  This function unregisters a previously registered pointer HID report callback function.
  Note: Only one callback registration is permitted.

  @param  This  - pointer to the current driver instance that this callback is being unregistered from.

  @retval EFI_SUCCESS     - Successfully unregistered the callback.
  @retval EFI_NOT_FOUND   - No previously registered callback.
  @retval other           - there was an implementation specific failure unregistering the callback.
**/
typedef
EFI_STATUS
(EFIAPI *UNREGISTER_POINTER_HID_REPORT_CALLBACK)(
  IN HID_POINTER_PROTOCOL *This
  );

//
// HID Pointer Protocol struct.
//
struct _HID_POINTER_PROTOCOL {
  REGISTER_POINTER_HID_REPORT_CALLBACK    RegisterPointerReportCallback;
  UNREGISTER_POINTER_HID_REPORT_CALLBACK  UnRegisterPointerReportCallback;
};

extern EFI_GUID gHidPointerProtocolGuid;

#endif //__HID_POINTER_PROTOCOL_H__
