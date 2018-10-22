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

#include "SimpleUIToolKitInternal.h"


//////////////////////////////////////////////////////////////////////////////
// Private
//
static
OBJECT_STATE
RenderCanvas(IN     Canvas              *this,
             IN     BOOLEAN             DrawHighlight,
             IN     SWM_INPUT_STATE     *pInputState,
             OUT    VOID                **pSelectionContext)
{
    UINTN                       Width, Height;
    UIT_CANVAS_CHILD_CONTROL    *pChildControl  = this->m_pControls;
    ControlBase                 *pControlBase;


    // Compute canvas width and height.
    //
    Width   = (this->m_CanvasBounds.Right  - this->m_CanvasBounds.Left + 1);
    Height  = (this->m_CanvasBounds.Bottom - this->m_CanvasBounds.Top + 1);

    // NOTE: We assume that it's not necessary to fill the entire canvas - it's alraedy been cleared for us.  If
    // we have to fill it every time we render it's a notable performance impact.  Instead, we clear individual control
    // rectangles on the canvas when we switch between canvases.
    //

    // Walk each of the child controls and ask them to draw themselves.
    //
    while (NULL != pChildControl)
    {
        pControlBase = (ControlBase *)pChildControl->pControl;

        // Call the control's base class draw routine.
        //
        pControlBase->Draw (pControlBase,
                            (this->m_pCurrentHighlight == pChildControl ? TRUE : FALSE),
                            pInputState,
                            pSelectionContext
                           );

        pChildControl = pChildControl->pNext;
    }

    return NORMAL;
}


static
EFI_STATUS
SetControlBounds (IN Canvas     *this,
                  IN SWM_RECT   Rect)
{
    EFI_STATUS                  Status          = EFI_SUCCESS;
    UINT32                      NewWidth        = (Rect.Right - Rect.Left + 1);
    UINT32                      NewHeight       = (Rect.Bottom - Rect.Top + 1);
    UIT_CANVAS_CHILD_CONTROL    *pChildControl  = this->m_pControls;
    ControlBase                 *pControlBase;
    SWM_RECT                    ControlRect;


    // We don't support shrinking the canvas - only repositioning.
    //
    if (NewWidth != (this->m_CanvasBounds.Right - this->m_CanvasBounds.Left + 1) ||
        NewHeight != (this->m_CanvasBounds.Bottom - this->m_CanvasBounds.Top + 1))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SUIT]: Not able to resize canvas.\r\n"));
        Status = EFI_UNSUPPORTED;
        goto Exit;
    }

    // Calculate the offset being requested.
    //
    INT32   XOffset = (Rect.Left - this->m_CanvasBounds.Left);
    INT32   YOffset = (Rect.Top - this->m_CanvasBounds.Top);

    // Update the canvas' bounding rectangle.
    //
    this->m_CanvasBounds.Left   += XOffset;
    this->m_CanvasBounds.Right  += XOffset;
    this->m_CanvasBounds.Top    += YOffset;
    this->m_CanvasBounds.Bottom += YOffset;


    // Walk each of the child controls and reposition them by the same amount.
    //
    while (NULL != pChildControl)
    {
        pControlBase = (ControlBase *)pChildControl->pControl;

        // Get the child control's current bounding rectangle.
        //
        pControlBase->GetControlBounds (pControlBase,
                                        &ControlRect
                                       );

        // Move the control's bounding rectangle by the same amount as the canvas.
        //
        ControlRect.Left    += XOffset;
        ControlRect.Right   += XOffset;
        ControlRect.Top     += YOffset;
        ControlRect.Bottom  += YOffset;

        // Set the child control's new bounding rectangle.
        //
        pControlBase->SetControlBounds (pControlBase,
                                        ControlRect
                                       );

        // Update the canvas' child control bookkeeping.
        //
        CopyMem (&pChildControl->ChildBounds, &ControlRect, sizeof(SWM_RECT));

        // Move to the next child control.
    //
        pChildControl = pChildControl->pNext;
    }

Exit:

    return Status;
}


static
EFI_STATUS
GetControlBounds (IN  Canvas    *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_CanvasBounds, sizeof(SWM_RECT));

    return Status;
}


/**
    Adds a UI control to the canvas' child controls list.

    @param[in] this                 Pointer to the canvas.
    @param[in] Highlightable        Indicates whether the child control should be highlightable when navigating the UI via keyboard.
    @param[in] Invisible            Indicates whether the child control should be drawn (and thus receive user input events).
    @param[in] pNewControl          Pointer to the new control to be added to the canvas.

    @retval EFI_SUCCESS             Successfully added the control to the canvas.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to add the interface to the list.

**/
static
EFI_STATUS
AddControl(IN Canvas    *this,
           IN BOOLEAN   Highlightable,
           IN BOOLEAN   Invisible,
           IN VOID      *pNewControl)
{
    EFI_STATUS                  Status              = EFI_SUCCESS;
    EFI_TPL                     PreviousTPL         = 0;
    UIT_CANVAS_CHILD_CONTROL    *pControlContext    = NULL;
    ControlBase                 *pControlBase;


    //DEBUG((DEBUG_INFO, "INFO [SUIT]: Adding control to canvas (Control=0x%x, Highlightable=%s).\r\n", (UINT32)pNewControl, (Highlightable ? L"TRUE" : L"FALSE")));

    // Raise the TPL to avoid race condition with the scan or delete routines.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Allocate space for the new control's context.
    //
    pControlContext = (UIT_CANVAS_CHILD_CONTROL *) AllocatePool(sizeof(UIT_CANVAS_CHILD_CONTROL));
    ASSERT (pControlContext != NULL);

    if (NULL == pControlContext)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }


    // Add the new control at the end of the canvas' controls list.
    //
    if (NULL == this->m_pControls)
    {
        // Empty control list - first add.
        //
        this->m_pControls               = pControlContext;
        pControlContext->pNext          = NULL;
        pControlContext->pPrev          = NULL;
    }
    else
    {
        UIT_CANVAS_CHILD_CONTROL    *pControlListNode = this->m_pControls;

        // Adding to an existing list of child controls - note that we need to add to the end of the list.
        //
        // NOTE: This order is important since it's the same order we're going to use for keyboard tab order across all the controls on the canvas.
        //
        while (NULL != pControlListNode->pNext)
        {
            pControlListNode = pControlListNode->pNext;
        }

        pControlListNode->pNext         = pControlContext;
        pControlContext->pNext          = NULL;
        pControlContext->pPrev          = pControlListNode;
    }


    // Save a pointer to the control and the control's outer bounds and highlightable (key navigation) status.
    //
    pControlContext->pControl           = pNewControl;
    pControlContext->Highlightable      = Highlightable;
    pControlContext->Invisible          = Invisible;

    pControlBase = (ControlBase *)pNewControl;
    pControlBase->GetControlBounds (pNewControl,
                                    &pControlContext->ChildBounds
                                   );

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
    Cleans up the canvas' child controls list.

    @param[in] this                 Pointer to the canvas.

    @retval EFI_SUCCESS             Successfully retrieved event state.

**/
static EFI_STATUS
FreeControlsList(IN  Canvas    *this)
{
    EFI_STATUS                  Status      = EFI_SUCCESS;
    EFI_TPL                     PreviousTPL = 0;
    UIT_CANVAS_CHILD_CONTROL    *pList      = this->m_pControls;
    UIT_CANVAS_CHILD_CONTROL    *pNext      = NULL;
    ControlBase                 *pControlBase;


    // Raise the TPL to avoid race condition with the add routine.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Walk the list and free the nodes.
    //
    while (NULL != pList)
    {
        pNext = pList->pNext;

        // Call the child controls destructor (in the base class definition).
        //
        pControlBase = (ControlBase *)pList->pControl;
        pControlBase->Dtor((VOID *)pControlBase);

        // Free the child control list node.
        //
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


static
EFI_STATUS
GetSelectedControl(IN  Canvas   *this,
                   OUT VOID     **pControl)
{
    EFI_STATUS  Status = EFI_SUCCESS;


    *pControl = this->m_pCurrentSelectedControl;


    return Status;
}


static
EFI_STATUS
ClearHighlight (IN Canvas   *this)
{
    EFI_STATUS  Status = EFI_SUCCESS;
    ControlBase *pControlBase;


    // Check whether we're highlighting anything currently.
    //
    if (NULL == this->m_pCurrentHighlight)
    {
        goto Exit;
    }

    // Call child control currently being highlighted and tell it to stop
    // drawing the highlight.
    //
    pControlBase = (ControlBase *)this->m_pCurrentHighlight->pControl;

    if (NULL == pControlBase)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    pControlBase->Draw (pControlBase,
                        FALSE,      // No highlight.
                        NULL,
                        NULL
                       );

    // Set the current highlight pointer to NULL since we're no longer highlighting.
    //
    this->m_pCurrentHighlight = NULL;

Exit:

    return Status;
}


static
EFI_STATUS
MoveHighlight (IN Canvas    *this,
    IN BOOLEAN   MoveNext)
{
    EFI_STATUS                  Status = EFI_SUCCESS;
    ControlBase                 *pControlBase;
    UIT_CANVAS_CHILD_CONTROL    *pControl;


    //DEBUG((DEBUG_INFO, "INFO [SUIT]: Moving highlight to %s control.\r\n", (TRUE == MoveNext ? L"NEXT" : L"PREVIOUS")));

    // If there aren't any child controls, there's nothing to be highlighted.
    //
    if (NULL == this->m_pControls)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    // Check whether we're highlighting anything currently.  If not, we'll start
    // with the first highlighable control in our list (not all controls can be highlighted).
    //
    if (NULL == this->m_pCurrentHighlight)
    {
        pControl = this->m_pControls;

        // If we're moving the highlight "forward", find the first highlightable control.  Otherwise
        // look for the last.
        //
        if (TRUE == MoveNext)
        {
            // Scan forward through the controls list until we find the first highlightable control.
            //
            while ((NULL != pControl) && ((FALSE == pControl->Highlightable) || (((ControlBase *)(pControl->pControl) != NULL) && (((ControlBase *)(pControl->pControl))->GetControlState(pControl->pControl) == GRAYED))))
        {
                pControl = pControl->pNext;
                //pControlBase = (ControlBase *)(pControl->pControl);
            }
        }
        else
        {
            UIT_CANVAS_CHILD_CONTROL    *pLastControl = NULL;

            // Scan to the end of the controls list looking for the last highlightable control.
            //
            while (NULL != pControl)
            {
                if ((TRUE == pControl->Highlightable) && (((ControlBase *)(pControl->pControl) != NULL) && (((ControlBase *)(pControl->pControl))->GetControlState(pControl->pControl) != GRAYED)))
                {
                    pLastControl = pControl;
                }

                pControl = pControl->pNext;
            }

            // Did we find one?
            //
            if (NULL != pLastControl)
            {
                pControl = pLastControl;
            }
        }

        // If we didn't find a control that can be highlighted, exit.
        //
        if (NULL == pControl)
        {
            Status = EFI_NOT_FOUND;
            goto Exit;
        }

        // Otherwise select the first highlightable control we found.
        //
        this->m_pCurrentHighlight = pControl;
    }
    else
    {
        // Move to the next highlightable control, based on the move direction specified by the caller.
        //
        pControl = this->m_pCurrentHighlight;
        do
        {
            pControl = (TRUE == MoveNext ? pControl->pNext : pControl->pPrev);
        } while ((NULL != pControl) && ((FALSE == pControl->Highlightable) || (((ControlBase *)(pControl->pControl) != NULL) && (((ControlBase *)(pControl->pControl))->GetControlState(pControl->pControl) == GRAYED))));

        // Clear the currently highlighted control.
        //
        ClearHighlight (this);

        // If there is no next control to highlight, return.
        //
        if (NULL == pControl)
        {
            Status = EFI_NOT_FOUND;
            goto Exit;
        }

        // Otherwise, update the highlight pointer to the control.
        //
        this->m_pCurrentHighlight = pControl;
    }

    // Call child control and tell it to highlight itself.
    //
    pControlBase = (ControlBase *)this->m_pCurrentHighlight->pControl;

    if (NULL == pControlBase)
    {
        Status = EFI_NOT_FOUND;
        goto Exit;
    }

    //DEBUG((DEBUG_INFO, "INFO [SUIT]: Highlighting control: 0x%x.\r\n", (UINT32)pControlBase));

    // Render the control with highlight.
    //
    pControlBase->Draw (pControlBase,
        TRUE,
        NULL,
        NULL
    );
Exit:

    return Status;
}


static
EFI_STATUS
SetHighlight (IN Canvas   *this,
              IN VOID     *pControl)
{
    EFI_STATUS                  Status = EFI_NOT_FOUND;
    UIT_CANVAS_CHILD_CONTROL    *pControlList = this->m_pControls;
    ControlBase                 *pControlBase;


    // Check whether we're highlighting anything currently.
    //
    if (NULL != this->m_pCurrentHighlight)
    {
        // Clear the currently highlighted control.
        //
        ClearHighlight (this);
    }

    // Walk through the list of child controls, looking for the caller-specified control.
    //
    while (NULL != pControlList)
    {
        // If we found it *and* it's highlightable, set current highlight to it.
        //
        if (pControlList->pControl == pControl && TRUE == pControlList->Highlightable)
        {
            this->m_pCurrentHighlight = pControlList;
            Status = EFI_SUCCESS;
            break;
        }

        pControlList = pControlList->pNext;
    }

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_INFO, "INFO [SUIT]: Failed to find canvas child control to set its highlight (%r).\r\n", Status));
        goto Exit;
    }

    // Draw the control with highlight (if it supports it).
    //
    pControlBase = (ControlBase *)this->m_pCurrentHighlight->pControl;

    pControlBase->Draw (pControlBase,
                        TRUE,      // Highlight.
                        NULL,
                        NULL
                       );

Exit:

    return Status;
}


static
EFI_STATUS
ClearCanvas(IN  Canvas    *this)
{
    EFI_STATUS                  Status          = EFI_SUCCESS;
    UIT_CANVAS_CHILD_CONTROL    *pChildControl  = this->m_pControls;
    ControlBase                 *pControlBase;
    SWM_RECT                    *pChildControlRect;
    UINT32                      FillWidth, FillHeight;


    // For whatever reason, GOP rendering performance is worse when wiping the entire canvas rectangle as
    // opposed to blitting the individual control rectangles.
    //

    // Walk through all the child controls for each, get their bounding rectangle and fill it with the canvas color.
    //
    while (NULL != pChildControl)
    {
        pControlBase        = (ControlBase *)pChildControl->pControl;
        pChildControlRect   = &pChildControl->ChildBounds;

        FillWidth   = (pChildControlRect->Right - pChildControlRect->Left + 1);
        FillHeight  = (pChildControlRect->Bottom - pChildControlRect->Top + 1);

        mUITSWM->BltWindow (mUITSWM,
                            mClientImageHandle,
                            &this->m_CanvasColor,
                            EfiBltVideoFill,
                            0,
                            0,
                            pChildControlRect->Left,
                            pChildControlRect->Top,
                            FillWidth,
                            FillHeight,
                            FillWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                           );

        pChildControl = pChildControl->pNext;
    }

    return Status;
}


static
EFI_STATUS
SetDefaultControl (IN struct _Canvas   *this,
                   IN VOID             *pControl)
{
    EFI_STATUS                  Status = EFI_NOT_FOUND;
    UIT_CANVAS_CHILD_CONTROL    *pControlList = this->m_pControls;
    ControlBase                 *pControlBase;


    // TODO: Need to check whether there's already a default control?

    // Walk through the list of child controls, looking for the caller-specified control.
    //
    while (NULL != pControlList)
    {
        // If we found it set it as the default control.
        //
        if (pControlList->pControl == pControl)
        {
            this->m_pDefaultControl = pControlList;
            Status = EFI_SUCCESS;
            break;
        }

        pControlList = pControlList->pNext;
    }

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_INFO, "INFO [SUIT]: Failed to find canvas child control to set as default (%r).\r\n", Status));
        goto Exit;
    }

    // Update the control's state so it can signal that it's the default (visual indicator) then draw.
    //
    pControlBase = (ControlBase *)this->m_pDefaultControl->pControl;

    pControlBase->SetControlState (pControlBase,
                                   KEYDEFAULT
                                  );

    pControlBase->Draw (pControlBase,
                        FALSE,
                        NULL,
                        NULL
                       );

Exit:

    return Status;
}


static
EFI_STATUS
SetControlState (IN Canvas          *this,
                 IN OBJECT_STATE    State)
{
    return EFI_SUCCESS;
}

static
OBJECT_STATE
GetControlState(IN Label          *this)
{
    return NORMAL; //Object state cannot be changed
}


static
EFI_STATUS
CopySettings (IN Canvas  *this,
              IN Canvas  *prev) {

    UIT_CANVAS_CHILD_CONTROL    *pThisChildControl  = this->m_pControls;
    UIT_CANVAS_CHILD_CONTROL    *pPrevChildControl  = prev->m_pControls;
    ControlBase                 *pThisControlBase;
    ControlBase                 *pPrevControlBase;
    EFI_STATUS                   Status;

    // Walk each of the child controls and ask them to copy their settings.
    //
    this->m_pCurrentHighlight = GetEquivalentControl(prev->m_pCurrentHighlight,prev,this);

    while ((NULL != pThisChildControl) && (NULL != pPrevChildControl))
    {
        ASSERT (pPrevChildControl != NULL);

        pThisControlBase = (ControlBase *)pThisChildControl->pControl;
        pPrevControlBase = (ControlBase *)pPrevChildControl->pControl;

        if (pThisControlBase->ControlType != pPrevControlBase->ControlType) {
            return EFI_INCOMPATIBLE_VERSION;
        }

        // Call the control to get its own settings.
        //
        Status = pThisControlBase->CopySettings (pThisControlBase,
                                                 pPrevControlBase);
        if (EFI_ERROR(Status)) {
            return EFI_INCOMPATIBLE_VERSION;
        }

        pThisChildControl = pThisChildControl->pNext;
        pPrevChildControl = pPrevChildControl->pNext;
    }

    if (pThisChildControl != pPrevChildControl) {
        DEBUG((DEBUG_ERROR, "CanvasCopySettings - unequal menus\n"));
        return EFI_INCOMPATIBLE_VERSION;
    }

    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    Canvas              *this,
      IN    BOOLEAN             DrawHighlight,          // Ignored by canvas class since the canvas itself doesn't get highlighted (but it's part of our base class).
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{
    OBJECT_STATE                    ControlState        = NORMAL;
    UIT_CANVAS_CHILD_CONTROL        *pChildControl      = this->m_pControls;
    ControlBase                     *pControlBase;
    SWM_RECT                        *pChildControlRect;
    MS_SWM_ABSOLUTE_POINTER_STATE   *pTouchState;


    // If user input state is NULL, draw the canvas and all child controls.  Otherwise we need to look at the
    // state to determine what to do.
    //
    if (NULL == pInputState || (NULL != pInputState && 0 == pInputState->InputType))
    {
        ControlState = RenderCanvas (this,
                                     FALSE,
                                     NULL,
                                     NULL
                                    );

        goto Exit;
    }

    // If user input state is a key press, process it.
    //
    if (SWM_INPUT_TYPE_KEY == pInputState->InputType)
    {
        // First, send the key press event to the highlighted control (if there is one).
        //
        if (NULL != this->m_pCurrentHighlight)
        {
            pChildControl   = this->m_pCurrentHighlight;
            pControlBase    = (ControlBase *)this->m_pCurrentHighlight->pControl;

            ControlState = pControlBase->Draw (pControlBase,
                                               TRUE,            // Highlight.
                                               pInputState,
                                               pSelectionContext
                                              );

            this->m_pCurrentSelectedControl = (VOID *)pControlBase;
        }

        // Next, if the control state is still normal (i.e., the highlighted control isn't signalling an action), send
        // the key press event to the default control (if there is one).
        //
        if (NORMAL == ControlState && NULL != this->m_pDefaultControl)
        {
            pChildControl   = this->m_pDefaultControl;
            pControlBase    = (ControlBase *)this->m_pDefaultControl->pControl;

            ControlState = pControlBase->Draw(pControlBase,
                                              FALSE,            // No highlight.
                                              pInputState,
                                              pSelectionContext
                                             );

            this->m_pCurrentSelectedControl = (VOID *)pControlBase;
        }

        goto Exit;
    }


    // Otherwise if it's a touch input event within our canvas area, walk the list of child controls and ask each of them to draw.
    //
    if (SWM_INPUT_TYPE_TOUCH == pInputState->InputType)
    {

        // Walk through all the child controls and draw each.
        //
        pTouchState         = &pInputState->State.TouchState;

#if 0 // Nobody sets m_pCapturedPointer
        if (NULL != this->m_pCapturedPointer)
        {
            pControlBase        = this->m_pCapturedPointer;

            ControlState = pControlBase->Draw(pControlBase,
                                              FALSE,           // No highlight.
                                              pInputState,
                                              pSelectionContext
                                             );
            if (pTouchState->ActiveButtons == 0x00)
            {
                this->m_pCapturedPointer = NULL;
            }
            this->m_pCurrentSelectedControl = (VOID *)pControlBase;

            return ControlState;
        }
#endif

        while (NULL != pChildControl)
        {
            pControlBase        = (ControlBase *)pChildControl->pControl;
            pChildControlRect   = &pChildControl->ChildBounds;

            if (FALSE == pChildControl->Invisible)
            {
                // Draw the child control, clearing the highlight rectangle now that the user is interacting via touch.
                //
                ControlState = pControlBase->Draw(pControlBase,
                                                  FALSE,           // No highlight.
                                                  pInputState,
                                                  pSelectionContext
                                                 );

                // If the control indicates it's being selected, clear the highlight.
                //
                if (SELECT == ControlState)
                {
                    this->ClearHighlight(this);
                    this->m_pCurrentSelectedControl = (VOID *)pControlBase;
                    goto Exit;
                }

                // If the control indicates it wants keyboard input focus, set the highlight. This is typical for an editbox
                // that a user selected with mouse/touch.
                //
                if (KEYFOCUS == ControlState &&
                    pTouchState->CurrentX >= pChildControlRect->Left && pTouchState->CurrentX <= pChildControlRect->Right &&
                    pTouchState->CurrentY >= pChildControlRect->Top  && pTouchState->CurrentY <= pChildControlRect->Bottom)
                {
                    // Set highlight on this control (automatically clears any control that might currently have highlight).
                    //
                    this->SetHighlight (this,
                                        pChildControl->pControl
                                       );

                    goto Exit;
                }
            }

            pChildControl = pChildControl->pNext;
        }
    }

Exit:

    return ControlState;
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _Canvas                 *this,
     IN SWM_RECT                       CanvasBounds,
     IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pColor)
{

    // Initialize variables.
    //
    this->m_CanvasColor     = *pColor;

    // Save the canvas bounding rectangle.
    //
    CopyMem(&this->m_CanvasBounds, &CanvasBounds, sizeof(SWM_RECT));


    // No child controls yet.
    //
    this->m_pControls           = NULL;

    // Nothing being highlighted, no default control.
    //
    this->m_pCurrentHighlight   = NULL;
    this->m_pDefaultControl     = NULL;
    this->m_pCapturedPointer    = NULL;

    // Member Variables
    this->Base.ControlType      = CANVAS;
    // Functions.
    //

    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;

    this->AddControl            = &AddControl;
    this->GetSelectedControl    = &GetSelectedControl;
    this->SetHighlight          = &SetHighlight;
    this->ClearHighlight        = &ClearHighlight;
    this->MoveHighlight         = &MoveHighlight;
    this->ClearCanvas           = &ClearCanvas;
    this->SetDefaultControl     = &SetDefaultControl;

    return;
}


static
VOID
Dtor(VOID *this)
{
    Canvas *privthis = (Canvas *)this;


    // Walk the controls list and free each.
    //
    FreeControlsList(privthis);


    if (NULL != privthis)
    {
        FreePool(privthis);
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
// Public
//
Canvas *new_Canvas(IN   SWM_RECT                        Rect,
                   IN   EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *pColor)
{

    Canvas *C = (Canvas *)AllocateZeroPool(sizeof(Canvas));
    ASSERT(NULL != C);

    if (NULL != C)
    {
        C->Ctor         = &Ctor;
        C->Base.Dtor    = &Dtor;

        C->Ctor(C,
                Rect,
                pColor);
    }

    return C;
}


VOID delete_Canvas(IN Canvas *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
