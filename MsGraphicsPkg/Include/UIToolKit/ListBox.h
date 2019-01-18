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

#ifndef _UIT_LIST_BOX_H_
#define _UIT_LIST_BOX_H_


// ListBox option flags.
//
#define UIT_LISTBOX_FLAGS_ORDERED_LIST                  0x00000001
#define UIT_LISTBOX_FLAGS_ALLOW_DELETE                  0x00000002
#define UIT_LISTBOX_FLAGS_CHECKBOX                      0x00000004


// PUBLIC: ListBox cell data.
//
typedef struct _CellData_tag_
{
    CHAR16  *CellText;
    BOOLEAN CheckBoxSelected;
    BOOLEAN TrashcanEnabled;
} UIT_LB_CELLDATA;

// PUBLIC: ListBox return data.
//
//  The listbox supports ONE_OF_OP lists and ORDERED_LIST_OP lists.  For simple list boxes, the
//  only action returned will be LB_ACTION_SELECT.
//
//  LB_ACTION_TOGGLE is only returned if FLAGS_CHECKBOX is set
//  LB_ACTION_DELETE is only returned if FLAGS_ALLOW_DELETE is set
//  LB_ACTION_MOVE is only returned on an ORDERED_LIST

typedef enum {
    LB_ACTION_NONE,
    LB_ACTION_SELECT,
    LB_ACTION_TOGGLE,
    LB_ACTION_DELETE,
    LB_ACTION_MOVE,
    LB_ACTION_BOOT
} LB_ACTION;

typedef enum {
    LB_MOVE_NONE,
    LB_MOVE_UP,
    LB_MOVE_DOWN,
    LB_MOVE_LEFT,
    LB_MOVE_RIGHT
} LB_DIRECTION;

typedef struct {
    LB_ACTION     Action;         // Which action
    UINT32        SelectedCell;   // Selected cell for all actions (Source for drag)
    UINT32        TargetCell;     // Target for move
    LB_DIRECTION  Direction;      //
} LB_RETURN_DATA;

// PRIVATE: ListBox cell display context.
//
typedef struct _CellDisplayInfo
{
    BOOLEAN         CheckboxSelected;
    CHAR16          *pCellText;
    SWM_RECT        CellBounds;
    SWM_RECT        CellTextBounds;
    SWM_RECT        CellCheckBoxBounds;
    BOOLEAN         TrashcanEnabled;
    SWM_RECT        CellTrashcanBounds;
    UINT32          OriginalOrder;
} CellDisplayInfo;

// PRIVATE: Captured Pointer belongs to which sub control.
//
typedef enum {
    LocationNone,
    LocationCheckbox,
    LocationListbox,
    LocationTrashcan
} TOUCH_LOCATION;

//////////////////////////////////////////////////////////////////////////////
// ListBox Class Definition
//
typedef struct _ListBox
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    EFI_FONT_INFO                  *m_FontInfo;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_NormalColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_HoverColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_SelectColor;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_GrayOutColor;

    CellDisplayInfo                 *m_pCells;
    SWM_RECT                        m_ListBoxBounds;
    UINT32                          m_NumberOfCells;
    UINT32                          m_SelectedCell;
    UINT32                          m_SourceCell;
    UINT32                          m_TargetCell;
    LB_DIRECTION                    m_Direction;
    UINT32                          m_HighlightedCell;
    UINT32                          m_Flags;
    LB_ACTION                       m_LastAction;
    VOID                            *m_pSelectionContext;
    TOUCH_LOCATION                  m_CaptureLocation;
    UINT32                          m_CaptureIndex;
    UINTN                           m_CapturePointX;
    OBJECT_STATE                    m_State;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _ListBox                *this,
            IN SWM_RECT                       CellBox,
            IN UINT32                         Flags,
            IN EFI_FONT_INFO                  *FontInfo,
            IN UINT32                         CellTextXOffset,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *NormalColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *HoverColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *SelectColor,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *GrayOutColor,
            IN UIT_LB_CELLDATA                *CellData,
            IN VOID                           *pSelectionContext);

    EFI_STATUS
    (* RenderCell)(IN struct _ListBox  *this,
                   IN UINT32            OrigX,
                   IN UINT32            OrigY,
                   IN OBJECT_STATE      State,
                   IN CHAR16            *pString);

    EFI_STATUS
    (*GetSelectedCellIndex)(IN  struct _ListBox *this,
                            OUT LB_RETURN_DATA  *pReturnData);

} ListBox;


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
                     IN VOID                            *pSelectionContext);

VOID delete_ListBox(IN ListBox *this);


#endif  // _UIT_LIST_BOX_H_.
