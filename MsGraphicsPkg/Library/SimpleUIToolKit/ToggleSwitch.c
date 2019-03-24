/** @file

  Implements a simple toggle switch control.

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

#include "SimpleUIToolKitInternal.h"


#define UIT_TS_HIGHLIGHT_RING_WIDTH     MsUiScaleByTheme(4)   // Number of pixels making up the toggle switch's highlight outer border (ring).
#define UIT_TS_HIGHTLIGHT_GAP_WIDTH     MsUiScaleByTheme(5)   // Number of pixels making up the gap between the switch and its highlight ring.
#define UIT_TS_OUTER_BORDER_WIDTH       MsUiScaleByTheme(6)   // Number of pixels making up the switch's outer border.
#define UIT_TS_INNER_GAP_WIDTH          MsUiScaleByTheme(6)   // Number of pixels between the outer border and inner switch "bullet".

//////////////////////////////////////////////////////////////////////////////
// Private
//

/**
    Draws a horizontal line in the color specified, starting from the (X,Y) pixel position within
    the specified bitmap buffer and extending right for the specified number of pixels.

    NOTE: the caller must ensure that the line lies within the bitmap buffer provided.

    @param[in]      this                Pointer to a ToggleSwitch.
    @param[in]      X                   Line X-coordinate starting position within Bitmap.
    @param[in]      Y                   Line Y-coordinate starting position within Bitmap.
    @param[in]      NumberOfPixels      Number of pixels making up the line, extending from (X,Y) to the right.
    @param[in]      Color               Line color.
    @param[out]     Bitmap              Bitmap buffer that will be draw into.

    @retval         EFI_SUCCESS

**/
static
EFI_STATUS
DrawHorizontalLine (IN  ToggleSwitch                    *this,
                    IN  UINT32                          X,
                    IN  UINT32                          Y,
                    IN  UINT32                          NumberOfPixels,
                    IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Color,
                    OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Bitmap)
{
    UINT32  PixelOffset = ((Y * this->m_pToggleSwitch->SwitchBitmapWidth) + X);
    UINT32  *FirstPixel = (UINT32 *)(Bitmap + PixelOffset);

    // Fill the bitmap buffer with a series of pixels from the starting pixel position.  Note that this SetMem32 should be
    // calling the SSE-accelerated version for best performance.
    //
    SetMem32(FirstPixel, (NumberOfPixels * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)), *(UINT32 *)Color);

    return  EFI_SUCCESS;
}


/**
    Draws a (potentially) elongated circle.  The elongated circle is defined by an (X,Y) origin which describes the upper
    left corner of the rectangle of the given width and height to which two half-circles are attached left and right.  A width
    of zero results in an actual circle.

                         (OrigX,OrigY)
                             , * ~ ~ ~ ~ ~  ,
                         , '   |          |   ' ,
                       ,       |          |       ,
                      ,      H |          |        ,
                     ,       E |          |         ,
                     ,       I |          |         ,
                     ,       G |          |         ,
                      ,      H |          |        ,
                       ,     T |          |       ,
                         ,     |          |    , '
                           ' - | _ _ _ _ _|, '
                                  WIDTH

    NOTE: the caller must ensure that the line lies within the bitmap buffer provided.


    @param[in]      this                Pointer to a ToggleSwitch.
    @param[in]      OrigX               Origin X-coordinate of the rectangle described above.
    @param[in]      OrigY               Origin Y-coordinate of the rectangle described above.
    @param[in]      Width               Width of the rectangle described above.
    @param[in]      Height              Height of the rectangle described above.
    @param[in]      FillColor           Color to be used when filling in the elongated circle.
    @param[out]     Bitmap              Bitmap buffer that will be draw into.

    @retval         EFI_SUCCESS

**/
static
EFI_STATUS
DrawElongatedCircle (IN   ToggleSwitch                    *this,
                     IN   UINT32                          OrigX,
                     IN   UINT32                          OrigY,
                     IN   UINT32                          Width,
                     IN   UINT32                          Height,
                     IN   EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *FillColor,
                     OUT  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Bitmap)
{
    UINT32      HalfHeight = (Height / 2);
    UINT32      Step;
    UINT32      Xstart, Xend, Length;
    UINT32      Y1, Y2;
    double      Xarc;


    // Starting from the vertical center, step to the outer edge of the circle, calculate the line starting position and
    // length making up the horizontal slice and mirror it to create both the top and bottom halves of the circle.
    //
    for (Step=0 ; Step < HalfHeight ; Step++)
    {
        Xarc    = sqrt_d((double)((HalfHeight * HalfHeight) - (Step * Step)));     // Not sure why '^' doesn't work.

        Xstart  = (OrigX - (UINT32)Xarc);
        Xend    = (OrigX + Width + (UINT32)Xarc);
        Length  = (Xend - Xstart);

        Y1 = (OrigY + HalfHeight - Step);
        Y2 = (OrigY + HalfHeight + Step);

        DrawHorizontalLine (this, Xstart, Y1, Length, FillColor, Bitmap);
        DrawHorizontalLine (this, Xstart, Y2, Length, FillColor, Bitmap);
    }


    return EFI_SUCCESS;
}

/**
    Creates toggle switch "on" and "off" bitmaps which are cached and rendered later based on switch state.

    @param[in]      this                Pointer to a ToggleSwitch.

    @retval         EFI_SUCCESS
                    EFI_OUT_OF_RESOURCES

**/
static
EFI_STATUS
CreateToggleSwitchBitmaps (IN ToggleSwitch *this)
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    UINT32      OrigX   = 0;
    UINT32      OrigY   = 0;
    UINT32      Width   = 0;
    UINT32      Height  = 0;
    UINT32      BitmapSize;
    UINT32      AdjustedOrigX;
    UINT32      AdjustedWidth;


    // Calculate the width and height of the bitmaps, leaving room for the keyboard TAB highlight ring.
    //
    Width       = (this->m_pToggleSwitch->ToggleSwitchBounds.Right  - this->m_pToggleSwitch->ToggleSwitchBounds.Left + 1);
    Height      = (this->m_pToggleSwitch->ToggleSwitchBounds.Bottom - this->m_pToggleSwitch->ToggleSwitchBounds.Top + 1);
    BitmapSize  = (Width * Height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    // Save width and height for later.
    //
    this->m_pToggleSwitch->SwitchBitmapWidth  = Width;
    this->m_pToggleSwitch->SwitchBitmapHeight = Height;


    // Allocate bitmap buffers.
    //
    this->m_pToggleSwitch->SwitchOnBitmap = AllocatePool (BitmapSize);

    ASSERT (NULL != this->m_pToggleSwitch->SwitchOnBitmap);
    if (NULL == this->m_pToggleSwitch->SwitchOnBitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    this->m_pToggleSwitch->SwitchOffBitmap = AllocatePool (BitmapSize);

    ASSERT (NULL != this->m_pToggleSwitch->SwitchOffBitmap);
    if (NULL == this->m_pToggleSwitch->SwitchOffBitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    this->m_pToggleSwitch->GrayedSwitchOnBitmap = AllocatePool(BitmapSize);

    ASSERT(NULL != this->m_pToggleSwitch->GrayedSwitchOnBitmap);
    if (NULL == this->m_pToggleSwitch->GrayedSwitchOnBitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    this->m_pToggleSwitch->GrayedSwitchOffBitmap = AllocatePool(BitmapSize);

    ASSERT(NULL != this->m_pToggleSwitch->GrayedSwitchOffBitmap);
    if (NULL == this->m_pToggleSwitch->GrayedSwitchOffBitmap)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }


    // Fill the bitmap with the background color.
    //
    SetMem32(this->m_pToggleSwitch->SwitchOnBitmap,  BitmapSize, *(UINT32 *)&gMsColorTable.ToggleSwitchBackgroundColor);
    SetMem32(this->m_pToggleSwitch->SwitchOffBitmap, BitmapSize, *(UINT32 *)&gMsColorTable.ToggleSwitchBackgroundColor);
    SetMem32(this->m_pToggleSwitch->GrayedSwitchOnBitmap, BitmapSize, *(UINT32 *)&gMsColorTable.ToggleSwitchBackgroundColor);
    SetMem32(this->m_pToggleSwitch->GrayedSwitchOffBitmap, BitmapSize, *(UINT32 *)&gMsColorTable.ToggleSwitchBackgroundColor);


    // The actual toggle switch body size needs to be reduced to allow room for the keyboard focus highlight ring.
    //
    Width   -= (2 * (UIT_TS_HIGHLIGHT_RING_WIDTH + UIT_TS_HIGHTLIGHT_GAP_WIDTH));
    Height  -= (2 * (UIT_TS_HIGHLIGHT_RING_WIDTH + UIT_TS_HIGHTLIGHT_GAP_WIDTH));
    OrigX   += (UIT_TS_HIGHLIGHT_RING_WIDTH + UIT_TS_HIGHTLIGHT_GAP_WIDTH);
    OrigY   += (UIT_TS_HIGHLIGHT_RING_WIDTH + UIT_TS_HIGHTLIGHT_GAP_WIDTH);


    // Adjust width to compensate for the fact that from this point forward, width *excludes* the rounded ends and refers
    // only to the rectangle noted above in the function header comments.
    //
    AdjustedOrigX  = (OrigX + (Height / 2));
    AdjustedWidth  = (Width - (2 * (Height / 2)));

    // Draw the "on" toggle switch's main body.
    //

    DrawElongatedCircle (this,
                         AdjustedOrigX,
                         OrigY,
                         AdjustedWidth,
                         Height,
                         &this->m_OnColor,
                         this->m_pToggleSwitch->SwitchOnBitmap
                        );

    // Draw the "off" toggle switch's main body.
    //
    DrawElongatedCircle (this,
                         AdjustedOrigX,
                         OrigY,
                         AdjustedWidth,
                         Height,
                         &this->m_OffColor,
                         this->m_pToggleSwitch->SwitchOffBitmap
                        );

    // Draw the GrayOut "on" toggle switch's main body.
    //
    DrawElongatedCircle(this,
        AdjustedOrigX,
        OrigY,
        AdjustedWidth,
        Height,
        &this->m_GrayOutColor,
        this->m_pToggleSwitch->GrayedSwitchOnBitmap
        );

    // Draw the GrayedOut"off" toggle switch's main body.
    //
    DrawElongatedCircle(this,
        AdjustedOrigX,
        OrigY,
        AdjustedWidth,
        Height,
        &this->m_GrayOutColor,
        this->m_pToggleSwitch->GrayedSwitchOffBitmap
        );

    // Draw the "off" toggle switch's background-filled main body area.
    //
    Width   -= (2 * UIT_TS_OUTER_BORDER_WIDTH);
    Height  -= (2 * UIT_TS_OUTER_BORDER_WIDTH);
    OrigX   += UIT_TS_OUTER_BORDER_WIDTH;
    OrigY   += UIT_TS_OUTER_BORDER_WIDTH;

    // Adjust width to compensate for the fact that from this point forward, width *excludes* the rounded ends and refers
    // only to the rectangle noted above in the function header comments.
    //
    AdjustedOrigX  = (OrigX + (Height / 2));
    AdjustedWidth  = (Width - (2 * (Height / 2)));

    DrawElongatedCircle (this,
                         AdjustedOrigX,
                         OrigY,
                         AdjustedWidth,
                         Height,
                         &gMsColorTable.ToggleSwitchBackgroundColor,
                         this->m_pToggleSwitch->SwitchOffBitmap
                        );

    //Repeat the same to create a GrayedOutSwitchOffBitMap
    DrawElongatedCircle(this,
        AdjustedOrigX,
        OrigY,
        AdjustedWidth,
        Height,
        &gMsColorTable.ToggleSwitchBackgroundColor,
        this->m_pToggleSwitch->GrayedSwitchOffBitmap
        );


    // Draw the "on" and "off" toggle switch's inner switch circle.
    //
    Width   -= (2 * UIT_TS_INNER_GAP_WIDTH);
    Height  -= (2 * UIT_TS_INNER_GAP_WIDTH);
    OrigY   += UIT_TS_INNER_GAP_WIDTH;

    UINT32 SwitchOnOrigX   = (OrigX + UIT_TS_INNER_GAP_WIDTH + Width - Height);

    // Adjust width to compensate for the fact that from this point forward, width *excludes* the rounded ends and refers
    // only to the rectangle noted above in the function header comments.
    //
    AdjustedOrigX  = (SwitchOnOrigX + (Height / 2));

    DrawElongatedCircle (this,
                         AdjustedOrigX,
                         OrigY,
                         0,             // No width == circle.
                         Height,
                         &gMsColorTable.ToggleSwitchBackgroundColor,
                         this->m_pToggleSwitch->SwitchOnBitmap
                        );

    //Repeat the same to create a GrayedOutSwitchOnBitMap
    DrawElongatedCircle(this,
        AdjustedOrigX,
        OrigY,
        0,             // No width == circle.
        Height,
        &gMsColorTable.ToggleSwitchCircleGrayoutColor,
        this->m_pToggleSwitch->GrayedSwitchOnBitmap
        );

    UINT32 SwitchOffOrigX = (OrigX + UIT_TS_INNER_GAP_WIDTH);

    // Adjust width to compensate for the fact that from this point forward, width *excludes* the rounded ends and refers
    // only to the rectangle noted above in the function header comments.
    //
    AdjustedOrigX  = (SwitchOffOrigX + (Height / 2));

    DrawElongatedCircle (this,
                         AdjustedOrigX,
                         OrigY,
                         0,             // No width == circle.
                         Height,
                         &this->m_OffColor,
                         this->m_pToggleSwitch->SwitchOffBitmap
                        );

   //Repeat the same to create a GrayedOutSwitchOffBitMap
    DrawElongatedCircle(this,
        AdjustedOrigX,
        OrigY,
        0,             // No width == circle.
        Height,
        &this->m_GrayOutColor,
        this->m_pToggleSwitch->GrayedSwitchOffBitmap
        );

Exit:

    return Status;
}


static
EFI_STATUS
RenderToggleSwitch(IN ToggleSwitch  *this,
                   IN BOOLEAN       DrawHighlight)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    UINT32                          MaxGlyphDescent;
    SWM_RECT                        StringRect;
    EFI_FONT_DISPLAY_INFO           *StringInfo = NULL;
    EFI_IMAGE_OUTPUT                *pBltBuffer = NULL;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *DrawBitMap;


    // Text color.
    //
    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    CopyMem (&StringInfo->BackgroundColor, &gMsColorTable.ToggleSwitchTextBGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    CopyMem (&StringInfo->ForegroundColor, &gMsColorTable.ToggleSwitchTextFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    // Prepare string blitting buffer.
    //
    pBltBuffer = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
    ASSERT (pBltBuffer != NULL);

    if (NULL == pBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    pBltBuffer->Width        = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
    pBltBuffer->Height       = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    pBltBuffer->Image.Screen = mUITGop;

// Check if the state is GRAYED or NORMAL to pick the correct  ON/OFF Bitmap for the switch.
    if (this->m_CurrentState == FALSE){
        if ((this->m_pToggleSwitch->State == GRAYED)){
            DrawBitMap = this->m_pToggleSwitch->GrayedSwitchOffBitmap;
            CopyMem(&StringInfo->ForegroundColor, &this->m_GrayOutColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        }
        else{
            DrawBitMap = this->m_pToggleSwitch->SwitchOffBitmap;
        }
    } else{
        if ((this->m_pToggleSwitch->State == GRAYED)){
            DrawBitMap = this->m_pToggleSwitch->GrayedSwitchOnBitmap;
            CopyMem(&StringInfo->ForegroundColor, &this->m_GrayOutColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        }
        else{
            DrawBitMap = this->m_pToggleSwitch->SwitchOnBitmap;
        }
    }

    // Draw the toggle switch.
    //
    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        DrawBitMap,
                        EfiBltBufferToVideo,
                        0,
                        0,
                        this->m_pToggleSwitch->ToggleSwitchBounds.Left,
                        this->m_pToggleSwitch->ToggleSwitchBounds.Top,
                        this->m_pToggleSwitch->SwitchBitmapWidth,
                        this->m_pToggleSwitch->SwitchBitmapHeight,
                        0
                       );

    // Draw the keyboard control highlight if needed.
    //
    if (TRUE == DrawHighlight)
    {
        DrawRectangleOutline (this->m_pToggleSwitch->ToggleSwitchBounds.Left,
                              this->m_pToggleSwitch->ToggleSwitchBounds.Top,
                              this->m_pToggleSwitch->SwitchBitmapWidth,
                              this->m_pToggleSwitch->SwitchBitmapHeight,
                              UIT_TS_HIGHLIGHT_RING_WIDTH,
                              &gMsColorTable.ToggleSwitchHighlightBGColor
                             );
    }

    // Draw toggle switch text.
    //
    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    // Determine the correct control text to display.
    //
    CHAR16 *pString = (TRUE == this->m_CurrentState ? this->m_pToggleSwitch->pToggleSwitchOnText : this->m_pToggleSwitch->pToggleSwitchOffText);

    // Get the string bitmap bounding rectangle.
    //
    // TODO - bounds should apply to the toggle switch and it's text, combined.
    //
    CopyMem (&StringRect, &this->m_pToggleSwitch->ToggleSwitchBounds, sizeof(SWM_RECT));
    GetTextStringBitmapSize (pString,
                             &StringInfo->FontInfo,
                             TRUE,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK,
                             &StringRect,
                             &MaxGlyphDescent
                            );

    SWM_RECT *pRect = &this->m_pToggleSwitch->ToggleSwitchBounds;
    UINTN SwitchOrigY = (pRect->Top + ((pRect->Bottom - pRect->Top + 1) / 2) - ((StringRect.Bottom - StringRect.Top + 1) / 2));

    mUITSWM->StringToWindow (mUITSWM,
                             mClientImageHandle,
                             EFI_HII_OUT_FLAG_CLIP |
                             EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                             EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                             pString,
                             StringInfo,
                             &pBltBuffer,
                             pRect->Right + MsUiScaleByTheme(20),  // TODO
                             SwitchOrigY,
                             NULL,
                             NULL,
                             NULL
                            );

Exit:

    if (NULL != pBltBuffer)
    {
        FreePool(pBltBuffer);
    }

    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }

    return Status;
}


static
EFI_STATUS
SetControlBounds (IN ToggleSwitch   *this,
                  IN SWM_RECT       Bounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    // NOTE: The text associated with the toggle switch isn't considered part of the control itself.  This is because the string's location depends on
    // whether the control is grouped in a horizontally-arranged grid or in a vertically-aligned default canvas configuration.
    //

    // Translate (and possibly resize) the toggle switch bounding box.
    //
    CopyMem (&this->m_pToggleSwitch->ToggleSwitchBounds, &Bounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  ToggleSwitch      *this,
                  OUT SWM_RECT          *pBounds)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_pToggleSwitch->ToggleSwitchBounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
SetControlState (IN ToggleSwitch    *this,
                 IN OBJECT_STATE    State)
{
    this->m_pToggleSwitch->State = State;
    return EFI_SUCCESS;
}

static
OBJECT_STATE
GetControlState(IN ToggleSwitch         *this)
{
    return this->m_pToggleSwitch->State;
}

static
EFI_STATUS
CopySettings (IN ToggleSwitch  *this,
              IN ToggleSwitch  *prev) {

    this->m_CurrentState = prev->m_CurrentState;

    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN  ToggleSwitch          *this,
      IN   BOOLEAN              DrawHighlight,
      IN   SWM_INPUT_STATE      *pInputState,
      OUT VOID                  **pSelectionContext)
{
    EFI_STATUS  Status      = EFI_SUCCESS;
    SWM_RECT    *pRect      = &this->m_pToggleSwitch->ToggleSwitchBounds;
    VOID        *Context    = NULL;

    // If there is no input state, simply draw the toggle switch then return.
    //
    if (NULL == pInputState || this->m_pToggleSwitch->State == GRAYED)
    {
        Status = RenderToggleSwitch (this,
                                     DrawHighlight
                                    );

        goto Exit;
    }

    // If there is user keyboard input, handle it here.  For buttons, we only recognize
    // <RIGHT-ARROW> and <LEFT-ARROW> as valid keys.
    //
    if (SWM_INPUT_TYPE_KEY == pInputState->InputType)
    {
        EFI_KEY_DATA *pKey = &pInputState->State.KeyState;

        if (SCAN_LEFT == pKey->Key.ScanCode || SCAN_DOWN == pKey->Key.ScanCode)
        {
            if (TRUE == this->m_CurrentState)
            {
                this->m_pToggleSwitch->State = SELECT;      // Mouse button isn't pressed and switch will move On -> Off - select.
                Context = this->m_pSelectionContext;
            }
            this->m_CurrentState = FALSE;       // Off.
        }
        else if (SCAN_RIGHT == pKey->Key.ScanCode || SCAN_UP == pKey->Key.ScanCode)
        {
            if (FALSE == this->m_CurrentState)
            {
                this->m_pToggleSwitch->State = SELECT;      // Mouse button isn't pressed and switch will move Off -> On - select.
                Context = this->m_pSelectionContext;
            }
            this->m_CurrentState = TRUE;       // On.
        }
        else if (L' ' == pKey->Key.UnicodeChar)             // Space key toggles.
        {
            this->m_CurrentState = !this->m_CurrentState;
            this->m_pToggleSwitch->State = SELECT;
            Context = this->m_pSelectionContext;
        }
        else
        {
            // Unrecognized keyboard input - simply exit.
            //
            goto Exit;
        }

        // Draw the toggle switch.
        //
        Status = RenderToggleSwitch (this,
                                     TRUE
                                    );

        // We're done, Exit.
        //
        goto Exit;
    }

    if (this->m_pToggleSwitch->State == SELECT) {
        this->m_pToggleSwitch->State = NORMAL;
    }

    // Check whether the pointer location falls within the toggle switch's bounding box.
    //
    if (SWM_INPUT_TYPE_TOUCH == pInputState->InputType &&
        pInputState->State.TouchState.CurrentX >= pRect->Left && pInputState->State.TouchState.CurrentX <= pRect->Right &&
        pInputState->State.TouchState.CurrentY >= pRect->Top  && pInputState->State.TouchState.CurrentY <= pRect->Bottom)
    {
        BOOLEAN ButtonDown = ((pInputState->State.TouchState.ActiveButtons & 0x1) == 1);

        // If the mouse button isn't being pressed and the switch is off, select the hover color.
        //
        if (FALSE == ButtonDown && FALSE == this->m_CurrentState)
        {
            // TODO
            //this->m_pToggleSwitch->State = HOVER;       // Mouse button isn't pressed and switch is off - hover.
            this->m_pToggleSwitch->State = NORMAL;      // Mouse button isn't pressed and switch is on - normal.
        }
        else if (FALSE == ButtonDown && TRUE == this->m_CurrentState)
        {
            this->m_pToggleSwitch->State = NORMAL;      // Mouse button isn't pressed and switch is on - normal.
        }
        else if (TRUE == ButtonDown)
        {
            this->m_pToggleSwitch->State = NORMAL;      // Indicate not selected at this time.
            // Calculate whether switch should be turned on or off.
            //
            if (pInputState->State.TouchState.CurrentX < (pRect->Left + ((pRect->Right - pRect->Left) / 2)))
            {
                if (TRUE == this->m_CurrentState)
                {
                    this->m_pToggleSwitch->State = SELECT;      // Mouse button isn't pressed and switch will move On -> Off - select.
                    Context = this->m_pSelectionContext;
                    this->m_CurrentState = FALSE;       // Off.
                }
            }
            else
            {
                if (FALSE == this->m_CurrentState)
                {
                    this->m_pToggleSwitch->State = SELECT;      // Mouse button isn't pressed and switch will move Off -> On - select.
                    Context = this->m_pSelectionContext;
                    this->m_CurrentState = TRUE;       // On.
                }
            }
        }
    }

    // Draw the toggle switch.
    //
    Status = RenderToggleSwitch (this,
                                DrawHighlight
                               );

Exit:

    // Return appropriate context to the caller.
    //
    if (NULL != pSelectionContext)
    {
        *pSelectionContext = Context;
    }

    return (this->m_pToggleSwitch->State);
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _ToggleSwitch           *this,
     IN SWM_RECT                       ToggleSwitchBox,
     IN EFI_FONT_INFO                  *FontInfo,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  OnColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  OffColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  HoverColor,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  GrayOutColor,
     IN CHAR16                         *pToggleSwitchOnText,
     IN CHAR16                         *pToggleSwitchOffText,
     IN BOOLEAN                        InitialState,
     IN VOID                           *pSelectionContext)
{


    // Initialize variables.
    //
    this->m_FontInfo = DupFontInfo (FontInfo);
    if (NULL == this->m_FontInfo)
    {
        goto Exit;
    }

    this->m_OnColor         = OnColor;
    this->m_OffColor        = OffColor;
    this->m_HoverColor      = HoverColor;
    this->m_GrayOutColor    = GrayOutColor;

    this->m_pToggleSwitch = AllocateZeroPool(sizeof(ToggleSwitchDisplayInfo));
    ASSERT(NULL != this->m_pToggleSwitch);

    if (NULL == this->m_pToggleSwitch)
    {
        goto Exit;
    }

    // Save the associated selection context pointer.
    //
    this->m_pSelectionContext = pSelectionContext;

    // Save the initial state.
    //
    this->m_CurrentState = InitialState;

    // Save pointer to the toggle switch's text.
    //
    this->m_pToggleSwitch->pToggleSwitchOnText  = pToggleSwitchOnText;      // TODO - need to allocate buffer and save?
    this->m_pToggleSwitch->pToggleSwitchOffText = pToggleSwitchOffText;

    // Save the toggle switch bounding box.
    //
    CopyMem(&this->m_pToggleSwitch->ToggleSwitchBounds, &ToggleSwitchBox, sizeof(SWM_RECT));

    // Create toggle switch bitmaps.
    //
    CreateToggleSwitchBitmaps(this);

    // Configure button state.
    //
    //
    this->m_pToggleSwitch->State      = NORMAL;

    // Member Variables
    this->Base.ControlType      = TOGGLESWITCH;

    // Functions.
    //
    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;

Exit:

    return;
}


static
VOID Dtor(VOID *this)
{
    ToggleSwitch *privthis = (ToggleSwitch *)this;


    // Clean-up allocated buffers.
    //
    if (NULL != privthis->m_pToggleSwitch)
    {
        if (NULL != privthis->m_pToggleSwitch->SwitchOnBitmap)
        {
            FreePool(privthis->m_pToggleSwitch->SwitchOnBitmap);
        }
        if (NULL != privthis->m_pToggleSwitch->SwitchOffBitmap)
        {
            FreePool(privthis->m_pToggleSwitch->SwitchOffBitmap);
        }
        if (NULL != privthis->m_FontInfo)
        {
            FreePool (privthis->m_FontInfo);
        }

        FreePool(privthis->m_pToggleSwitch);
    }

    if (NULL != privthis)
    {
        FreePool(privthis);
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
// Public
//
ToggleSwitch *new_ToggleSwitch(IN UINT32                            OrigX,
                               IN UINT32                            OrigY,
                               IN UINT32                            ToggleSwitchWidth,
                               IN UINT32                            ToggleSwitchHeight,
                               IN EFI_FONT_INFO                     *FontInfo,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     OnColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     OffColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     HoverColor,
                               IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL     GrayOutColor,
                               IN CHAR16                            *pToggleSwitchOnText,
                               IN CHAR16                            *pToggleSwitchOffText,
                               IN BOOLEAN                           InitialState,
                               IN VOID                              *pSelectionContext)
{
    SWM_RECT    Rect;


    ToggleSwitch *S = (ToggleSwitch *)AllocateZeroPool(sizeof(ToggleSwitch));
    ASSERT(NULL != S);

    if (NULL != S)
    {
        S->Ctor         = &Ctor;
        S->Base.Dtor    = &Dtor;

        Rect.Left       = OrigX;
        Rect.Right      = (OrigX + ToggleSwitchWidth - 1);
        Rect.Top        = OrigY;
        Rect.Bottom     = (OrigY + ToggleSwitchHeight - 1);

        S->Ctor(S,
                Rect,
                FontInfo,
                OnColor,
                OffColor,
                HoverColor,
                GrayOutColor,
                pToggleSwitchOnText,
                pToggleSwitchOffText,
                InitialState,
                pSelectionContext
               );
    }

    return S;
}


VOID delete_ToggleSwitch(IN ToggleSwitch *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
