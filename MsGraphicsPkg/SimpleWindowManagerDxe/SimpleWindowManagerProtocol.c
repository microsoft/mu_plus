/** @file

  Implements a Simple Window Manager (SWM) protocol

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


/**
    Resets the aggregate pointer event state queue and providers.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] ExtendedVerifiation  Ignored.

    @retval EFI_SUCCESS             Successfully reset.

**/
EFI_STATUS
SWMAbsolutePointerReset (IN EFI_ABSOLUTE_POINTER_PROTOCOL *this,
                         IN BOOLEAN                        ExtendedVerification)
{
    EFI_STATUS                      Status      = EFI_SUCCESS;
    WINMGR_AP_WATCHLIST            *pProvider  = mSWM.AbsolutePointerProviders;
    WINMGR_CLIENT                  *Client = WINMGR_CLIENT_FROM_ABS_PTR(this);
    EFI_TPL                         PreviousTPL;


    DEBUG((DEBUG_INFO, "INFO [SWM]: Purging event queue and resetting all Absolute Pointer sources.\r\n"));


    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Purge the event queue (removes old pending events).
    //
    Client->Queue.bQueueEmpty     = TRUE;
    Client->Queue.QueueInputPosition = 0;
    Client->Queue.QueueOutputPosition = 0;

    // Restore the TPL
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    // Call each aggregated Absolute Pointer protocol providers Reset function.
    //
    while (pProvider != NULL)
    {
        Status = pProvider->AbsolutePointer->Reset(pProvider->AbsolutePointer, ExtendedVerification);

        if (EFI_ERROR(Status))
        {
            DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to reset Absolute Pointer (Provider=0x%x), Status: %r\r\n", (UINTN)pProvider, Status));
            goto Exit;
        }
        pProvider = pProvider->pNext;
    }

Exit:
    return Status;
}


/**
    Get pointer state from the aggregate pointer event state queue.

    @param[in]  This        Pointer to the instance of this driver.
    @param[out] State       Pointer state to be filled in from queue data.

    @retval EFI_SUCCESS     Successfully retrieved state from the queue.
    @retval EFI_NOT_READY   Nothing in the queue.

**/
EFI_STATUS
SWMAbsolutePointerGetState (IN EFI_ABSOLUTE_POINTER_PROTOCOL     *this,
                            IN OUT MS_SWM_ABSOLUTE_POINTER_STATE *State)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    WINMGR_CLIENT  *Client = WINMGR_CLIENT_FROM_ABS_PTR(this);

    // Check whether there's data pending in the pointer state input queue.
    //
    if (TRUE == Client->Queue.bQueueEmpty)
    {
        Status = EFI_NOT_READY;
        goto Exit;
    }
    gBS->CheckEvent (this->WaitForInput);
    // Retrieve pointer state from the aggregated event queue and return it.
    //
    Status = ExtractAbsolutePointerEventFromQueue (Client, State);

Exit:

    return Status;
}

/**
    Checks the AP event queue for a pending event.

    @param[in] Event            Client event to be signalled if there's a pending event in the aggregate event queue.
    @param[in] Context          Pointer to the client context.

    @retval None.

**/
VOID
EFIAPI
AbsolutePointerWaitForInput (IN  EFI_EVENT    Event,
                             IN  VOID         *Context)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    MS_SWM_ABSOLUTE_POINTER_STATE   PointerState;
    WINMGR_CLIENT                  *Client = (WINMGR_CLIENT*) Context;

    // Peek at the next event in the queue (if there is one).
    //
    Status = PeekAtAbsolutePointerEventInQueue (Client, &PointerState);
    if (EFI_ERROR(Status))
    {
        goto Exit;
    }

    // There's pointer event data in the queue, signal the event.
    //
    gBS->SignalEvent (Event);

Exit:
    return;
}

/**
    Registers the specified client for receiving Simple Window Manager services.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be supported.
    @param[in]  Z_Order      Where in the Z_Order does this Client exist.
    @param[in]  FrameRect    Client's window frame rectangle.
    @param[in]  DataNotificationCallback Routine to be called when data is avialable for this client. This routine is optional.
    @param[in]  Context      Pointer from Client to be passed into the DataNotificationCallback function.
    @param[out] AbsolutePointerProtocol  Where to store the clients AbsolutePointerProtocol pointer.
    @param[out] PaintEvent   Per-client event to be provided used for screen paint event notifications (NULL means no window surface required).

    @retval EFI_SUCCESS             Successfully registered the client.
    @retval EFI_ALREADY_STARTED     Client already registered.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to register the client.

**/
EFI_STATUS
EFIAPI
SWMRegisterClient (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                   IN  EFI_HANDLE                           ImageHandle,
                   IN  UINT32                               Z_Order,
                   IN  SWM_RECT                            *FrameRect,
                   IN  MS_SWM_CLIENT_NOTFICATION_CALLBACK   DataNotificationCallback OPTIONAL,
                   IN  VOID                                *Context,
                   OUT EFI_ABSOLUTE_POINTER_PROTOCOL      **AbsolutePointer,
                   OUT EFI_EVENT                           *PaintEvent)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    WINMGR_CLIENT   *pList = mSWM.Clients;
    WINMGR_CLIENT   *NewClient;
    WINMGR_CLIENT   *pPrev;
    EFI_TPL         PreviousTPL = 0;


    DEBUG((DEBUG_INFO, "INFO [SWM]: Registering new client (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

    // Insure Z_Order requirement of BASE must be first registered client.
    if (((Z_Order == SWM_Z_ORDER_BASE) && (pList != NULL)) ||
        ((Z_Order != SWM_Z_ORDER_BASE) && (pList == NULL))) {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }
    // Check whether this client has already been registered.
    //
    while (pList != NULL)
    {
        if (pList->ImageHandle == ImageHandle)
        {
            Status = EFI_ALREADY_STARTED;
            goto Exit;
        }
        pList = pList->pNext;
    }

    // Allocate a new node for this client.
    //
    NewClient = (WINMGR_CLIENT *) AllocateZeroPool(sizeof(WINMGR_CLIENT));
    ASSERT (NewClient != NULL);
    if (NULL == NewClient)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    // Capture client information.
    //
    NewClient->Signature             = WINMGR_CLIENT_SIGNATURE;
    NewClient->pNext                 = NULL;
    NewClient->pPrev                 = NULL;
    NewClient->ImageHandle           = ImageHandle;
    NewClient->Active                = FALSE;
    NewClient->HasDisplaySurface     = FALSE;
    NewClient->ClientAbsPtr.Mode     = &mAbsPointerMode;
    NewClient->Z_Order               = Z_Order;
    NewClient->DataNotificationCallback = DataNotificationCallback;
    NewClient->DataNotificationContext  = Context;
    NewClient->ClientAbsPtr.Reset    = SWMAbsolutePointerReset;            // SWM functions
    NewClient->ClientAbsPtr.GetState = SWMAbsolutePointerGetState;
    NewClient->Queue.bQueueEmpty     = TRUE;
    NewClient->Queue.QueueInputPosition = 0;
    NewClient->Queue.QueueOutputPosition = 0;
    // Return Abs Pointer Protocol to client
    *AbsolutePointer = &NewClient->ClientAbsPtr;

    // Clients that have a data notification callback don't need a WaitForInput event.
    // Creating it for compatibility in case the client uses it.
    Status = gBS->CreateEvent(EVT_NOTIFY_WAIT,
                              TPL_NOTIFY,
                              AbsolutePointerWaitForInput,
                              (VOID *)NewClient,
                              &NewClient->ClientAbsPtr.WaitForInput
                              );

    CopyMem (&NewClient->WindowFrame, FrameRect, sizeof(SWM_RECT));

    // Create a surface if caller provided a paint event.
    //
    if (NULL != PaintEvent)
    {
        NewClient->HasDisplaySurface = TRUE;

        // Create a rendering engine surface for this client window.
        //
        Status = mRenderingEngine->CreateSurface (mRenderingEngine,
                                                  ImageHandle,
                                                  *FrameRect,
                                                  PaintEvent
                                                 );

        if (EFI_ERROR (Status))
        {
            DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to create rendering engine surface, Status: %r\r\n", Status));
            FreePool(NewClient);
            goto Exit;
        }
    }

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    DEBUG((DEBUG_INFO, "INFO [SWM]: Registering Image %p with event=%p.\r\n", NewClient->ImageHandle,NewClient->ClientAbsPtr.WaitForInput));

    // Attach it to the list in Z_Order
    // Z_Order == 0 is always added first, so we only have to add in front of a current entry.
    if (mSWM.Clients == NULL) {       // Adding first element to the Queue
        mSWM.Clients = NewClient;
    } else {
        pList = mSWM.Clients;
        pPrev = NULL;
        while (pList != NULL)
        {
            if (NewClient->Z_Order > pList->Z_Order)   // Z_Order - 0 is bottom window
            {
                if (pPrev == NULL) {               // adding to the head of the queue
                    NewClient->pNext = mSWM.Clients;
                    if (mSWM.Clients != NULL) {
                        mSWM.Clients->pPrev = NewClient;
                    }
                    mSWM.Clients = NewClient;
                } else {                           // Inserting before current node
                    NewClient->pPrev = pPrev;
                    NewClient->pNext = pList;
                    pList->pPrev = NewClient;
                    pPrev->pNext = NewClient;
                }
                break;
            }
            pPrev = pList;
            pList = pList->pNext;
        }
    }

Exit:

    // Restore the TPL
    //
    if (PreviousTPL)
    {
        gBS->RestoreTPL (PreviousTPL);
    }

    // Display client list for debugging purposes.
    //
    DEBUG((DEBUG_INFO, "INFO [SWM]: Client list:\r\n"));
    pList = mSWM.Clients;
    while (pList != NULL)
    {
        DEBUG((DEBUG_INFO, "            - ImageHandle=0x%x, Active=%s, Z=%3d, Event=%p Window=L[%d]:R[%d]:T[%d]:B[%d]\r\n", (UINTN)pList->ImageHandle,
                                                                                                                            (pList->Active ? L"YES" : L"NO"),
                                                                                                                             pList->Z_Order,
                                                                                                                             pList->ClientAbsPtr.WaitForInput,
                                                                                                                             pList->WindowFrame.Left,
                                                                                                                             pList->WindowFrame.Right,
                                                                                                                             pList->WindowFrame.Top,
                                                                                                                             pList->WindowFrame.Bottom));

        pList = pList->pNext;
    }

    return Status;
}


/**
    Unregisters the specified client and stop receiving Simple Window Manager services.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be unregistered.

    @retval EFI_SUCCESS             Successfully registered the client.
    @retval EFI_ALREADY_STARTED     Client already registered.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to register the client.

**/
EFI_STATUS
EFIAPI
SWMUnregisterClient (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                     IN  EFI_HANDLE                          ImageHandle)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    WINMGR_CLIENT                   *pList = mSWM.Clients;

    DEBUG((DEBUG_INFO, "INFO [SWM]: Unregistering client (ImageHandle=0x%x).\r\n", (UINTN)ImageHandle));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Set focus for the specified client (note this assumes we'll find a match for ImageHandle).
    //
    while (pList != NULL)
    {
        if (pList->ImageHandle == ImageHandle)
        {
            // Delete the rendering engine surface used for this client window.
            //
            if (pList->HasDisplaySurface) {
                Status = mRenderingEngine->DeleteSurface(mRenderingEngine,
                                                          ImageHandle
                                                         );

                if (EFI_ERROR (Status))
                {
                    DEBUG ((DEBUG_WARN, "WARN [SWM]: Failed to delete rendering engine surface, Status: %r\r\n", Status));
                }
            }

            // Unlink the current client node and free it.
            //
            if (NULL == pList->pPrev)
            {
                mSWM.Clients = pList->pNext;
                if (NULL != mSWM.Clients)
                {
                    mSWM.Clients->pPrev = NULL;
                }
            }
            else
            {
                pList->pPrev->pNext = pList->pNext;
                if (NULL != pList->pNext)
                {
                    pList->pNext->pPrev = pList->pPrev;
                }
            }
            gBS->CloseEvent(pList->ClientAbsPtr.WaitForInput);
            FreePool(pList);

            break;
        }

        pList = pList->pNext;
    }

    // Print out a debug message if we didn't remove anything.
    //
    if (NULL == pList)
    {
        DEBUG ((DEBUG_WARN, "WARN [SWM]: Failed to unregister client with image handle %p.\r\n", (UINTN)ImageHandle));
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    // Display client list for debugging purposes.
    //
    DEBUG((DEBUG_INFO, "INFO [SWM]: Client list:\r\n"));
    pList = mSWM.Clients;
    while (pList != NULL)
    {
        DEBUG((DEBUG_INFO, "            - ImageHandle=0x%x, Active=%s, Z=%3d, Event=%p, Window=L[%d]:R[%d]:T[%d]:B[%d]\r\n", (UINTN)pList->ImageHandle,
                                                                                                                             (pList->Active ? L"YES" : L"NO"),
                                                                                                                              pList->Z_Order,
                                                                                                                              pList->ClientAbsPtr.WaitForInput,
                                                                                                                              pList->WindowFrame.Left,
                                                                                                                              pList->WindowFrame.Right,
                                                                                                                              pList->WindowFrame.Top,
                                                                                                                              pList->WindowFrame.Bottom));
        pList = pList->pNext;
    }

    return Status;
}


/**
    Tells the Simple Window Manager that the client is active and will be handling events.

    NOTE: An active client that fails to process its events will block other clients from receiving
          their event notifications since we use a common FIFO to aggregate incoming events.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be supported.
    @param[in]  MakeActive   TRUE == Client is Active, FALSE == Client is Inactive.

    @retval EFI_SUCCESS             Successfully changed active state.
    @retval EFI_INVALID_PARAMETER   Failed to find client.

**/
EFI_STATUS
EFIAPI
SWMActivateWindow (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
                   IN EFI_HANDLE                           ImageHandle,
                   IN BOOLEAN                              MakeActive)
{
    EFI_STATUS      Status = EFI_INVALID_PARAMETER;
    WINMGR_CLIENT   *pList = mSWM.Clients;


    DEBUG ((DEBUG_INFO, "INFO [SWM]: Setting client active (ImageHandle=0x%x, MakeActive=%s).\r\n", (UINTN)ImageHandle, (TRUE == MakeActive ? L"TRUE" : L"FALSE")));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Set active state for the specified client (note this assumes we'll find a match for ImageHandle).
    //
    while (pList != NULL)
    {
        if (pList->ImageHandle == ImageHandle)
        {
            pList->Active  = MakeActive;
            Status         = EFI_SUCCESS;

            if (TRUE == pList->HasDisplaySurface)
            {
                // Activate the rendering engine surface used for this client window.
                //
                Status = mRenderingEngine->ActivateSurface (mRenderingEngine,
                                                            ImageHandle,
                                                            MakeActive
                                                           );

                if (EFI_ERROR (Status))
                {
                    DEBUG ((DEBUG_WARN, "WARN [SWM]: Failed to activate rendering engine surface (%r).\r\n", Status));
                }
            }

            break;
        }

        pList = pList->pNext;
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    return Status;
}


/**
    Sets the outer window frame (bouding rectangle) for the client window.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be supported.
    @param[in]  FrameRect    Window frame rectangle (bounding rectangle).

    @retval EFI_SUCCESS      Successfully set the window frame.

**/
EFI_STATUS
EFIAPI
SWMSetWindowFrame (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
                   IN EFI_HANDLE                           ImageHandle,
                   IN SWM_RECT                             *FrameRect)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    WINMGR_CLIENT   *pList = mSWM.Clients;


    DEBUG ((DEBUG_INFO, "INFO [SWM]: Setting client window frame (ImageHandle=0x%x, Frame=L[%d]:R[%d]:T[%d]:B[%d]).\r\n", (UINTN)ImageHandle,
                                                                                                                          FrameRect->Left,
                                                                                                                          FrameRect->Right,
                                                                                                                          FrameRect->Top,
                                                                                                                          FrameRect->Bottom));

    // Raise the TPL to avoid getting interrupted while we access shared data structures.
    //
    EFI_TPL  PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Locates the client by the image handle provided and sets the window frame.
    //
    while (pList != NULL)
    {
        if (pList->ImageHandle == ImageHandle)
        {
            CopyMem(&pList->WindowFrame, FrameRect, sizeof(SWM_RECT));

            // If the client has a display surface, resize it based on the new size specified.
            //
            if (TRUE == pList->HasDisplaySurface)
            {
                Status = mRenderingEngine->ResizeSurface (mRenderingEngine,
                                                          ImageHandle,
                                                          FrameRect
                                                         );

                if (EFI_ERROR (Status))
                {
                    DEBUG ((DEBUG_WARN, "WARN [SWM]: Failed to resize rendering engine surface (%r).\r\n", Status));
                }
            }
            break;
        }

        pList = pList->pNext;
    }

    // Check whether we found the specified client.
    //
    if (NULL == pList)
    {
        Status = EFI_NOT_FOUND;
        DEBUG ((DEBUG_ERROR, "ERROR [SWM]: Failed to update clients window frame (bounding rectangle), Status: %r\r\n", Status));
    }

    // Restore the TPL.
    //
    gBS->RestoreTPL (PreviousTPL);

    return Status;
}


/**
    Performs a block copy (blit) to the client window associated with the image handle provided.  Blitting to
    a window is different from general GOP blitting in that the rendering engine knows to ignore any rectangle overlaps
    with the client's window.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client (window) to be supported.
    @param[in]  BltBuffer    The data to transfer to the graphics screen.
                             Size is at least Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL).
    @param[in]  BltOperation The operation to perform when copying BltBuffer on to the graphics screen.
    @param[in]  SourceX      The X coordinate of source for the BltOperation.
    @param[in]  SourceY      The Y coordinate of source for the BltOperation.
    @param[in]  DestinationX The X coordinate of destination for the BltOperation.
    @param[in]  DestinationY The Y coordinate of destination for the BltOperation.
    @param[in]  Width        The width of a rectangle in the blt rectangle in pixels.
    @param[in]  Height       The height of a rectangle in the blt rectangle in pixels.
    @param[in]  Delta        Not used for EfiBltVideoFill or the EfiBltVideoToVideo operation.
                             If a Delta of zero is used, the entire BltBuffer is being operated on.
                             If a subrectangle of the BltBuffer is being used then Delta
                             represents the number of bytes in a row of the BltBuffer.

    @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
    @retval EFI_INVALID_PARAMETER BltOperation is not valid.
    @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.

**/
EFI_STATUS
EFIAPI
SWMBltWindow (IN  MS_SIMPLE_WINDOW_MANAGER_PROTOCOL       *This,
              IN  EFI_HANDLE                              ImageHandle,
              IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,   OPTIONAL
              IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
              IN  UINTN                                   SourceX,
              IN  UINTN                                   SourceY,
              IN  UINTN                                   DestinationX,
              IN  UINTN                                   DestinationY,
              IN  UINTN                                   Width,
              IN  UINTN                                   Height,
              IN  UINTN                                   Delta         OPTIONAL)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    // Denote the start of surface updating.
    //
    mRenderingEngine->SetModeSurface (mRenderingEngine,
                                      ImageHandle,
                                      PAINT_BEGIN
                                     );

    // Update the surface.
    //
    Status = mGop->Blt (mGop,
                        BltBuffer,
                        BltOperation,
                        SourceX,
                        SourceY,
                        DestinationX,
                        DestinationY,
                        Width,
                        Height,
                        Delta
                       );

    // Denote the end of surface updating.
    //
    mRenderingEngine->SetModeSurface (mRenderingEngine,
                                      ImageHandle,
                                      PAINT_END
                                     );

    return Status;
}


/**
    Draws a string in the specified format to a client window associated with the specified image handle.  Drawing to
    a window is different from general StringToImage in that the rendering engine knows to ignore any rectangle overlaps
    with the client's window.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client (window) to be supported.
    @param[in]  Flags        Describes how the string is to be drawn.

    @param[in]  String       Points to the null-terminated string to be

    @param[in]  StringInfo  Points to the string output information,
                            including the color and font. If NULL, then
                            the string will be output in the default
                            system font and color.

    @param[in]  Blt         If this points to a non-NULL on entry, this points
                            to the image, which is Width pixels wide and
                            Height pixels high. The string will be drawn onto
                            this image and EFI_HII_OUT_FLAG_CLIP is implied.
                            If this points to a NULL on entry, then a buffer
                            will be allocated to hold the generated image and
                            the pointer updated on exit. It is the caller's
                            responsibility to free this buffer.

    @param[in]  BltX, BltY  Specifies the offset from the left and top
                            edge of the image of the first character
                            cell in the image.

    @param[in]  RowInfoArray        If this is non-NULL on entry, then on
                                    exit, this will point to an allocated buffer
                                    containing row information and
                                    RowInfoArraySize will be updated to contain
                                    the number of elements. This array describes
                                    the characters that were at least partially
                                    drawn and the heights of the rows. It is the
                                    caller's responsibility to free this buffer.

    @param[in]  RowInfoArraySize    If this is non-NULL on entry, then on
                                    exit it contains the number of
                                    elements in RowInfoArray.

    @param[in]  ColumnInfoArray     If this is non-NULL, then on return it
                                    will be filled with the horizontal
                                    offset for each character in the
                                    string on the row where it is
                                    displayed. Non-printing characters
                                    will have the offset ~0. The caller is
                                    responsible for allocating a buffer large
                                    enough so that there is one entry for
                                    each character in the string, not
                                    including the null-terminator. It is
                                    possible when character display is
                                    normalized that some character cells
                                    overlap.

    @retval EFI_SUCCESS             The string was successfully updated.
    @retval EFI_OUT_OF_RESOURCES    Unable to allocate an output buffer for RowInfoArray or Blt.
    @retval EFI_INVALID_PARAMETER   The String or Blt was NULL.
    @retval EFI_INVALID_PARAMETER   Flags were invalid combination.

**/
EFI_STATUS
EFIAPI
SWMStringToWindow (IN        MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                   IN        EFI_HANDLE                          ImageHandle,
                   IN        EFI_HII_OUT_FLAGS                   Flags,
                   IN        EFI_STRING                          String,
                   IN        EFI_FONT_DISPLAY_INFO               *StringInfo,
                   IN OUT    EFI_IMAGE_OUTPUT                    **Blt,
                   IN        UINTN                               BltX,
                   IN        UINTN                               BltY,
                   OUT       EFI_HII_ROW_INFO                    **RowInfoArray OPTIONAL,
                   OUT       UINTN                               *RowInfoArraySize OPTIONAL,
                   OUT       UINTN                               *ColumnInfoArray OPTIONAL)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    // Denote the start of surface updating.
    //
    mRenderingEngine->SetModeSurface (mRenderingEngine,
                                      ImageHandle,
                                      PAINT_BEGIN
                                     );

    // Update the surface.
    //
    Status = mFont->StringToImage (mFont,
                                   Flags,
                                   String,
                                   StringInfo,
                                   Blt,
                                   BltX,
                                   BltY,
                                   RowInfoArray,
                                   RowInfoArraySize,
                                   ColumnInfoArray
                                  );

    // Denote the end of surface updating.
    //
    mRenderingEngine->SetModeSurface (mRenderingEngine,
                                      ImageHandle,
                                      PAINT_END
                                     );

    return Status;
}


/**
    Enables the mouse pointer to be displayed if the Absolute Pointer provider is a "mouse" (i.e., not touch).

    @param[in]  This            Pointer to the instance of this driver.
    @param[in]  bEnableMouse    TRUE == Display mouse pointer.

    @retval EFI_SUCCESS      Successfully set the window frame.

**/
EFI_STATUS
EFIAPI
SWMEnableMousePointer (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                       IN BOOLEAN                             bEnableMouse)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // Save the enabled state for later.
    //
    mSWM.bMousePointerEnabled       = bEnableMouse;

    // Hide the mouse pointer if we're disabling it.
    //
    if (FALSE == bEnableMouse)
    {
        Status = HideMousePointer();
    }
    else if (TRUE == mSWM.bLastMoveRequiredMousePointer)
    {
        // If the last absolute pointer event required rendering the mouse pointer, we'll
        // render it immediately here instead of waiting for the next event.
        //
        Status = ShowMousePointer();
    }

    return Status;
}