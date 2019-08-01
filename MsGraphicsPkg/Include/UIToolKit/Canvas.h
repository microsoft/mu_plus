/** @file

  Implements a simple canvas control for collecting and managing child controls.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _UIT_CANVAS_H_
#define _UIT_CANVAS_H_


// Canvas controls list context.
//
typedef struct _CANVAS_CHILD_CONTROL_
{
    VOID                                *pControl;          // Child control.
    SWM_RECT                            ChildBounds;        // Child control outer bounds.
    BOOLEAN                             Highlightable;      // TRUE == child control supports highlighting.
    BOOLEAN                             Invisible;          // TRUE == child control should be drawn (and receive user input).
    struct _CANVAS_CHILD_CONTROL_       *pNext;             // Next provider in the list.
    struct _CANVAS_CHILD_CONTROL_       *pPrev;             // Previous provider in the list.

} UIT_CANVAS_CHILD_CONTROL;


//////////////////////////////////////////////////////////////////////////////
// Canvas Class Definition
//
typedef struct _Canvas
{
    // *** Base Class ***
    //
    ControlBase                     Base;

    // *** Member variables ***
    //
    SWM_RECT                        m_CanvasBounds;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL   m_CanvasColor;
    UIT_CANVAS_CHILD_CONTROL        *m_pControls;
    UIT_CANVAS_CHILD_CONTROL        *m_pCurrentHighlight;
    UIT_CANVAS_CHILD_CONTROL        *m_pDefaultControl;
    ControlBase                     *m_pCapturedPointer;
    VOID                            *m_pCurrentSelectedControl;

    // *** Functions ***
    //
    VOID
    (*Ctor)(IN struct _Canvas                 *this,
            IN SWM_RECT                       CanvasBounds,
            IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pColor);

    EFI_STATUS
    (*AddControl)(IN struct _Canvas     *this,
                  IN BOOLEAN            Highlightable,
                  IN BOOLEAN            Invisible,
                  IN VOID               *pNewControl);

    EFI_STATUS
    (*GetSelectedControl)(IN  struct _Canvas    *this,
                          OUT VOID              **pControl);

    EFI_STATUS
    (*MoveHighlight)(IN struct _Canvas     *this,
                     IN BOOLEAN             MoveNext);

    EFI_STATUS
    (*SetHighlight)(IN struct _Canvas   *this,
                    IN VOID             *pControl);

    EFI_STATUS
    (*ClearHighlight)(IN struct _Canvas     *this);

    EFI_STATUS
    (*ClearCanvas)(IN struct _Canvas     *this);

    EFI_STATUS
    (*SetDefaultControl)(IN struct _Canvas   *this,
                         IN VOID             *pControl);

} Canvas;


//////////////////////////////////////////////////////////////////////////////
// Public
//
Canvas *new_Canvas(IN SWM_RECT                      Rect,
                   IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pColor);

VOID delete_Canvas(IN Canvas *this);


#endif  // _UIT_CANVAS_H_.
