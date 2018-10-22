/** @file

  Implements common structures and constants for a simple on-screen virtual keyboard.

  Copyright (c) 2015 - 2018, Microsoft Corporation.

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

#ifndef _WINDOW_MANAGER_H_
#define _WINDOW_MANAGER_H_

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsUiThemeLib.h>
#include <Library/SwmDialogsLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiFont.h>
#include <Protocol/OnScreenKeyboard.h>
#include <Protocol/RenderingEngine.h>
#include <Protocol/SimpleWindowManager.h>

#include <UIToolKit/SimpleUIToolKit.h>

#include <Library/MsColorTableLib.h>

#include "SimpleWindowManagerProtocol.h"


// ****** Preprocessor constants ******
//

#define POINTER_STATE_INPUT_QUEUE_SIZE                  50                  // Depth of aggregate pointer event queue.
#define PERIODIC_REFRESH_INTERVAL                       (5 * 10 * 1000)     // Interval for scanning AP providers: 5ms in 100ns units.

#define SMALL_ASSET_MAX_SCREEN_WIDTH                    1280                // Maximum screen resolution that still supports "small" mouse pointer & screen assets.

#define SWM_POINTER_EVENT_FILTER_BOX_SIZE_PERCENT       50                  // Filter window in fraction of a percent (0.50%) of the absolute pointer maximum width.

// Common variables with SimpleWindowManager
extern EFI_HII_HANDLE             mSWMHiiHandle;
extern EFI_ABSOLUTE_POINTER_MODE  mAbsPointerMode;


// Pointer state event input queue (holds pointer event data until consumer reads them out, FIFO)
//
typedef struct {
    BOOLEAN                             bQueueEmpty;
    UINTN                               QueueInputPosition;
    UINTN                               QueueOutputPosition;
    MS_SWM_ABSOLUTE_POINTER_STATE       PointerStateQueue[POINTER_STATE_INPUT_QUEUE_SIZE];
} MS_SWM_ABSOLUTE_POINTER_QUEUE;

// ****** Function prototypes ******
//


/**
    Checks whether the specified controller exposes the Absolute Pointer interface that we will manage.

    @param[in] this                 Pointer to the instance of this driver.
    @param[in] Controller           Handle to be checked for Absolute Pointer support.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Simple Window Manager can support this controller.
    @retval EFI_ALREADY_STARTED     Simple Window Manager is already managing this controller.
    @retval EFI_UNSUPPORTED         Controller isn't one we'll manage.

**/
EFI_STATUS
EFIAPI
SWMDriverSupported (IN EFI_DRIVER_BINDING_PROTOCOL  *this,
                    IN EFI_HANDLE                   Controller,
                    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath);

/**
    Start supporting specified controller exposing Absolute Pointer interface.

    @param[in] this                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller to be managed.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Simple Window Manager will support this controller.
    @retval EFI_ALREADY_STARTED     Simple Window Manager is already managing this controller.
    @retval EFI_UNSUPPORTED         Controller isn't one we'll manage.

**/
EFI_STATUS
EFIAPI
SWMDriverStart (IN EFI_DRIVER_BINDING_PROTOCOL      *this,
                IN EFI_HANDLE                       Controller,
                IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath);

/**
    Stops supporting specified controller exposing Absolute Pointer interface.

    @param[in] this                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller being managed.
    @param[in] NumberOfChildren     Ignored.
    @param[in] ChildHandleBuffer    Ignored.

    @retval EFI_SUCCESS             Successfully stopped managing the specified controller.

**/
EFI_STATUS
EFIAPI
SWMDriverStop (IN  EFI_DRIVER_BINDING_PROTOCOL     *this,
               IN  EFI_HANDLE                      Controller,
               IN  UINTN                           NumberOfChildren,
               IN  EFI_HANDLE                      *ChildHandleBuffer);



/**
    Hides the mouse pointer.

    @param  None

    @retval EFI_SUCCESS             Successfully hid the pointer.

**/
EFI_STATUS
HideMousePointer (VOID);


/**
    Shows the mouse pointer.

    @param  None

    @retval EFI_SUCCESS             Successfully showing the pointer.
    @retval EFI_UNSUPPORTED         Mouse can't be shown because it's been disabled.

**/
EFI_STATUS
ShowMousePointer (VOID);

/**
 * Wait for an event, and display the POWER OFF dialog if the Power Timer expires
 *
 * @param NumberOfEvents   The number of events the user is waiting on
 * @param Events           Array of events the user is waiting on
 * @param EventTypes       Array of event types the user is waiting on
 * @param Timeout          Time out (allows for refresh, etc) as long as < POWER OFF timer
 * @param ShutdownRemaining Used to keep track of power off delay for WaitForEvents with a refresh timer
 *
 * @return SWM_EVENT_TYPE  The content of the event type array element of the event
 */
EFI_STATUS
WaitForEventInternal (
    IN UINTN           NumberOfEvents,
    IN EFI_EVENT      *Events,
    IN UINTN          *Index,
    IN UINT64          Timeout,
    IN BOOLEAN         ContinueTimer);

/**
    Displays a modal dialog box for obtaining or confirming a password. The password dialog returns a typed integer value
    // that indicates which button the user clicked along with a password string.

    NOTE: Password dialog layout is designed high resolution displays and won't necessarily look
          good at lower resolutions.

    @param[in]  this            Pointer to the instance of this driver.
    @param[in]  pTitleBarText   Dialog title bar text.
    @param[in]  pCaptionText    Dialog box title.
    @param[in]  pBodyText       Dialog message text.
    @param[in]  pErrorText      (OPTIONAL) Error message message text.
    @param[in]  Type            Contents and behavior of the dialog box.
    @param[out] Result          Button selection result.
    @param[out] Password        (OPTIONAL) Pointer to the password string.
    @param[out] Thumbprint      (OPTIONAL) Pointer to the Thumbprint string.

    @retval EFI_SUCCESS         Successfully processed password dialog input.

**/
EFI_STATUS
PasswordDialogInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
                        IN  CHAR16                              *pTitleBarText,
                        IN  CHAR16                              *pCaptionText,
                        IN  CHAR16                              *pBodyText,
                        IN  CHAR16                              *pErrorText,
                        IN  SWM_PWD_DIALOG_TYPE                 Type,
                        OUT SWM_MB_RESULT                       *Result,
                        OUT CHAR16                              **Password);


/**
    Displays a modal dialog box that presents a list of choices to the user and allows them to select
    an option. Title, caption, and body text are customizable, as are the options in the list. Dialog
    will contain a submit and a cancel button and will inform the user of which button was pressed.

    @param[in]  This            Pointer to the instance of this driver.
    @param[in]  pTitleBarText   Dialog title bar text.
    @param[in]  pCaption        Dialog box title.
    @param[in]  pBodyText       Message to be displayed.
    @param[in]  ppOptionsList   Pointer to a list of string pointers that will be used as options.
    @param[in]  OptionsCount    The number of options in the list.
    @param[out] Result          Button selection result.
    @param[out] SelectedIndex   An index of the selected option.

    @retval EFI_SUCCESS         Successfully processed simple list dialog input.

**/
EFI_STATUS
SingleSelectDialogInternal (
  IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
  IN  CHAR16                              *pTitleBarText,
  IN  CHAR16                              *pCaptionText,
  IN  CHAR16                              *pBodyText,
  IN  CHAR16                              **ppOptionsList,
  IN  UINTN                               OptionsCount,
  OUT SWM_MB_RESULT                       *Result,
  OUT UINTN                               *SelectedIndex
  );

/**
Displays a modal dialog box for obtaining or confirming a password. The password dialog returns a typed integer value
// that indicates which button the user clicked along with a password string.

NOTE: Password dialog layout is designed high resolution displays and won't necessarily look
good at lower resolutions.

@param[in]  this            Pointer to the instance of this driver.
@param[in]  pTitleBarText   Dialog title bar text.
@param[in]  pCaptionText    Dialog box title.
@param[in]  pBodyText       Dialog message text.
@param[in]  pErrorText      (OPTIONAL) Error message message text.
@param[in]  Type            Contents and behavior of the dialog box.
@param[out] Result          Button selection result.
@param[out] Password        (OPTIONAL) Pointer to the password string.
@param[out] Thumbprint      (OPTIONAL) Pointer to the Thumbprint string.

@retval EFI_SUCCESS         Successfully processed password dialog input.

**/
EFI_STATUS
SemmAuthDialogInternal (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *this,
IN  CHAR16                              *pTitleBarText,
IN  CHAR16                              *pCaptionText,
IN  CHAR16                              *pBodyText,
IN  CHAR16                              *pCertText,
IN  CHAR16                              *pConfirmText,
IN  CHAR16                              *pErrorText,
IN  SWM_PWD_DIALOG_TYPE                 Type,
OUT SWM_MB_RESULT                       *Result,
OUT CHAR16                              **Password OPTIONAL,
OUT CHAR16                              **Thumbprint OPTIONAL);

// ****** Common data structures ******
//
#define WINMGR_CLIENT_SIGNATURE SIGNATURE_32 ('W', 'i', 'n', 'M')

// Client being supported via the SWM protocol.
//
typedef struct _WINMGR_CLIENTLIST_tag
{
    UINTN                               Signature;          // Sinature of this block
    BOOLEAN                             HasDisplaySurface;  // TRUE = client has an associated display surface (i.e., not just a user input queue).
    BOOLEAN                             Active;             // TRUE = currently active and processing events.
    SWM_RECT                            WindowFrame;        // Clients on-screen window frame rectangle (used for hit detection).
    BOOLEAN                             WindowAreaCaptured; // TRUE = screen underlying the client's window has been captured.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pCaptureBuffer;    // Buffer for capturing screen contents underlying the client's window area.
    EFI_HANDLE                          ImageHandle;        // Image handle associated with the client context.
    struct _WINMGR_CLIENTLIST_tag       *pNext;             // Next client in the list.
    struct _WINMGR_CLIENTLIST_tag       *pPrev;             // Previous client in the list.
    // Per Client Absolute Pointer protocol
    //
    EFI_ABSOLUTE_POINTER_PROTOCOL       ClientAbsPtr;       // Absolute Pointer Protocol for each client that registers.
    MS_SWM_ABSOLUTE_POINTER_QUEUE       Queue;              // Queue for this Window
    MS_SWM_CLIENT_NOTFICATION_CALLBACK  DataNotificationCallback; // Function to call when data is available.  Optional
    VOID                               *DataNotificationContext;  // Client Context parameter for Callback.
    UINT32                              Z_Order;            // Limited Z-order (fixed, not dynamic)
} WINMGR_CLIENT;

#define WINMGR_CLIENT_FROM_ABS_PTR(a) \
    CR(a, WINMGR_CLIENT, ClientAbsPtr, WINMGR_CLIENT_SIGNATURE)

// Absolute Pointer provider being watched for pointer state events.
//
typedef struct _WINMGR_AP_WATCHLIST_tag
{
    EFI_HANDLE                          Controller;         // Handle of the controller providing this interface.
    EFI_ABSOLUTE_POINTER_PROTOCOL       *AbsolutePointer;   // Absolute Pointer provider interface.
    BOOLEAN                             bNeedsMousePointer; // Needs the mouse pointer to be displayed.
    struct _WINMGR_AP_WATCHLIST_tag     *pNext;             // Next provider in the list.
    struct _WINMGR_AP_WATCHLIST_tag     *pPrev;             // Previous provider in the list.

} WINMGR_AP_WATCHLIST;

// Mouse pointer bitmap information.
//
typedef struct _MOUSEPTR_BITMAPINFO_tag
{
    const UINT32 *pBitmap;
    UINT32 Width;
    UINT32 Height;
} MOUSEPTR_BITMAP_INFO;

// Simple Window Manager context
//
typedef struct _WINMGR_CONTEXT_tag
{
    // Current screen resolution.
    //
    UINTN                               ScreenWidth;
    UINTN                               ScreenHeight;

    // Whether or not to display the mouse pointer and buffers to manage rendering
    //
    BOOLEAN                             bDisplayingMousePointer;        // Tracks whether or not we're currently displaying the mouse pointer.
    BOOLEAN                             bMousePointerEnabled;           // Global flag to prohibit mouse pointer from being rendered regardless of the Absolute Pointer protocol providers configuration.
    BOOLEAN                             bLastMoveRequiredMousePointer;  // Used when (re)enabling the mouse pointer via the SWM protocol - if enabling the pointer and the last move required the mouse pointer we should immediately render it else we'll wait until the mouse moves to render.
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pBltBuffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *pRestoreBuffer;

    // Current mouse pointer position and button state.
    //
    UINTN                               CurrentMousePointerOrigX;
    UINTN                               CurrentMousePointerOrigY;
    MOUSEPTR_BITMAP_INFO                MousePointer;

    // SWM protocol
    //
    MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   SWMProtocol;

    // User Abolute Pointer protocol
    EFI_ABSOLUTE_POINTER_PROTOCOL       *UserAbsolutePointerProtocol;

    // List of Absolute Pointer protocol providers to watch & aggregate
    //
    WINMGR_AP_WATCHLIST                 *AbsolutePointerProviders;

    // List of clients supported by the window manager
    //
    WINMGR_CLIENT                       *Clients;
} WINMGR_CONTEXT;

// Font package definition.
//
//#pragma pack (push, 1)
//typedef struct _SWM_FONT_PACKAGE_tag_
//{
//    EFI_HII_FONT_PACKAGE_HDR FontHeader;
//    CHAR16 FontFamilyNameContd[25];
//} SWM_FONT_PACKAGE_HEADER;
//#pragma pack (pop)


// ****** External definitions ******
//
extern WINMGR_CONTEXT                       mSWM;
extern EFI_GRAPHICS_OUTPUT_PROTOCOL         *mGop;
extern EFI_HII_FONT_PROTOCOL                *mFont;
extern EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL    *mSimpleTextInEx;
extern MS_RENDERING_ENGINE_PROTOCOL         *mRenderingEngine;
extern EFI_HANDLE                           mImageHandle;
extern EFI_HANDLE                           gPriorityHandle;
extern MS_UI_THEME_DESCRIPTION              *mTheme;


/**
    Peeks at the pending pointer event state in the aggregate event queue.

    @param[out] PointerState        Pointer event to be filled in from the event queue.

    @retval EFI_SUCCESS             Successfully retrieved event state.
    @retval EFI_NOT_FOUND           No data found in the queue.

**/
EFI_STATUS
PeekAtAbsolutePointerEventInQueue (IN WINMGR_CLIENT                  *Client,
                                   OUT MS_SWM_ABSOLUTE_POINTER_STATE *PointerState);


/**
    Removes pending pointer event state from the aggregate event queue.

    @param[out] PointerState        Pointer event to be filled in from the event queue.

    @retval EFI_SUCCESS             Successfully retrieved event state.
    @retval EFI_NOT_FOUND           No data found in the queue.

**/
EFI_STATUS
ExtractAbsolutePointerEventFromQueue (IN WINMGR_CLIENT                  *Client,
                                      OUT MS_SWM_ABSOLUTE_POINTER_STATE *pPointerState);

#endif  // _WINDOW_MANAGER_H_
