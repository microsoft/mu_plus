/** @file

  Implements a simple progress bar control for showing incremental progress.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UIT_PROGRESSBAR_H_
#define _UIT_PROGRESSBAR_H_

//
typedef struct _ProgressBarDisplayInfo {
  SWM_RECT    ProgressBarBoundsLimit;                             // Absolute maximum progress bar bounds allowed.
  SWM_RECT    ProgressBarBoundsCurrent;                           // Actual progress bar bounds required for the current configuration.
} ProgressBarDisplayInfo;

//////////////////////////////////////////////////////////////////////////////
// ProgressBar Class Definition
//
typedef struct _ProgressBar {
  // *** Base Class ***
  //
  ControlBase                      Base;

  // *** Member variables ***
  //
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    m_BarColor;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL    m_BarBackgroundColor;
  UINT8                            m_BarPercent;

  ProgressBarDisplayInfo           *m_pProgressBar;

  // *** Functions ***
  //
  VOID
  (*Ctor)(
    IN struct _ProgressBar           *this,
    IN SWM_RECT                      *ProgressBarBox,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBarColor,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBarBackgroundColor,
    IN UINT8                         InitialPercent
    );

  EFI_STATUS
  (*UpdateProgressPercent)(
    IN struct _ProgressBar *this,
    IN UINT8               NewPercent
    );
} ProgressBar;

//////////////////////////////////////////////////////////////////////////////
// Public
//
ProgressBar *
new_ProgressBar (
  IN UINT32                        OrigX,
  IN UINT32                        OrigY,
  IN UINT32                        ProgressBarWidth,
  IN UINT32                        ProgressBarHeight,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBarColor,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pBarBackgroundColor,
  IN UINT8                         BarPercent
  );

VOID
delete_ProgressBar (
  IN ProgressBar *this
  );

#endif // _UIT_PROGRESSBAR_H_.
