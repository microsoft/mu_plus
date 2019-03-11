/** @file

Implements a simple on-screen virtual keyboard for text input.

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

// TODO list:
//
// - Enable key text rotation (register pre-rotated font packages?)

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsUiThemeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MsColorTableLib.h>

#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiFont.h>
#include <Protocol/OnScreenKeyboard.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleWindowManager.h>

#include <Guid/ConsoleInDevice.h>
#include <Guid/OSKDevicePath.h>

#include <UIToolKit/SimpleUIToolKit.h>

#include "OnScreenKeyboard.h"
#include "OnScreenKeyboardProtocol.h"
#include "DisplayTransform.h"
#include "KeyMapping.h"

// *** Medium format ***
#include <Resources/KeyboardIcon_Medium.h>
// *** Small format ***
#include <Resources/KeyboardIcon_Small.h>

STATIC EFI_HANDLE mControllerHandle = NULL;

//
// Onscreen Keyboard Vendor Device Path
//
STATIC OSK_DEVICE_PATH mPlatformOSKDevice = {
    {
        {
            HARDWARE_DEVICE_PATH,
            HW_VENDOR_DP,
            {
                (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
                (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
            }
        },
        OSK_DEVICE_PATH_GUID
    },
    {
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        {
            END_DEVICE_PATH_LENGTH,
            0
        }
    }
};

// Preprocessor Constants
//

// Global variables
//
EFI_HANDLE                          mImageHandle;

// Common structures and protocols
EFI_GRAPHICS_OUTPUT_PROTOCOL        *mGop;
EFI_HII_FONT_PROTOCOL               *mFont;
MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *mSWMProtocol;

EFI_EVENT                           mKeyRepeatTimerEvent;
EFI_EVENT                           mCheckDisplayModeTimerEvent;
EFI_ABSOLUTE_POINTER_PROTOCOL      *mOSKPointerProtocol;
EFI_EVENT                           mOSKPaintEvent;
KEYBOARD_CONTEXT                    mOSK;

// Local function prototypes
//
EFI_STATUS
NormalizeKeyRectsForRendering (IN SCREEN_ANGLE Angle);

EFI_STATUS
ShowKeyboardIcon (IN BOOLEAN bShowKeyboard);

VOID
GetKeyboardIconBoundingRect (OUT SWM_RECT *pRect);

EFI_STATUS
EFIAPI
OSKDriverInit();

/**
Checks to see if the incoming handle has a OSK device path installed on it.
The handle information was saved at the driver entry point

@param  This                 Protocol instance pointer.
@param  ControllerHandle     Handle of device to test.
@param  RemainingDevicePath  Optional parameter use to pick a specific child
device to start.

@retval EFI_SUCCESS          This driver supports this device.
@retval EFI_UNSUPPORTED      Debug Port device is not supported.
@retval EFI_OUT_OF_RESOURCES Fails to allocate memory for device.
@retval others               Some error occurs.

**/
EFI_STATUS
EFIAPI
OSKDriverBindingSupported(
IN EFI_DRIVER_BINDING_PROTOCOL  *This,
IN EFI_HANDLE                   Controller,
IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
)
{
    EFI_STATUS       Status;

    OSK_DEVICE_PATH OskDevicePath;
    //Check if the incoming handle is the same handle as what you installed your device path on
    if (Controller != mControllerHandle){
        return EFI_UNSUPPORTED;
    }

    //
    // Try to Bind to Device Path Protocol.
    //
    Status = gBS->OpenProtocol(
        Controller,
        &gEfiDevicePathProtocolGuid,
        (VOID **)&OskDevicePath,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
        );
    if (EFI_ERROR(Status)) {
        return Status;
    }

    gBS->CloseProtocol(
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

    return EFI_SUCCESS;
}

/**
Binds exclusively to Onscreen keyboard device path on the controller handle, produces OSK
protocol.

@param  This                 Protocol instance pointer.
@param  ControllerHandle     Handle of device to bind driver to.
@param  RemainingDevicePath  Optional parameter use to pick a specific child
device to start.

@retval EFI_SUCCESS          This driver is added to ControllerHandle.
@retval EFI_OUT_OF_RESOURCES Fails to allocate memory for device.
@retval others               Some error occurs.

**/
EFI_STATUS
EFIAPI
OSKDriverBindingStart(
IN EFI_DRIVER_BINDING_PROTOCOL  *This,
IN EFI_HANDLE                   Controller,
IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
)
{
    EFI_STATUS       Status = EFI_SUCCESS;
    OSK_DEVICE_PATH OskDevicePath;

    //Check if the incoming handle is the same handle as what you installed your device path on
    if (Controller != mControllerHandle){
        return EFI_UNSUPPORTED;
    }

    Status = gBS->OpenProtocol(
        Controller,
        &gEfiDevicePathProtocolGuid,
        (VOID **)&OskDevicePath,
        This->DriverBindingHandle,
        Controller,
        EFI_OPEN_PROTOCOL_BY_DRIVER
        );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_INFO, "INFO [OSK]: Device Path already opened (%r).\r\n", Status));
        return Status;
    }

    // Determine if the Simple Window Manager protocol is available
    //
    Status = gBS->LocateProtocol(&gMsSWMProtocolGuid,
        NULL,
        (VOID **)&mSWMProtocol
        );

    if (EFI_ERROR(Status))
    {
        mSWMProtocol = NULL;
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to find Simple Window Manager protocol (%r).\r\n", Status));
        goto ErrorExit;
    }

    // Determine if the Graphics Output Protocol is available
    //
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
        NULL,
        (VOID **)&mGop
        );

    if (EFI_ERROR(Status))
    {
        mGop = NULL;
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to find GOP protocol (%r).\r\n", Status));
        goto ErrorExit;
    }

    // Initialize OSK
    Status = OSKDriverInit();
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Init OSK Failed (%r).\r\n", Status));

        if (mKeyRepeatTimerEvent != NULL){
            // you would be here if the driver init failed
            gBS->CloseEvent(mKeyRepeatTimerEvent);
            //if we got this far the protocol got installed, lets try to uninstall it
            gBS->UninstallMultipleProtocolInterfaces(Controller,
                &gEfiSimpleTextInProtocolGuid,
                (VOID**)&mOSK.SimpleTextIn,
                &gEfiSimpleTextInputExProtocolGuid,
                (VOID**)&mOSK.SimpleTextInEx,
                &gEfiConsoleInDeviceGuid,
                NULL,
                NULL
                );

            if (mCheckDisplayModeTimerEvent != NULL){
                gBS->CloseEvent(mCheckDisplayModeTimerEvent);
            }
        }

        if (mOSK.SimpleTextIn.WaitForKey != NULL){
            gBS->CloseEvent(mOSK.SimpleTextIn.WaitForKey);
        }
        if (mOSK.SimpleTextInEx.WaitForKeyEx != NULL){
            gBS->CloseEvent(mOSK.SimpleTextInEx.WaitForKeyEx);
        }
        goto ErrorExit;
    }

    //everything successful
    DEBUG((DEBUG_INFO, "INFO [OSK]: Init OSK Successful (%r).\r\n", Status));
    goto Exit;

ErrorExit:
    gBS->CloseProtocol(
        Controller,
        &gEfiDevicePathProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

Exit:
    return Status;
}

/**
Stop this driver on ControllerHandle by removing OSK SimpleTextIn SimpleTextInEx protocol on
the ControllerHandle.

@param  This              Protocol instance pointer.
@param  ControllerHandle  Handle of device to stop driver on
@param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
children is zero stop the entire bus driver.
@param  ChildHandleBuffer List of Child Handles to Stop.

@retval EFI_SUCCESS       This driver is removed ControllerHandle.
@retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
OSKDriverBindingStop(
IN EFI_DRIVER_BINDING_PROTOCOL  *This,
IN EFI_HANDLE                   ControllerHandle,
IN UINTN                        NumberOfChildren,
IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
)
{
    EFI_STATUS Status;
    DEBUG((DEBUG_INFO, "INFO [OSK]: DriverBindingStop. \r\n"));
    Status = gBS->UninstallMultipleProtocolInterfaces(ControllerHandle,
        &gEfiSimpleTextInProtocolGuid,
        (VOID**)&mOSK.SimpleTextIn,
        &gEfiSimpleTextInputExProtocolGuid,
        (VOID**)&mOSK.SimpleTextInEx,
        &gEfiConsoleInDeviceGuid,
        NULL,
        NULL
        );

    if (EFI_ERROR(Status)) {
        return EFI_UNSUPPORTED;
    }
    if (mOSK.SimpleTextIn.WaitForKey != NULL){
        gBS->CloseEvent(mOSK.SimpleTextIn.WaitForKey);
    }
    if (mOSK.SimpleTextInEx.WaitForKeyEx != NULL){
        gBS->CloseEvent(mOSK.SimpleTextInEx.WaitForKeyEx);
    }
    if (mKeyRepeatTimerEvent != NULL){
        gBS->CloseEvent(mKeyRepeatTimerEvent);
    }
    if (mCheckDisplayModeTimerEvent != NULL){
        gBS->CloseEvent(mCheckDisplayModeTimerEvent);
    }

    return EFI_SUCCESS;
}

///
/// Driver Binding Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED EFI_DRIVER_BINDING_PROTOCOL gOSKDriverBinding = {
    OSKDriverBindingSupported,
    OSKDriverBindingStart,
    OSKDriverBindingStop,
    0x01,
    NULL,
    NULL
};

/**
Allocates working buffers for managing screen assets.

@param      None.

@retval     EFI_SUCCESS     Successfully allocated required buffers.
**/
EFI_STATUS
AllocateBackBuffers (VOID)
{

    // Compute maximum (to screen limits) keyboard dimensions possible (including rotation scenarios).
    //
    UINTN Width, Height;
    if (mGop->Mode->Info->HorizontalResolution > mGop->Mode->Info->VerticalResolution)
    {
        Width  = mGop->Mode->Info->HorizontalResolution;        // Landscape
        Height = mGop->Mode->Info->VerticalResolution;
    }
    else
    {
        Width  = mGop->Mode->Info->VerticalResolution;          // Portrait
        Height = mGop->Mode->Info->HorizontalResolution;
    }

    mOSK.KeyboardMaxWidth  = Width;
    mOSK.KeyboardMaxHeight = (UINTN)((mOSK.KeyboardRectOriginal.botR.pt.y / mOSK.KeyboardRectOriginal.botR.pt.x) * (float)Width);

    // Allocate back buffer and capture buffer.
    //
    UINTN BufferSize = (mOSK.KeyboardMaxWidth * mOSK.KeyboardMaxHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    if (NULL != mOSK.pBackBuffer) {
        FreePool (mOSK.pBackBuffer);
    }
    mOSK.pBackBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)AllocatePool(BufferSize);
    ASSERT (mOSK.pBackBuffer != NULL);

    // Allocate string rendering buffer.
    //
    mOSK.pKeyTextBltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    ASSERT (mOSK.pKeyTextBltBuffer != NULL);

    if (NULL != mOSK.pKeyTextBltBuffer)
    {
        // Define current display resolution.
        //
        mOSK.pKeyTextBltBuffer->Width        = (UINT16)mGop->Mode->Info->HorizontalResolution;
        mOSK.pKeyTextBltBuffer->Height       = (UINT16)mGop->Mode->Info->VerticalResolution;
        mOSK.pKeyTextBltBuffer->Image.Screen = mGop;
    }

    return ((NULL != mOSK.pBackBuffer && NULL != mOSK.pKeyTextBltBuffer) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES);
}


/**
Calculates the bitmap width and height of the specified text string based on the current font size & style.

@param      None.

@retval     EFI_SUCCESS     Successfully calculated all key label sizes
**/
STATIC
EFI_STATUS
LocalGetTextStringBitmapSize (IN CHAR16    *pString,
OUT UINTN    *Width,
OUT UINTN    *Height)
{
    EFI_STATUS              Status = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO   *StringInfo;
    EFI_IMAGE_OUTPUT        *pBltBuffer;
    EFI_HII_ROW_INFO        *pStringRowInfo;
    UINTN                   RowInfoSize;


    // Set default values.
    //
    *Width  = MsUiGetMediumFontWidth();
    *Height = MsUiGetMediumFontHeight();

    // Get the current preferred font size and style (selected based on current display resolution).
    //
    StringInfo = BuildFontDisplayInfoFromFontInfo (&mOSK.PreferredFontInfo);
    if (NULL == StringInfo) {
        return EFI_OUT_OF_RESOURCES;
    }
    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    // Set to NULL to have a buffers allocated for us.
    //
    pBltBuffer      = NULL;
    pStringRowInfo  = NULL;

    // Draw the key label into a bitmap in order to get width and height.
    //
    Status = mSWMProtocol->StringToWindow (mSWMProtocol,
        mImageHandle,
        EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_IGNORE_LINE_BREAK,       // NOTE: clipping isn't possible when rendering to a bitmap buffer.
        pString,
        StringInfo,
        &pBltBuffer,
        0,
        0,
        &pStringRowInfo,
        &RowInfoSize,
        NULL
        );

    // Store the label size based on results.
    //
    if (EFI_SUCCESS == Status && NULL != pStringRowInfo)
    {
        // We know key label is only a single row.  Get the width and height.
        //
        *Width  = pStringRowInfo->LineWidth;
        *Height = pStringRowInfo->LineHeight;
    }

    // Free the buffers allocated by StringToWindow.
    //
    if (NULL != pBltBuffer && NULL != pBltBuffer->Image.Bitmap)
    {
        FreePool(pBltBuffer->Image.Bitmap);
    }
    if (NULL != pBltBuffer)
    {
        FreePool(pBltBuffer);
    }
    if (NULL != pStringRowInfo)
    {
        FreePool(pStringRowInfo);
    }
    if (NULL != StringInfo) {
        FreePool (StringInfo);
    }

    return Status;
}


/**
Calculates the width and height of all key labels based on current font size/style.

@param      None.

@retval     EFI_SUCCESS     Successfully calculated all key label sizes
**/
STATIC
EFI_STATUS
CalculateKeyLabelSizes (VOID)
{
    EFI_STATUS          Status = EFI_SUCCESS;
    UINTN               TableCount;
    UINTN               KeyCount;
    OSK_KEY_MAPPING     *pKeyMap;
    VOID                *Collection[] = { mOSK_StdMode_US_EN,        // NOTE: US-EN only supported at the moment.
        mOSK_ShiftMode_US_EN,
        mOSK_NumSymMode_US_EN,
        mOSK_FnctMode_US_EN,
        NULL
    };


    // Walk through each mapping table in the collection to update key label sizes.
    //
    for (TableCount=0 ; Collection[TableCount] != NULL ; TableCount++)
    {
        // Get the key mapping table.
        //
        pKeyMap = Collection[TableCount];

        // Walk through each key in the mapping table to update key label sizes.
        //
        for (KeyCount=0 ; KeyCount < NUMBER_OF_KEYS ; KeyCount++)
        {
            LocalGetTextStringBitmapSize(pKeyMap[KeyCount].KeyLabel,
                &pKeyMap[KeyCount].KeyLabelWidth,
                &pKeyMap[KeyCount].KeyLabelHeight
                );
        }
    }

    return Status;
}


/**
Calculates the width and height of all special OSK button labels based on current font size/style.

@param      None.

@retval     EFI_SUCCESS     Successfully calculated all key label sizes
**/
STATIC
EFI_STATUS
CalculateSpecialButtonSizes (VOID)
{
    EFI_STATUS              Status = EFI_SUCCESS;


    // Keyboard close button.
    //
    mOSK.KeyboardCloseButton.pBitmap = NULL;     // Using Unicode character.
    LocalGetTextStringBitmapSize(mCloseButtonLabel,
        &mOSK.KeyboardCloseButton.Width,
        &mOSK.KeyboardCloseButton.Height
        );

    // Keyboard dock button.
    //
    mOSK.KeyboardDockButton.pBitmap = NULL;     // Using Unicode character.
    LocalGetTextStringBitmapSize(mDockButtonLabel,
        &mOSK.KeyboardDockButton.Width,
        &mOSK.KeyboardDockButton.Height
        );

    // Keyboard undock button.
    //
    mOSK.KeyboardUndockButton.pBitmap = NULL;     // Using Unicode character.
    LocalGetTextStringBitmapSize(mUndockButtonLabel,
        &mOSK.KeyboardUndockButton.Width,
        &mOSK.KeyboardUndockButton.Height
        );

    return Status;
}


/**
Recalculates OSK assets when the screen resolution changes.

@param      None.

@retval     EFI_SUCCESS     Success.
**/
EFI_STATUS
HandleDisplayModeChange (UINT32 ScreenWidth,
UINT32 ScreenHeight)
{
    EFI_STATUS              Status              = EFI_SUCCESS;
    BOOLEAN                 bShowKeyboardIcon   = mOSK.bDisplayKeyboardIcon;
    BOOLEAN                 bShowKeyboard       = mOSK.bDisplayKeyboard;
    SWM_RECT                Rect;


    DEBUG((DEBUG_INFO, "INFO [OSK]: Display mode change detected (Old=%dx%d  New=%dx%d).\r\n", mOSK.ScreenResolutionWidth, mOSK.ScreenResolutionHeight, ScreenWidth, ScreenHeight));

    AllocateBackBuffers ();

    // Hide keyboard and icon.
    //
    ShowKeyboard(FALSE);
    ShowKeyboardIcon(FALSE);

    // Select screen size-appropriate bitmaps.
    //
    if (ScreenWidth >= SMALL_ASSET_MAX_SCREEN_WIDTH)
    {
        // Keyboard icon.
        mOSK.KeyboardIcon.pBitmap               = gKeyboardIcon_Medium;
        mOSK.KeyboardIcon.Width                 = KEYBOARD_ICON_BMPWIDTH_MEDIUM;
        mOSK.KeyboardIcon.Height                = KEYBOARD_ICON_BMPHEIGHT_MEDIUM;

        // Select preferred font display large size/format.
        mOSK.PreferredFontInfo.FontSize         = MsUiGetMediumFontHeight();
        mOSK.PreferredFontInfo.FontStyle        = EFI_HII_FONT_STYLE_NORMAL;
    }
    else
    {
        // Keyboard icon.
        mOSK.KeyboardIcon.pBitmap               = gKeyboardIcon_Small;
        mOSK.KeyboardIcon.Width                 = KEYBOARD_ICON_BMPWIDTH_SMALL;
        mOSK.KeyboardIcon.Height                = KEYBOARD_ICON_BMPHEIGHT_SMALL;

        // Select preferred font display small size/format.
        mOSK.PreferredFontInfo.FontSize         = MsUiGetSmallOSKFontHeight();
        mOSK.PreferredFontInfo.FontStyle        = EFI_HII_FONT_STYLE_NORMAL;
    }

    // Recalculate key label sizes based on current font.
    //
    CalculateKeyLabelSizes();

    // Recalculate special button sizes based on current font.
    //
    CalculateSpecialButtonSizes();

    // Recalculate all on-screen geometries relative to the current display resolution.
    //
    SetKeyboardSize(mOSK.PercentOfScreenWidth);
    SetKeyboardPosition(mOSK.KeyboardPosition, mOSK.DockedState);
    RotateKeyboard(mOSK.KeyboardAngle);

    SetKeyboardIconPosition(mOSK.KeyboardIconPosition);

    // Set the appropriate window frame (bounding rectangle) and display as appropriate.
    //
    if (TRUE == bShowKeyboardIcon)          // *** Show Keyboard Icon ***
    {
        GetKeyboardIconBoundingRect (&Rect);

        mSWMProtocol->SetWindowFrame(mSWMProtocol,
            mImageHandle,
            (SWM_RECT *)&Rect
            );

        ShowKeyboardIcon(TRUE);
    }
    else if (TRUE == bShowKeyboard)         // *** Show Keyboard ***
    {
        GetKeyboardBoundingRect (&Rect);

        mSWMProtocol->SetWindowFrame(mSWMProtocol,
            mImageHandle,
            (SWM_RECT *)&Rect
            );

        ShowKeyboard(TRUE);
    }

    // Capture the screen resolution used to compute location and size of keyboard assets.
    //
    mOSK.ScreenResolutionWidth  = mGop->Mode->Info->HorizontalResolution;
    mOSK.ScreenResolutionHeight = mGop->Mode->Info->VerticalResolution;

    return Status;
}


/**
Initialize the default keyboard context.

@param      None.

@retval EFI_SUCCESS     This function always complete successfully.

**/
EFI_STATUS
InitializeKeyboardContext (VOID)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Configure key press input queue initial state
    //
    mOSK.bQueueEmpty                        = TRUE;
    mOSK.QueueInputPosition                 = 0;
    mOSK.QueueOutputPosition                = 0;

    // Configure initial keyboard display state
    //
    mOSK.bKeyboardMoving                    = FALSE;
    mOSK.bKeyboardIconAutoEnable            = FALSE;
    mOSK.bKeyboardSelfRefresh               = FALSE;
    mOSK.bDisplayKeyboardIcon               = FALSE;
    mOSK.bDisplayKeyboard                   = FALSE;
    mOSK.bKeyboardStateChanged              = FALSE;
    mOSK.bKeyboardSizeChanged               = TRUE;
    mOSK.bShowDockAndCloseButtons           = TRUE;

    // Key selection state.
    //
    mOSK.SelectedKey                        = NUMBER_OF_KEYS;
    mOSK.DeselectKey                        = NUMBER_OF_KEYS;

    // Configure default docking state
    //
    mOSK.DockedState                        = Docked;

    // Set default keyboard icon location
    //
    mOSK.KeyboardIconPosition               = DEFAULT_OSK_ICON_LOCATION;

    // Set default keyboard position, angle, and size.
    //
    mOSK.KeyboardPosition                   = DEFAULT_OSK_LOCATION;
    mOSK.KeyboardAngle                      = DEFAULT_OSK_ANGLE;
    mOSK.PercentOfScreenWidth               = DEFAULT_OSK_SIZE;

    // Set default keyboard bitmaps (by default choose small format).
    //
    mOSK.KeyboardIcon.pBitmap               = gKeyboardIcon_Small;
    mOSK.KeyboardIcon.Width                 = KEYBOARD_ICON_BMPWIDTH_SMALL;
    mOSK.KeyboardIcon.Height                = KEYBOARD_ICON_BMPHEIGHT_SMALL;

    mOSK.KeyboardCloseButton.pBitmap        = NULL;
    mOSK.KeyboardCloseButton.Width          = 0;
    mOSK.KeyboardCloseButton.Height         = 0;

    mOSK.KeyboardDockButton.pBitmap         = NULL;
    mOSK.KeyboardDockButton.Width           = 0;
    mOSK.KeyboardDockButton.Height          = 0;

    mOSK.KeyboardUndockButton.pBitmap       = NULL;
    mOSK.KeyboardUndockButton.Width         = 0;
    mOSK.KeyboardUndockButton.Height        = 0;

    // Set default custom font size/style (by default choose small format).
    //
    mOSK.PreferredFontInfo.FontSize         = MsUiGetSmallFontHeight();
    mOSK.PreferredFontInfo.FontStyle        = EFI_HII_FONT_STYLE_NORMAL;

    // NOTE: A font name cannot be specified unless there is space allocated for
    //       the name.  See OnScreenKeyboard.h for more info.
    mOSK.PreferredFontInfo.FontName[0]      = L'\0';

    return Status;
}


/**
    Creates the initial keyboard layout irrespective of screen dimensions/restrictions.

    @param      None.

    @retval EFI_SUCCESS     Success.

**/
EFI_STATUS
InitializeKeyboardGeometry (VOID)
{
    UINTN KeyCount;
    float KeyOrigX, KeyOrigY;
    float KeyWidth, KeyHeight;
    float KeySpacing;


    // Configure default key mapping table (US-EN)
    //
    mOSK.pKeyMap = mOSK_StdMode_US_EN;

    // Keyboard origin is (0,0,0) however it may be translated to another location for rendering
    //
    KeyOrigX   = (INDENT_SPACING_PERCENT * STANDARD_KEY_WIDTH);
    KeySpacing = (KEY_SPACING_PERCENT    * STANDARD_KEY_WIDTH);
    KeyOrigY   = ((TOP_BORDER_HEIGHT_PERCENT * STANDARD_KEY_HEIGHT) + KeySpacing);

    for (KeyCount=0 ; KeyCount < NUMBER_OF_KEYS ; KeyCount++)
    {
        // Determine key size
        //
        switch(mOSK.pKeyMap[KeyCount].EfiKey)
        {
        case EfiKeyBackSpace:   // Backspace
            KeyWidth  = (STANDARD_KEY_WIDTH * BKSP_KEY_WIDTH_PERCENT);
            KeyHeight = STANDARD_KEY_HEIGHT;
            break;
        case EfiKeyEnter:       // Enter
            KeyWidth  = (STANDARD_KEY_WIDTH * ENTER_KEY_WIDTH_PERCENT);
            KeyHeight = STANDARD_KEY_HEIGHT;
            break;
        case EfiKeySpaceBar:    // Space
            KeyWidth  = (STANDARD_KEY_WIDTH * SPACE_KEY_WIDTH_PERCENT);
            KeyHeight = STANDARD_KEY_HEIGHT;
            break;
        default:                // All other keys
            KeyWidth  = STANDARD_KEY_WIDTH;
            KeyHeight = STANDARD_KEY_HEIGHT;
            break;
        }

        // Compute key bounding box
        //
        mOSK.KeyRectOriginal[KeyCount].topL.pt.x    = KeyOrigX;
        mOSK.KeyRectOriginal[KeyCount].topL.pt.y    = KeyOrigY;
        mOSK.KeyRectOriginal[KeyCount].topL.pt.z    = 0.0;
        mOSK.KeyRectOriginal[KeyCount].topL.pt.rsvd = 1.0;

        mOSK.KeyRectOriginal[KeyCount].topR.pt.x    = (KeyOrigX + KeyWidth);
        mOSK.KeyRectOriginal[KeyCount].topR.pt.y    = KeyOrigY;
        mOSK.KeyRectOriginal[KeyCount].topR.pt.z    = 0.0;
        mOSK.KeyRectOriginal[KeyCount].topR.pt.rsvd = 1.0;

        mOSK.KeyRectOriginal[KeyCount].botL.pt.x    = KeyOrigX;
        mOSK.KeyRectOriginal[KeyCount].botL.pt.y    = (KeyOrigY + KeyHeight);
        mOSK.KeyRectOriginal[KeyCount].botL.pt.z    = 0.0;
        mOSK.KeyRectOriginal[KeyCount].botL.pt.rsvd = 1.0;

        mOSK.KeyRectOriginal[KeyCount].botR.pt.x    = (KeyOrigX + KeyWidth);
        mOSK.KeyRectOriginal[KeyCount].botR.pt.y    = (KeyOrigY + KeyHeight);
        mOSK.KeyRectOriginal[KeyCount].botR.pt.z    = 0.0;
        mOSK.KeyRectOriginal[KeyCount].botR.pt.rsvd = 1.0;

        // Determine next row indent
        //
        switch(mOSK.pKeyMap[KeyCount].EfiKey)
        {
        case EfiKeyBackSpace:   // Backspace
            KeyOrigX  = (INDENT2_SPACING_PERCENT * STANDARD_KEY_WIDTH);
            KeyOrigY += (KeyHeight + KeySpacing);
            break;
        case EfiKeyEnter:       // Enter
        case EfiKeyRShift:      // Right Shift
            KeyOrigX  = (INDENT_SPACING_PERCENT * STANDARD_KEY_WIDTH);
            KeyOrigY += (KeyHeight + KeySpacing);
            break;
        default:                // All other keys
            KeyOrigX += (KeyWidth + KeySpacing);
            break;
        }
    }

    // Compute keyboard bounding box
    //
    mOSK.KeyboardRectOriginal.topL.pt.x    = 0.0;
    mOSK.KeyboardRectOriginal.topL.pt.y    = 0.0;
    mOSK.KeyboardRectOriginal.topL.pt.z    = 0.0;
    mOSK.KeyboardRectOriginal.topL.pt.rsvd = 1.0;

    mOSK.KeyboardRectOriginal.topR.pt.x    = (KeyOrigX - KeySpacing + (RIGHT_SPACING_PERCENT * STANDARD_KEY_WIDTH));
    mOSK.KeyboardRectOriginal.topR.pt.y    = 0.0;
    mOSK.KeyboardRectOriginal.topR.pt.z    = 0.0;
    mOSK.KeyboardRectOriginal.topR.pt.rsvd = 1.0;

    mOSK.KeyboardRectOriginal.botL.pt.x    = 0.0;
    mOSK.KeyboardRectOriginal.botL.pt.y    = (KeyOrigY + KeyHeight + KeySpacing);
    mOSK.KeyboardRectOriginal.botL.pt.z    = 0.0;
    mOSK.KeyboardRectOriginal.botL.pt.rsvd = 1.0;

    mOSK.KeyboardRectOriginal.botR.pt.x    = (KeyOrigX - KeySpacing + (RIGHT_SPACING_PERCENT * STANDARD_KEY_WIDTH));
    mOSK.KeyboardRectOriginal.botR.pt.y    = (KeyOrigY + KeyHeight + KeySpacing);
    mOSK.KeyboardRectOriginal.botR.pt.z    = 0.0;
    mOSK.KeyboardRectOriginal.botR.pt.rsvd = 1.0;

    // Compute Un/Dock & Close button center points
    //
    mOSK.DockingButtonOriginal.pt.x        = (mOSK.KeyboardRectOriginal.topR.pt.x * DOCK_BUTTON_X_PERCENT);
    mOSK.DockingButtonOriginal.pt.y        = (float)((TOP_BORDER_HEIGHT_PERCENT * STANDARD_KEY_HEIGHT) / 2);
    mOSK.DockingButtonOriginal.pt.z        = 0.0;
    mOSK.DockingButtonOriginal.pt.rsvd     = 1.0;

    mOSK.CloseButtonOriginal.pt.x          = (mOSK.KeyboardRectOriginal.topR.pt.x * CLOSE_BUTTON_X_PERCENT);
    mOSK.CloseButtonOriginal.pt.y          = (float)((TOP_BORDER_HEIGHT_PERCENT * STANDARD_KEY_HEIGHT) / 2);
    mOSK.CloseButtonOriginal.pt.z          = 0.0;
    mOSK.CloseButtonOriginal.pt.rsvd       = 1.0;

    // Copy original keyboard pointsets to display-ready pointsets.  Since screen and touch coordinate systems don't change with
    // keyboard rotation angle, the display-ready pointsets are used to compensate and allow blit and touch point hit detect routines
    // to function as normal despite possible keyboard rotation angle changes.
    //
    NormalizeKeyRectsForRendering(mOSK.KeyboardAngle);

    // Allocate Capture, Back, and String blt buffers
    //
    return (AllocateBackBuffers());
}


/**
    Updates the "hit rectangle" for each key, used to determine key selection.  The area is computed based on the
    currently applied display transform.

    @param[out]     pKeyList            Pointer to updated keys.
    @param[in]      pTransformRectSet   Pointer to the list of transformed key rectangles.
    @param[in]      NumberOfKeys        Total number of keys in the list.

    @retval         None.

**/
VOID
UpdateKeyDisplayHitRect (OUT KEY_INFO *pKeyList,
                         IN  RECT3D   *pTransformRectSet,
                         IN  UINTN     NumberOfKeys)
{
    UINTN    Count;

    for (Count=0 ; Count < NumberOfKeys ; Count++)
    {
        pKeyList[Count].KeyDisplayHitRect.Left   = (UINTN)pTransformRectSet[Count].topL.pt.x;
        pKeyList[Count].KeyDisplayHitRect.Top    = (UINTN)pTransformRectSet[Count].topL.pt.y;
        pKeyList[Count].KeyDisplayHitRect.Right  = (UINTN)pTransformRectSet[Count].botR.pt.x;
        pKeyList[Count].KeyDisplayHitRect.Bottom = (UINTN)pTransformRectSet[Count].botR.pt.y;
    }

    return;
}


/**
    Initialize default key information.

    @param[out]     pKeyList            Pointer to updated keys.
    @param[in]      pTransformRectSet   Pointer to the list of transformed key rectangles.
    @param[in]      NumberOfKeys        Total number of keys in the list.

    @retval         None.

**/
VOID
InitializeKeyInformation(OUT KEY_INFO *pKeyList,
                         IN  RECT3D   *pTransformRectSet,
                         IN  UINTN     NumberOfKeys)
{
    UINTN     Count;
    KEY_INFO *pKey;

    for (Count=0 ; Count < NumberOfKeys ; Count++)
    {
        pKey = &pKeyList[Count];

        // Select key text and fill colors
        //
        pKey->pKeyLabelColor   = &gMsColorTable.KeyLabelColor;
        switch(mOSK.pKeyMap[Count].EfiKey)
        {
        case EfiKeyLShift:
        case EfiKeyRShift:
        case EfiKeyA0:
        case EfiKeyA2:
        case EfiKeyUpArrow:
        case EfiKeyDownArrow:
        case EfiKeyLeftArrow:
        case EfiKeyRightArrow:
            pKey->pKeyFillColor = &gMsColorTable.KeyShiftnNavFillColor;
            break;
        default:
            pKey->pKeyFillColor = &gMsColorTable.KeyDefaultFillColor;
            break;
        }

        pKey->pKeyBoundingRect  = &pTransformRectSet[Count];
    }

    return;
}


/**
    Apply the current transform matrix to all keyboard pointsets.

    @param[in]      bKeyboardFrameOnly  Apply the transform only to the keyboard outer frame (i.e., used then dragging the keyboard).

    @retval         None.

**/
VOID
Apply3DTransform (IN BOOLEAN bKeyboardFrameOnly)
{

    // Transform keyboard bounding rectangle pointset
    //
    TransformPointSet((POINT3D *)&mOSK.KeyboardRectDisplay, (POINT3D *)&mOSK.KeyboardRectXformed, 4);

    // Optimization - when the keyboard is dragged, no need to transform everything until dragging stops
    //
    if (FALSE == bKeyboardFrameOnly)
    {
        // Transform the key pointset
        //
        TransformPointSet((POINT3D *)mOSK.KeyRectDisplay, (POINT3D *)mOSK.KeyRectXformed, (4 * NUMBER_OF_KEYS));

        // Transform Un/Dock and Close button points
        //
        TransformPointSet((POINT3D *)&mOSK.CloseButtonDisplay, (POINT3D *)&mOSK.CloseButtonXformed, 1);
        TransformPointSet((POINT3D *)&mOSK.DockingButtonDisplay, (POINT3D *)&mOSK.DockingButtonXformed, 1);

        // Update individual key "hit" rectangles for matching against touch/mouse coordinate
        //
        UpdateKeyDisplayHitRect(mOSK.KeyList, (RECT3D *)mOSK.KeyRectXformed, NUMBER_OF_KEYS);
    }

    return;
}


/**
    Gets the current keyboard icon bounding (outer) rectangle.

    @param[out]     pRect       Keyboard icon bounding rectangle (Left, Top, Right, Bottom)

    @retval         None.

**/
VOID
GetKeyboardIconBoundingRect (OUT SWM_RECT    *pRect)
{
    UINT32 ScreenWidth  = mGop->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = mGop->Mode->Info->VerticalResolution;
    UINT32 IconOrigX    = 0;
    UINT32 IconOrigY    = 0;
    UINT32 IconWidth    = (UINT32)mOSK.KeyboardIcon.Width;
    UINT32 IconHeight   = (UINT32)mOSK.KeyboardIcon.Height;


    // Compute icon screen coordinate based on icon position.
    //
    switch (mOSK.KeyboardIconPosition)
    {
    case BottomLeft:
        IconOrigX = 0;
        IconOrigY = (ScreenHeight - IconHeight);
        break;
    case TopRight:
        IconOrigX = (ScreenWidth  - IconWidth);
        IconOrigY = 0;
        break;
    case TopLeft:
        IconOrigX = 0;
        IconOrigY = 0;
        break;
    case BottomRight:
    default:
        IconOrigX = (ScreenWidth  - IconWidth);
        IconOrigY = (ScreenHeight - IconHeight);
        break;
    }

    pRect->Left   = IconOrigX;
    pRect->Top    = IconOrigY;
    pRect->Right  = (IconOrigX + IconWidth  - 1);
    pRect->Bottom = (IconOrigY + IconHeight - 1);

    return;
}


/**
Gets the current keyboard bounding (outer) rectangle.

@param[out]     pRect       Keyboard bounding rectangle (Left, Top, Right, Bottom)

@retval         None.

**/
VOID
GetKeyboardBoundingRect (OUT SWM_RECT    *pRect)
{
    pRect->Left   = (UINT32)mOSK.KeyboardRectXformed.topL.pt.x;
    pRect->Top    = (UINT32)mOSK.KeyboardRectXformed.topL.pt.y;
    pRect->Right  = (UINT32)mOSK.KeyboardRectXformed.topR.pt.x;
    pRect->Bottom = (UINT32)mOSK.KeyboardRectXformed.botL.pt.y;

    return;
}


/**
Renders the keyboard.

@param[in]      bShowKeyLabels      Indicates whether or not to display key labels when rendering the keys.

@retval         EFI_SUCCESS         Successfully rendered the keyboard.

**/
EFI_STATUS
RenderKeyboard (IN BOOLEAN bShowKeyLabels)
{
    EFI_STATUS              Status = EFI_SUCCESS;
    UINTN                   KeyOrigX, KeyOrigY, KeyWidth, KeyHeight;
    SWM_RECT                Rect;
    UINT32                  KeyboardWidth, KeyboardHeight;
    UINTN                   KeyLabelOrigX, KeyLabelOrigY;
    UINTN                   Count;
    EFI_FONT_DISPLAY_INFO   *StringInfo = NULL;


    // First check whether there's something to do.
    //
    if (FALSE == mOSK.bDisplayKeyboard)
    {
        goto Exit;
    }

    StringInfo = BuildFontDisplayInfoFromFontInfo (&mOSK.PreferredFontInfo);
    if (NULL == StringInfo)
    {
        goto Exit;
    }

    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    // Determine the keyboard outer bounding rectangle
    //
    GetKeyboardBoundingRect(&Rect);
    KeyboardWidth   = (Rect.Right - Rect.Left + 1);
    KeyboardHeight  = (Rect.Bottom - Rect.Top + 1);

    // If the keyboard hasn't (visually) changed, we can just blt the captured buffer for better performance
    //
    if (FALSE == mOSK.bKeyboardSizeChanged && FALSE == mOSK.bKeyboardStateChanged && NUMBER_OF_KEYS == mOSK.SelectedKey)
    {
        mSWMProtocol->BltWindow (mSWMProtocol,
            mImageHandle,
            mOSK.pBackBuffer,
            EfiBltBufferToVideo,
            0,
            0,
            Rect.Left,
            Rect.Top,
            KeyboardWidth,
            KeyboardHeight,
            KeyboardWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
            );

        goto Exit;
    }

    // If the keyboard has changed dimensionally, re-render the background and (optionally) the buttons otherwise
    // we can continue to refresh the keyboard image with a single, stored blit buffer
    //
    if (TRUE == mOSK.bKeyboardSizeChanged)
    {
        // Draw a near-black box where the keyboard will be rendered
        //
        mSWMProtocol->BltWindow (mSWMProtocol,
            mImageHandle,
            &gMsColorTable.KeyboardSizeChangeBackgroundColor,
            EfiBltVideoFill,
            0,
            0,
            Rect.Left,
            Rect.Top,
            KeyboardWidth,
            (Rect.Bottom - Rect.Top + 1),
            KeyboardWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
            );

        // Draw close and docking buttons.
        //
        if (TRUE == mOSK.bShowDockAndCloseButtons)
        {
            CHAR16  *pButtonLabel;
            UINTN   ButtonWidth, ButtonHeight;
            UINTN   ButtonOrigX, ButtonOrigY;


            // Select preferred font size and style for these buttons.
            //
            CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyLabelColor,                     sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardDocknCloseBackgroundColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

            // Draw the Un/Dock button
            //
            if (Docked == mOSK.DockedState)
            {
                pButtonLabel = mDockButtonLabel;
                ButtonWidth  = mOSK.KeyboardDockButton.Width;
                ButtonHeight = mOSK.KeyboardDockButton.Height;
            }
            else
            {
                pButtonLabel = mUndockButtonLabel;
                ButtonWidth  = mOSK.KeyboardUndockButton.Width;
                ButtonHeight = mOSK.KeyboardUndockButton.Height;
            }

            ButtonOrigX = (UINTN)(mOSK.DockingButtonXformed.pt.x - (float)(ButtonWidth  / 2));
            ButtonOrigY = (UINTN)(mOSK.DockingButtonXformed.pt.y - (float)(ButtonHeight / 2));

            mSWMProtocol->StringToWindow (mSWMProtocol,
                mImageHandle,
                EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_OUT_FLAG_CLIP |
                EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                pButtonLabel,
                StringInfo,
                &mOSK.pKeyTextBltBuffer,
                ButtonOrigX,
                ButtonOrigY,
                NULL,
                NULL,
                NULL
                );

            // Draw the Close button
            //
            pButtonLabel = mCloseButtonLabel;
            ButtonWidth  = mOSK.KeyboardCloseButton.Width;
            ButtonHeight = mOSK.KeyboardCloseButton.Height;
            ButtonOrigX  = (UINTN)(mOSK.CloseButtonXformed.pt.x - (float)(ButtonWidth  / 2));
            ButtonOrigY  = (UINTN)(mOSK.CloseButtonXformed.pt.y - (float)(ButtonHeight / 2));

            mSWMProtocol->StringToWindow (mSWMProtocol,
                mImageHandle,
                EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_OUT_FLAG_CLIP |
                EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                pButtonLabel,
                StringInfo,
                &mOSK.pKeyTextBltBuffer,
                ButtonOrigX,
                ButtonOrigY,
                NULL,
                NULL,
                NULL
                );

        }
    }

    // Draw each of the individual keys based on key mapping, keyboard modifier state, and color scheme
    //
    for (Count=0 ; Count < NUMBER_OF_KEYS ; Count++)
    {
        // Optimization - if the keyboard size and state aren't being changed then we only need to draw the currently selected key
        // or the previously selected (now deselected) key.
        //
        if (FALSE == mOSK.bKeyboardSizeChanged && FALSE == mOSK.bKeyboardStateChanged && Count != mOSK.SelectedKey && Count != mOSK.DeselectKey)
        {
            // Only process the key(s) that are selected or need to be deselected if the keyboard isn't changing.
            //
            continue;
        }

        KeyWidth  = (mOSK.KeyList[Count].KeyDisplayHitRect.Right  - mOSK.KeyList[Count].KeyDisplayHitRect.Left);
        KeyHeight = (mOSK.KeyList[Count].KeyDisplayHitRect.Bottom - mOSK.KeyList[Count].KeyDisplayHitRect.Top);
        KeyOrigX  =  mOSK.KeyList[Count].KeyDisplayHitRect.Left;
        KeyOrigY  =  mOSK.KeyList[Count].KeyDisplayHitRect.Top;

        // Fill the key background with the correct color based on state
        //
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pFillColor = mOSK.KeyList[Count].pKeyFillColor;

        if (Shift == mOSK.KeyModifierState && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
        {
            pFillColor = &gMsColorTable.KeyboardShiftStateKeyColor;
        }
        else if (CapsLock == mOSK.KeyModifierState && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
        {
            pFillColor = &gMsColorTable.KeyboardCapsLockStateKeyColor;
        }
        else if ((NumSym == mOSK.KeyModifierState || Function == mOSK.KeyModifierState) && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
        {
            pFillColor = mOSK.KeyList[Count].pKeyFillColor;
        }
        else if (NumSym == mOSK.KeyModifierState && EfiKeyA0 == mOSK.pKeyMap[Count].EfiKey)
        {
            pFillColor = &gMsColorTable.KeyboardNumSymStateKeyColor;
        }
        else if (Function == mOSK.KeyModifierState && EfiKeyA2 == mOSK.pKeyMap[Count].EfiKey)
        {
            pFillColor = &gMsColorTable.KeyboardFunctionStateKeyColor;
        }
        else if (mOSK.SelectedKey == Count)
        {
            pFillColor = &gMsColorTable.KeyboardSelectedStateKeyColor;
        }

        mSWMProtocol->BltWindow (mSWMProtocol,
            mImageHandle,
            pFillColor,
            EfiBltVideoFill,
            0,
            0,
            KeyOrigX,
            KeyOrigY,
            KeyWidth,
            KeyHeight,
            KeyWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
            );

        // Draw key text if requested
        //
        if (TRUE == bShowKeyLabels)
        {

            // Use correct color for key text, based on state
            //
            if (Shift == mOSK.KeyModifierState && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardShiftStateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardShiftStateBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else if (CapsLock == mOSK.KeyModifierState && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardCapsLockStateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardCapsLockStateBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else if ((NumSym == mOSK.KeyModifierState || Function == mOSK.KeyModifierState) && (EfiKeyLShift == mOSK.pKeyMap[Count].EfiKey || EfiKeyRShift == mOSK.pKeyMap[Count].EfiKey))
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardNumSymStateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));            // Gray-out shift keys in these modes
                CopyMem (&StringInfo->BackgroundColor, mOSK.KeyList[Count].pKeyFillColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else if (NumSym == mOSK.KeyModifierState && EfiKeyA0 == mOSK.pKeyMap[Count].EfiKey)
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardNumSymA0StateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardNumSymA0StateBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else if (Function == mOSK.KeyModifierState && EfiKeyA2 == mOSK.pKeyMap[Count].EfiKey)
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardFunctionStateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardFunctionStateBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else if (mOSK.SelectedKey == Count)
            {
                CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.KeyboardSelectedStateFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.KeyboardSelectedStateBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }
            else
            {
                CopyMem (&StringInfo->ForegroundColor, mOSK.KeyList[Count].pKeyLabelColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
                CopyMem (&StringInfo->BackgroundColor, mOSK.KeyList[Count].pKeyFillColor,  sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            }

            // Select preferred font size/style.
            //
            StringInfo->FontInfoMask   = EFI_FONT_INFO_ANY_FONT;

            // Center the label on the key.
            //
            KeyLabelOrigX = (KeyOrigX + (KeyWidth  / 2) - (mOSK.pKeyMap[Count].KeyLabelWidth  / 2));
            KeyLabelOrigY = (KeyOrigY + (KeyHeight / 2) - (mOSK.pKeyMap[Count].KeyLabelHeight / 2));

            // Draw the key label.
            //
            mSWMProtocol->StringToWindow (mSWMProtocol,
                mImageHandle,
                EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_OUT_FLAG_CLIP |
                EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                mOSK.pKeyMap[Count].KeyLabel,
                StringInfo,
                &mOSK.pKeyTextBltBuffer,
                KeyLabelOrigX,
                KeyLabelOrigY,
                NULL,
                NULL,
                NULL
                );
        }
    }

    // Capture the keyboard to the back buffer so we can directly blt it later if the keyboard hasn't changed.  Note that if it wasn't a full
    // keyboard render, don't capture since the keyboard may have been stepped on by other rendering (ex: Shell).
    //
    if (TRUE == mOSK.bKeyboardSizeChanged && NUMBER_OF_KEYS == mOSK.SelectedKey)
    {
        // Disable the mouse pointer so we don't capture it
        //
        mSWMProtocol->EnableMousePointer(mSWMProtocol,
            FALSE
            );

        // Capture the keyboard to the back buffer.
        //
        mSWMProtocol->BltWindow (mSWMProtocol,
            mImageHandle,
            mOSK.pBackBuffer,
            EfiBltVideoToBltBuffer,
            Rect.Left,
            Rect.Top,
            0,
            0,
            KeyboardWidth,
            KeyboardHeight,
            KeyboardWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
            );

    }

    // Enable the mouse pointer.
    //
    mSWMProtocol->EnableMousePointer(mSWMProtocol,
        TRUE
        );

    // If there is no selected key, we may have just rendered a key deselection.  Now that
    // we're done rendering, clear the deselection state.
    //
    mOSK.DeselectKey = NUMBER_OF_KEYS;

    // Reset keyboard size change tracking flag now that we've rendered and captured the updated keyboard.
    //
    mOSK.bKeyboardSizeChanged = FALSE;

Exit:

    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }

    return Status;
}


/**
Translates the keyboard in the xy plane while enforcing screen boundaries since we don't support clipping.

@param[in]      dx      Relative X-axis offset.
@param[in]      dy      Relative Y-axis offset.

@retval         EFI_SUCCESS         Successfully translated the keyboard's location.

**/
EFI_STATUS
TranslateKeyboardLocation (IN float dx,
IN float dy)
{
    UINTN ScreenWidth    = (UINTN)mGop->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight   = (UINTN)mGop->Mode->Info->VerticalResolution;
    SWM_RECT Rect;


    // Keep keyboard within screen bounds (blitting isn't clipped)
    //
    GetKeyboardBoundingRect (&Rect);
    if (((INTN)Rect.Left + (INTN)dx) < 0)
    {
        dx = (float)Rect.Left;                                              // Limit at current position
    }
    else if ((UINTN)((INTN)(Rect.Right) + (INTN)dx) >= ScreenWidth)
    {
        dx = (float)((INTN)(ScreenWidth - 1) - (INTN)(Rect.Right));         // Limit at screen width
    }

    if (((INTN)Rect.Top + (INTN)dy) < 0)
    {
        dy = (float)Rect.Top;                                               // Limit at current position
    }
    else if ((UINTN)((INTN)(Rect.Bottom) + (INTN)dy) >= ScreenHeight)
    {
        dy = (float)((INTN)(ScreenHeight - 1) - (INTN)(Rect.Bottom));       // Limit at screen height
    }

    // Translate the keyboard position by a relative amount
    //
    //
    Translate(dx, dy, 0.0);

    // Apply the initial transform to the keyboard geometry point set
    //
    Apply3DTransform(TRUE);
    mOSK.bKeyboardStateChanged  = FALSE;  // Force blitting of the stored keyboard image
    mOSK.bKeyboardSizeChanged   = FALSE;  //  "

    // Update client window frame.
    //
    GetKeyboardBoundingRect (&Rect);

    mSWMProtocol->SetWindowFrame(mSWMProtocol,
        mImageHandle,
        (SWM_RECT *)&Rect
        );

    // Render the keyboard with key text
    //
    RenderKeyboard(TRUE);


    return EFI_SUCCESS;
}


EFI_STATUS
SetKeyboardIconPosition (IN SCREEN_POSITION Position)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Unsupported positions
    //
    if (TopCenter == Position || BottomCenter == Position)
    {
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    mOSK.KeyboardIconPosition = Position;

Exit:

    return Status;
}


/**
Sets the keyboard's position on the screen (justified position) and docking state.

@param[in]      Position                Where to render the keyboard on the screen (justified position).
@param[in]      DockedState             Whether to dock the keyboard or not (allow it to be dragged).

@retval         EFI_SUCCESS             Successfully set the keyboard's location.

**/
EFI_STATUS
SetKeyboardPosition (IN SCREEN_POSITION      Position,
IN OSK_DOCKED_STATE     DockedState)
{
    EFI_STATUS Status    = EFI_SUCCESS;

    if (mGop == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK] Cannot set keyboardposition, GOP not yet initialized %x %x\n", Position, DockedState));
        mOSK.KeyboardPosition = Position;
        mOSK.DockedState      = DockedState;
        goto Exit;
    }
    if (mSWMProtocol == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK] Cannot set keyboardposition, SWM protocol not yet initialized %x %x\n", Position, DockedState));
        mOSK.KeyboardPosition = Position;
        mOSK.DockedState = DockedState;
        goto Exit;
    }

    UINTN ScreenWidth    = (UINTN)mGop->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight   = (UINTN)mGop->Mode->Info->VerticalResolution;
    SWM_RECT Rect;
    UINT32 KeyboardWidth, KeyboardHeight;
    SCREEN_POSITION AdjustedPosition;
    float dx = 0.0;
    float dy = 0.0;

    // Get current keyboard location and size
    //
    GetKeyboardBoundingRect(&Rect);
    KeyboardWidth  = (Rect.Right - Rect.Left + 1);
    KeyboardHeight = (Rect.Bottom - Rect.Top + 1);

    // Save keyboard position for later use.
    //
    mOSK.KeyboardPosition = Position;

    // Adjust screen position based on keyboard rotation angle.  Screen position when the keyboard is rotated needs to be transformed
    // into a "universal" non-rotated position.
    //
    switch (Position)
    {
    case BottomLeft:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = TopLeft;
            break;
        case Angle_180:
            AdjustedPosition = TopRight;
            break;
        case Angle_270:
            AdjustedPosition = BottomRight;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    case BottomCenter:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = LeftCenter;
            break;
        case Angle_180:
            AdjustedPosition = TopCenter;
            break;
        case Angle_270:
            AdjustedPosition = RightCenter;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    case BottomRight:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = BottomLeft;
            break;
        case Angle_180:
            AdjustedPosition = TopLeft;
            break;
        case Angle_270:
            AdjustedPosition = TopRight;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    case TopLeft:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = TopRight;
            break;
        case Angle_180:
            AdjustedPosition = BottomRight;
            break;
        case Angle_270:
            AdjustedPosition = BottomLeft;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    case TopRight:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = BottomRight;
            break;
        case Angle_180:
            AdjustedPosition = BottomLeft;
            break;
        case Angle_270:
            AdjustedPosition = TopLeft;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    case TopCenter:
    default:
        switch (mOSK.KeyboardAngle)
        {
        case Angle_90:
            AdjustedPosition = RightCenter;
            break;
        case Angle_180:
            AdjustedPosition = BottomCenter;
            break;
        case Angle_270:
            AdjustedPosition = LeftCenter;
            break;
        case Angle_0:
        default:
            AdjustedPosition = Position;
            break;
        }
        break;
    }

    // Compute x,y screen coordinate based on location specifier
    //
    switch (AdjustedPosition)
    {
    case BottomLeft:
        dx = (float)((INTN)Rect.Left * -1);
        dy = (float)((INTN)(ScreenHeight - KeyboardHeight) - (INTN)Rect.Top);
        break;
    case BottomCenter:
        dx = (float)((INTN)((ScreenWidth - KeyboardWidth) / 2) - (INTN)Rect.Left);
        dy = (float)((INTN)(ScreenHeight - KeyboardHeight) - (INTN)Rect.Top);
        break;
    case BottomRight:
        dx = (float)((INTN)(ScreenWidth - KeyboardWidth) - (INTN)Rect.Left);
        dy = (float)((INTN)(ScreenHeight - KeyboardHeight) - (INTN)Rect.Top);
        break;
    case LeftCenter:
        // TODO
        break;
    case TopLeft:
        dx = (float)((INTN)Rect.Left * -1);
        dy = (float)((INTN)Rect.Top * -1);
        break;
    case TopRight:
        dx = (float)((INTN)(ScreenWidth - KeyboardWidth) - (INTN)Rect.Left);
        dy = (float)((INTN)Rect.Top * -1);
        break;
    case RightCenter:
        // TODO
        break;
    case TopCenter:
    default:
        dx = (float)((INTN)((ScreenWidth - KeyboardWidth) / 2) - (INTN)Rect.Left);
        dy = (float)((INTN)Rect.Top * -1);
        break;
    }

    // Configure dock state
    //
    mOSK.DockedState = DockedState;

    // Translate the keyboard location
    //
    Translate(dx, dy, 0.0);

    // Apply the initial transform to the keyboard geometry point set
    //
    Apply3DTransform(FALSE);

    // Update client window frame.
    //
    GetKeyboardBoundingRect (&Rect);

    mSWMProtocol->SetWindowFrame(mSWMProtocol,
        mImageHandle,
        (SWM_RECT *)&Rect
        );

    // Note that the keyboard size has changed so the renderer can refresh correctly
    //
    mOSK.bKeyboardSizeChanged = TRUE;

    // Render the keyboard with key text if it should be displayed
    //
    if (TRUE == mOSK.bDisplayKeyboard)
    {
        // Render the keyboard in the new position.
        //
        RenderKeyboard(TRUE);
    }
Exit:
    return Status;
}


/**
Sets the keyboard's overall size as a percentage of the total screen "width".

@param[in]      PercentOfScreenWidth        Size of the keyboard specified in percent of screen width (i.e., 0->100).

@retval         EFI_SUCCESS     Successfully set the keyboard's size.

**/
EFI_STATUS
SetKeyboardSize (IN float    PercentOfScreenWidth)
{
    EFI_STATUS Status    = EFI_SUCCESS;
    SWM_RECT Rect;

    if (mGop == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK] GOP not yet initialized. Cannot set the keyboard size. Default size will be retained \n"));
        mOSK.PercentOfScreenWidth = PercentOfScreenWidth;
        goto Exit;
    }
    if (mSWMProtocol == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK] SWM protocol not yet initialized. Cannot set the keyboard size. Default size will be retained \n"));
        mOSK.PercentOfScreenWidth = PercentOfScreenWidth;
        goto Exit;
    }

    UINTN ScreenWidth    = (UINTN)mGop->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight   = (UINTN)mGop->Mode->Info->VerticalResolution;
    UINTN KeyboardWidth  = (UINTN)(mOSK.KeyboardRectOriginal.topR.pt.x - mOSK.KeyboardRectOriginal.topL.pt.x);
    UINTN KeyboardHeight = (UINTN)(mOSK.KeyboardRectOriginal.botL.pt.y - mOSK.KeyboardRectOriginal.topL.pt.y);


    // Compute the maximum keyboard size scale factor based on screen dimensions
    //
    float ScaleFactor = ((float)ScreenWidth / (float)KeyboardWidth);

    // Reduce the scale factor by the amount specified
    //
    ScaleFactor *= (float)PercentOfScreenWidth;

    // Make sure height doens't scale past screen limits since we haven't implemented clipping
    //
    if ((UINTN)((float)KeyboardHeight * ScaleFactor) >= ScreenHeight)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // Save the specified size for later use.
    //
    mOSK.PercentOfScreenWidth = PercentOfScreenWidth;

    // Initialize display transform
    //
    // TODO - Maintain rotation angle and origin when the scale is changed
    //
    InitializeXformWithParams (ScaleFactor,     // Scale
        0.0,             // X-axis angle
        0.0,             // Y-axis angle
        0.0              // Z-axis angle
        );

    // Apply the initial transform to the keyboard geometry point set
    //
    Apply3DTransform(FALSE);

    // Note that the keyboard size has changed so the renderer can refresh correctly
    //
    mOSK.bKeyboardSizeChanged = TRUE;

    // Update client window frame.
    //
    GetKeyboardBoundingRect (&Rect);

    mSWMProtocol->SetWindowFrame(mSWMProtocol,
        mImageHandle,
        (SWM_RECT *)&Rect
        );

    // Render the keyboard with key text if it needs to be displayed
    //
    if (TRUE == mOSK.bDisplayKeyboard)
    {
        // Render the keyboard at the new size.
        //
        RenderKeyboard(TRUE);
    }

Exit:

    return Status;
}


/**
Retrieves the keyboard's current operating mode(s).

@param[in]      *ModeBitfield   Bitfield representing current mode(s).

@retval         EFI_SUCCESS     Successfully retrieved the keyboard's mode.

**/
EFI_STATUS
GetKeyboardMode (IN UINT32  *ModeBitfield)
{
    EFI_STATUS Status    = EFI_SUCCESS;


    *ModeBitfield = 0;

    // Icon auto-enable mode (used to automatically display the OSK icon when a client waits on an input event).
    //
    if (TRUE == mOSK.bKeyboardIconAutoEnable)
    {
        *ModeBitfield |= OSK_MODE_AUTOENABLEICON;
    }

    // Keyboard self-refresh mode (periodically redraws the keyboard).
    //
    if (TRUE == mOSK.bKeyboardSelfRefresh)
    {
        *ModeBitfield |= OSK_MODE_SELF_REFRESH;
    }

    DEBUG((DEBUG_INFO, "INFO [OSK]: Retrieved keyboard mode 0x%08x.  Status = %r\r\n", *ModeBitfield, Status));

    return Status;
}


/**
Sets the keyboard's current operating mode.

@param[in]      ModeBitfield    Bitfield representing keyboard mode selection(s).

@retval         EFI_SUCCESS     Successfully set the keyboard's mode.

**/
EFI_STATUS
SetKeyboardMode (IN UINT32  ModeBitfield)
{
    EFI_STATUS Status    = EFI_SUCCESS;


    // Configure icon auto-enable mode (used to automatically display the OSK icon when a client waits on an input event).
    //
    mOSK.bKeyboardIconAutoEnable = ((ModeBitfield & OSK_MODE_AUTOENABLEICON) ? TRUE : FALSE);

    // Configure keyboard self-refresh mode (periodically redraws the keyboard).
    //
    mOSK.bKeyboardSelfRefresh = ((ModeBitfield & OSK_MODE_SELF_REFRESH) ? TRUE : FALSE);

    DEBUG((DEBUG_INFO, "INFO [OSK]: Set keyboard mode 0x%08x.  Status = %r\r\n", ModeBitfield, Status));
    // Disable the key repeat timer
    //
    gBS->SetTimer (mKeyRepeatTimerEvent,
        TimerCancel,
        0
        );


    return Status;
}


/**
Configures the keyboard docked/undocked state (undocked allows it to be dragged around the screen).

@param[in]  State           Docked state to configure (i.e., docked or undocked).

@retval     EFI_SUCCESS     Successfully undocked the keyboard.

**/
EFI_STATUS
SetKeyboardDockState (IN OSK_DOCKED_STATE State)
{
    EFI_STATUS Status    = EFI_SUCCESS;


    Status = SetKeyboardPosition(mOSK.KeyboardPosition, State);

    return Status;
}


EFI_STATUS
ShowKeyboard (IN BOOLEAN bShowKeyboard)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Disable the key repeat timer
    //
    gBS->SetTimer (mKeyRepeatTimerEvent,
                   TimerCancel,
                   0
                   );

    // First check whether there's something to do.  We don't want to go through the process of
    // showing the keyboard if it's already being showed in order to avoid capturing it in the back-buffer
    // and later restoring the OSK image instead of what underlies it.
    //
    if (bShowKeyboard == mOSK.bDisplayKeyboard)
    {
        goto Exit;
    }

    if (mSWMProtocol == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: SWM protocol not yet initialized. Cannot change ShowKeyboard mode 0x%08x.\n", bShowKeyboard));
        mOSK.bDisplayKeyboard = bShowKeyboard;
        goto Exit;
    }

    if (TRUE == bShowKeyboard)      // *** Show ***
    {
        // Note that the keyboard size has changed so the renderer can refresh correctly
        //
        mOSK.bKeyboardSizeChanged = TRUE;

        // Indicate that we're now showing the keyboard (needs to be set so RenderKeyboard does something).
        //
        mOSK.bDisplayKeyboard = TRUE;

        // Make ourselves active with the window manager now that we're displaying.
        //
        mSWMProtocol->ActivateWindow(mSWMProtocol,
            mImageHandle,
            TRUE
            );

        // Render keyboard with key text
        //
        RenderKeyboard(TRUE);

        // Enable the mouse pointer to be displayed.
        //
        mSWMProtocol->EnableMousePointer(mSWMProtocol,
            TRUE
            );
    }
    else                            // *** Hide ***
    {
        // Indicate that we're no longer showing the keyboard.  Note that this should come *before* we restore the
        // underlying screen to avoid a race condition with RenderKeyboard.
        //
        mOSK.bDisplayKeyboard = FALSE;

        // Make ourselves inactive with the window manager now that we're *not* displaying.
        //
        mSWMProtocol->ActivateWindow(mSWMProtocol,
            mImageHandle,
            FALSE
            );
    }

Exit:
    return (Status);
}


EFI_STATUS
ShowKeyboardIcon (IN BOOLEAN bShowKeyboardIcon)
{
    EFI_STATUS Status   = EFI_SUCCESS;

    if (mGop == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Cannot change ShowKeyboardIcon. GOP not found 0x%08x\n", bShowKeyboardIcon));
        mOSK.bDisplayKeyboardIcon = bShowKeyboardIcon;
        goto Exit;
    }

    if (mSWMProtocol == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: SWM protocol not yet initialized. Cannot change ShowKeyboardIcon 0x%08x\n", bShowKeyboardIcon));
        mOSK.bDisplayKeyboardIcon = bShowKeyboardIcon;
        goto Exit;
    }

    UINT32 ScreenWidth  = mGop->Mode->Info->HorizontalResolution;
    UINT32 ScreenHeight = mGop->Mode->Info->VerticalResolution;
    UINT32 IconOrigX, IconOrigY;
    UINT32 IconWidth    = (UINT32)mOSK.KeyboardIcon.Width;
    UINT32 IconHeight   = (UINT32)mOSK.KeyboardIcon.Height;
    SWM_RECT Rect;


    // First check whether there's something to do (allow both conditions being true to pass
    // through and allow for icon refreshing).
    //
    if ((FALSE == bShowKeyboardIcon && FALSE == mOSK.bDisplayKeyboardIcon))
    {
        goto Exit;
    }

    // Compute screen coordinate based on location specifier
    //
    switch (mOSK.KeyboardIconPosition)
    {
    case BottomLeft:
        IconOrigX = 0;
        IconOrigY = (ScreenHeight - IconHeight);
        break;
    case TopRight:
        IconOrigX = (ScreenWidth  - IconWidth);
        IconOrigY = 0;
        break;
    case TopLeft:
        IconOrigX = 0;
        IconOrigY = 0;
        break;
    case BottomRight:
    default:
        IconOrigX = (ScreenWidth  - IconWidth);
        IconOrigY = (ScreenHeight - IconHeight);
        break;
    }

    // If we hadn't previously been showing the keyboard icon, update the client window frame now.
    //
    if (TRUE == bShowKeyboardIcon)
    {
        if (FALSE == mOSK.bDisplayKeyboardIcon)
        {
            // Update client window frame.
            //
            GetKeyboardIconBoundingRect (&Rect);

            mSWMProtocol->SetWindowFrame(mSWMProtocol,
                mImageHandle,
                (SWM_RECT *)&Rect
                );

            // Set client focus for the icon "window".
            //
            mSWMProtocol->ActivateWindow(mSWMProtocol,
                mImageHandle,
                TRUE
                );
        }

        EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBltBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)mOSK.KeyboardIcon.pBitmap;

        // Display the keyboard icon or blank it it out if we're hiding it.
        //
        mSWMProtocol->BltWindow (mSWMProtocol,
            mImageHandle,
            pBltBuffer,
            EfiBltBufferToVideo,
            0,
            0,
            IconOrigX,
            IconOrigY,
            IconWidth,
            IconHeight,
            IconWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
            );

    }
    else if (FALSE == bShowKeyboardIcon)
    {
        // Set make ourselves inactive (messages will by default go to the default client).
        //
        mSWMProtocol->ActivateWindow(mSWMProtocol,
            mImageHandle,
            FALSE
            );
    }

    // Save keyboard icon display state.
    //
    mOSK.bDisplayKeyboardIcon = bShowKeyboardIcon;

Exit:
    return Status;
}


EFI_STATUS
CheckForKeyboardIconHit (IN UINT32 TouchX,
IN UINT32 TouchY)
{
    SWM_RECT Rect;


    // If the icon isn't being displayed, there's no possibility of selecting it.
    //
    if (FALSE == mOSK.bDisplayKeyboardIcon)
    {
        return EFI_NO_MAPPING;
    }

    GetKeyboardIconBoundingRect (&Rect);


    if (TouchX >= Rect.Left   &&
        TouchX <= Rect.Right  &&
        TouchY >= Rect.Top    &&
        TouchY <= Rect.Bottom )
    {
        return EFI_SUCCESS;
    }

    return EFI_NO_MAPPING;
}


EFI_STATUS
CheckForKeyboardFrameHit (IN UINTN TouchX,
IN UINTN TouchY)
{
    UINTN FrameLeft   = (UINTN)mOSK.KeyboardRectXformed.topL.pt.x;
    UINTN FrameRight  = (UINTN)mOSK.KeyboardRectXformed.topR.pt.x;
    UINTN FrameTop    = (UINTN)mOSK.KeyboardRectXformed.topR.pt.y;
    UINTN FrameBottom = (UINTN)mOSK.KeyboardRectXformed.botL.pt.y;

    if (TouchX >= FrameLeft  &&
        TouchX <= FrameRight &&
        TouchY >= FrameTop   &&
        TouchY <= FrameBottom)
    {
        return EFI_SUCCESS;
    }

    return EFI_NO_MAPPING;
}


EFI_STATUS
CheckForDockingButtonHit (IN UINTN TouchX,
IN UINTN TouchY)
{
    UINTN ButtonLeft   = ((UINTN)mOSK.DockingButtonXformed.pt.x - (mOSK.KeyboardDockButton.Width  / 2));
    UINTN ButtonRight  = ((UINTN)mOSK.DockingButtonXformed.pt.x + (mOSK.KeyboardDockButton.Width  / 2));
    UINTN ButtonTop    = ((UINTN)mOSK.DockingButtonXformed.pt.y - (mOSK.KeyboardDockButton.Height / 2));
    UINTN ButtonBottom = ((UINTN)mOSK.DockingButtonXformed.pt.y + (mOSK.KeyboardDockButton.Height / 2));

    // If the button isn't being displayed, it shouldn't be selectable.
    //
    if (FALSE == mOSK.bShowDockAndCloseButtons)
    {
        goto Exit;
    }

    if (TouchX >= ButtonLeft  &&
        TouchX <= ButtonRight &&
        TouchY >= ButtonTop   &&
        TouchY <= ButtonBottom)
    {
        return EFI_SUCCESS;
    }

Exit:

    return EFI_NO_MAPPING;
}


EFI_STATUS
CheckForCloseButtonHit (IN UINTN TouchX,
IN UINTN TouchY)
{
    UINTN ButtonLeft   = ((UINTN)mOSK.CloseButtonXformed.pt.x - (mOSK.KeyboardCloseButton.Width  / 2));
    UINTN ButtonRight  = ((UINTN)mOSK.CloseButtonXformed.pt.x + (mOSK.KeyboardCloseButton.Width  / 2));
    UINTN ButtonTop    = ((UINTN)mOSK.CloseButtonXformed.pt.y - (mOSK.KeyboardCloseButton.Height / 2));
    UINTN ButtonBottom = ((UINTN)mOSK.CloseButtonXformed.pt.y + (mOSK.KeyboardCloseButton.Height / 2));

    // If the button isn't being displayed, it shouldn't be selectable.
    //
    if (FALSE == mOSK.bShowDockAndCloseButtons)
    {
        goto Exit;
    }

    if (TouchX >= ButtonLeft  &&
        TouchX <= ButtonRight &&
        TouchY >= ButtonTop   &&
        TouchY <= ButtonBottom)
    {
        return EFI_SUCCESS;
    }

Exit:

    return EFI_NO_MAPPING;
}


/**

Copy original keyboard pointsets to display-ready pointsets.  Since screen and touch coordinate systems don't change with
keyboard rotation angle, the display-ready pointsets are used to compensate and allow blit and touch point hit detect routines
to function as normal despite possible keyboard rotation angle changes.

Basically, GOP's blitting function expects width and height to be positive relative to the rect's origin.  Similarly, touch/mouse
coordinate space, used for key hit detection doesn't change with keyboard rotation thus this routine re-orders the rect vertices
such that "top left" is always "top left" in screen and touch coordinate space.

@param[in]      Angle           Keyboard rotation angle (i.e., 0, 90, 180, and 270 degrees).

@retval         EFI_SUCCESS     Successfully "normalized" keyboard rects for screen/touch coordinate space.

**/
EFI_STATUS
NormalizeKeyRectsForRendering(IN SCREEN_ANGLE Angle)
{
    EFI_STATUS Status = EFI_SUCCESS;
    RECT3D Temp;
    UINTN KeyPointCount = 0;


    mOSK.DockingButtonDisplay   = mOSK.DockingButtonOriginal;
    mOSK.CloseButtonDisplay     = mOSK.CloseButtonOriginal;

    Temp.topL = mOSK.KeyboardRectOriginal.topL;
    Temp.topR = mOSK.KeyboardRectOriginal.topR;
    Temp.botL = mOSK.KeyboardRectOriginal.botL;
    Temp.botR = mOSK.KeyboardRectOriginal.botR;

    switch (Angle)
    {
    case Angle_90:
    {
        // Transform keyboard bounding rectangle pointset
        //
        mOSK.KeyboardRectDisplay.topL = Temp.botL;
        mOSK.KeyboardRectDisplay.topR = Temp.topL;
        mOSK.KeyboardRectDisplay.botL = Temp.botR;
        mOSK.KeyboardRectDisplay.botR = Temp.topR;

        // Transform the key pointset
        //
        for (KeyPointCount=0 ; KeyPointCount < NUMBER_OF_KEYS ; KeyPointCount++)
        {
            Temp.topL = mOSK.KeyRectOriginal[KeyPointCount].topL;
            Temp.topR = mOSK.KeyRectOriginal[KeyPointCount].topR;
            Temp.botL = mOSK.KeyRectOriginal[KeyPointCount].botL;
            Temp.botR = mOSK.KeyRectOriginal[KeyPointCount].botR;

            mOSK.KeyRectDisplay[KeyPointCount].topL = Temp.botL;
            mOSK.KeyRectDisplay[KeyPointCount].topR = Temp.topL;
            mOSK.KeyRectDisplay[KeyPointCount].botL = Temp.botR;
            mOSK.KeyRectDisplay[KeyPointCount].botR = Temp.topR;
        }
    }
    break;
    case Angle_180:
    {
        // Transform keyboard bounding rectangle pointset
        //
        mOSK.KeyboardRectDisplay.topL = Temp.botR;
        mOSK.KeyboardRectDisplay.topR = Temp.botL;
        mOSK.KeyboardRectDisplay.botL = Temp.topR;
        mOSK.KeyboardRectDisplay.botR = Temp.topL;

        // Transform the key pointset
        //
        for (KeyPointCount=0 ; KeyPointCount < NUMBER_OF_KEYS ; KeyPointCount++)
        {
            Temp.topL = mOSK.KeyRectOriginal[KeyPointCount].topL;
            Temp.topR = mOSK.KeyRectOriginal[KeyPointCount].topR;
            Temp.botL = mOSK.KeyRectOriginal[KeyPointCount].botL;
            Temp.botR = mOSK.KeyRectOriginal[KeyPointCount].botR;

            mOSK.KeyRectDisplay[KeyPointCount].topL = Temp.botR;
            mOSK.KeyRectDisplay[KeyPointCount].topR = Temp.botL;
            mOSK.KeyRectDisplay[KeyPointCount].botL = Temp.topR;
            mOSK.KeyRectDisplay[KeyPointCount].botR = Temp.topL;
        }
    }
    break;
    case Angle_270:
    {
        // Transform keyboard bounding rectangle pointset
        //
        mOSK.KeyboardRectDisplay.topL = Temp.topR;
        mOSK.KeyboardRectDisplay.topR = Temp.botR;
        mOSK.KeyboardRectDisplay.botL = Temp.topL;
        mOSK.KeyboardRectDisplay.botR = Temp.botL;

        // Transform the key pointset
        //
        for (KeyPointCount=0 ; KeyPointCount < NUMBER_OF_KEYS ; KeyPointCount++)
        {
            Temp.topL = mOSK.KeyRectOriginal[KeyPointCount].topL;
            Temp.topR = mOSK.KeyRectOriginal[KeyPointCount].topR;
            Temp.botL = mOSK.KeyRectOriginal[KeyPointCount].botL;
            Temp.botR = mOSK.KeyRectOriginal[KeyPointCount].botR;

            mOSK.KeyRectDisplay[KeyPointCount].topL = Temp.topR;
            mOSK.KeyRectDisplay[KeyPointCount].topR = Temp.botR;
            mOSK.KeyRectDisplay[KeyPointCount].botL = Temp.topL;
            mOSK.KeyRectDisplay[KeyPointCount].botR = Temp.botL;
        }
    }
    break;
    case Angle_0:
    default:
    {
        // Transform keyboard bounding rectangle pointset
        //
        mOSK.KeyboardRectDisplay.topL = Temp.topL;
        mOSK.KeyboardRectDisplay.topR = Temp.topR;
        mOSK.KeyboardRectDisplay.botL = Temp.botL;
        mOSK.KeyboardRectDisplay.botR = Temp.botR;

        // Transform the key pointset
        //
        for (KeyPointCount=0 ; KeyPointCount < NUMBER_OF_KEYS ; KeyPointCount++)
        {
            Temp.topL = mOSK.KeyRectOriginal[KeyPointCount].topL;
            Temp.topR = mOSK.KeyRectOriginal[KeyPointCount].topR;
            Temp.botL = mOSK.KeyRectOriginal[KeyPointCount].botL;
            Temp.botR = mOSK.KeyRectOriginal[KeyPointCount].botR;

            mOSK.KeyRectDisplay[KeyPointCount].topL = Temp.topL;
            mOSK.KeyRectDisplay[KeyPointCount].topR = Temp.topR;
            mOSK.KeyRectDisplay[KeyPointCount].botL = Temp.botL;
            mOSK.KeyRectDisplay[KeyPointCount].botR = Temp.botR;
        }
    }
    break;
    }

    return Status;
}


/**

Rotates the keyboard about the z-axis by the fixed angle specified.

@param[in]      Angle           Keyboard rotation angle (i.e., 0, 90, 180, and 270 degrees).

@retval         EFI_SUCCESS     Successfully rotated the keyboard.

**/
EFI_STATUS
RotateKeyboard(IN SCREEN_ANGLE    Angle)
{
    EFI_STATUS Status = EFI_SUCCESS;
    float ScaleFactor = 0.0;
    float Zang = 0.0;

    if (mGop == NULL){
        DEBUG((DEBUG_ERROR, "ERROR [OSK] Failed to find GOP protocol \n"));
        mOSK.KeyboardAngle = Angle;
        goto Exit;
    }

    UINTN ScreenWidth    = (UINTN)mGop->Mode->Info->HorizontalResolution;
    UINTN ScreenHeight   = (UINTN)mGop->Mode->Info->VerticalResolution;
    UINTN KeyboardWidth  = (UINTN)(mOSK.KeyboardRectOriginal.topR.pt.x - mOSK.KeyboardRectOriginal.topL.pt.x);


    // Save keyboard angle for later.
    //
    mOSK.KeyboardAngle = Angle;

    // Configure keyboard rectangles to be compatible with display and touch coordinate systems based on rotation angle.
    //
    NormalizeKeyRectsForRendering(Angle);

    // Compute the maximum keyboard size scale factor based on screen dimensions and set rotation angle.
    //
    switch (Angle)
    {
    case Angle_90:
        ScaleFactor = ((float)ScreenHeight / (float)KeyboardWidth);
        Zang        = HALF_PI;
        break;
    case Angle_180:
        ScaleFactor = ((float)ScreenWidth / (float)KeyboardWidth);
        Zang        = PI;
        break;
    case Angle_270:
        ScaleFactor = ((float)ScreenHeight / (float)KeyboardWidth);
        Zang        = PI + HALF_PI;
        break;
    case Angle_0:
    default:
        ScaleFactor = ((float)ScreenWidth / (float)KeyboardWidth);
        Zang        = 0.0;
        break;
    }

    // Reduce the scale factor by the amount specified.  Note that we adjust scaling to ensure the same percentage of screen width
    // irrespective of rotation angle.
    //
    ScaleFactor *= (float)mOSK.PercentOfScreenWidth;

    // Initialize display transform
    //
    InitializeXformWithParams (ScaleFactor,           // Scale
        0.0,                   // X-axis angle
        0.0,                   // Y-axis angle
        Zang                   // Z-axis angle
        );

    // Apply the initial transform to the keyboard geometry point set
    //
    Apply3DTransform(FALSE);

    // Update the keyboard position (and render) based on the rotation result.
    //
    Status = SetKeyboardPosition(mOSK.KeyboardPosition, mOSK.DockedState);
Exit:
    return Status;
}


EFI_STATUS
CheckForKeyHit (IN  UINTN TouchX,
IN  UINTN TouchY,
OUT UINTN *pKeyNumber)
{
    UINTN Count;

    // TODO - need to optimize this routine and/or point set...

    for (Count=0 ; Count < NUMBER_OF_KEYS ; Count++)
    {
        if (TouchX >= mOSK.KeyList[Count].KeyDisplayHitRect.Left  &&
            TouchX <= mOSK.KeyList[Count].KeyDisplayHitRect.Right &&
            TouchY >= mOSK.KeyList[Count].KeyDisplayHitRect.Top   &&
            TouchY <= mOSK.KeyList[Count].KeyDisplayHitRect.Bottom)
        {
            *pKeyNumber = Count;
            return EFI_SUCCESS;
        }
    }

    return EFI_NOT_FOUND;
}


EFI_STATUS
InsertKeyPressIntoQueue (IN UINT16 ScanCode,
IN CHAR16 UnicodeChar)
{

    // If queue input and output positions collide, there is a buffer overflow
    //
    if (mOSK.QueueInputPosition == mOSK.QueueOutputPosition && FALSE == mOSK.bQueueEmpty)
    {
        DEBUG((DEBUG_INFO, "INFO [OSK]: Key press input queue overflow!\r\n"));
        return EFI_OUT_OF_RESOURCES;
    }

    // Store key press data in the queue
    //
    mOSK.KeyPressQueue[mOSK.QueueInputPosition].ScanCode    = ScanCode;
    mOSK.KeyPressQueue[mOSK.QueueInputPosition].UnicodeChar = UnicodeChar;

    // Increment the input position to the next slot and handle wrap-around
    //
    ++mOSK.QueueInputPosition;
    mOSK.QueueInputPosition %= KEYBOARD_INPUT_QUEUE_SIZE;

    // No longer the first insertion
    //
    mOSK.bQueueEmpty = FALSE;

    return EFI_SUCCESS;
}


EFI_STATUS
ExtractKeyPressFromQueue (OUT EFI_INPUT_KEY *pKey)
{

    pKey->UnicodeChar = mOSK.KeyPressQueue[mOSK.QueueOutputPosition].UnicodeChar;
    pKey->ScanCode    = mOSK.KeyPressQueue[mOSK.QueueOutputPosition].ScanCode;

    // Increment the output position to the next slot and handle wrap-around
    //
    ++mOSK.QueueOutputPosition;
    mOSK.QueueOutputPosition %= KEYBOARD_INPUT_QUEUE_SIZE;

    // If queue input and output positions are the same, the queue is empty
    //
    if (mOSK.QueueInputPosition == mOSK.QueueOutputPosition)
    {
        mOSK.bQueueEmpty = TRUE;
    }

    return EFI_SUCCESS;
}


// Return Value: TRUE == Key press was a modifier key therefore we don't need to insert it into the key input queue
//
BOOLEAN
KeyModifierStateMachine (IN EFI_KEY Key)
{
    BOOLEAN bModifierKey = FALSE;
    STATIC BOOLEAN bDelayedTransitionfromShiftState = FALSE;    // Delays the transition from shift state until after the next key press

    // Was a modifier key pressed?
    //
    if (EfiKeyLShift == Key ||  // Left Shift
        EfiKeyRShift == Key ||  // Right Shift
        EfiKeyA2     == Key ||  // Function
        EfiKeyA0     == Key)    // Number & Symbols
    {
        bModifierKey = TRUE;
    }

    // Manage modifier key transitions
    //
    switch (mOSK.KeyModifierState)
    {
    case Normal:
        if (EfiKeyLShift == Key ||
            EfiKeyRShift == Key)
        {
            mOSK.KeyModifierState = Shift;
            bDelayedTransitionfromShiftState = TRUE;
        }
        else if (EfiKeyA0 == Key)
        {
            mOSK.KeyModifierState = NumSym;
        }
        else if (EfiKeyA2 == Key)
        {
            mOSK.KeyModifierState = Function;
        }
        break;
    case Shift:
        if (EfiKeyLShift == Key ||
            EfiKeyRShift == Key)
        {
            mOSK.KeyModifierState = CapsLock;
            bDelayedTransitionfromShiftState = FALSE;
        }
        else if (EfiKeyA0 == Key)
        {
            mOSK.KeyModifierState = NumSym;
            bDelayedTransitionfromShiftState = FALSE;
        }
        else if (EfiKeyA2 == Key)
        {
            mOSK.KeyModifierState = Function;
            bDelayedTransitionfromShiftState = FALSE;
        }
        else
        {
            mOSK.KeyModifierState = Normal;
        }
        break;
    case CapsLock:
        if (EfiKeyLShift == Key ||
            EfiKeyRShift == Key)
        {
            mOSK.KeyModifierState = Normal;
        }
        else if (EfiKeyA0 == Key)
        {
            mOSK.KeyModifierState = NumSym;
        }
        else if (EfiKeyA2 == Key)
        {
            mOSK.KeyModifierState = Function;
        }
        break;
    case NumSym:
        if (EfiKeyA0 == Key)
        {
            mOSK.KeyModifierState = Normal;
        }
        else if (EfiKeyA2 == Key)
        {
            mOSK.KeyModifierState = Function;
        }
        break;
    case Function:
        if (EfiKeyA0 == Key)
        {
            mOSK.KeyModifierState = NumSym;
        }
        else if (EfiKeyA2 == Key)
        {
            mOSK.KeyModifierState = Normal;
        }
        break;
    default:
        break;
    }

    // Select the correct key mapping table based on the modifier state
    //
    switch (mOSK.KeyModifierState)
    {
    case Shift:
    case CapsLock:
        mOSK.pKeyMap = mOSK_ShiftMode_US_EN;
        break;
    case NumSym:
        mOSK.pKeyMap = mOSK_NumSymMode_US_EN;
        break;
    case Function:
        mOSK.pKeyMap = mOSK_FnctMode_US_EN;
        break;
    case Normal:
    default:
        if (TRUE == bDelayedTransitionfromShiftState)
        {
            mOSK.pKeyMap = mOSK_ShiftMode_US_EN;
            bDelayedTransitionfromShiftState = FALSE;
        }
        else
        {
            mOSK.pKeyMap = mOSK_StdMode_US_EN;
        }
        break;
    }


    return (bModifierKey);
}


VOID
KeyboardInputHandler (IN MS_SWM_ABSOLUTE_POINTER_STATE  *pTouchState)
{
    EFI_STATUS Status = EFI_SUCCESS;

    // Capture the touch state
    //
    BOOLEAN bFingerDown    = ((pTouchState->ActiveButtons & 0x1) == 1);
    UINTN   AdjustedTouchX = (UINTN)pTouchState->CurrentX;
    UINTN   AdjustedTouchY = (UINTN)pTouchState->CurrentY;
    UINTN   KeyNumber      = 0;


    // If the keyboard is in the process of being dragged, compute new dx, dy offset and look for finger-up to terminate the operation
    //
    if (TRUE == mOSK.bKeyboardMoving)
    {
        // Stop dragging if finger was lifted
        //
        if (FALSE == bFingerDown)
        {
            // Apply transform to all pointsets now that the keybard is in the final location.
            //
            Apply3DTransform(FALSE);

            mOSK.bKeyboardMoving = FALSE;
            goto Exit;
        }

        float dx = (float)((INTN)AdjustedTouchX - (INTN)mOSK.KeyboardDragOrigX);
        float dy = (float)((INTN)AdjustedTouchY - (INTN)mOSK.KeyboardDragOrigY);

        // Translate the keyboard's location
        //
        TranslateKeyboardLocation(dx, dy);

        // Capture latest sampling position
        //
        mOSK.KeyboardDragOrigX = AdjustedTouchX;
        mOSK.KeyboardDragOrigY = AdjustedTouchY;

        goto Exit;
    }

    // Check whether there's a touch "hit" on the keyboard
    //
    Status = CheckForKeyHit(AdjustedTouchX, AdjustedTouchY, &KeyNumber);
    if (EFI_ERROR(Status))
    {
        // No hit.  If this is a finger-up event, force keyboard rendering to deselect the highlighted key(s).
        //
        if (FALSE == bFingerDown)
        {
            goto RefreshKeyboard;
        }

        // Check whether the keyboard "close" button is being presented and is selected
        //
        Status = CheckForCloseButtonHit(AdjustedTouchX, AdjustedTouchY);
        if (EFI_ERROR(Status))
        {
            // Check whether the keyboard "docking" button  is being presented and is selected
            //
            Status = CheckForDockingButtonHit(AdjustedTouchX, AdjustedTouchY);
            if (EFI_ERROR(Status))
            {
                // Check whether the keyboard frame is selected
                //
                Status = CheckForKeyboardFrameHit(AdjustedTouchX, AdjustedTouchY);
                if (EFI_ERROR(Status))
                {
                    goto Exit;
                }

                // The keyboard can only be dragged if it's undocked
                //
                if (Docked == mOSK.DockedState)
                {
                    goto Exit;
                }

                //
                // Keyboard frame was selected - translate keyboard location...
                //
                mOSK.KeyboardDragOrigX = AdjustedTouchX;
                mOSK.KeyboardDragOrigY = AdjustedTouchY;
                mOSK.bKeyboardMoving   = TRUE;

                goto Exit;
            }

            DEBUG((DEBUG_INFO, "INFO [OSK]: Keyboard dock-undock button selected.\r\n"));

            //
            // Docking/Undocking button was selected - toggle docked state.
            //
            Status = SetKeyboardDockState((Docked == mOSK.DockedState ? Undocked : Docked));

            goto Exit;
        }

        //
        // Close button was selected - dismiss the keyboard...
        //
        DEBUG((DEBUG_INFO, "INFO [OSK]: Keyboard close button selected.\r\n"));

        // Hide the keyboard and show the keyboard icon
        //
        ShowKeyboard(FALSE);
        ShowKeyboardIcon(TRUE);

        goto Exit;
    }

    // Handle key press processing from this point forward...
    //

    // If the same key is being selected again (i.e., finger/button weren't lifted first), there's no need to insert a key-press event
    // again since the key repeat timer will handle this if needed.
    //
    if (TRUE == bFingerDown && mOSK.SelectedKey == KeyNumber)
    {
        goto Exit;
    }

    // If the event is a finger/button down event, add the selected key to the queue...
    //
    if (TRUE == bFingerDown)
    {
        if (FALSE == KeyModifierStateMachine(mOSK.pKeyMap[KeyNumber].EfiKey))
        {
            // Insert the key press data
            //
            InsertKeyPressIntoQueue(mOSK.pKeyMap[KeyNumber].ScanCode, mOSK.pKeyMap[KeyNumber].Unicode);

            // Start the key repeat timer - initial interval is different from later steady-state value
            //
            Status = gBS->SetTimer (mKeyRepeatTimerEvent,
                TimerRelative,
                INITIAL_KEYREPEAT_INTERVAL
                );

            if (EFI_ERROR (Status))
            {
                DEBUG((DEBUG_WARN, "WARN [OSK]: Failed to start key repeat timer.  Status = %r\r\n", Status));
            }
        }
        else
        {
            // Keyboard modifier state changed.
            //
            mOSK.bKeyboardStateChanged = TRUE;
        }
    }
    else
    {
        // Disable the key repeat timer
        //
        gBS->SetTimer (mKeyRepeatTimerEvent,
            TimerCancel,
            0
            );

        // Special case: When shift is pressed once we transition to "shift" modifier state.  When a second non-modifier key
        // is pressed, we delay transition from "shift" modifier state in order to use the "shift" mapping table for the key
        // lookup, however this impacts key text rendering (i.e., key text still shows the "shift" mapping until a third
        // non-modifier key is pressed).  Instead, update the key mapping table now that we've inserted the key into the queue
        // and any key repeat activity has ended.
        //
        if (Normal == mOSK.KeyModifierState)
        {
            mOSK.pKeyMap = mOSK_StdMode_US_EN;
        }
    }


RefreshKeyboard:

    // Keep track of the keys to be selected and (possibly) deselected.  Note that it could be the same key.
    //
    mOSK.DeselectKey = mOSK.SelectedKey;
    mOSK.SelectedKey = (TRUE == bFingerDown ? KeyNumber : NUMBER_OF_KEYS);

    // Render the keyboard with key text (key mapping/text may have changed) if it should be displayed.
    //
    if (TRUE == mOSK.bDisplayKeyboard)
    {
        RenderKeyboard(TRUE);
    }

Exit:

    return;
}

EFI_STATUS
OSKResetInputDevice (IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
IN BOOLEAN                          ExtendedVerification)
{
    EFI_STATUS Status = EFI_SUCCESS;

    mOSK.QueueInputPosition  = 0;
    mOSK.QueueOutputPosition = 0;
    mOSK.bQueueEmpty         = TRUE;

    return Status;
}


EFI_STATUS
OSKReadKeyStroke (IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL     *This,
OUT EFI_INPUT_KEY                      *pKey)
{

    // BDS uses the OSK protocol to enable "icon auto activate mode" when booting to Windows.  In this
    // routine we'll check to see if both the mode is enabled *and* the keyboard & icon aren't being displayed
    // (but a caller is trying to read a keystroke from us).  If so we'll automatically present the keyboard icon.
    // This is primarily for the Bitlocker PIN screen which first tries to reads a keystroke rather than waiting
    // on the event to signal that there is one.
    //
    if (FALSE == mOSK.bDisplayKeyboardIcon && FALSE == mOSK.bDisplayKeyboard && TRUE == mOSK.bKeyboardIconAutoEnable)
    {
        DEBUG((DEBUG_INFO, "INFO [OSK]: OSKReadKeyStroke: Auto-activating the keyboard icon.\r\n"));

        // Display the keyboard icon.  Assume the keyboard and icon positions, sizes, and states have already been configured.
        //
        ShowKeyboardIcon(TRUE);
    }


    // Check whether there's data pending in the key press input queue
    //
    if (mOSK.QueueOutputPosition == mOSK.QueueInputPosition)
    {
        return EFI_NOT_READY;
    }

    return (ExtractKeyPressFromQueue(pKey));
}


EFI_STATUS
OSKResetInputDeviceEx (IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
IN BOOLEAN                             ExtendedVerification)
{
    return OSKResetInputDevice(&mOSK.SimpleTextIn, ExtendedVerification);
}


EFI_STATUS
OSKReadKeyStrokeEx (IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL     *This,
OUT EFI_KEY_DATA                          *pKey)
{
    ZeroMem(pKey, sizeof(EFI_KEY_DATA));

    pKey->KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID;
    pKey->KeyState.KeyToggleState = EFI_TOGGLE_STATE_VALID;

    return (OSKReadKeyStroke(&mOSK.SimpleTextIn, &pKey->Key));
}


EFI_STATUS
OSKSetState (IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
IN EFI_KEY_TOGGLE_STATE               *KeyToggleState)
{
    return EFI_SUCCESS;
}


EFI_STATUS
OSKRegisterKeyNotify (IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
IN EFI_KEY_DATA                       *KeyData,
IN EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
OUT EFI_HANDLE                        *NotifyHandle)
{
    return EFI_SUCCESS;
}


EFI_STATUS
OSKUnregisterKeyNotify (IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
IN EFI_HANDLE                         NotificationHandle)
{
    return EFI_SUCCESS;
}


VOID
EFIAPI
OSKWaitForKey (IN  EFI_EVENT    Event,
IN  VOID         *Context)
{

    // BDS uses the OSK protocol to enable "icon auto activate mode" when booting to Windows.  In this
    // routine we'll check to see if both the mode is enabled *and* the keyboard & icon aren't being displayed
    // (but a caller is waiting on our simple text input event).  If so we'll automatically present the keyboard icon.
    // This is primarily for the Bitlocker PIN screen which first tries to reads a keystroke rather than waiting
    // on the event to signal that there is one.
    //
    if (FALSE == mOSK.bDisplayKeyboardIcon && FALSE == mOSK.bDisplayKeyboard && TRUE == mOSK.bKeyboardIconAutoEnable)
    {
        DEBUG((DEBUG_INFO, "INFO [OSK]: OSKWaitForKey: Auto-activating the keyboard icon.\r\n"));

        // Display the keyboard icon.  Assume the keyboard and icon positions, sizes, and states have already been configured.
        //
        ShowKeyboardIcon(TRUE);
    }

    // Check whether there's data pending in the key press input queue
    //
    if (mOSK.QueueOutputPosition == mOSK.QueueInputPosition)
    {
        return;
    }

    // If there is pending key press, signal the event
    //
    gBS->SignalEvent (Event);
}


/**
    Handles Repeat timer callback

    @param[in]  Event           Event that was signalled.
    @param[in]  Context         Context passed into CreateEventEx.
**/
VOID
EFIAPI
OSKKeyRepeatCallback (IN EFI_EVENT  Event,
                      IN VOID      *Context)
{
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN KeyCount;

    // Find the currently selected key
    //
    KeyCount = mOSK.SelectedKey;

    if (NUMBER_OF_KEYS == KeyCount)
    {
        // Didn't find a selected key...
        //
        return;
    }

    // Re-insert the last key pressed
    //
    InsertKeyPressIntoQueue(mOSK.pKeyMap[KeyCount].ScanCode, mOSK.pKeyMap[KeyCount].Unicode);

    // Update the key repeat interval to a faster steady-state value
    //
    Status = gBS->SetTimer (mKeyRepeatTimerEvent,
        TimerRelative,
        STEADYST_KEYREPEAT_INTERVAL
        );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_WARN, "WARN [OSK]: Failed to update key repeat timer interval.  Status = %r\r\n", Status));
    }
}


BOOLEAN
EFIAPI
OSKProcessPointerCallback (IN VOID     *Context)
{
    EFI_STATUS                     Status = EFI_SUCCESS;
    MS_SWM_ABSOLUTE_POINTER_STATE  TouchState;
    SWM_RECT                       Rect;
    STATIC BOOLEAN                 WatchForFirstFingerUpEvent = FALSE;
    BOOLEAN                        WatchForFirstFingerUpEvent2;


    // If the OSK icon and keyboard aren't being shown, ignore touch/mouse events
    //
    if (FALSE == mOSK.bDisplayKeyboard && FALSE == mOSK.bDisplayKeyboardIcon)
    {
        return FALSE;
    }

    // Get touch state (i.e, x, y, and finger up/down)
    //
    Status = mOSKPointerProtocol->GetState (mOSKPointerProtocol,
                                           &TouchState
                                            );

    if (EFI_ERROR(Status))
    {
        return FALSE;
    }

    // Filter out all extra pointer moves with finger UP.
    WatchForFirstFingerUpEvent2 = WatchForFirstFingerUpEvent;
    WatchForFirstFingerUpEvent = SWM_IS_FINGER_DOWN(TouchState);
    if (!SWM_IS_FINGER_DOWN (TouchState) && (FALSE == WatchForFirstFingerUpEvent2))
    {
        return FALSE;
    }


    // If the keyboard is being displayed, input handler should process the touch point
    //
    if (TRUE == mOSK.bDisplayKeyboard)
    {
        // Process keyboard input until the keyboard is dismissed
        //
        KeyboardInputHandler(&TouchState);

        return FALSE;
    }

    // Determine whether the keyboard icon is selected. Ignore finger-up events.
    //
    Status = CheckForKeyboardIconHit ((UINT32)TouchState.CurrentX, (UINT32)TouchState.CurrentY);
    if (Status != EFI_SUCCESS || (TouchState.ActiveButtons & 0x1) == 0)
    {
        return FALSE;
    }

    DEBUG((DEBUG_INFO, "INFO [OSK]: Keyboard icon selected.\r\n"));

    // Determine the keyboard outer bounding rectangle
    //
    GetKeyboardBoundingRect(&Rect);

    // Update the window manager to let it know our size and location
    //
    mSWMProtocol->SetWindowFrame(mSWMProtocol,
        mImageHandle,
        (SWM_RECT *)&Rect
        );

    // Hide the keyboard icon and show the keyboard
    //
    ShowKeyboardIcon(FALSE);
    ShowKeyboard(TRUE);

    return FALSE;
}


VOID
EFIAPI
OSKCheckDisplayModeTimerCallback (IN EFI_EVENT  Event,
IN VOID       *Context)
{

    // Check whether the display mode has changed since we last computed screen asset locations.
    //
    if (mOSK.ScreenResolutionWidth  != mGop->Mode->Info->HorizontalResolution ||
        mOSK.ScreenResolutionHeight != mGop->Mode->Info->VerticalResolution)
    {
        HandleDisplayModeChange(mGop->Mode->Info->HorizontalResolution, mGop->Mode->Info->VerticalResolution);
    }

    // Check whether there's a paint event to handle.
    //
    if (gBS->CheckEvent(mOSKPaintEvent) == EFI_SUCCESS)
    {
        // Refresh the keyboard or icon as needed.
        //
        if (TRUE == mOSK.bDisplayKeyboardIcon)
        {
            // Refresh keyboard icon.
            //
            ShowKeyboardIcon(TRUE);
        }
        else if (TRUE == mOSK.bDisplayKeyboard)
        {
            // Refresh keyboard (do a full redraw).
            //
            mOSK.bKeyboardSizeChanged = TRUE;

            RenderKeyboard(TRUE);
        }
    }
}

/**
Main entry point for this driver.

@param ImageHandle     Image handle this driver.
@param SystemTable     Pointer to SystemTable.

@retval EFI_SUCCESS    This function always complete successfully.

**/
EFI_STATUS
EFIAPI
OSKDriverInit()
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    EFI_HANDLE ImageHandle = mImageHandle;
    SWM_RECT    FrameRect;
    DEBUG((DEBUG_INFO, "OSK Init \n"));

    // Install Simple Text Input and Simple Text Extended protocol handlers
    //
    mOSK.SimpleTextIn.Reset = OSKResetInputDevice;
    mOSK.SimpleTextIn.ReadKeyStroke = OSKReadKeyStroke;

    mOSK.SimpleTextInEx.Reset = OSKResetInputDeviceEx;
    mOSK.SimpleTextInEx.ReadKeyStrokeEx = OSKReadKeyStrokeEx;
    mOSK.SimpleTextInEx.SetState = OSKSetState;
    mOSK.SimpleTextInEx.RegisterKeyNotify = OSKRegisterKeyNotify;
    mOSK.SimpleTextInEx.UnregisterKeyNotify = OSKUnregisterKeyNotify;

    mOSK.pBackBuffer = NULL;

    Status = gBS->InstallMultipleProtocolInterfaces(&mControllerHandle,
        &gEfiSimpleTextInProtocolGuid,         // 2. Simple Text In Protocol.
        (VOID**)&mOSK.SimpleTextIn,            //
        &gEfiSimpleTextInputExProtocolGuid,    // 3. Simple Text In Ex Protocol.
        (VOID**)&mOSK.SimpleTextInEx,          //
        &gEfiConsoleInDeviceGuid,              // 4. Indicates that OSK is a ConIn device (picked up by Consplitter).
        NULL,                                  //
        NULL
        );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK] - Failed to install OSK protocol, Status: %r\r\n", Status));
        goto Exit;
    }

    // Create a periodic timer for key repeat
    //
    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        OSKKeyRepeatCallback,
        NULL,
        &mKeyRepeatTimerEvent
        );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to create key repeat timer event.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Create a periodic timer for checking whether the display mode changed.
    //
    Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        OSKCheckDisplayModeTimerCallback,
        NULL,
        &mCheckDisplayModeTimerEvent
        );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to create display mode timer callback event.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Full screen.
    //
    FrameRect.Left      = 0;
    FrameRect.Top       = 0;
    FrameRect.Right     = mGop->Mode->Info->HorizontalResolution;
    FrameRect.Bottom    = mGop->Mode->Info->VerticalResolution;

    // Register with the Simple Window Manager to get pointer input events.
    //
    Status = mSWMProtocol->RegisterClient(mSWMProtocol,
        ImageHandle,
        SWM_Z_ORDER_OSK,
        &FrameRect,
        OSKProcessPointerCallback,
        NULL,
        &mOSKPointerProtocol,
        &mOSKPaintEvent
        );

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to register with the Simple Window Manager.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Initialize keyboard layout
    //
    Status = InitializeKeyboardGeometry();

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to initialize keyboard geometry.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Initialize key information
    //
    InitializeKeyInformation(mOSK.KeyList, (RECT3D *)mOSK.KeyRectXformed, NUMBER_OF_KEYS);

    // Perform final calculations based on current screen resolution.
    //
    HandleDisplayModeChange(mGop->Mode->Info->HorizontalResolution, mGop->Mode->Info->VerticalResolution);

    // Disable the watchdog timer
    //
    gBS->SetWatchdogTimer (0, 0, 0, NULL);

    // Start periodic timer for keyboard/icon refresh.
    //
    Status = gBS->SetTimer (mCheckDisplayModeTimerEvent,
        TimerPeriodic,
        PERIODIC_REFRESH_INTERVAL
        );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK]: Failed to start keyboard/icon refresh timer.  Status = %r\r\n", Status));
        goto Exit;
    }

Exit:

    // TODO - Close Protocol on Absolute Pointer if an error occurs?

    return Status;
}


EFI_STATUS
EFIAPI
DriverUnload (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;

    // TODO - Needs to be implemented

    return Status;
}


/**
This is the declaration of an EFI image entry point. This entry point is
the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
both device drivers and bus drivers.

@param  ImageHandle           The firmware allocated handle for the UEFI image.
@param  SystemTable           A pointer to the EFI System Table.

@retval EFI_SUCCESS           The operation completed successfully.
@retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
OSKDriverEntryPoint(
IN EFI_HANDLE        ImageHandle,
IN EFI_SYSTEM_TABLE  *SystemTable
)
{
    EFI_STATUS              Status;


    // Create Controller handle with the proper device path protocol
    Status = gBS->InstallProtocolInterface(
        &mControllerHandle,
        &gEfiDevicePathProtocolGuid,
        EFI_NATIVE_INTERFACE,
        &mPlatformOSKDevice
        );
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "%a OSK DEVICE Handle %x\n", __FUNCTION__, mControllerHandle));
    //
    // Install UEFI Driver Model protocol(s).
    //
    Status = EfiLibInstallDriverBindingComponentName2(
        ImageHandle,
        SystemTable,
        &gOSKDriverBinding,
        ImageHandle,
        NULL,
        NULL
        );
    ASSERT_EFI_ERROR(Status);

    // Save the image handle for later
    //
    mImageHandle = ImageHandle;


    // Initialize the Simple Text Input and Simple Text Input Extended wait events
    //
    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT,
        TPL_NOTIFY,
        OSKWaitForKey,
        NULL,
        &mOSK.SimpleTextIn.WaitForKey
        );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK] - Failed to initialize Simple Text Input protocol wait event, Status: %r\r\n", Status));
        goto Exit;
    }

    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT,
        TPL_NOTIFY,
        OSKWaitForKey,
        NULL,
        &mOSK.SimpleTextInEx.WaitForKeyEx
        );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK] - Failed to initialize Simple Text Input Extended protocol wait event, Status: %r\r\n", Status));
        goto Exit;

    }


    // Install OSK protocol handlers
    //
    mOSK.OSKProtocol.ShowKeyboard = OSKShowKeyboard;
    mOSK.OSKProtocol.ShowKeyboardIcon = OSKShowIcon;
    mOSK.OSKProtocol.ShowDockAndCloseButtons = OSKShowDockAndCloseButtons;
    mOSK.OSKProtocol.SetKeyboardIconPosition = OSKSetIconPosition;
    mOSK.OSKProtocol.SetKeyboardPosition = OSKSetKeyboardPosition;
    mOSK.OSKProtocol.SetKeyboardRotationAngle = OSKSetKeyboardRotationAngle;
    mOSK.OSKProtocol.SetKeyboardSize = OSKSetKeyboardSize;
    mOSK.OSKProtocol.GetKeyboardMode = OSKGetKeyboardMode;
    mOSK.OSKProtocol.SetKeyboardMode = OSKSetKeyboardMode;
    mOSK.OSKProtocol.GetKeyboardBounds = OSKGetKeyboardBounds;

    Status = gBS->InstallMultipleProtocolInterfaces(&mControllerHandle,
        &gMsOSKProtocolGuid,                   // 1. OSK Protocol for controlling OSK presentation.
        (VOID**)&mOSK.OSKProtocol,             //
        NULL
        );

    ASSERT_EFI_ERROR(Status);

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [OSK] - Failed to install OSK protocol, Status: %r\r\n", Status));
        goto Exit;
    }
    // Context information should be initialized here since it can be changed by different drivers after this  ex: password dialog, bds boot .
    // We dont want to change this later in the UEFI driver binding start and lose the settings set by other drivers
    // Initialize keyboard context (initial operating state)
    //
    InitializeKeyboardContext();

Exit:
    return Status;
}