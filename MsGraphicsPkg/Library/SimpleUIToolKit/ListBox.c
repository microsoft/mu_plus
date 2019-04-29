/** @file

  Implements a simple list box control.

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


#define UIT_LB_HIGHLIGHT_RING_WIDTH         MsUiScaleByTheme(4)   // Number of pixels making up the listbox cell's highlight ring.
#define UIT_LB_OUTER_BORDER_WIDTH           MsUiScaleByTheme(5)   // Number of pixels making up the listbox cell's outer border (highlight right offset).
#define UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH  MsUiScaleByTheme(2)   // Number of pixels making up the listbox cell's checkbox outer border (ring).
#define UIT_LB_CHECKBOX_INNER_GAP_WIDTH     MsUiScaleByTheme(4)   // Number of pixels making up the listbox cell's checkbox inner gap (between check and outer border).


//////////////////////////////////////////////////////////////////////////////
// Private
//
static
TOUCH_LOCATION
QueryPointerLocation (IN ListBox         *this,
                      IN SWM_INPUT_STATE *pInputState,
                      IN UINTN            Index) {
    SWM_RECT        *BoundingRect;

    if (UIT_LISTBOX_FLAGS_CHECKBOX == (this->m_Flags & UIT_LISTBOX_FLAGS_CHECKBOX))
    {
        BoundingRect = &this->m_pCells[Index].CellCheckBoxBounds;
        if (pInputState->State.TouchState.CurrentX >= BoundingRect->Left && pInputState->State.TouchState.CurrentX <= BoundingRect->Right &&
            pInputState->State.TouchState.CurrentY >= BoundingRect->Top  && pInputState->State.TouchState.CurrentY <= BoundingRect->Bottom)
        {
            return LocationCheckbox;
        }
    }
    if (this->m_pCells[Index].TrashcanEnabled)
    {
        BoundingRect = &this->m_pCells[Index].CellTrashcanBounds;
        if (pInputState->State.TouchState.CurrentX >= BoundingRect->Left && pInputState->State.TouchState.CurrentX <= BoundingRect->Right &&
            pInputState->State.TouchState.CurrentY >= BoundingRect->Top  && pInputState->State.TouchState.CurrentY <= BoundingRect->Bottom)
        {
            return LocationTrashcan;
        }
    }
    BoundingRect = &this->m_pCells[Index].CellBounds;
    if (pInputState->State.TouchState.CurrentX >= BoundingRect->Left && pInputState->State.TouchState.CurrentX <= BoundingRect->Right &&
        pInputState->State.TouchState.CurrentY >= BoundingRect->Top  && pInputState->State.TouchState.CurrentY <= BoundingRect->Bottom)
    {
        return LocationListbox;
    }
    return LocationNone;
}


static
EFI_STATUS
RenderCellCheckBox(IN  ListBox  *this,
                    IN  UINT32  OrigX,
                    IN  UINT32  OrigY,
                    IN  UINT32  Width,
                    IN  UINT32  Height,
                    IN  BOOLEAN Selected)
{
    EFI_STATUS  Status = EFI_SUCCESS;
    UINT32      FillOrigX, FillOrigY, FillWidth, FillHeight;


    // Render checkbox white fill first.
    //
    mUITSWM->BltWindow (mUITSWM,
                        mClientImageHandle,
                        &gMsColorTable.ListBoxCheckBoxBackgroundColor,
                        EfiBltVideoFill,
                        0,
                        0,
                        OrigX,
                        OrigY,
                        Width,
                        Height,
                        0
                       );
    if (this->m_State == GRAYED){

        DrawRectangleOutline(OrigX,
            OrigY,
            Width,
            Height,
            UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH,
            &gMsColorTable.ListBoxCheckBoxBoundGrayoutColor
            );

    } else{
    // Draw the outer rectangle outline.
    //
        DrawRectangleOutline (OrigX,
                          OrigY,
                          Width,
                          Height,
                          UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH,
                          &gMsColorTable.ListBoxHighlightBoundColor
                         );
     }

    // If the checkbox is selected, draw a "check".
    //
    if (TRUE == Selected)
    {
        FillOrigX   = (OrigX  + UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH + UIT_LB_CHECKBOX_INNER_GAP_WIDTH);
        FillOrigY   = (OrigY  + UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH + UIT_LB_CHECKBOX_INNER_GAP_WIDTH);
        FillWidth   = (Width  - (2 * UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH) - (2 * UIT_LB_CHECKBOX_INNER_GAP_WIDTH));
        FillHeight  = (Height - (2 * UIT_LB_CHECKBOX_OUTER_BORDER_WIDTH) - (2 * UIT_LB_CHECKBOX_INNER_GAP_WIDTH));
        if (this->m_State == GRAYED){

            mUITSWM->BltWindow(mUITSWM,
                mClientImageHandle,
                &gMsColorTable.ListBoxCheckBoxSelectBGGrayoutColor,
                EfiBltVideoFill,
                0,
                0,
                FillOrigX,
                FillOrigY,
                FillWidth,
                FillHeight,
                0
                );

        } else{

        mUITSWM->BltWindow (mUITSWM,
                            mClientImageHandle,
                            &gMsColorTable.ListBoxCheckBoxNormalBGGrayoutColor,
                            EfiBltVideoFill,
                            0,
                            0,
                            FillOrigX,
                            FillOrigY,
                            FillWidth,
                            FillHeight,
                            0
                           );
        }
    }

    return Status;
}


static
EFI_STATUS
RenderCellTrashcan (IN  ListBox  *this,
                    IN  UINT32   CellIndex) {
    EFI_STATUS                       Status;
    EFI_FONT_DISPLAY_INFO            *StringInfo = NULL;
    CHAR16                           TrashcanString[] = {(CHAR16)0xE107, 0x00 };
    EFI_STRING                       Trashcan = (EFI_STRING) &TrashcanString;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pFillColor;
    EFI_IMAGE_OUTPUT                *pBltBuffer = NULL;
    UINTN                            Left;
    UINTN                            Top;

    if (!this->m_pCells[CellIndex].TrashcanEnabled)
    {
        return EFI_SUCCESS;
    }

    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    if (CellIndex == this->m_SelectedCell)
    {
        pFillColor = &this->m_SelectColor;
        // TODO - need to pass in font color.
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxTranshanSelectColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }
    else
    {
        pFillColor = &this->m_NormalColor;
        // TODO - need to pass in font color.
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxTranshanNormalColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }

    if (this->m_State == GRAYED){
        pFillColor = &this->m_GrayOutColor;
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxTranshanGrayoutColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }

    CopyMem(&StringInfo->BackgroundColor, pFillColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    StringInfo->FontInfoMask     = EFI_FONT_INFO_ANY_FONT;
    StringInfo->FontInfo.FontSize = MsUiGetLargeFontHeight ();

    // Prepare string blitting buffer.
    //
    pBltBuffer = (EFI_IMAGE_OUTPUT *)AllocateZeroPool(sizeof(EFI_IMAGE_OUTPUT));
    ASSERT(pBltBuffer != NULL);

    if (NULL == pBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    pBltBuffer->Width        = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
    pBltBuffer->Height       = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    pBltBuffer->Image.Screen = mUITGop;

    Left = this->m_pCells[CellIndex].CellTrashcanBounds.Left +
        (this->m_pCells[CellIndex].CellTrashcanBounds.Right - this->m_pCells[CellIndex].CellTrashcanBounds.Left -  MsUiGetLargeFontHeight ()) / 2;
    Top = this->m_pCells[CellIndex].CellTrashcanBounds.Top +
        (this->m_pCells[CellIndex].CellTrashcanBounds.Bottom - this->m_pCells[CellIndex].CellTrashcanBounds.Top -  MsUiGetLargeFontHeight ()) / 2;

    Status = mUITSWM->StringToWindow(mUITSWM,
                            mClientImageHandle,
                            EFI_HII_DIRECT_TO_SCREEN,
                            Trashcan,
                            StringInfo,
                            &pBltBuffer,
                            Left,
                            Top,
                            NULL,
                            NULL,
                            NULL
                           );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"StringToWindow error %r\n",Status));
    }

Exit:

    if (NULL != StringInfo)
    {
        FreePool (StringInfo);
    }

    if (NULL != pBltBuffer)
    {
        FreePool(pBltBuffer);
    }

    return EFI_SUCCESS;
}


static
EFI_STATUS
GetSelectedCellIndex (IN  ListBox   *this,
                      OUT LB_RETURN_DATA *ReturnData)
{

    EFI_STATUS  Status = EFI_SUCCESS;


    if (NULL == ReturnData)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    ReturnData->Action = this->m_LastAction;
    if (ReturnData->Action == LB_ACTION_MOVE)
    {
        ReturnData->SelectedCell = this->m_SourceCell;
        ReturnData->TargetCell = this->m_TargetCell;
        ReturnData->Direction = this->m_Direction;
    }
    else
    {
        ReturnData->SelectedCell = this->m_SelectedCell;
        ReturnData->TargetCell = this->m_SelectedCell;
        ReturnData->Direction = LB_MOVE_NONE;
    }

    //
    // Validate Return Data
    //
    if (((ReturnData->SelectedCell < 0) || (ReturnData->SelectedCell >= this->m_NumberOfCells)) ||
        ((ReturnData->TargetCell < 0)   || (ReturnData->TargetCell >= this->m_NumberOfCells)))
    {
        DEBUG((DEBUG_INFO, "Ignoring input due to range error Sel=%d, Tgt=%d\n",
                           ReturnData->SelectedCell,
                           ReturnData->TargetCell));
        ReturnData->Action = LB_ACTION_NONE;
        ReturnData->SelectedCell =0;
        ReturnData->TargetCell = 0;
        ReturnData->Direction = LB_MOVE_NONE;
    }

Exit:
    return Status;
}


static
EFI_STATUS
RenderCell(IN ListBox       *this,
           IN UINT32        CellIndex)
{
    EFI_STATUS                      Status = EFI_SUCCESS;
    EFI_FONT_DISPLAY_INFO           *StringInfo = NULL;
    EFI_IMAGE_OUTPUT                *pBltBuffer = NULL;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pFillColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pCellRingColor;

    if (CellIndex >= this->m_NumberOfCells)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    CellDisplayInfo    *pCell = &this->m_pCells[CellIndex];

    // Color.
    // TODO - Hover.
    //

    StringInfo = BuildFontDisplayInfoFromFontInfo (this->m_FontInfo);
    if (NULL == StringInfo)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    if (CellIndex == this->m_SelectedCell)
    {
        pFillColor = &this->m_SelectColor;
        // TODO - need to pass in font color.
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxSelectFGColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }
    else
    {
        pFillColor = &this->m_NormalColor;
        // TODO - need to pass in font color.
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxNormalFGColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }

    if (this->m_State == GRAYED){
        pFillColor = &this->m_GrayOutColor;
        CopyMem(&StringInfo->ForegroundColor, &gMsColorTable.ListBoxGrayoutFGColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    }

    CopyMem(&StringInfo->BackgroundColor, pFillColor, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    // Render cell background.
    //
    UINT32 CellWidth  = (pCell->CellBounds.Right - pCell->CellBounds.Left + 1);
    UINT32 CellHeight = (pCell->CellBounds.Bottom - pCell->CellBounds.Top + 1);

    mUITSWM->BltWindow(mUITSWM,
                       mClientImageHandle,
                       pFillColor,
                       EfiBltVideoFill,
                       0,
                       0,
                       pCell->CellBounds.Left,
                       pCell->CellBounds.Top,
                       CellWidth,
                       CellHeight,
                       CellWidth * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                      );


    // Render cell highlight ring (will be the same color as the cell background if highlight is off).  Note that it's faster
    // (and visually looks better) to draw four individual line segments than to do a single large rect fill.
    //
    pCellRingColor = (CellIndex == this->m_HighlightedCell ? &gMsColorTable.ListBoxHighlightBoundColor : pFillColor);

    CellWidth  = (pCell->CellBounds.Right - pCell->CellBounds.Left -(2 * UIT_LB_OUTER_BORDER_WIDTH)+ 1);
    CellHeight = (pCell->CellBounds.Bottom - pCell->CellBounds.Top -(2 * UIT_LB_OUTER_BORDER_WIDTH)+ 1);

    DrawRectangleOutline ((pCell->CellBounds.Left + UIT_LB_OUTER_BORDER_WIDTH),
                          (pCell->CellBounds.Top  + UIT_LB_OUTER_BORDER_WIDTH),
                          CellWidth,
                          CellHeight,
                          UIT_LB_HIGHLIGHT_RING_WIDTH,
                          pCellRingColor
                         );

    // If the listbox was created with the checkbox option flag, draw a checkbox.
    //
    if ((this->m_Flags & UIT_LISTBOX_FLAGS_CHECKBOX) == UIT_LISTBOX_FLAGS_CHECKBOX)
    {
        UINT32  CheckBoxHitAreaHeight   = (pCell->CellCheckBoxBounds.Bottom - pCell->CellCheckBoxBounds.Top + 1);
        UINT32  CheckBoxHeight          = (CheckBoxHitAreaHeight / 3);  // TODO - 1/3 the height of a listbox cell?
        UINT32  CheckBoxWidth           = CheckBoxHeight;
        UINT32  CheckBoxOrigY           = (pCell->CellCheckBoxBounds.Top +  (CheckBoxHitAreaHeight / 2) - (CheckBoxHeight / 2));
        UINT32  CheckBoxOrigX           = (pCell->CellCheckBoxBounds.Left + (CheckBoxHitAreaHeight / 2) - (CheckBoxHeight / 2));

        RenderCellCheckBox(this, CheckBoxOrigX,
                           CheckBoxOrigY,
                           CheckBoxWidth,
                           CheckBoxHeight,
                           pCell->CheckboxSelected
                          );
    }
    // If the listbox was created with the allow delete option flag, draw a trashcan
    //
    if ((this->m_Flags & UIT_LISTBOX_FLAGS_ALLOW_DELETE) == UIT_LISTBOX_FLAGS_ALLOW_DELETE)
    {
        RenderCellTrashcan(this, CellIndex);
    }

    // Prepare string blitting buffer.
    //
    pBltBuffer = (EFI_IMAGE_OUTPUT *)AllocateZeroPool(sizeof(EFI_IMAGE_OUTPUT));
    ASSERT (pBltBuffer != NULL);

    if (NULL == pBltBuffer)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    pBltBuffer->Width        = (UINT16)mUITGop->Mode->Info->HorizontalResolution;
    pBltBuffer->Height       = (UINT16)mUITGop->Mode->Info->VerticalResolution;
    pBltBuffer->Image.Screen = mUITGop;


    StringInfo->FontInfoMask = EFI_FONT_INFO_ANY_FONT;

    // TODO - for checkbox type listbox cells, how to handle text indent - indent checkbox as well?  For now
    // we keep the checkbox left-justified in the cell and the caller-specified text indent value affects the cell text only.

    // Draw cell text.
    //
    mUITSWM->StringToWindow(mUITSWM,
                            mClientImageHandle,
                            EFI_HII_OUT_FLAG_CLIP |
                            EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                            EFI_HII_IGNORE_LINE_BREAK | EFI_HII_DIRECT_TO_SCREEN,
                            pCell->pCellText,
                            StringInfo,
                            &pBltBuffer,
                            pCell->CellTextBounds.Left,
                            pCell->CellTextBounds.Top,
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
OBJECT_STATE
Draw (IN    ListBox             *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext) {
    UINT32          Index;
    VOID            *Context        = NULL;
    BOOLEAN         RefreshCtl      = TRUE;
    TOUCH_LOCATION  TouchLocation   = LocationNone;


    DEBUG((DEBUG_INFO,"Listbox Draw - Sel=%d, this=%p, Highlight=%d, Input=%p, HighCell=%d\n",this->m_SelectedCell,this,(UINT32)DrawHighlight,pInputState,this->m_HighlightedCell));
    // Clear the cell highlight if we aren't asked to draw it.
    //
    if (FALSE == DrawHighlight && UIT_INVALID_SELECTION != this->m_HighlightedCell)
    {
        // Clear the currently highlighted index.
        //
        this->m_HighlightedCell = UIT_INVALID_SELECTION;
    }

    // Select an initial highlight cell if the listbox control should now be highlighted and there isn't a highlighted cell.
    //
    if (TRUE == DrawHighlight && UIT_INVALID_SELECTION == this->m_HighlightedCell)
    {
        // Set this to be the highlighted cell.
        this->m_HighlightedCell = this->m_SelectedCell;
    }

    // If there is no user input state, simply draw all listbox cells then exit.
    //
    if (NULL == pInputState || this->m_State == GRAYED)
    {
        DEBUG((DEBUG_INFO,"Listbox Draw 3- Sel=%d, this=%p, Highlight=%d, HighlightCel=%d, Num=%d\n",this->m_SelectedCell,this,(UINT32)DrawHighlight,this->m_HighlightedCell,this->m_NumberOfCells));
        for (Index = 0; Index < this->m_NumberOfCells; Index++)
        {
            RenderCell(this, Index);
        }

        goto Exit;
    }

    this->m_State = NORMAL;

    // If we received user keyboard input, process that here.  For a listbox, we only support <UP-ARROW>,
    // <DOWN-ARROW>, <ENTER>, and <SPACE>.
    //
    if (SWM_INPUT_TYPE_KEY == pInputState->InputType)
    {
        EFI_KEY_DATA    *pKey = &pInputState->State.KeyState;

        // If the ListBox was created with the checkbox option flag, toggle the checkbox select state.
        //
        if (L' ' == pKey->Key.UnicodeChar && UIT_LISTBOX_FLAGS_CHECKBOX == (this->m_Flags & UIT_LISTBOX_FLAGS_CHECKBOX))
        {
            if (UIT_INVALID_SELECTION != this->m_HighlightedCell)
            {
                // Selected cell becomes the highlighted cell.
                this->m_SelectedCell = this->m_HighlightedCell;
            }
            if (UIT_INVALID_SELECTION != this->m_SelectedCell)
            {
                // Indicate that the control is in a select state.
                //
                this->m_LastAction = LB_ACTION_TOGGLE;
                Context = this->m_pSelectionContext;
                this->m_State   = SELECT;
            }
        }
        if (this->m_Flags & UIT_LISTBOX_FLAGS_ORDERED_LIST)
        {
            if (L'+' == pKey->Key.UnicodeChar) {
                if (UIT_INVALID_SELECTION != this->m_HighlightedCell) {
                    // Selected cell becomes the highlighted cell.
                    //
                    this->m_SelectedCell = this->m_HighlightedCell;
                }

                // Indicate that the control is in a select state.
                //
                if (UIT_INVALID_SELECTION != this->m_SelectedCell) {
                    this->m_LastAction = LB_ACTION_MOVE;
                    this->m_SourceCell = this->m_SelectedCell;
                    if (this->m_SelectedCell > 0) {
                        this->m_Direction = LB_MOVE_UP;
                        this->m_SelectedCell -= 1;
                        this->m_TargetCell = this->m_SelectedCell;
                        this->m_HighlightedCell = this->m_SelectedCell;
                    }
                    Context = this->m_pSelectionContext;
                    this->m_State   = SELECT;
                }
            }
            if (L'-' == pKey->Key.UnicodeChar) {
                if (UIT_INVALID_SELECTION != this->m_HighlightedCell) {
                    // Selected cell becomes the highlighted cell.
                    //
                    this->m_SelectedCell = this->m_HighlightedCell;
                }

                // Indicate that the control is in a select state.
                //
                if (UIT_INVALID_SELECTION != this->m_SelectedCell) {
                    this->m_LastAction = LB_ACTION_MOVE;
                    this->m_SourceCell = this->m_SelectedCell;
                    if (this->m_SelectedCell < (this->m_NumberOfCells - 1))  {
                        this->m_Direction = LB_MOVE_DOWN;
                        this->m_SelectedCell += 1;
                        this->m_TargetCell = this->m_SelectedCell;
                        this->m_HighlightedCell = this->m_SelectedCell;
                    }
                    Context = this->m_pSelectionContext;
                    this->m_State = SELECT;
                }
            }
        }
        if (CHAR_CARRIAGE_RETURN == pKey->Key.UnicodeChar) {
            if (UIT_INVALID_SELECTION != this->m_HighlightedCell) {
                // Selected cell becomes the highlighted cell.
                //
                this->m_SelectedCell = this->m_HighlightedCell;
            }

            // Indicate that the control is in a select state.
            //
            this->m_LastAction = LB_ACTION_SELECT;
            if (this->m_Flags & UIT_LISTBOX_FLAGS_ORDERED_LIST) {
                if (UIT_INVALID_SELECTION != this->m_SelectedCell) {
                    this->m_LastAction = LB_ACTION_BOOT;
                }
            }
            Context = this->m_pSelectionContext;
            this->m_State = SELECT;
        }
        else if (SCAN_DOWN == pKey->Key.ScanCode)
        {
            // Move cell selection to the next cell, stopping at the end of the list of cells.
            this->m_HighlightedCell++;
            if (this->m_HighlightedCell >= this->m_NumberOfCells)
            {
                this->m_HighlightedCell = (this->m_NumberOfCells - 1);
            }
        }
        else if (SCAN_UP == pKey->Key.ScanCode)
        {
            // Move cell selection to the cell above, stopping at the start of the list of cells.
            if (this->m_HighlightedCell > 0)
            {
                this->m_HighlightedCell--;
            }
        }
        else if (SCAN_DELETE == pKey->Key.ScanCode)
        {
            if (this->m_Flags & UIT_LISTBOX_FLAGS_ALLOW_DELETE)
            {
                // Indicate that the control is in a select state.
                //
                this->m_LastAction = LB_ACTION_DELETE;
                this->m_TargetCell = this->m_SelectedCell;
                Context = this->m_pSelectionContext;
                this->m_State = SELECT;
            }
        }

        // Render all cells to visually reflect their current state.
        //
        DEBUG((DEBUG_INFO,"Listbox Draw 2- Sel=%d, this=%p, Highlight=%d, HighlightCel=%d, Num=%d\n",this->m_SelectedCell,this,(UINT32)DrawHighlight,this->m_HighlightedCell,this->m_NumberOfCells));
        for (Index = 0; Index < this->m_NumberOfCells; Index++)
        {
            RenderCell(this, Index);
        }

        // We're done, exit.
        //
        goto Exit;
    }

    // If we've gotten this far and the remaining user input type isn't touch, there's nothing for us to do.
    //
    if (SWM_INPUT_TYPE_TOUCH != pInputState->InputType)
    {
        goto Exit;
    }

    // Remove cell highlighting (if it's set).
    //
    this->m_HighlightedCell = UIT_INVALID_SELECTION;

    // Scan through all cells and update state (by default each is in a normal, non-selected state).
    //
    for (Index = 0; Index < this->m_NumberOfCells; Index++)
    {

        TouchLocation = QueryPointerLocation(this, pInputState, Index);

        if (LocationNone != TouchLocation)
        {
            break;
        }
    }
    if (Index >= this->m_NumberOfCells)
    {
        Index = UIT_INVALID_SELECTION;
        this->m_CaptureLocation = LocationNone;
    }
    if (LocationNone != this->m_CaptureLocation)  // Capture Pointer in effect
    {
        switch (this->m_CaptureLocation) {
        case LocationCheckbox:
        case LocationTrashcan:
            if (pInputState->State.TouchState.ActiveButtons == 0x01)
            {
                if ((Index != this->m_CaptureIndex) && (UIT_INVALID_SELECTION != this->m_SelectedCell))
                {
                    this->m_SelectedCell = UIT_INVALID_SELECTION;
                }
                else
                {
                    if ((Index == this->m_CaptureIndex) && (UIT_INVALID_SELECTION == this->m_SelectedCell))
                    {
                        this->m_SelectedCell = Index;
                    }
                    else
                    {
                        RefreshCtl = FALSE;
                    }
                }
            }
            else
            {
                if (Index == this->m_CaptureIndex)
                {
                    this->m_SelectedCell = Index;
                    if (LocationTrashcan == this->m_CaptureLocation)
                    {
                        this->m_LastAction = LB_ACTION_DELETE;
                    }
                    else
                    {
                        this->m_LastAction = LB_ACTION_TOGGLE;
                    }
                    this->m_State = SELECT;
                }
                Context = this->m_pSelectionContext;
                this->m_CaptureLocation = LocationNone;
            }
            break;

        case LocationListbox:
            if (pInputState->State.TouchState.ActiveButtons == 0x01)
            {
                if (Index != this->m_CaptureIndex)
                {
                    this->m_SelectedCell = Index;
                    this->m_CaptureIndex = Index;
                }
                else
                {
                    RefreshCtl = FALSE;
                }
            }
            else
            {
                if (this->m_SelectedCell != UIT_INVALID_SELECTION)
                {
                    this->m_LastAction = LB_ACTION_NONE;
                    this->m_SelectedCell = Index;
                    this->m_TargetCell = Index;
                    if (this->m_Flags & UIT_LISTBOX_FLAGS_ORDERED_LIST)
                    {
                        this->m_LastAction = LB_ACTION_NONE;
                        if (this->m_TargetCell == this->m_SourceCell)
                        {
                            // SWIPE LEFT for BOOT?
                            if ((this->m_CapturePointX > pInputState->State.TouchState.CurrentX) &&
                                (128 < (this->m_CapturePointX - pInputState->State.TouchState.CurrentX)))
                            {
                                this->m_LastAction = LB_ACTION_BOOT;
                            }
                            // BUG 10861 Remove Swipe right for delete.  Remove #if to restore delete by swipe
#if 0
                            // SWIPE RIGHT for DELETE ?  (only allowed if entry is can be deleted)
                            if (this->m_pCells[this->m_SourceCell].TrashcanEnabled)
                            {
                                if ((this->m_CapturePointX < pInputState->State.TouchState.CurrentX) &&
                                    (128 < (pInputState->State.TouchState.CurrentX - this->m_CapturePointX)))
                                {
                                    this->m_LastAction = LB_ACTION_DELETE;
                                }
                            }
#endif
                        }
                        else
                        {
                            this->m_LastAction = LB_ACTION_MOVE;
                        }
                    }
                    else
                    {
                        this->m_LastAction = LB_ACTION_SELECT;
                    }
                    this->m_State = SELECT;
                }
                this->m_CaptureLocation = LocationNone;
                Context = this->m_pSelectionContext;
            }
            break;

        case LocationNone:
        default:
            DEBUG((DEBUG_ERROR,"ERROR - Invalid location in listbox processing. Locatione=%d\n",this->m_CaptureLocation));
            //               ASSERT(FALSE);
            break;
        }
    }
    else if (pInputState->State.TouchState.ActiveButtons == 0x01)  // First Touch event
    {
        switch (TouchLocation) {
        case LocationListbox:
            this->m_CapturePointX = pInputState->State.TouchState.CurrentX;
            // Fall trough
        case LocationCheckbox:
        case LocationTrashcan:
            // Select the current cell.
            //
            this->m_SelectedCell = Index;
            this->m_SourceCell = Index;
            this->m_TargetCell = Index;
            this->m_CaptureLocation = TouchLocation;
            this->m_CaptureIndex = Index;
            break;

        case LocationNone:  // Discard touch events outside the region
            RefreshCtl = FALSE;
        default:
            break;
        }
    }


    if (pInputState->State.TouchState.ActiveButtons == 0x00)   // Clear capture mode on any up event
    {
        this->m_CaptureLocation = LocationNone;
    }

    // Now render each cell, except on repeated down events with no change
    //
    if (RefreshCtl)
    {
        for (Index = 0; Index < this->m_NumberOfCells; Index++)
        {
            RenderCell(this, Index);
        }
    }
Exit:
    DEBUG((DEBUG_INFO,"Exit Listbox - State = %d, Sel=%d\n",this->m_State,this->m_SelectedCell));

    // Return selected context to the caller.
    //
    if (NULL != pSelectionContext)
    {
        *pSelectionContext = Context;
    }

    return this->m_State;
}


static
EFI_STATUS
SetControlBounds (IN ListBox    *this,
                  IN SWM_RECT   Bounds)
{
    EFI_STATUS  Status  = EFI_SUCCESS;
    UINTN       Index;
    INT32       XOffset = (Bounds.Left - this->m_ListBoxBounds.Left);
    INT32       YOffset = (Bounds.Top - this->m_ListBoxBounds.Top);
    // Translate (and possibly truncate) the current label bounding box.
    //
    CopyMem (&this->m_ListBoxBounds, &Bounds, sizeof(SWM_RECT));
    for (Index = 0; Index < this->m_NumberOfCells; Index++)
    {
        this->m_pCells[Index].CellBounds.Left   += XOffset;
        this->m_pCells[Index].CellBounds.Right  += XOffset;
        this->m_pCells[Index].CellBounds.Top    += YOffset;
        this->m_pCells[Index].CellBounds.Bottom += YOffset;
        this->m_pCells[Index].CellTextBounds.Left   += XOffset;
        this->m_pCells[Index].CellTextBounds.Right  += XOffset;
        this->m_pCells[Index].CellTextBounds.Top    += YOffset;
        this->m_pCells[Index].CellTextBounds.Bottom += YOffset;
        this->m_pCells[Index].CellCheckBoxBounds.Left   += XOffset;
        this->m_pCells[Index].CellCheckBoxBounds.Right  += XOffset;
        this->m_pCells[Index].CellCheckBoxBounds.Top    += YOffset;
        this->m_pCells[Index].CellCheckBoxBounds.Bottom += YOffset;
        if (this->m_pCells[Index].TrashcanEnabled) {
            this->m_pCells[Index].CellTrashcanBounds.Left   += XOffset;
            this->m_pCells[Index].CellTrashcanBounds.Right  += XOffset;
            this->m_pCells[Index].CellTrashcanBounds.Top    += YOffset;
            this->m_pCells[Index].CellTrashcanBounds.Bottom += YOffset;
        }
    }
    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  ListBox   *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_ListBoxBounds, sizeof(SWM_RECT));

    return Status;
}


static
EFI_STATUS
SetControlState (IN ListBox         *this,
                 IN OBJECT_STATE    State)
{
    this->m_State = State;
    return EFI_SUCCESS;
}

static
OBJECT_STATE
GetControlState(IN ListBox         *this)
{
    return this->m_State;
}


static
EFI_STATUS
CopySettings (IN ListBox  *this,
              IN ListBox  *prev) {

    this->m_SelectedCell = prev->m_SelectedCell;
    if (UIT_INVALID_SELECTION != this->m_SelectedCell &&
        this->m_SelectedCell >= this->m_NumberOfCells)
    {
        this->m_SelectedCell = this->m_NumberOfCells;
    }

    this->m_HighlightedCell = prev->m_HighlightedCell;
    if (UIT_INVALID_SELECTION != this->m_HighlightedCell &&
        this->m_HighlightedCell >= this->m_NumberOfCells)
    {
        this->m_HighlightedCell = this->m_NumberOfCells;
    }
    DEBUG((DEBUG_INFO,"Listbox CopySettings. Selected=%d, Highlighted=%d\n",this->m_SelectedCell, this->m_HighlightedCell));
    return EFI_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID Ctor(IN ListBox                        *this,
          IN SWM_RECT                       CellBox,
          IN UINT32                         Flags,
          IN EFI_FONT_INFO                  *FontInfo,
          IN UINT32                         CellTextXOffset,
          IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *NormalColor,
          IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HoverColor,
          IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SelectColor,
          IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *GrayOutColor,
          IN UIT_LB_CELLDATA                *CellData,
          IN VOID                           *pSelectionContext)
{
    UINT32      Index;
    SWM_RECT    Rect;
    UINT32      MaxGlyphDescent;
    UINT32      CheckBoxHitAreaWidth = 0;
    UINT32      TrashcanHitAreaWidth = 0;

    // Initialize variables.
    //
    this->m_FontInfo = DupFontInfo (FontInfo);
    if (NULL == this->m_FontInfo)
    {
        goto Exit;
    }

    this->m_NormalColor         = *NormalColor;
    this->m_HoverColor          = *HoverColor;
    this->m_SelectColor         = *SelectColor;
    this->m_GrayOutColor        = *GrayOutColor;

    this->m_SelectedCell        = 0;
    this->m_TargetCell          = 0;
    this->m_LastAction          = LB_ACTION_NONE;

    this->m_HighlightedCell     = UIT_INVALID_SELECTION;
    this->m_Flags               = Flags;
    this->m_pSelectionContext   = pSelectionContext;
    this->m_State               = NORMAL;

    // Determine how many listbox entries (cells) and allocate space to store details.  At the same time
    // calculate the overall control's outer bounds.
    //
    for (this->m_NumberOfCells = 0 ; CellData[this->m_NumberOfCells].CellText != NULL ; this->m_NumberOfCells++)
    {
    }

    this->m_ListBoxBounds.Left      = CellBox.Left;
    this->m_ListBoxBounds.Top       = CellBox.Top;
    this->m_ListBoxBounds.Right     = CellBox.Right;
    this->m_ListBoxBounds.Bottom    = (CellBox.Top + ((CellBox.Bottom - CellBox.Top + 1) * this->m_NumberOfCells));

    this->m_pCells = AllocateZeroPool(this->m_NumberOfCells * sizeof(CellDisplayInfo));
    ASSERT(NULL != this->m_pCells);

    if (NULL == this->m_pCells)
    {
        goto Exit;
    }

    // Capture first cell bounding rectangle.
    //
    CopyMem (&Rect, &CellBox, sizeof(SWM_RECT));

    // Iterate through cell list and compute bounding rectangle, checkbox bounding rectangle (optional) and string bitmap rectangle.
    //
    for (Index = 0; CellData[Index].CellText != NULL && Index < this->m_NumberOfCells; Index++)
    {
        this->m_pCells[Index].OriginalOrder     = Index;
        this->m_pCells[Index].pCellText         = AllocateCopyPool(StrSize(CellData[Index].CellText), CellData[Index].CellText);
        this->m_pCells[Index].CheckboxSelected  = CellData[Index].CheckBoxSelected;
        this->m_pCells[Index].TrashcanEnabled   = CellData[Index].TrashcanEnabled;

        CopyMem(&this->m_pCells[Index].CellBounds, &Rect, sizeof(SWM_RECT));

        // If this is a checkbox type ListBox, compute the checkbox bounding rectangle.
        //
        if (UIT_LISTBOX_FLAGS_CHECKBOX == (this->m_Flags & UIT_LISTBOX_FLAGS_CHECKBOX))
        {
            SWM_RECT    *CellBounds             = &this->m_pCells[Index].CellBounds;

            CopyMem(&this->m_pCells[Index].CellCheckBoxBounds, CellBounds, sizeof(SWM_RECT));

            CheckBoxHitAreaWidth    = (CellBounds->Bottom - CellBounds->Top + 1);
            this->m_pCells[Index].CellCheckBoxBounds.Right = (CellBounds->Left + CheckBoxHitAreaWidth);
        }
        // If this is a checkbox type ListBox, compute the checkbox bounding rectangle.
        //
        if (UIT_LISTBOX_FLAGS_ALLOW_DELETE == (this->m_Flags & UIT_LISTBOX_FLAGS_ALLOW_DELETE))
        {
            SWM_RECT    *CellBounds             = &this->m_pCells[Index].CellBounds;

            CopyMem(&this->m_pCells[Index].CellTrashcanBounds, CellBounds, sizeof(SWM_RECT));

            TrashcanHitAreaWidth    = (CellBounds->Bottom - CellBounds->Top + 1);
            this->m_pCells[Index].CellTrashcanBounds.Left = (CellBounds->Right - TrashcanHitAreaWidth);
        }

        // Calculate cell text bounding rectangle (cell text should be vertically centered in the cell, accounting for the maximum font glyph descent).  Also,
        // the text may be indented from the left edge of the cell.
        //
        CopyMem(&this->m_pCells[Index].CellTextBounds, &Rect, sizeof(SWM_RECT));

        // Offset for the checkbox if one exists.
        //
        this->m_pCells[Index].CellTextBounds.Left += (CheckBoxHitAreaWidth + CellTextXOffset);
        this->m_pCells[Index].CellTextBounds.Right -= TrashcanHitAreaWidth;

        GetTextStringBitmapSize (CellData[Index].CellText,
                                 FontInfo,
                                 TRUE,
                                 EFI_HII_OUT_FLAG_CLIP |
                                 EFI_HII_OUT_FLAG_CLIP_CLEAN_X | EFI_HII_OUT_FLAG_CLIP_CLEAN_Y |
                                 EFI_HII_IGNORE_LINE_BREAK,
                                 &this->m_pCells[Index].CellTextBounds,
                                 &MaxGlyphDescent
                                );

        UINT32 CellHeight   = (this->m_pCells[Index].CellBounds.Bottom - this->m_pCells[Index].CellBounds.Top + 1);
        UINT32 StringWidth  = (this->m_pCells[Index].CellTextBounds.Right - this->m_pCells[Index].CellTextBounds.Left + 1);
        UINT32 StringHeight = (this->m_pCells[Index].CellTextBounds.Bottom - this->m_pCells[Index].CellTextBounds.Top + 1);

        this->m_pCells[Index].CellTextBounds.Right   = (this->m_pCells[Index].CellTextBounds.Left + StringWidth - 1);
        this->m_pCells[Index].CellTextBounds.Top    += ((CellHeight / 2) - (StringHeight / 2) + MaxGlyphDescent);
        this->m_pCells[Index].CellTextBounds.Bottom  = (this->m_pCells[Index].CellTextBounds.Top + StringHeight + MaxGlyphDescent - 1);

        // Increment to the next cell position.
        //
        Rect.Top    += (CellBox.Bottom - CellBox.Top + 1);
        Rect.Bottom += (CellBox.Bottom - CellBox.Top + 1);
    }

    // Member Variables
    this->Base.ControlType      = LISTBOX;

    // Functions.
    //
    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;

    this->GetSelectedCellIndex  = &GetSelectedCellIndex;

Exit:

    return;
}


static
VOID Dtor(VOID *this)
{
    UINTN   Index;

    ListBox *privthis = (ListBox *)this;

    for (Index = 0; Index < privthis->m_NumberOfCells; Index++) {
        if (NULL != privthis->m_pCells[Index].pCellText)
        {
            FreePool(privthis->m_pCells[Index].pCellText);
        }
    }

    if (NULL != privthis->m_pCells)
    {
        FreePool(privthis->m_pCells);
    }

    if (NULL != privthis->m_FontInfo)
    {
        FreePool(privthis->m_FontInfo);
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
ListBox *new_ListBox(IN UINT32                          OrigX,
                     IN UINT32                          OrigY,
                     IN UINT32                          CellWidth,
                     IN UINT32                          CellHeight,
                     IN UINT32                          Flags,
                     IN EFI_FONT_INFO                   *FontInfo,
                     IN UINT32                          CellTextXOffset,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *NormalColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *HoverColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *SelectColor,
                     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *GrayOutColor,
                     IN UIT_LB_CELLDATA                 *CellData,
                     IN VOID                            *pSelectionContext)
{
    SWM_RECT    Rect;


    // Validate caller arguments.
    //
    ASSERT (NULL != FontInfo && NULL != NormalColor && NULL != HoverColor && NULL != SelectColor && NULL != CellData);
    if (NULL == FontInfo || NULL == NormalColor || NULL == HoverColor || NULL == SelectColor || NULL == CellData)
    {
        return NULL;
    }

    ListBox *LB = (ListBox *)AllocateZeroPool(sizeof(ListBox));
    ASSERT(NULL != LB);

    if (NULL != LB)
    {
        LB->Ctor        = &Ctor;
        LB->Base.Dtor   = &Dtor;

        Rect.Left       = OrigX;
        Rect.Right      = (OrigX + CellWidth - 1);
        Rect.Top        = OrigY;
        Rect.Bottom     = (OrigY + CellHeight - 1);

        LB->Ctor(LB,
                 Rect,
                 Flags,
                 FontInfo,
                 CellTextXOffset,
                 NormalColor,
                 HoverColor,
                 SelectColor,
                 GrayOutColor,
                 CellData,
                 pSelectionContext);
    }

    return LB;
}


VOID delete_ListBox(IN ListBox *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
