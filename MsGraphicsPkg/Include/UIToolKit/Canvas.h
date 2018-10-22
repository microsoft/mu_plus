/** @file

  Implements a simple canvas control for collecting and managing child controls.

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
