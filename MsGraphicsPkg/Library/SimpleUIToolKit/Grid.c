/** @file

  Implements a simple grid control for aligning child controls on a canvas.

  NOTE: This is a primitive version and it simply translates the child control's
  origin to align with the defined grid before it's added to the canvas as normal.
  Child controls remain children of the canvas only.

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
EFI_STATUS
SetControlBounds (IN Grid       *this,
                  IN SWM_RECT   Bounds)
{
    // For now, do nothing.
    return EFI_SUCCESS;
}


static
EFI_STATUS
GetControlBounds (IN  Grid      *this,
                  OUT SWM_RECT  *pBounds)
{
    EFI_STATUS Status = EFI_SUCCESS;


    CopyMem (pBounds, &this->m_GridBounds, sizeof(SWM_RECT));

    return Status;
}


/**
    Adjusts the client controls origin to align with the grid location specified (the child's xy origin
    is an *offset* from the origin of the specified grid cell) and adds the adjusted client control to
    the parent canvas' controls list.  The child control is also added to the grid's controls list in
    order to support dynamically repositioning controls after they've been added to the canvas.

    @param[in] this                 Pointer to the grid.
    @param[in] Row                  Grid row where the child control should be added.
    @param[in] Column               Grid column where the child control should be added.
    @param[in] pNewControl          Pointer to the new control to be added to the grid & parent canvas.

    @retval EFI_SUCCESS             Successfully added the control.
    @retval EFI_OUT_OF_RESOURCES    Insufficient resources to add the interface to the list.

**/
static
EFI_STATUS
AddControl(IN Grid      *this,
           IN BOOLEAN   Highlightable,
           IN BOOLEAN   Invisible,
           IN UINT32    Row,
           IN UINT32    Column,
           IN VOID      *pNewControl)
{
    EFI_STATUS                  Status              = EFI_SUCCESS;
    EFI_TPL                     PreviousTPL         = 0;
    UIT_GRID_CHILD_CONTROL      *pControlList       = NULL;
    ControlBase                 *pControlBase;
    SWM_RECT                    ControlBounds;


    //DEBUG((DEBUG_INFO, "INFO [SUIT]: Adding control to grid (Control=0x%x, Row=%d, Column=%d).\r\n", (UINT32)pNewControl, Row, Column));

    // Validate caller parameters.  Make sure the specified row-column address falls within the grid.
    //
    if (NULL == this || NULL == pNewControl || Row >= this->m_Rows || Column >= this->m_Columns)
    {
        Status = EFI_INVALID_PARAMETER;
        goto Exit;
    }

    // Raise the TPL to avoid race condition with the scan or delete routines.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Allocate space for the new control's context and add it to the list.
    //
    pControlList = this->m_pControls;
    this->m_pControls = (UIT_GRID_CHILD_CONTROL *) AllocatePool(sizeof(UIT_GRID_CHILD_CONTROL));

    ASSERT (NULL != this->m_pControls);
    if (NULL == this->m_pControls)
    {
        Status = EFI_OUT_OF_RESOURCES;
        goto Exit;
    }

    this->m_pControls->pControl = pNewControl;
    this->m_pControls->Row      = Row;
    this->m_pControls->Column   = Column;

    this->m_pControls->pNext = pControlList;
    this->m_pControls->pPrev = NULL;

    if (NULL != pControlList)
    {
        pControlList->pPrev = this->m_pControls;
    }

    // Get the child controls origin - this will be the offset from the origin of the grid row-column address specified.
    //
    pControlBase = (ControlBase *)pNewControl;
    Status = pControlBase->GetControlBounds (pNewControl,
                                             &ControlBounds
                                            );

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SUIT] - Grid class failed to obtain controls bounding rectangle.\r\n"));
        goto Exit;
    }

    // Compute the screen location associated with the specified grid row-column address.  Note that this may also restrict the
    // control's size so it doesn't flow into the neighbors cell.
    //
    SWM_RECT    NewBounds;
    UINT32      CellOrigX       = (this->m_GridBounds.Left + (Column * this->m_GridCellWidth));
    UINT32      CellOrigY       = (this->m_GridBounds.Top + (Row * this->m_GridCellHeight));
    UINT32      CellEndX        = (CellOrigX + this->m_GridCellWidth - 1);
    UINT32      CellEndY        = (CellOrigY + this->m_GridCellHeight - 1);
    UINT32      ControlWidth    = (ControlBounds.Right - ControlBounds.Left + 1);
    UINT32      ControlHeight   = (ControlBounds.Bottom - ControlBounds.Top + 1);
    UINT32      VerticalAdjust  = (this->m_GridCellHeight - ControlHeight) / 2;

    if (ControlHeight > this->m_GridCellHeight) {
        DEBUG ((DEBUG_ERROR, "ERROR [Grid]: Found Grid element larger than specified height. GridH=%d,ElemetH=%d.\r\n",this->m_GridCellHeight,ControlHeight));
        VerticalAdjust = 0;
        this->m_GridCellHeight = ControlHeight;
    }

    NewBounds.Left      = (CellOrigX + ControlBounds.Left);
    NewBounds.Top       = (CellOrigY + ControlBounds.Top + VerticalAdjust);
    NewBounds.Right     = (0 != ControlWidth  ? (NewBounds.Left + ControlWidth - 1) : NewBounds.Left);
    NewBounds.Bottom    = (0 != ControlHeight ? (NewBounds.Top + ControlHeight - 1) : NewBounds.Top);

    // If the child control is larger than the grid cell size and the truncate flag is set, limit the size to the cell size.
    //
    if ((NewBounds.Right > CellEndX) && (TRUE == this->m_TruncateControl))
    {
        NewBounds.Right = CellEndX;
    }

    if ((NewBounds.Bottom > CellEndY) && (TRUE == this->m_TruncateControl))
    {
        NewBounds.Bottom = CellEndY;
    }

    // Reposition the child control by moving it to the grid-specified cell location and offset.
    //
    Status = pControlBase->SetControlBounds (pNewControl,
                                             NewBounds
                                            );

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SUIT] - Grid class failed to place child control on the grid.\r\n"));
        goto Exit;
    }

    // Now that the child control has been relocated, add it to the parent canvas controls list so it's rendered and managed.
    //
    Status = this->m_ParentCanvas->AddControl (this->m_ParentCanvas,
                                               Highlightable,
                                               Invisible,
                                               pNewControl);

    if (EFI_ERROR(Status))
    {
        DEBUG ((DEBUG_ERROR, "ERROR [SUIT] - Grid class failed to add child control to the parent canvas.\r\n"));
        goto Exit;
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
    Cleans up the grid's child controls list.

    @param[in] this                 Pointer to the grid.

    @retval EFI_SUCCESS             Successfully retrieved event state.

**/
static EFI_STATUS
FreeControlsList(IN  Grid    *this)
{
    EFI_STATUS                  Status      = EFI_SUCCESS;
    EFI_TPL                     PreviousTPL = 0;
    UIT_GRID_CHILD_CONTROL      *pList      = this->m_pControls;
    UIT_GRID_CHILD_CONTROL      *pNext      = NULL;


    // Raise the TPL to avoid race condition with the add routine.
    //
    PreviousTPL = gBS->RaiseTPL (TPL_NOTIFY);

    // Walk the list and free the nodes.
    //
    while (NULL != pList)
    {
        pNext = pList->pNext;

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
SetControlState (IN Grid            *this,
                 IN OBJECT_STATE    State)
{
    return EFI_SUCCESS; //Object state cannot be changed
}

static
OBJECT_STATE
GetControlState(IN Grid            *this)
{
    return NORMAL; //Object state not maintained for this control
}


static
EFI_STATUS
CopySettings (IN Grid  *this,
              IN Grid  *prev) {

    return EFI_SUCCESS;
}


static
OBJECT_STATE
Draw (IN    Grid                *this,
      IN    BOOLEAN             DrawHighlight,
      IN    SWM_INPUT_STATE     *pInputState,
      OUT   VOID                **pSelectionContext)
{
    // Grids don't draw.
    //
    return NORMAL;
}


//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor(IN struct _Grid     *this,
     IN Canvas           *ParentCanvas,
     IN SWM_RECT         GridBounds,
     IN UINT32           Rows,
     IN UINT32           Columns,
     IN BOOLEAN          TruncateChildControl)
{

    // Save parent canvas.
    //
    this->m_ParentCanvas = ParentCanvas;

    // Save number of columns and rows.
    //
    this->m_Columns = Columns;
    this->m_Rows    = Rows;

    // Remember whether we should truncate the child control to fix the grid cell's bounding box.
    //
    this->m_TruncateControl = TruncateChildControl;

    // Save the grid bounding rectangle.
    //
    CopyMem(&this->m_GridBounds, &GridBounds, sizeof(SWM_RECT));

    // Compute grid cell size based on overall side and number of rows & columns.
    //
    this->m_GridCellWidth   = ((GridBounds.Right - GridBounds.Left + 1) / Columns);
    this->m_GridCellHeight  = ((GridBounds.Bottom - GridBounds.Top + 1) / Rows);
    this->m_GridInitialHeight = this->m_GridCellHeight;

    // No child controls yet.
    //
    this->m_pControls           = NULL;

    // Member Variables
    this->Base.ControlType      = GRID;

    // Functions.
    //
    this->Base.Draw             = (DrawFunctionPtr) &Draw;
    this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
    this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
    this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
    this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
    this->Base.CopySettings     = (CopySettingsFunctionPtr) &CopySettings;
    this->AddControl            = &AddControl;

    return;
}


static
VOID
Dtor(VOID *this)
{
    Grid *privthis = (Grid *)this;


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
Grid *new_Grid(IN Canvas    *ParentCanvas,
               IN SWM_RECT  Rect,
               IN UINT32    Rows,
               IN UINT32    Columns,
               IN BOOLEAN   TruncateChildControl)
{

    Grid *G = (Grid *)AllocateZeroPool(sizeof(Grid));
    ASSERT(NULL != G);

    if (NULL != G)
    {
        G->Ctor         = &Ctor;
        G->Base.Dtor    = &Dtor;

        G->Ctor(G,
                ParentCanvas,
                Rect,
                Rows,
                Columns,
                TruncateChildControl
               );
    }

    //DEBUG ((DEBUG_INFO, "INFO [SUIT]: Created new Grid (Rows=%d, Columns=%d).\r\n", Rows, Columns));

    return G;
}


VOID delete_Grid(IN Grid *this)
{
    if (NULL != this)
    {
        this->Base.Dtor((VOID *)this);
    }
}
