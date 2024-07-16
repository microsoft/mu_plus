/** @file

  Implements a simple progress bar control for showing incremental progress.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SimpleUIToolKitInternal.h"

//////////////////////////////////////////////////////////////////////////////
// Private
//
static
EFI_STATUS
UpdateProgressPercent (
  IN     ProgressBar  *this,
  IN     UINT8        NewPercent
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if (NewPercent > 100) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Store the updated percent value.
  //
  this->m_BarPercent = NewPercent;

Exit:

  return Status;
}

static
EFI_STATUS
RenderProgressBar (
  IN ProgressBar  *this
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINTN       Width, Height;

  // Compute progres bar width and height.
  //
  Width  = SWM_RECT_WIDTH (this->m_pProgressBar->ProgressBarBoundsCurrent);
  Height = SWM_RECT_HEIGHT (this->m_pProgressBar->ProgressBarBoundsCurrent);

  // Draw the progress bar background first.
  //
  mUITSWM->BltWindow (
             mUITSWM,
             mClientImageHandle,
             &this->m_BarBackgroundColor,
             EfiBltVideoFill,
             0,
             0,
             this->m_pProgressBar->ProgressBarBoundsCurrent.Left,
             this->m_pProgressBar->ProgressBarBoundsCurrent.Top,
             Width,
             Height,
             Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
             );

  // Draw the progress bar color next.
  //

  Width = ((SWM_RECT_WIDTH (this->m_pProgressBar->ProgressBarBoundsCurrent) * this->m_BarPercent) / 100);

  mUITSWM->BltWindow (
             mUITSWM,
             mClientImageHandle,
             &this->m_BarColor,
             EfiBltVideoFill,
             0,
             0,
             this->m_pProgressBar->ProgressBarBoundsCurrent.Left,
             this->m_pProgressBar->ProgressBarBoundsCurrent.Top,
             Width,
             Height,
             Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
             );

  return Status;
}

static
EFI_STATUS
SetControlBounds (
  IN ProgressBar  *this,
  IN SWM_RECT     Bounds
  )
{
  EFI_STATUS  Status  = EFI_SUCCESS;
  INT32       XOffset = (Bounds.Left - this->m_pProgressBar->ProgressBarBoundsCurrent.Left);
  INT32       YOffset = (Bounds.Top - this->m_pProgressBar->ProgressBarBoundsCurrent.Top);

  // Translate (and possibly truncate) the current progress bar bounding box.
  //
  CopyMem (&this->m_pProgressBar->ProgressBarBoundsCurrent, &Bounds, sizeof (SWM_RECT));

  // Also translate the bounding box limit.
  //
  this->m_pProgressBar->ProgressBarBoundsLimit.Left   += XOffset;
  this->m_pProgressBar->ProgressBarBoundsLimit.Right  += XOffset;
  this->m_pProgressBar->ProgressBarBoundsLimit.Top    += YOffset;
  this->m_pProgressBar->ProgressBarBoundsLimit.Bottom += YOffset;

  return Status;
}

static
EFI_STATUS
GetControlBounds (
  IN  ProgressBar  *this,
  OUT SWM_RECT     *pBounds
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  CopyMem (pBounds, &this->m_pProgressBar->ProgressBarBoundsCurrent, sizeof (SWM_RECT));

  return Status;
}

static
EFI_STATUS
SetControlState (
  IN ProgressBar   *this,
  IN OBJECT_STATE  State
  )
{
  return EFI_SUCCESS;  // Object state cannot be changed
}

static
OBJECT_STATE
GetControlState (
  IN ProgressBar  *this
  )
{
  return NORMAL;   // Object state not maintained for this control. Return the default
}

static
OBJECT_STATE
Draw (
  IN    ProgressBar      *this,
  IN    BOOLEAN          DrawHighlight,
  IN    SWM_INPUT_STATE  *pInputState,
  OUT   VOID             **pSelectionContext
  )
{
  // Draw the progress bar.
  //
  RenderProgressBar (this);

  // No selection context associated with a progress bar.
  //
  if (NULL != pSelectionContext) {
    *pSelectionContext = NULL;
  }

  return NORMAL;
}

//////////////////////////////////////////////////////////////////////////////
//
//
static
VOID
Ctor (
  IN struct _ProgressBar            *this,
  IN SWM_RECT                       *ProgressBarBox,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBarColor,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBarBackgroundColor,
  IN UINT8                          InitialPercent
  )
{
  // Capture bar and bar background colors.
  //
  this->m_BarColor           = *pBarColor;
  this->m_BarBackgroundColor = *pBarBackgroundColor;

  // Allocate space for storing display information.
  //
  this->m_pProgressBar = AllocateZeroPool (sizeof (ProgressBarDisplayInfo));

  ASSERT (NULL != this->m_pProgressBar);
  if (NULL == this->m_pProgressBar) {
    goto Exit;
  }

  // Capture the bounding box for the progress bar.
  //
  CopyMem (&this->m_pProgressBar->ProgressBarBoundsCurrent, ProgressBarBox, sizeof (SWM_RECT));

  // Capture the initial percent value.
  this->m_BarPercent = InitialPercent;

  // Member Variables
  this->Base.ControlType = PROGRESSBAR;

  // Functions.
  //
  this->Base.Draw             = (DrawFunctionPtr) &Draw;
  this->Base.SetControlBounds = (SetControlBoundsFunctionPtr) &SetControlBounds;
  this->Base.GetControlBounds = (GetControlBoundsFunctionPtr) &GetControlBounds;
  this->Base.SetControlState  = (SetControlStateFunctionPtr) &SetControlState;
  this->Base.GetControlState  = (GetControlStateFunctionPtr) &GetControlState;
  this->UpdateProgressPercent = &UpdateProgressPercent;

Exit:

  return;
}

static
VOID
Dtor (
  VOID  *this
  )
{
  ProgressBar  *privthis = (ProgressBar *)this;

  if (NULL != privthis->m_pProgressBar) {
    FreePool (privthis->m_pProgressBar);
  }

  if (NULL != privthis) {
    FreePool (privthis);
  }

  return;
}

//////////////////////////////////////////////////////////////////////////////
// Public
//
ProgressBar *
new_ProgressBar (
  IN UINT32                         OrigX,
  IN UINT32                         OrigY,
  IN UINT32                         ProgressBarWidth,
  IN UINT32                         ProgressBarHeight,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBarColor,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *pBarBackgroundColor,
  IN UINT8                          InitialPercent
  )
{
  SWM_RECT  Rect;

  ProgressBar  *P = (ProgressBar *)AllocateZeroPool (sizeof (ProgressBar));

  ASSERT (NULL != P);

  if (NULL != P) {
    P->Ctor      = &Ctor;
    P->Base.Dtor = &Dtor;

    SWM_RECT_INIT2 (
      Rect,
      OrigX,
      OrigY,
      ProgressBarWidth,
      ProgressBarHeight
      );

    P->Ctor (
         P,
         &Rect,
         pBarColor,
         pBarBackgroundColor,
         InitialPercent
         );
  }

  return P;
}

VOID
delete_ProgressBar (
  IN ProgressBar  *this
  )
{
  if (NULL != this) {
    this->Base.Dtor ((VOID *)this);
  }
}
