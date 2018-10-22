/** @file
  Define the Simple Window Manager (SWM) constants and common structures.

  Copyright (c) 2014 - 2018, Microsoft Corporation.

  All rights reserved.
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef _SIMPLE_WINDOW_MANAGER_H_
#define _SIMPLE_WINDOW_MANAGER_H_

#include <Protocol/AbsolutePointer.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiFont.h>

// Global ID for the Simple Window Manager Protocol
//
#define MS_SIMPLE_WINDOW_MANAGER_PROTOCOL_GUID                                      \
  {                                                                                 \
    0x9d400d20, 0x6f35, 0x4268, { 0x90, 0x4f, 0xdc, 0x04, 0xb1, 0x87, 0x7b, 0x62 }  \
  }

// ****** Common data structures ******
//

typedef struct  _MS_SIMPLE_WINDOW_MANAGER_PROTOCOL  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL;

// Pointer Mode - defines coordinate limits.
//
//typedef struct
//{
//    UINT64 AbsoluteMinX;
//    UINT64 AbsoluteMinY;
//    UINT64 AbsoluteMinZ;
//    UINT64 AbsoluteMaxX;
//    UINT64 AbsoluteMaxY;
//    UINT64 AbsoluteMaxZ;
//    UINT32 Attributes;
//} MS_SWM_ABSOLUTE_POINTER_MODE;
#define MS_SWM_ABSOLUTE_POINTER_MODE EFI_ABSOLUTE_POINTER_MODE

// Pointer State - coordinate and button information.
//
//typedef struct
//{
//    UINT64 CurrentX;
//    UINT64 CurrentY;
//    UINT64 CurrentZ;
//    UINT32 ActiveButtons;
//} MS_SWM_ABSOLUTE_POINTER_STATE;
#define  MS_SWM_ABSOLUTE_POINTER_STATE  EFI_ABSOLUTE_POINTER_STATE
// Standard bounding rectangle.
//
typedef struct _SWM_RECT_tag
{
    UINT32    Left;
    UINT32    Top;
    UINT32    Right;
    UINT32    Bottom;
} SWM_RECT;

// Supported user input types.
//
#define SWM_INPUT_TYPE_TOUCH   0x00000001
#define SWM_INPUT_TYPE_KEY     0x00000002

// Input State - aggregated touch and keyboard input state.
//
typedef struct
{
    UINT32  InputType;
    union
    {
        MS_SWM_ABSOLUTE_POINTER_STATE   TouchState;
        EFI_KEY_DATA                    KeyState;
    } State;
} SWM_INPUT_STATE;

// ****** Preprocessor constants ******
//

// Simple Window Manager registration flags and limited Z-Order
//
#define SWM_Z_ORDER_OSK             0x00000040      // Top most window
#define SWM_Z_ORDER_POPUP2          0x00000030      // Priority Popup (Power Down)
#define SWM_Z_ORDER_POPUP           0x00000020      // Popups
#define SWM_Z_ORDER_CLIENT          0x00000010      // Front Page
#define SWM_Z_ORDER_BASE            0x00000000      // Default Client

// Macro to check for left-button/finger down state.
//
#define SWM_IS_FINGER_DOWN(State)   (((State).ActiveButtons & 0x1) == 1)


// ****** Function prototypes ******
//

/**
    Function Prototype for Data Notification callback function

    This client routine is called when data is available for the client to read

    @param[in]  Context        Pointer given by client at Register Client.

    @retval TRUE               Signal WaitForEvent
    @retval FALSE              Do not signal WaitForEvent

**/
typedef
BOOLEAN
(EFIAPI *MS_SWM_CLIENT_NOTFICATION_CALLBACK) (IN VOID *Context);

/*++

  Routine Description:
    Retrieves the aggregated current state of a pointer device from all active Absolute Pointer sources.

  Arguments:
    This                  - Protocol instance pointer.
    State                 - A pointer to the state information on the pointer device.

  Returns:
    EFI_SUCCESS           - The state of the pointer device was returned in State..
    EFI_NOT_READY         - The state of the pointer device has not changed since the last call to
                            GetState().
    EFI_DEVICE_ERROR      - A device error occurred while attempting to retrieve the pointer
                            device's current state.
--*/
typedef
EFI_STATUS
(EFIAPI *MS_SWM_REGISTER_CLIENT) (
    IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
    IN  EFI_HANDLE                          ImageHandle,
    IN  UINT32                              Z_Order,
    IN  SWM_RECT                           *FrameRect,
    IN  MS_SWM_CLIENT_NOTFICATION_CALLBACK  DataNotificationCallback OPTIONAL,
    IN  VOID                                *Context,
    OUT EFI_ABSOLUTE_POINTER_PROTOCOL      **AbsolutePointer,
    OUT EFI_EVENT                           *PaintEvent
);


typedef
EFI_STATUS
(EFIAPI *MS_SWM_UNREGISTER_CLIENT) (
    IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
    IN  EFI_HANDLE                          ImageHandle
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_ACTIVATE_WINDOW) (
    IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
    IN EFI_HANDLE                           ImageHandle,
    IN BOOLEAN                              MakeActive
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_SET_WINDOW_FRAME) (
    IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
    IN EFI_HANDLE                           ImageHandle,
    IN SWM_RECT                             *FrameRect
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_BLT_WINDOW) (
  IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL       *This,
  IN  EFI_HANDLE                              ImageHandle,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_STRING_TO_WINDOW) (
  IN        MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
  IN        EFI_HANDLE                          ImageHandle,
  IN        EFI_HII_OUT_FLAGS                   Flags,
  IN        EFI_STRING                          String,
  IN        EFI_FONT_DISPLAY_INFO               *StringInfo,
  IN OUT    EFI_IMAGE_OUTPUT                    **Blt,
  IN        UINTN                               BltX,
  IN        UINTN                               BltY,
  OUT       EFI_HII_ROW_INFO                    **RowInfoArray OPTIONAL,
  OUT       UINTN                               *RowInfoArraySize OPTIONAL,
  OUT       UINTN                               *ColumnInfoArray OPTIONAL
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_ENABLE_MOUSE_POINTER) (
    IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
    IN BOOLEAN                              bEnablePointer
);

typedef
EFI_STATUS
(EFIAPI *MS_SWM_WAIT_FOR_EVENT) (
    IN UINTN           NumberOfEvents,
    IN EFI_EVENT      *Events,
    IN UINTN          *Index,
    IN UINT64          Timeout,
    IN BOOLEAN         ContinueTimer
);

// SWM protocol structure
//
struct _MS_SIMPLE_WINDOW_MANAGER_PROTOCOL
{
    // Client Messaging and Window Interface
    //
    MS_SWM_REGISTER_CLIENT              RegisterClient;
    MS_SWM_UNREGISTER_CLIENT            UnregisterClient;
    MS_SWM_ACTIVATE_WINDOW              ActivateWindow;
    MS_SWM_SET_WINDOW_FRAME             SetWindowFrame;
    MS_SWM_BLT_WINDOW                   BltWindow;
    MS_SWM_STRING_TO_WINDOW             StringToWindow;
    MS_SWM_ENABLE_MOUSE_POINTER         EnableMousePointer;
    MS_SWM_WAIT_FOR_EVENT               WaitForEvent;
};

extern EFI_GUID     gMsSWMProtocolGuid;

#endif      // _SIMPLE_WINDOW_MANAGER_H_
