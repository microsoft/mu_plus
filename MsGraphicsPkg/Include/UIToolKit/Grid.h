/** @file

  Implements a simple grid control for aligning child controls on a canvas.

  NOTE: This is a primitive version and it simply translates the child control's
  origin to align with the defined grid before it's added to the canvas as normal.
  Child controls remain children of the canvas only.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _UIT_GRID_H_
#define _UIT_GRID_H_


// Grid controls list context.
//
typedef struct _GRID_CHILD_CONTROL_
{
    VOID                                *pControl;          // Child control.
    UINT32                              Row;                // Row in the grid where the child control is located.
    UINT32                              Column;             // Column in the grid where the child control is located.
    struct _GRID_CHILD_CONTROL_         *pNext;             // Next provider in the list.
    struct _GRID_CHILD_CONTROL_         *pPrev;             // Previous provider in the list.

} UIT_GRID_CHILD_CONTROL;

//////////////////////////////////////////////////////////////////////////////
// Grid Class Definition
//
typedef struct _Grid
{
    // *** Base Class ***
    //
    ControlBase                 Base;

    // *** Member variables ***
    //
    SWM_RECT                    m_GridBounds;
    UINT32                      m_Rows;
    UINT32                      m_Columns;
    UINT32                      m_GridCellWidth;
    UINT32                      m_GridCellHeight;
    UINT32                      m_GridInitialHeight;
    BOOLEAN                     m_TruncateControl;
    UIT_GRID_CHILD_CONTROL      *m_pControls;
    Canvas                      *m_ParentCanvas;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _Grid     *this,
            IN Canvas           *ParentCanvas,
            IN SWM_RECT         GridBounds,
            IN UINT32           Rows,
            IN UINT32           Columns,
            IN BOOLEAN          TruncateChildControl);

    EFI_STATUS
    (*AddControl)(IN struct _Grid   *this,
                  IN BOOLEAN        Highlightable,
                  IN BOOLEAN        Invisible,
                  IN UINT32         Row,
                  IN UINT32         Column,
                  IN VOID           *pNewControl);

} Grid;


//////////////////////////////////////////////////////////////////////////////
// Public
//
Grid *new_Grid(IN Canvas    *ParentCanvas,
               IN SWM_RECT  Rect,
               IN UINT32    Rows,
               IN UINT32    Columns,
               IN BOOLEAN   TruncateChildControl);

VOID delete_Grid(IN Grid *this);


#endif  // _UIT_GRID_H_.
