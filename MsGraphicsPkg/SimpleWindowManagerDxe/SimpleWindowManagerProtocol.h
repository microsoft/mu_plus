/** @file

  Implements a simple window manager (SWM) protocol constants

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

#ifndef _SIMPLE_WINDOW_MANAGER_PROTOCOL_H_
#define _SIMPLE_WINDOW_MANAGER_PROTOCOL_H_


/**
    Resets the aggregate pointer event state queue and providers.

    @param[in] This                 Pointer to the instance of this driver.
    @param[in] ExtendedVerifiation  Ignored.

    @retval EFI_SUCCESS             Successfully reset.

**/
EFI_STATUS
EFIAPI
SWMAbsolutePointerReset (IN EFI_ABSOLUTE_POINTER_PROTOCOL    *This,
                         IN BOOLEAN                           ExtendedVerification);

/**
    Get pointer stat from the aggregate pointer event state queue.

    @param[in]  This        Pointer to the instance of this driver.
    @param[out] State       Pointer state to be filled in from queue data.

    @retval EFI_SUCCESS     Successfully retrieved state from the queue.
    @retval EFI_NOT_READY   Nothing in the queue.

**/
EFI_STATUS
EFIAPI
SWMAbsolutePointerGetState (IN EFI_ABSOLUTE_POINTER_PROTOCOL      *This,
                            IN OUT MS_SWM_ABSOLUTE_POINTER_STATE  *State);

/**
    Registers the specified client for receiving Simple Window Manager services.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be supported.
    @param[in]  Flags        Client requirements.
    @param[in]  FrameRect    Client's window frame rectangle.
    @param[in]  DataNotificationCallback Routine to be called when data is avialable for this client. This routine runs at TPL_NOTIFY.
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
                   IN  UINT32                               Flags,
                   IN  SWM_RECT                            *FrameRect,
                   IN  MS_SWM_CLIENT_NOTFICATION_CALLBACK   DataNotificationCallback OPTIONAL,
                   IN  VOID                                *Context,
                   OUT EFI_ABSOLUTE_POINTER_PROTOCOL      **AbsolutePointer,
                   OUT EFI_EVENT                           *PaintEvent);


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
                     IN  EFI_HANDLE                          ImageHandle);


/**
    Tells the Simple Window Manager that the client is active and will be handling events.

    NOTE: An active client that fails to process its events will block other clients from receiving
          their event notifications since we use a common FIFO to aggregate incoming events.

    @param[in]  This         Pointer to the instance of this driver.
    @param[in]  ImageHandle  Image handle representing the client to be supported.
    @param[in]  MakeActive   TRUE == Client is Active, FALSE == Client is Inactive.

    @retval EFI_SUCCESS      Successfully changed active state.

**/
EFI_STATUS
EFIAPI
SWMActivateWindow (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL    *This,
                   IN EFI_HANDLE                           ImageHandle,
                   IN BOOLEAN                              MakeActive);


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
                   IN SWM_RECT                             *FrameRect);


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
              IN  UINTN                                   Delta         OPTIONAL);


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
                   OUT       UINTN                               *ColumnInfoArray OPTIONAL);


/**
    Enables the mouse pointer to be displayed if the Absolute Pointer provider is a "mouse" (i.e., not touch).

    @param[in]  This            Pointer to the instance of this driver.
    @param[in]  bEnableMouse    TRUE == Display mouse pointer.

    @retval EFI_SUCCESS      Successfully set the window frame.

**/
EFI_STATUS
EFIAPI
SWMEnableMousePointer (IN MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *This,
                       IN BOOLEAN                             bEnableMouse);

/**
 * Wait for an event, and display the POWER OFF dialog if the Power Timer expires
 *
 * @param NumberOfEvents   The number of events the user is waiting on
 * @param Events           Array of events the user is waiting on
 * @param EventTypes       Array of event types the user is waiting on
 * @param Timeout          Time out (allows for refresh, etc) as long as < POWER OFF timer
 * @param ContinueTimer    Continues the PowerOff timer - don't count the last interation
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
SWMWaitForEvent (IN UINTN           NumberOfEvents,
                 IN EFI_EVENT      *Events,
                 IN UINTN          *Index,
                 IN UINT64          Timeout,
                 IN BOOLEAN         ContinueTimer);

#endif  // _SIMPLE_WINDOW_MANAGER_PROTOCOL_H_