/** @file

  Simple Window Manger (SWM) implementation

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

#include "WindowManager.h"


// Mouse pointer bitmaps
//
#include <Resources/MousePointer_Small.h>
#include <Resources/MousePointer_Medium.h>


// SWM HII Package GUID: {79BBF37A-2AAA-4CE3-AD5C-4AB728AA9290}
#define SWM_HII_PACKAGE_LIST_GUID                                                  \
{                                                                                  \
    0x79bbf37a, 0x2aaa, 0x4ce3, { 0xad, 0x5c, 0x4a, 0xb7, 0x28, 0xaa, 0x92, 0x90 } \
}


// ****** Global variables ******
//
EFI_GUID                 mSWMHiiPackageListGuid = SWM_HII_PACKAGE_LIST_GUID;
EFI_HII_HANDLE           mSWMHiiHandle = NULL;
extern unsigned char     SimpleWindowManagerDxeStrings[];
MS_UI_THEME_DESCRIPTION *mTheme = NULL;

// Common structures and protocols
// BASE WINMGR_CLIENT Protocol Guid
#define SWM_BASE_CLIENT_PROTOCOL_GUID /* 22ef30ad-f794-4c83-9bf9-b1a6d74f108d */   \
{                                                                                  \
    0x22ef30ad, 0xf794, 0x4c83, { 0x9b, 0xf9, 0xb1, 0xa6, 0xd7, 0x4f, 0x10, 0x8d } \
}

EFI_HANDLE                          mImageHandle;
EFI_GUID                            gBaseClientGuid = SWM_BASE_CLIENT_PROTOCOL_GUID;
EFI_HANDLE                          gBaseClientHandle;
EFI_GRAPHICS_OUTPUT_PROTOCOL        *mGop;
MS_RENDERING_ENGINE_PROTOCOL        *mRenderingEngine = NULL;
EFI_HII_FONT_PROTOCOL               *mFont;
EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL   *mSimpleTextInEx;

EFI_EVENT                           mGopRegisterEvent;
VOID                                *mGopRegistration;
WINMGR_CONTEXT                      mSWM;
EFI_EVENT                           mSWMWatchListTimerEvent;
EFI_ABSOLUTE_POINTER_MODE           mAbsPointerMode;
EFI_ABSOLUTE_POINTER_PROTOCOL       *mConsplitterAbsolutePointer = NULL;

// SWM driver binding protocol support
EFI_DRIVER_BINDING_PROTOCOL     mSWMDriverBinding =
{
    SWMDriverSupported,
    SWMDriverStart,
    SWMDriverStop,
    0x11,
    NULL,
    NULL
};


// ****** Local function prototypes ******
//
static EFI_STATUS
EFIAPI
DriverCleanUp (IN EFI_HANDLE  ImageHandle);

static EFI_STATUS
SelectMousePointer (VOID);

// ****** External definitions ******
//
// NOTE: Touch panel isn't available on the NT32 Emulator.
//
#ifndef NT32EMUL
extern EFI_GUID     gMsTouchPanelGuid;
#endif


// ****** Function declarations ******
//

/**
    Initializes the default window manager context.

    @param  None

    @retval EFI_SUCCESS             Successfully initialized context.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to complete the request.

**/
static EFI_STATUS
InitializeWindowManagerContext (VOID)
{


    // By default don't show the mouse pointer.
    //
    mSWM.bDisplayingMousePointer    = FALSE;
    mSWM.bMousePointerEnabled       = FALSE;

    // Configure mouse pointer bitmap (default: medium size).
    //
    mSWM.MousePointer.pBitmap       = gMousePointer_Medium;
    mSWM.MousePointer.Width         = MOUSE_POINTER_WIDTH_MEDIUM;
    mSWM.MousePointer.Height        = MOUSE_POINTER_HEIGHT_MEDIUM;

    return EFI_SUCCESS;
}


/**
    Registers custom fonts for use by the rest of the system.

    @param  None

    @retval EFI_SUCCESS             Successfully registered fonts.
    @retval EFI_UNSUPPORTED         Font protocol unavailable.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to complete the request.

**/
static
EFI_STATUS
RegisterSwmHiiPackages (VOID)
{
    EFI_STATUS                 Status              = EFI_SUCCESS;
    MS_UI_FONT_PACKAGE_HEADER *pFontPackageHdr    = NULL;
    UINT8                     *pFontPackages = NULL;
    UINT32                     CollectionSize;


    // Determine if the Font Protocol is available
    //
    if (NULL == mFont)
    {
        Status = EFI_UNSUPPORTED;
        DEBUG((DEBUG_INFO, "INFO [SWM]: Failed to find Font protocol (%r).\r\n", Status));
        goto Exit;
    }

    // Calculate the total size of the font package collection.  Includes 32-bit "header" which is the total size of all
    // font packages included.
    //
    CollectionSize     = sizeof(UINT32) +
                         (FONT_PTR_GET mTheme->SmallFont)->PackageSize    +  (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize    +
                         (FONT_PTR_GET mTheme->StandardFont)->PackageSize +  (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize +
                         (FONT_PTR_GET mTheme->MediumFont)->PackageSize   +  (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize   +
                         (FONT_PTR_GET mTheme->LargeFont)->PackageSize    +  (FONT_PTR_GET mTheme->LargeFont)->GlyphsSize    +
                         (FONT_PTR_GET mTheme->FixedFont)->PackageSize    +  (FONT_PTR_GET mTheme->FixedFont)->GlyphsSize    +
                         (FONT_PTR_GET mTheme->SmallOSKFont)->PackageSize +  (FONT_PTR_GET mTheme->SmallOSKFont)->GlyphsSize;

    // Allocate space for all the combined custom font packages.
    //
    pFontPackages = (UINT8 *)AllocatePool(CollectionSize);

    ASSERT(NULL != pFontPackages);
    if (NULL == pFontPackages)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Save the collection size (multiple font packages).
    //
    *(UINT32 *)pFontPackages = CollectionSize;

    // Add Small font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32));
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->SmallFont)->Package, (FONT_PTR_GET mTheme->SmallFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->SmallFont)->PackageSize + (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1), GLYPH_PTR_GET (FONT_PTR_GET mTheme->SmallFont)->Glyphs, (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize);

    // Add Standard font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32) +
                                                  (FONT_PTR_GET mTheme->SmallFont)->PackageSize +  (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize);
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->StandardFont)->Package, (FONT_PTR_GET mTheme->StandardFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->StandardFont)->PackageSize + (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1),  GLYPH_PTR_GET (FONT_PTR_GET mTheme->StandardFont)->Glyphs, (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize);

    // Add Medium font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32) +
                                                  (FONT_PTR_GET mTheme->SmallFont)->PackageSize    + (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize +
                                                  (FONT_PTR_GET mTheme->StandardFont)->PackageSize + (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize);
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->MediumFont)->Package, (FONT_PTR_GET mTheme->MediumFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->MediumFont)->PackageSize + (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1), GLYPH_PTR_GET (FONT_PTR_GET mTheme->MediumFont)->Glyphs, (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize);

    // Add Large font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32) +
                                                  (FONT_PTR_GET mTheme->SmallFont)->PackageSize    + (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize    +
                                                  (FONT_PTR_GET mTheme->StandardFont)->PackageSize + (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize +
                                                  (FONT_PTR_GET mTheme->MediumFont)->PackageSize   + (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize);
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->LargeFont)->Package, (FONT_PTR_GET mTheme->LargeFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->LargeFont)->PackageSize + (FONT_PTR_GET mTheme->LargeFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1), GLYPH_PTR_GET (FONT_PTR_GET mTheme->LargeFont)->Glyphs, (FONT_PTR_GET mTheme->LargeFont)->GlyphsSize);

    // Add Fixed font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32) + \
                                                  (FONT_PTR_GET mTheme->SmallFont)->PackageSize    + (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize    +
                                                  (FONT_PTR_GET mTheme->StandardFont)->PackageSize + (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize +
                                                  (FONT_PTR_GET mTheme->MediumFont)->PackageSize   + (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize   +
                                                  (FONT_PTR_GET mTheme->LargeFont)->PackageSize    + (FONT_PTR_GET mTheme->LargeFont)->GlyphsSize);
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->FixedFont)->Package, (FONT_PTR_GET mTheme->FixedFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->FixedFont)->PackageSize + (FONT_PTR_GET mTheme->FixedFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1), GLYPH_PTR_GET (FONT_PTR_GET mTheme->FixedFont)->Glyphs, (FONT_PTR_GET mTheme->FixedFont)->GlyphsSize);

    // Add SmallOSK font package.
    //
    pFontPackageHdr = (MS_UI_FONT_PACKAGE_HEADER *)(pFontPackages + sizeof(UINT32) + \
                                                  (FONT_PTR_GET mTheme->SmallFont)->PackageSize    + (FONT_PTR_GET mTheme->SmallFont)->GlyphsSize    +
                                                  (FONT_PTR_GET mTheme->StandardFont)->PackageSize + (FONT_PTR_GET mTheme->StandardFont)->GlyphsSize +
                                                  (FONT_PTR_GET mTheme->MediumFont)->PackageSize   + (FONT_PTR_GET mTheme->MediumFont)->GlyphsSize   +
                                                  (FONT_PTR_GET mTheme->LargeFont)->PackageSize    + (FONT_PTR_GET mTheme->LargeFont)->GlyphsSize    +
                                                  (FONT_PTR_GET mTheme->FixedFont)->PackageSize    + (FONT_PTR_GET mTheme->FixedFont)->GlyphsSize);
    CopyMem((UINT8 *)pFontPackageHdr, PACKAGE_PTR_GET (FONT_PTR_GET mTheme->SmallOSKFont)->Package, (FONT_PTR_GET mTheme->SmallOSKFont)->PackageSize);
    pFontPackageHdr->FontHeader.Header.Length = (FONT_PTR_GET mTheme->SmallOSKFont)->PackageSize + (FONT_PTR_GET mTheme->SmallOSKFont)->GlyphsSize;
    CopyMem((UINT8 *)(pFontPackageHdr + 1), GLYPH_PTR_GET (FONT_PTR_GET mTheme->SmallOSKFont)->Glyphs, (FONT_PTR_GET mTheme->SmallOSKFont)->GlyphsSize);

    // Register all HII packages.
    //
    mSWMHiiHandle = HiiAddPackages (&mSWMHiiPackageListGuid,
                                    mImageHandle,
                                    pFontPackages,
                                    SimpleWindowManagerDxeStrings,
                                    NULL
                                   );

    if (NULL == mSWMHiiHandle)
    {
        Status = EFI_OUT_OF_RESOURCES;
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to register HII packages (%r).\r\n", Status));
        goto Exit;
    }

    DEBUG((DEBUG_INFO, "INFO [SWM]: Registered HII packages (%r).\r\n", Status));

Exit:

    // Clean up after error conditions.
    //
    if (EFI_ERROR(Status) && NULL != pFontPackages)
    {
        FreePool (pFontPackages);
    }

    return Status;
}


/**
    Moves the mouse pointer to the specified location and optionally restores screen contents under the pointer.

    @param  None
    @param[in] PositionX                Mouse pointer x coordinate.
    @param[in] PositionY                Mouse pointer y coordinate.
    @param[in] bRestorePreviousScreen   TRUE == restore underlying screen before rendering at new location.

    @retval EFI_SUCCESS                 Successfully allocated buffers.
    @retval EFI_OUT_OF_RESOURCES        Insufficient resources to complete the request.

**/
static EFI_STATUS
MoveMousePointer(IN UINTN   PositionX,
                 IN UINTN   PositionY,
                 IN BOOLEAN bRestorePreviousScreen)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // If we aren't displaying the mouse pointer or if it's not enabled, don't show it.
    //
    if (FALSE == mSWM.bMousePointerEnabled)
    {
        goto Exit;
    }

    // Make sure the mouse pointer position falls within the allowable display area.  There
    // may have been a graphics mode switch.  Reset the pointer to mid-screen as a starting point.
    //
    if (PositionX >= mGop->Mode->Info->HorizontalResolution ||
        PositionY >= mGop->Mode->Info->VerticalResolution)
    {
        PositionX = (mGop->Mode->Info->HorizontalResolution / 2);
        PositionY = (mGop->Mode->Info->VerticalResolution   / 2);
    }

    Status = mRenderingEngine->MoveMousePointer(mRenderingEngine,
                                                (UINT32)PositionX,
                                                (UINT32)PositionY
                                               );

Exit:
    return (Status);
}


/**
    Hides the mouse pointer.

    @param  None

    @retval EFI_SUCCESS             Successfully hid the pointer.

**/
EFI_STATUS
HideMousePointer (VOID)
{
    EFI_STATUS Status = EFI_SUCCESS;


    Status = mRenderingEngine->ShowMousePointer(mRenderingEngine,
                                                FALSE
                                               );

    if (!EFI_ERROR(Status))
    {
        // Indicate that we're no longer showing the mouse pointer
        //
        mSWM.bDisplayingMousePointer = FALSE;
    }

    return Status;
}


/**
    Shows the mouse pointer.

    @param  None

    @retval EFI_SUCCESS             Successfully showing the pointer.
    @retval EFI_UNSUPPORTED         Mouse can't be shown because it's been disabled.

**/
EFI_STATUS
ShowMousePointer (VOID)
{
    EFI_STATUS Status = EFI_SUCCESS;


    Status = mRenderingEngine->ShowMousePointer(mRenderingEngine,
                                                TRUE
                                               );

    if (!EFI_ERROR(Status))
    {
        // Indicate that we're showing the mouse pointer
        //
        mSWM.bDisplayingMousePointer = TRUE;
    }

    return Status;
}

/**
*   Signal Client that data is available
*
*
**/
VOID SignalClient (WINMGR_CLIENT *Client) {
    BOOLEAN  Signal = TRUE;

    // If there is a notifcation callback, call it.  It returns TRUE to also signal AbsPtr WaitForInput
    if (NULL != Client->DataNotificationCallback) {
       Signal = Client->DataNotificationCallback(Client->DataNotificationContext);
    }

    if (Signal) {
        gBS->SignalEvent (Client->ClientAbsPtr.WaitForInput);
    }
}

/**
    Inserts the specified pointer event state into an aggregate event queue (FIFO).

    @param[in] PointerState         Pointer event to insert.

    @retval EFI_SUCCESS             Successfully inserted the event state.
    @retval EFI_OUT_OF_RESOURCES    Event queue overflowed.

**/
static EFI_STATUS
InsertPointerEventIntoQueue (IN WINMGR_CLIENT                 *Client,
                             IN EFI_ABSOLUTE_POINTER_STATE    *PointerState)
{
    EFI_STATUS                     Status      = EFI_SUCCESS;
    EFI_TPL                        PreviousTPL = 0;
    MS_SWM_ABSOLUTE_POINTER_QUEUE *Queue       = &Client->Queue;


    // Raise the TPL to avoid race condition with the peek-extract routines.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // If queue input and output positions collide, there is a buffer overflow
    //
    if (Queue->QueueInputPosition == Queue->QueueOutputPosition && FALSE == Queue->bQueueEmpty)
    {
        DEBUG((DEBUG_WARN, "WARN [SWM]: Pointer event %p queue overflow!\r\n",Queue));
        // Just throw the queue away and queue the next point
    }

    // Store pointer state data in the queue
    //
    Queue->PointerStateQueue[Queue->QueueInputPosition].CurrentX        = PointerState->CurrentX;
    Queue->PointerStateQueue[Queue->QueueInputPosition].CurrentY        = PointerState->CurrentY;
    Queue->PointerStateQueue[Queue->QueueInputPosition].CurrentZ        = 0;                                    // Z should always be 0.
    Queue->PointerStateQueue[Queue->QueueInputPosition].ActiveButtons   = (PointerState->ActiveButtons & 0x1);   // We only recognize the LSB.

    // Increment the input position to the next slot and handle wrap-around
    //
    ++Queue->QueueInputPosition;
    Queue->QueueInputPosition %= POINTER_STATE_INPUT_QUEUE_SIZE;

    // No longer the first insertion
    //
    Queue->bQueueEmpty = FALSE;

    // Signal Client
    //
    SignalClient (Client);

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


/**
    Peeks at the pending pointer event state in the aggregate event queue.

    @param[out] PointerState        Pointer event to be filled in from the event queue.

    @retval EFI_SUCCESS             Successfully retrieved event state.
    @retval EFI_NOT_FOUND           No data found in the queue.

**/
EFI_STATUS
PeekAtAbsolutePointerEventInQueue (IN WINMGR_CLIENT               *Client,
                                   OUT EFI_ABSOLUTE_POINTER_STATE *PointerState)
{
    EFI_STATUS                     Status      = EFI_SUCCESS;
    EFI_TPL                        PreviousTPL = 0;
    MS_SWM_ABSOLUTE_POINTER_QUEUE *Queue       = &Client->Queue;


    // Raise the TPL to avoid race condition with the insert-extract routines.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // If the queue is empty, there's nothing to retrieve
    //
    if (TRUE == Queue->bQueueEmpty)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    // Retrieve a pointer event from the queue
    //
    PointerState->CurrentX      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentX;
    PointerState->CurrentY      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentY;
    PointerState->CurrentZ      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentZ;
    PointerState->ActiveButtons = Queue->PointerStateQueue[Queue->QueueOutputPosition].ActiveButtons;

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


/**
    Removes pending pointer event state from the aggregate event queue.

    @param[out] PointerState        Pointer event to be filled in from the event queue.

    @retval EFI_SUCCESS             Successfully retrieved event state.
    @retval EFI_NOT_FOUND           No data found in the queue.

**/
EFI_STATUS
ExtractAbsolutePointerEventFromQueue (IN WINMGR_CLIENT               *Client,
                                      OUT EFI_ABSOLUTE_POINTER_STATE *PointerState)
{
    EFI_STATUS                     Status = EFI_SUCCESS;
    EFI_TPL                        PreviousTPL = 0;
    MS_SWM_ABSOLUTE_POINTER_QUEUE *Queue       = &Client->Queue;

    // Raise the TPL to avoid race condition with the peek-insert routines.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // If the queue is empty, there's nothing to retrieve
    //
    if (TRUE == Queue->bQueueEmpty)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    // Retrieve a pointer event from the queue
    //
    PointerState->CurrentX      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentX;
    PointerState->CurrentY      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentY;
    PointerState->CurrentZ      = Queue->PointerStateQueue[Queue->QueueOutputPosition].CurrentZ;
    PointerState->ActiveButtons = Queue->PointerStateQueue[Queue->QueueOutputPosition].ActiveButtons;

    // Increment the output position to the next slot and handle wrap-around
    //
    ++Queue->QueueOutputPosition;
    Queue->QueueOutputPosition %= POINTER_STATE_INPUT_QUEUE_SIZE;

    // If queue input and output positions are the same, the queue is empty
    //
    if (Queue->QueueInputPosition == Queue->QueueOutputPosition)
    {
        Queue->bQueueEmpty = TRUE;
    } else {
        SignalClient (Client);
    }

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}

/**
    FilterPointerState      Returns the WINMGR_CLIENT that matches the pointer state.

    @param[in] PointerState     Pointer State from a coalesced AbsolutePointerProtocol.

    @retval WINMGR_CLIENT       The Window Client to get the PointerState.

**/
WINMGR_CLIENT * FilterPointerState (IN MS_SWM_ABSOLUTE_POINTER_STATE *PointerState)
{
    WINMGR_CLIENT                   *pList;

    // Scan through the list of clients:
    //
    // * If the pointer event hits an active window, it's theirs.
    // * If it doesn't and there's an active default client, it's theirs.
    // * If none of the above, we give it to the current caller.
    //
    DEBUG((DEBUG_INFO, "%a - X=%5d, Y=%5d, Buttons=%x\n", __FUNCTION__, PointerState->CurrentX, PointerState->CurrentY, PointerState->ActiveButtons));

    pList = mSWM.Clients;
    while (pList != NULL)
    {
        DEBUG((DEBUG_INFO, "  - SignalChecking - ImageHandle=0x%x, Active=%s, Z=%3d, Event=%p, Window=L[%d]:R[%d]:T[%d]:B[%d]\r\n", (UINTN)pList->ImageHandle,
                                                                                                                   (pList->Active ? L"YES" : L"NO"),

                                                                                                                    pList->Z_Order,
                                                                                                                    pList->ClientAbsPtr.WaitForInput,
                                                                                                                    pList->WindowFrame.Left,
                                                                                                                    pList->WindowFrame.Right,
                                                                                                                    pList->WindowFrame.Top,
                                                                                                                    pList->WindowFrame.Bottom));

        // Check whether the queued pointer event will "hit" the this client's window.
        //
        if ((pList->pNext == NULL) || (pList->Active &&
              (PointerState->CurrentX >= pList->WindowFrame.Left  &&
               PointerState->CurrentY >= pList->WindowFrame.Top   &&
               PointerState->CurrentX <= pList->WindowFrame.Right &&
               PointerState->CurrentY <= pList->WindowFrame.Bottom )))
        {
            return pList;
        }
        pList = pList->pNext;
    }

    DEBUG((DEBUG_ERROR, "%a No WINMGR_CLIENT found for PointerState\n", __FUNCTION__));

    ASSERT(FALSE);

    return NULL;   // No window matched
}

/**
    Adds a discovered Absolute Pointer provider's interface to the watchlist.

    @param[in] Controller           Handle of the Controller to be added to the watchlist.
    @param[in] AbsolutePointer      Interface to be added to the watchlist.
    @param[in] bNeedsMousePointer   TRUE = this provider needs a mouse pointer (i.e., it's not touch).

    @retval EFI_SUCCESS             Successfully retrieved event state.
    @retval EFI_ALREADY_STARTED     Specified interface is already in the list.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to add the interface to the list.

**/
static EFI_STATUS
AddAbsolutePointerInterfaceToWatchList(IN EFI_HANDLE                    Controller,
                                       IN EFI_ABSOLUTE_POINTER_PROTOCOL *AbsolutePointer,
                                       IN BOOLEAN                       bNeedsMousePointer)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    EFI_TPL             PreviousTPL = 0;
    WINMGR_AP_WATCHLIST *pList      = mSWM.AbsolutePointerProviders;


    DEBUG((DEBUG_INFO, "INFO [SWM]: Adding Absolute Pointer protocol provider to watchlist (Controller=0x%x, NeedsMousePointer=%s).\r\n", (UINTN)Controller, (bNeedsMousePointer ? L"YES" : L"NO")));

    // Raise the TPL to avoid race condition with the add routine.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // First confirm that we're not already watching this interface
    //
    while (NULL != pList)
    {
        if (pList->AbsolutePointer == AbsolutePointer)
        {
            Status = EFI_ALREADY_STARTED;
            goto Exit;
        }
        pList = pList->pNext;
    }
    pList = mSWM.AbsolutePointerProviders;

    // Add the new provider to the watchlist.
    //
    mSWM.AbsolutePointerProviders = (WINMGR_AP_WATCHLIST *) AllocatePool(sizeof(WINMGR_AP_WATCHLIST));
    ASSERT (mSWM.AbsolutePointerProviders != NULL);
    if (NULL == mSWM.AbsolutePointerProviders)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    mSWM.AbsolutePointerProviders->AbsolutePointer      = AbsolutePointer;
    mSWM.AbsolutePointerProviders->Controller           = Controller;
    mSWM.AbsolutePointerProviders->bNeedsMousePointer   = bNeedsMousePointer;
    mSWM.AbsolutePointerProviders->pNext                = pList;
    mSWM.AbsolutePointerProviders->pPrev                = NULL;

    if (NULL != pList)
    {
        pList->pPrev = mSWM.AbsolutePointerProviders;
    }

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


/**
    Removes the specified Absolute Pointer provider interface from the watchlist.

    @param[in]  Controller          Handle of the controller to be removed from the watchlist.
    @param[out] bNeedsMousePointer  Pointer to a boolean that receives: TRUE == interface removed needed a mouse pointer.

    @retval EFI_SUCCESS             Successfully removed the interface.
    @retval EFI_NOT_FOUND           Unable to find the specified interface.

**/
static EFI_STATUS
RemoveAbsolutePointerInterfaceFromWatchList(IN EFI_HANDLE Controller,
                                            OUT BOOLEAN *bNeedsMousePointer)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    EFI_TPL             PreviousTPL = 0;
    WINMGR_AP_WATCHLIST *pList      = mSWM.AbsolutePointerProviders;


    DEBUG((DEBUG_INFO, "INFO [SWM]: Removing Absolute Pointer protocol provider from watchlist (Controller=0x%x).\r\n", (UINTN)Controller));

    // Raise the TPL to avoid race condition with the add routine.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // First confirm that the provider interface is in the watchlist.
    //
    while (NULL != pList)
    {
        if (pList->Controller == Controller)
        {
            break;
        }
        pList = pList->pNext;
    }

    if (NULL == pList)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    // Unlink the node to be freed.
    //
    if (NULL != pList->pPrev)
    {
        // Remove an intermediate or end-of-list node.
        //
        pList->pPrev->pNext = pList->pNext;
        if (NULL != pList->pNext)
        {
            pList->pNext->pPrev = pList->pPrev;
        }
    }
    else
    {
        // Remove the head of the list.
        //
        mSWM.AbsolutePointerProviders = pList->pNext;
        if (NULL != pList->pNext)
        {
            pList->pNext->pPrev = NULL;
        }
    }

    // Tell the caller whether this provider needed the mouse pointer to be displayed or not.
    //
    *bNeedsMousePointer = pList->bNeedsMousePointer;

    // Free the node buffer.
    //
    FreePool(pList);

Exit:

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


/**
    Cleans up the Absolute Pointer provider watchlist.

    @param[in] None.

    @retval EFI_SUCCESS             Successfully retrieved event state.

**/
static EFI_STATUS
FreeAbsolutePointerInterfaceWatchList(VOID)
{
    EFI_STATUS          Status      = EFI_SUCCESS;
    EFI_TPL             PreviousTPL = 0;
    WINMGR_AP_WATCHLIST *pList      = mSWM.AbsolutePointerProviders;
    WINMGR_AP_WATCHLIST *pNext      = NULL;


    // Raise the TPL to avoid race condition with the add routine.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Walk the list and free the nodes.
    //
    while (NULL != pList)
    {
        pNext = pList->pNext;
        FreePool(pList);
        pList = pNext;
    }

    // Restore the TPL.
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    return Status;
}


/**
    Selectively filters "redundant" absolute pointer events.

    @param[in] PointerState         New pointer event information.
    @param[in] PointerMode          Pointer coordinate space information.

    @retval TRUE == Event should be filtered.

    Note:: There is no filtering done by this function

**/
static BOOLEAN
EFIAPI
FilterPointerEvent (IN  EFI_ABSOLUTE_POINTER_STATE      *PointerState,
                    IN  EFI_ABSOLUTE_POINTER_MODE       *PointerMode)
{
    BOOLEAN                                 FilterEvent = FALSE;


    // Never filter finger/button "up" or mouse move events.
    //
    if (0 == (PointerState->ActiveButtons & 0x1))
    {
        return FALSE;
    }

    // Do not do any filtering here except the invalid keys  [ this is done above]
#if 0
    // Compute dimensions of a bounding box around the last point selected, used as a filter window.
    //
    // NOTE: The bounding box is computed based on a fractional percent of the absolute pointer's XY point space.  Dividing by 10000 is done to support the fractional
    //       percent without requiring floating point support.  Dividing by 2 is done to center the bounding box around the last XY point.
    //
    INT32 FilterBoxSizeHalf = abs(((((INT32)PointerMode->AbsoluteMaxX - (INT32)PointerMode->AbsoluteMinX) * SWM_POINTER_EVENT_FILTER_BOX_SIZE_PERCENT) / 10000) / 2);

    // Compute the delta between the last point and the current one.
    //
    INT32 DeltaX            = (INT32)LastPointerState.CurrentX - (INT32)PointerState->CurrentX;
    INT32 DeltaY            = (INT32)LastPointerState.CurrentY - (INT32)PointerState->CurrentY;

    // If the new pointer event's XY position falls within a filter (spacial) window surrounding the last one, filter it out.
    //
    if (abs(DeltaX) < FilterBoxSizeHalf && abs(DeltaY) < FilterBoxSizeHalf)
    {
        FilterEvent = TRUE;
    }

    // Capture the current pointer state.
    //
    CopyMem(&LastPointerState, PointerState, sizeof(MS_SWM_ABSOLUTE_POINTER_STATE));
#endif
    return FilterEvent;
}


static
EFI_STATUS
SelectMousePointer (VOID)
{
    EFI_STATUS Status   = EFI_SUCCESS;


    // Select an appropriate mouse pointer bitmap.
    //
    if (mSWM.ScreenWidth >= SMALL_ASSET_MAX_SCREEN_WIDTH)
    {
        mSWM.MousePointer.pBitmap       = gMousePointer_Medium;
        mSWM.MousePointer.Width         = MOUSE_POINTER_WIDTH_MEDIUM;
        mSWM.MousePointer.Height        = MOUSE_POINTER_HEIGHT_MEDIUM;
    }
    else
    {
        mSWM.MousePointer.pBitmap       = gMousePointer_Small;
        mSWM.MousePointer.Width         = MOUSE_POINTER_WIDTH_SMALL;
        mSWM.MousePointer.Height        = MOUSE_POINTER_HEIGHT_SMALL;
    }

    // Register the mouse pointer with the rendering engine.
    //
    Status = mRenderingEngine->SetMousePointer (mRenderingEngine,
                                                mSWM.MousePointer.pBitmap,
                                                mSWM.MousePointer.Width,
                                                mSWM.MousePointer.Height,
                                                32                              // 32bpp
                                               );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to register a mouse pointer (%r).\r\n", Status));
        goto Exit;
    }

Exit:

    return Status;
}


/**
    Timer Callback that Polls the Absolute Pointer provider watchlist for incoming pointer events,
    and Queus them to the proper queue.

    @param[in] Event                Ignored.
    @param[in] Context              Ignored.

    @retval None.

**/
static VOID
EFIAPI
CheckWatchListCallback (IN EFI_EVENT  Event,
                        IN VOID       *Context)
{
    EFI_STATUS             Status = EFI_SUCCESS;
    WINMGR_AP_WATCHLIST   *pList = mSWM.AbsolutePointerProviders;
    WINMGR_CLIENT         *Client;
    UINTN                  ScreenMaxX, ScreenMaxY;
    // Get screen coordinate space maximums.
    //
    ScreenMaxX = mGop->Mode->Info->HorizontalResolution;
    ScreenMaxY = mGop->Mode->Info->VerticalResolution;

    // Use this periodic timer callback to see if the display resolution has changed and to select an appropriately-sized mouse pointer.
    //
    if (mSWM.ScreenWidth != ScreenMaxX || mSWM.ScreenHeight != ScreenMaxY)
    {
        mSWM.ScreenWidth    = ScreenMaxX;
        mSWM.ScreenHeight   = ScreenMaxY;

        HideMousePointer();
        SelectMousePointer();
    }


    // Scan the Absolute Pointer provider watchlist and check for signalled events
    // indicating there's state to be read.
    //
    while (pList != NULL)
    {
        if (gBS->CheckEvent (pList->AbsolutePointer->WaitForInput) == EFI_SUCCESS)
        {
            EFI_ABSOLUTE_POINTER_STATE   PointerState;
            UINTN   AbsolutePointerMaxX, AbsolutePointerMaxY;

            Status = pList->AbsolutePointer->GetState (pList->AbsolutePointer,
                                                       &PointerState
                                                      );

            if (!EFI_ERROR(Status))
            {
                // Conditionally filter the raw pointer event.  For now, don't filter mouse pointer events (only touch).
                //
                if (FALSE == pList->bNeedsMousePointer && TRUE == FilterPointerEvent (&PointerState, pList->AbsolutePointer->Mode))
                {
                    // Toss the pointer event.
                    //
                    continue;
                }

                // Get Absolute Pointer coordinate space maximums.
                //
                AbsolutePointerMaxX = pList->AbsolutePointer->Mode->AbsoluteMaxX;
                AbsolutePointerMaxY = pList->AbsolutePointer->Mode->AbsoluteMaxY;

                // Get screen coordinate space maximums.
                //
                ScreenMaxX = mGop->Mode->Info->HorizontalResolution;
                ScreenMaxY = mGop->Mode->Info->VerticalResolution;

                // Fudge in case the request for the mode occurs after a mode set.
                mAbsPointerMode.AbsoluteMaxX = mGop->Mode->Info->HorizontalResolution;
                mAbsPointerMode.AbsoluteMaxY = mGop->Mode->Info->VerticalResolution;

                // Map touch coordinate space to the current graphics mode coordinate space.
                //
                PointerState.CurrentX = MultU64x32(PointerState.CurrentX, (UINT32)ScreenMaxX);
                PointerState.CurrentX = DivU64x32(PointerState.CurrentX, (UINT32)AbsolutePointerMaxX);

                PointerState.CurrentY = MultU64x32(PointerState.CurrentY, (UINT32)ScreenMaxY);
                PointerState.CurrentY = DivU64x32(PointerState.CurrentY, (UINT32)AbsolutePointerMaxY);

                // Range-check the mouse pointer location based on screen size.
                //
                if (TRUE == pList->bNeedsMousePointer)
                {
                    if (PointerState.CurrentX >= (ScreenMaxX - mSWM.MousePointer.Width))
                    {
                        PointerState.CurrentX  = (ScreenMaxX - mSWM.MousePointer.Width - 1);
                    }
                    if (PointerState.CurrentY >= (ScreenMaxY - mSWM.MousePointer.Height))
                    {
                        PointerState.CurrentY  = (ScreenMaxY - mSWM.MousePointer.Height - 1);
                    }
                }

                // Display the mouse pointer if needed and update the the mouse pointer location else hide it as needed.
                //
                if (FALSE == pList->bNeedsMousePointer && TRUE == mSWM.bDisplayingMousePointer)
                {
                    DEBUG((DEBUG_INFO, "INFO [SWM]: Hiding mouse pointer.\r\n"));
                    HideMousePointer();
                }
                else if (TRUE == pList->bNeedsMousePointer && TRUE == mSWM.bMousePointerEnabled && FALSE == mSWM.bDisplayingMousePointer)
                {
                    DEBUG((DEBUG_INFO, "INFO [SWM]: Showing mouse pointer.\r\n"));
                    ShowMousePointer();
                }
                else
                {
                    MoveMousePointer((UINTN)PointerState.CurrentX, (UINTN)PointerState.CurrentY, TRUE);
                }

                // Remember whether this pointer event required drawing the mouse pointer.  We need this should a client (re)enable the mouse pointer
                // from the SWM protocol interface.  If the last move didn't require a mouse pointer then enabling the pointer will wait for a move event that
                // requires it instead of immediately rendering the pointer.
                //
                mSWM.bLastMoveRequiredMousePointer = pList->bNeedsMousePointer;

                Client = FilterPointerState (&PointerState);
                InsertPointerEventIntoQueue (Client,&PointerState);
            }
        }

        // Move to the next Absolute Pointer provider.
        //
        pList = pList->pNext;
    }
}

/**
    Checks whether the specified controller exposes the Absolute Pointer interface that we will manage.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle to be checked for Absolute Pointer support.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Simple Window Manager can support this controller.
    @retval EFI_UNSUPPORTED         Controller isn't one we'll manage.

**/
EFI_STATUS
EFIAPI
SWMDriverSupported (IN EFI_DRIVER_BINDING_PROTOCOL  *This,
                    IN EFI_HANDLE                   Controller,
                    IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath)
{
    EFI_STATUS                     Status;
    EFI_ABSOLUTE_POINTER_PROTOCOL *AbsolutePointer;


    // Make sure the Simple Window Manager does not attempt to attach to itself (since it exposes an
    // Absolute Pointer protocol interface as well as aggregates Absolute Pointer protocol providers).
    // Note that we may attach through ConSplitter so we need to check for this condition below.
    //
    if (Controller == mImageHandle)
    {
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    // Determine if the Absolute Pointer Protocol is available.
    //
    Status = gBS->OpenProtocol (Controller,
                                &gEfiAbsolutePointerProtocolGuid,
                                (VOID *) &AbsolutePointer,
                                NULL,
                                NULL,
                                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                               );

    if (EFI_ERROR (Status))
    {
        goto Exit;
    }

    // Find AbsolutePointer protocol for ConIn - which is really ConSplitter
    if (mConsplitterAbsolutePointer == NULL)
    {
        Status = gBS->HandleProtocol (gST->ConsoleInHandle, &gEfiAbsolutePointerProtocolGuid, (VOID **) &mConsplitterAbsolutePointer);
    }

    // if the current AbsolutePointer is the ConIn AbsolutePointer, skip it
    if ( mConsplitterAbsolutePointer == AbsolutePointer )
    {
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    Status = EFI_SUCCESS;
Exit:

    return Status;
}


/**
    Start supporting specified controller exposing Absolute Pointer interface.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller to be managed.
    @param[in] RemainingDevicePath  Ignored.

    @retval EFI_SUCCESS             Simple Window Manager will support this controller.
    @retval EFI_ALREADY_STARTED     Simple Window Manager is already managing this controller.
    @retval EFI_UNSUPPORTED         Controller isn't one we'll manage.

**/
EFI_STATUS
EFIAPI
SWMDriverStart (IN EFI_DRIVER_BINDING_PROTOCOL      *This,
                IN EFI_HANDLE                       Controller,
                IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath)
{
    EFI_STATUS                  Status              = EFI_SUCCESS;
    BOOLEAN                     bNeedsMousePointer  = FALSE;
    VOID                        *DriverProtocol     = NULL;
    EFI_DEVICE_PATH_PROTOCOL    *DevicePath;


    DEBUG((DEBUG_INFO, "INFO [SWM]: SWMDriverStart (Controller=0x%x).\r\n", (UINTN)Controller));

    // Open the Absolute Pointer Protocol exclusively (this will force others who may be holding it [ex: Consplitter] to disconnect).
    //
    Status = gBS->OpenProtocol (Controller,
                                &gEfiAbsolutePointerProtocolGuid,
                                &DriverProtocol,
                                This->DriverBindingHandle,
                                Controller,
                                EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE
                               );

    if (Status == EFI_ALREADY_STARTED)
    {
      DEBUG((DEBUG_INFO, "ERROR [SWM]: Failed to open Absolute Pointer protocol (Controller=0x%x): %r.\r\n", (UINTN)Controller, Status));
      goto Exit;
    }
    else if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to open Absolute Pointer protocol (Controller=0x%x): %r.\r\n", (UINTN)Controller, Status));
        goto Exit;
    }

    // NOTE: Touch panel isn't available on emulator and we don't require a mouse pointer since the emulator uses the standard Windows mouse pointer.
    //
#ifndef  NT32EMUL
    // Check whether this is the touch panel providing the absolute pointer interface.
    //
    Status = gBS->OpenProtocol (Controller,
                                &gMsTouchPanelGuid,
                                NULL,
                                This->DriverBindingHandle,
                                Controller,
                                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                               );

    if (!EFI_ERROR (Status))
    {
        // Found the touch panel guid - close.
        //
        gBS->CloseProtocol (Controller,
                            &gMsTouchPanelGuid,
                            This->DriverBindingHandle,
                            Controller
                           );
    }
    else
    {
        // If we didn't find the touch panel guid, assume this is a device that requires the mouse pointer.
        //
        bNeedsMousePointer = TRUE;
    }
#endif

    // Add the protocol pointer into the watch list.
    //
    Status = AddAbsolutePointerInterfaceToWatchList (Controller,
                                                     (EFI_ABSOLUTE_POINTER_PROTOCOL *)DriverProtocol,
                                                     bNeedsMousePointer
                                                    );

    if (EFI_ALREADY_STARTED == Status)
    {
        Status = EFI_SUCCESS;
        goto Exit;
    }

    // Send information about the controller just added to the debug port.
    //
    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to add Absolute Pointer protocol provider to watchlist (Controller=0x%x).\r\n", (UINTN)Controller));
        goto Exit;
    }

    // Display details about the protocol provider, if it exposes the Device Path protocol.
    //
    Status = gBS->OpenProtocol (Controller,
                                &gEfiDevicePathProtocolGuid,
                                (VOID**)&DevicePath,
                                This->DriverBindingHandle,
                                Controller,
                                EFI_OPEN_PROTOCOL_BY_DRIVER
                               );

    if (!EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "INFO [SWM]: Added Absolute Pointer protocol provider to watchlist (Controller=0x%x  Device: Type=0x%x SubType=0x%x).\r\n", (UINTN)Controller, (UINTN)DevicePath->Type, (UINTN)DevicePath->SubType));

        // Close the DevicePath Protocol.
        //
        gBS->CloseProtocol (Controller,
                            &gEfiDevicePathProtocolGuid,
                            This->DriverBindingHandle,
                            Controller
                           );
    }
    else
    {
        DEBUG ((DEBUG_ERROR, "INFO [SWM]: Added Absolute Pointer protocol provider to watchlist (Controller=0x%x  Device: Type=Unknown SubType=Unknown).\r\n", (UINTN)Controller));
        Status = EFI_SUCCESS;
    }

Exit:

  return Status;

}


/**
    Stops supporting specified controller exposing Absolute Pointer interface.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] Controller           Handle of controller being managed.
    @param[in] NumberOfChildren     Ignored.
    @param[in] ChildHandleBuffer    Ignored.

    @retval EFI_SUCCESS             Successfully stopped managing the specified controller.
    @retval EFI_NOT_FOUND           Unable to find the specified controller in the watchlist.

**/
EFI_STATUS
EFIAPI
SWMDriverStop (IN EFI_DRIVER_BINDING_PROTOCOL     *This,
               IN EFI_HANDLE                      Controller,
               IN UINTN                           NumberOfChildren,
               IN EFI_HANDLE                      *ChildHandleBuffer)
{
    EFI_STATUS  Status              = EFI_SUCCESS;
    BOOLEAN     bNeedsMousePointer  = FALSE;


    // Remove the protocol pointer from the watchlist.
    //
    Status = RemoveAbsolutePointerInterfaceFromWatchList (Controller,
                                                          &bNeedsMousePointer);
    ASSERT_EFI_ERROR (Status);

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to remove Absolute Pointer protocol provider to watchlist (Controller=0x%x).\r\n", (UINTN)Controller));
        goto Exit;
    }

    // Close Absolute Pointer protocol.
    //
    gBS->CloseProtocol (Controller,
                        &gEfiAbsolutePointerProtocolGuid,
                        This->DriverBindingHandle,
                        Controller
                       );

    // If we just removed a provider that needs the mouse pointer, hide the pointer.  It'll be re-displayed if another provider that requires the pointer generates a pointer event.
    //
    if (TRUE == bNeedsMousePointer)
    {
        HideMousePointer();
    }

Exit:

    return Status;
}


/**
  Second half of driver initialization

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS           This function always complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to initialize.

**/
EFI_STATUS
EFIAPI
DriverInitStage2 (IN EFI_HANDLE         ImageHandle)
{
    EFI_STATUS                     Status = EFI_SUCCESS;
    SWM_RECT                       FrameRect;


    // Determine if the Simple Rendering Engine Protocol is available on the same Console Out handle.  The
    // Rendering Engine driver provides both Graphics Output and Rendering Engine protocols.
    //
    if (NULL == mRenderingEngine) {
        Status = gBS->LocateProtocol(&gMsSREProtocolGuid,
                                      NULL,
                                      (VOID **)&mRenderingEngine
                                     );

        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to find Rendering Engine after finding GOP (%r).\r\n", Status));
            goto Exit;
        }
    }
    // Determine if the Font Protocol is available
    //
    Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid,
                                  NULL,
                                  (VOID **)&mFont
                                 );

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        mFont = NULL;
        goto Exit;
    }


    // Register our custom fonts.
    //
    Status = RegisterSwmHiiPackages();

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to register custom fonts (%r).\r\n", Status));
        goto Exit;
    }

    // Open the Simple Text Ex protocol on the Console handle.
    //
    Status = gBS->HandleProtocol (gST->ConsoleInHandle,
                                  &gEfiSimpleTextInputExProtocolGuid,
                                  (VOID **)&mSimpleTextInEx
                                 );

    //ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        mSimpleTextInEx = NULL;
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to find Simple Text Input Ex protocol (%r).\r\n", Status));
        goto Exit;
    }

    // TODO - Configure Absolute Pointer protocol's mode structure.
    // Right now UEFI Windowmanager code uses points scaled to the current Gop mode.
    // Not sure what do about modesets, but I don't think the scaling factor is allowed
    // to change.  As a work around, just initialize to the screen resolution at initialization.
    // Nobody in UEFI cares and assumes that pointer events are normalized to the screen resolution.

    mAbsPointerMode.AbsoluteMinX = 0;
    mAbsPointerMode.AbsoluteMinY = 0;
    mAbsPointerMode.AbsoluteMinZ = 0;
    if (NULL != mGop) {
        mAbsPointerMode.AbsoluteMaxX = mGop->Mode->Info->HorizontalResolution;
        mAbsPointerMode.AbsoluteMaxY = mGop->Mode->Info->VerticalResolution;
    } else {
        mAbsPointerMode.AbsoluteMaxX = 32768;  // Just so they are not zero
        mAbsPointerMode.AbsoluteMaxY = 32768;
    }
    mAbsPointerMode.AbsoluteMaxZ = 0;
    mAbsPointerMode.Attributes = 0;

    // Create a handle to use for the base Winmgr Client
    gBaseClientHandle = NULL;
    Status = gBS->InstallProtocolInterface (&gBaseClientHandle,&gBaseClientGuid,EFI_NATIVE_INTERFACE,NULL );
    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to create Base Client Handle. Code=%r\n", Status));
    }

    // Register with the Simple Window Manager to get mouse and touch input events.
    //
    ZeroMem (&FrameRect, sizeof(FrameRect));      // No coordinates, but always matches....
    Status = SWMRegisterClient(&mSWM.SWMProtocol,
                               gBaseClientHandle,
                               SWM_Z_ORDER_BASE,
                               &FrameRect,
                               NULL,
                               NULL,
                               &mSWM.UserAbsolutePointerProtocol,
                               NULL
                              );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to register initial default client: %r.\r\n", Status));
        goto Exit;
    }

    Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                                                     &gEfiAbsolutePointerProtocolGuid,
                                                     (VOID*)mSWM.UserAbsolutePointerProtocol,
                                                     NULL
                                                    );

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to install the Absolute Pointer protocol interface, Status: %r\r\n", Status));
        goto Exit;
    }

    // Install the Simple Window Manager Protocol.
    //
    mSWM.SWMProtocol.RegisterClient             = SWMRegisterClient;
    mSWM.SWMProtocol.UnregisterClient           = SWMUnregisterClient;
    mSWM.SWMProtocol.ActivateWindow             = SWMActivateWindow;
    mSWM.SWMProtocol.SetWindowFrame             = SWMSetWindowFrame;
    mSWM.SWMProtocol.BltWindow                  = SWMBltWindow;
    mSWM.SWMProtocol.StringToWindow             = SWMStringToWindow;
    mSWM.SWMProtocol.EnableMousePointer         = SWMEnableMousePointer;
    mSWM.SWMProtocol.WaitForEvent               = SWMWaitForEvent;

    Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                                                     &gMsSWMProtocolGuid,
                                                     (VOID**)&mSWM.SWMProtocol,
                                                     NULL
                                                    );

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to install the Simple Window Manager protocol interface, Status: %r\r\n", Status));
        goto Exit;
    }

    // Create a periodic timer event for checking the Absolute Pointer protocol watchlist.
    //
    Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               CheckWatchListCallback,
                               NULL,
                               &mSWMWatchListTimerEvent
                              );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to create timer callback event for re-enabling the mouse pointer.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Initialize the Simple Window Manager.
    //
    InitializeWindowManagerContext();

    // Start periodic timer for scanning AP watchlist.
    //
    Status = gBS->SetTimer (mSWMWatchListTimerEvent,
                            TimerPeriodic,
                            PERIODIC_REFRESH_INTERVAL
                           );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to start provider watchlist scanning timer.  Status = %r\r\n", Status));
        goto Exit;
    }

    // Initialize the Simple UI ToolKit (for dialog rendering, etc.).  Note that the SUIT has a dependency on the SWM protocol so this call
    // needs to happen after we've called InstallMultipleProtocolInterfaces.
    //
    Status = InitializeUIToolKit(ImageHandle);

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to initialize UI toolkit (%r).\r\n", Status));
        goto Exit;
    }

    // Install Driver Binding Protocol to catch Absolute Pointer providers
    //
    Status = EfiLibInstallDriverBindingComponentName2 (ImageHandle,
                                                       gST,
                                                       &mSWMDriverBinding,
                                                       ImageHandle,
                                                       NULL,
                                                       NULL
                                                      );

    ASSERT_EFI_ERROR (Status);

Exit:

    return Status;
}


/**
  GOP registration notification callback

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event contxt.

  @retval None.

**/
VOID
EFIAPI
GopRegisteredCallback (IN  EFI_EVENT    Event,
                       IN  VOID         *Context)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // If we already found the Graphics Output Protocol we want, there's nothing to do.
    //
    if (NULL != mGop)
    {
        goto Exit;
    }

    // Determine if the Graphics Output Protocol is available on the Console Out handle.
    //
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid,
                                  NULL,
                                  (VOID **)&mGop
                                 );

    if (EFI_ERROR (Status))
    {
        mGop = NULL;
        goto Exit;
    }

    mAbsPointerMode.AbsoluteMaxX = mGop->Mode->Info->HorizontalResolution;
    mAbsPointerMode.AbsoluteMaxY = mGop->Mode->Info->VerticalResolution;

    // Determine if the Simple Rendering Engine Protocol is available on the same Console Out handle.  The
    // Rendering Engine driver provides both Graphics Output and Rendering Engine protocols.
    //
    Status = gBS->LocateProtocol (&gMsSREProtocolGuid,
                                  NULL,
                                  (VOID **)&mRenderingEngine
                                 );

    if (EFI_ERROR (Status))
    {
        goto Exit;
    }

    // Now that we found the Graphics Output Protocol, complete the second half of driver initialization.
    //
    Status = DriverInitStage2(mImageHandle);

    // Unfortunately we can't return an error status from this routine or fail driver initialization but we can
    // ensure that our own protocol isn't published by cleaning up.  This will also close the registration event
    // that triggers the call to this callback function.
    //
    //ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to complete stage two driver initialization (%r).\r\n", Status));
        DriverCleanUp(mImageHandle);
        goto Exit;
    }

Exit:

    return;
}


/**
  Main entry point for this driver.

  @param[in] ImageHandle    Image handle this driver.
  @param[in] SystemTable    Pointer to SystemTable.

  @retval EFI_SUCCESS           This function always complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to initialize.

**/
EFI_STATUS
EFIAPI
DriverInit (IN EFI_HANDLE         ImageHandle,
            IN EFI_SYSTEM_TABLE  *SystemTable)
{
    EFI_STATUS      Status = EFI_SUCCESS;


    // Save the image handle for later use.
    //
    mImageHandle = ImageHandle;

    // Acquire access to the current theme.
    mTheme = MsUiGetPlatformTheme ();

    // Determine if the Graphics Output Protocol is available on the Console Out handle.
    //
    // NOTE: Since we use the Driver Binding Protocol to catch Absolute Pointer Protocol provider registrations
    // and removals, our load order can't be based on Depex.  As such, we may load before GOP has been registered
    // in which case we need to register for GOP registration notifications to perform the latter half of our driver
    // initialization.  If we're lucky and GOP is available now, we can do it all here in one pass.
    //
    Status = gBS->HandleProtocol (gST->ConsoleOutHandle,
                                  &gEfiGraphicsOutputProtocolGuid,
                                  (VOID **)&mGop
                                 );

    if (!EFI_ERROR (Status))
    {
        // Graphics Output Protocol is available now, complete driver initialization.
        //
        Status = DriverInitStage2(ImageHandle);
        goto Exit;
    }

    // Graphics Output Protocol isn't availble now. Register for Graphics Output Protocol registration notifications.
    //
    mGop = NULL;

    Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                               TPL_CALLBACK,
                               GopRegisteredCallback,
                               NULL,
                               &mGopRegisterEvent
                              );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "INFO [SWM]: Failed to create GOP registration event (%r).\r\n", Status));
        goto Exit;
    }

    Status = gBS->RegisterProtocolNotify (&gEfiGraphicsOutputProtocolGuid,
                                          mGopRegisterEvent,
                                          &mGopRegistration
                                         );

    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR, "INFO [SWM]: Failed to register for GOP registration notifications (%r).\r\n", Status));
        goto Exit;
    }

Exit:

    return Status;
}


/**
  Driver Clean-Up

  @retval EFI_SUCCESS       This function always complete successfully.

**/
static EFI_STATUS
EFIAPI
DriverCleanUp (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;


    // Close the Graphics Output Protocol registration notification event.
    //
    if (NULL != mGopRegisterEvent)
    {
        Status = gBS->CloseEvent (mGopRegisterEvent);
    }

    // Cancel and clean-up watchlist timer.
    //
    if (NULL != mSWMWatchListTimerEvent)
    {
        // Cancel the provider watchlist scanning timer.
        //
        Status = gBS->SetTimer (mSWMWatchListTimerEvent,
                                TimerCancel,
                                0
                               );

        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR, "ERROR [SWM]: Failed to cancel provider watchlist scanning timer.  Status = %r\r\n", Status));
            goto Exit;
        }

        // Close the timer event.
        //
        Status = gBS->CloseEvent (mSWMWatchListTimerEvent);
    }

    // Clean up the provider watchlist.
    //
    FreeAbsolutePointerInterfaceWatchList();

    // Uninstall the Simple Window Manager protocol.
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (ImageHandle,
                                                       &gMsSWMProtocolGuid,
                                                       (VOID**)&mSWM.SWMProtocol,
                                                       NULL
                                                      );

    if (EFI_ERROR (Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to uninstall the Simple Window Manager protocol interface, Status: %r\r\n", Status));
        goto Exit;
    }

    // Free all the client buffers.
    //
    WINMGR_CLIENT   *pClient = mSWM.Clients;
    WINMGR_CLIENT   *pNext   = NULL;
    while (NULL != pClient)
    {
        pNext = pClient->pNext;
        FreePool(pClient);
        pClient = pNext;
    }

    // Hide the mouse pointer and free blt buffers.
    //
    HideMousePointer();

Exit:

    return Status;
}


/**
  Driver unload handler.

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS       This function always complete successfully.

**/
EFI_STATUS
EFIAPI
DriverUnload (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;

    Status = DriverCleanUp(ImageHandle);

    return Status;
}
