/** @file
Library used to display Device States set by DeviceStateLib

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEVICE_DISPLAY_STATE_LIB_H__
#define __DEVICE_DISPLAY_STATE_LIB_H__

/**
Function to Display all Active Device States

@param WidthInPixels     - Number of Columns in FrameBuffer
@param HeightInPixels    - Number of Rows in FrameBuffer
**/
VOID
EFIAPI
DisplayDeviceState (
  IN  INT32  WidthInPixels,
  IN  INT32  HeightInPixels
  );

#endif
