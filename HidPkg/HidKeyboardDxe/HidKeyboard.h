/** @file HidKeyboard.h

  Copyright (C) Microsoft Corporation. All rights reserved.

  Portions derived from UsbKbDxe:
  Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

--*/

#ifndef _HID_KEYBOARD_H_
#define _HID_KEYBOARD_H_

#include "HidKbDxe.h"

#define HID_KEYBOARD_KEY_COUNT  105

#define HID_KEYBOARD_LANGUAGE_STR_LEN     5         // RFC4646 Language Code: "en-US"
#define HID_KEYBOARD_DESCRIPTION_STR_LEN  (16 + 1)  // Description: "English Keyboard"

#pragma pack (1)
typedef struct {
  //
  // This 4-bytes total array length is required by PreparePackageList()
  //
  UINT32                    Length;

  //
  // Keyboard Layout package definition
  //
  EFI_HII_PACKAGE_HEADER    PackageHeader;
  UINT16                    LayoutCount;

  //
  // EFI_HII_KEYBOARD_LAYOUT
  //
  UINT16                    LayoutLength;
  EFI_GUID                  Guid;
  UINT32                    LayoutDescriptorStringOffset;
  UINT8                     DescriptorCount;
  EFI_KEY_DESCRIPTOR        KeyDescriptor[HID_KEYBOARD_KEY_COUNT];
  UINT16                    DescriptionCount;
  CHAR16                    Language[HID_KEYBOARD_LANGUAGE_STR_LEN];
  CHAR16                    Space;
  CHAR16                    DescriptionString[HID_KEYBOARD_DESCRIPTION_STR_LEN];
} HID_KEYBOARD_LAYOUT_PACK_BIN;
#pragma pack()

/**
  Initialize HID keyboard device and all private data
  structures.

  @param  HidKeyboardDevice  The HID_KB_DEV instance.

  @retval EFI_SUCCESS        Initialization is successful.
  @retval EFI_DEVICE_ERROR   Keyboard initialization failed.

**/
EFI_STATUS
InitHidKeyboard (
  IN OUT HID_KB_DEV  *HidKeyboardDevice
  );

/**
  Initialize USB keyboard layout.

  This function initializes Key Convertion Table for the USB keyboard device.
  It first tries to retrieve layout from HII database. If failed and default
  layout is enabled, then it just uses the default layout.

  @param  UsbKeyboardDevice      The USB_KB_DEV instance.

  @retval EFI_SUCCESS            Initialization succeeded.
  @retval EFI_NOT_READY          Keyboard layout cannot be retrieve from HII
                                 database, and default layout is disabled.
  @retval Other                  Fail to register event to EFI_HII_SET_KEYBOARD_LAYOUT_EVENT_GUID group.

**/
EFI_STATUS
InitKeyboardLayout (
  OUT HID_KB_DEV  *HidKeyboardDevice
  );

/**
  Destroy resources for keyboard layout.

  @param  UsbKeyboardDevice    The USB_KB_DEV instance.

**/
VOID
ReleaseKeyboardLayoutResources (
  IN OUT HID_KB_DEV  *HidKeyboardDevice
  );

/**
  Initial processing of the HID key report. Processes and queues individual keys
  in the key report.

  @param  *HidInputReportBuffer     - Pointer to the buffer containing HID key report.
  @param  HidInputReportBufferSize  - gives the size of the input report buffer.
  @param  *HidKeyboardDevice        - pointer to HID_KB_DEV struct.

  @retval VOID                  None

**/
VOID
ProcessKeyStroke (
  IN UINT8       *HidInputReportBuffer,
  IN UINTN       HidInputReportBufferSize,
  IN HID_KB_DEV  *HidKeyboardDevice
  );

/**
  This function parses the Modifier Key Code and sets the
  appropriate flags for Key Stroke processing.

  This function parses the modifier keycode and updates state of
  modifier key in HID_KB_DEV instance, and returns status.

  @param  HidKeyboardDevice    The HID_KB_DEV instance.

  @retval EFI_SUCCESS          Keycode successfully parsed.
  @retval EFI_NOT_READY        Keyboard buffer is not ready for a valid keycode

**/
EFI_STATUS
HIDProcessModifierKey (
  IN HID_KB_DEV  *HidKeyboardDevice,
  IN HID_KEY     *HIDKey
  );

/**
  Converts USB Keycode ranging from 0x4 to 0x65 to EFI_INPUT_KEY.

  @param  HidKeyboardDevice     The HID_KB_DEV instance.
  @param  KeyCode               Indicates the key code that will be interpreted.
  @param  KeyData               A pointer to a buffer that is filled in with
                                the keystroke information for the key that
                                was pressed.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER KeyCode is not in the range of 0x4 to 0x65.
  @retval EFI_INVALID_PARAMETER Translated EFI_INPUT_KEY has zero for both ScanCode and UnicodeChar.
  @retval EFI_NOT_READY         KeyCode represents a dead key with EFI_NS_KEY_MODIFIER
  @retval EFI_DEVICE_ERROR      Keyboard layout is invalid.

**/
EFI_STATUS
HIDKeyCodeToEfiInputKey (
  IN  HID_KB_DEV    *HidKeyboardDevice,
  IN  UINT8         KeyCode,
  OUT EFI_KEY_DATA  *KeyData
  );

/**
  Top-level function for handling key report form HID layer.

  @param  Interface - defines the format of the hid report.
  @param  *HidInputReportBuffer Pointer to the buffer containing
                                key strokes received.
  @param  HidInputReportBufferSize - gives the size of the input report buffer.
  @param  *Context              Context that we need which
                                should be the pointer to
                                HID_KB_DEV struct.

  @retval VOID                  None

**/
VOID
EFIAPI
HIDProcessKeyStrokesCallback (
  IN KEYBOARD_HID_INTERFACE  Interface,
  IN UINT8                   *HidInputReportBuffer,
  IN UINTN                   HidInputReportBufferSize,
  IN VOID                    *Context
  );

/**
  Create the queue.

  @param  Queue     Points to the queue.
  @param  ItemSize  Size of the single item.

**/
VOID
InitQueue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  IN      UINTN             ItemSize
  );

/**
  Destroy the queue

  @param Queue    Points to the queue.
**/
VOID
DestroyQueue (
  IN OUT HID_SIMPLE_QUEUE  *Queue
  );

/**
  Check whether the queue is empty.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is empty.
  @retval FALSE     Queue is not empty.

**/
BOOLEAN
IsQueueEmpty (
  IN  HID_SIMPLE_QUEUE  *Queue
  );

/**
  Check whether the queue is full.

  @param  Queue     Points to the queue.

  @retval TRUE      Queue is full.
  @retval FALSE     Queue is not full.

**/
BOOLEAN
IsQueueFull (
  IN  HID_SIMPLE_QUEUE  *Queue
  );

/**
  Enqueue the item to the queue.

  @param  Queue     Points to the queue.
  @param  Item      Points to the item to be enqueued.
  @param  ItemSize  Size of the item.
**/
VOID
Enqueue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  IN      VOID              *Item,
  IN      UINTN             ItemSize
  );

/**
  Dequeue a item from the queue.

  @param  Queue     Points to the queue.
  @param  Item      Receives the item.
  @param  ItemSize  Size of the item.

  @retval EFI_SUCCESS        Item was successfully dequeued.
  @retval EFI_DEVICE_ERROR   The queue is empty.

**/
EFI_STATUS
Dequeue (
  IN OUT  HID_SIMPLE_QUEUE  *Queue,
  OUT  VOID                 *Item,
  IN      UINTN             ItemSize
  );

/**
Sends CAPSLOCK LED status to keyboard

@param  HidKeyboardDevice     The HID_KB_DEV instance.
**/
VOID
SetKeyLED (
  IN  HID_KB_DEV  *HidKeyboardDevice
  );

/**
  Handler for Repeat Key event.

  This function is the handler for Repeat Key event triggered
  by timer.
  After a repeatable key is pressed, the event would be triggered
  with interval of USBKBD_REPEAT_DELAY. Once the event is triggered,
  following trigger will come with interval of USBKBD_REPEAT_RATE.

  @param  Event              The Repeat Key event.
  @param  Context            Points to the USB_KB_DEV instance.

**/
VOID
EFIAPI
HidKeyboardRepeatHandler (
  IN    EFI_EVENT  Event,
  IN    VOID       *Context
  );

#endif
