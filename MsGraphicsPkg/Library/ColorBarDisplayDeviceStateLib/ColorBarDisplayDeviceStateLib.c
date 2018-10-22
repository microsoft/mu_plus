/** @file
Library used to display Device State on screen using color bars.

See DeviceStateLib for code that is related to getting and setting the device state.

Copyright (c) 2018, Microsoft Corporation

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

#include <Base.h>
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DeviceStateLib.h>
#include <Library/DisplayDeviceStateLib.h>
#include <Library/DebugLib.h>
#include <Protocol/GraphicsOutput.h>  //structure defs
#include <Library/MemoryAllocationLib.h>
#include <UiPrimitiveSupport.h>
#include <Library/UiRectangleLib.h>

//Secure boot unlock icon
#include "Resources/UnlockBitmap.h"

//Height of single banner is percent of total screen height
#define HEIGHT_OF_SINGLE_BANNER (8)

//These widths are percent of banner height
#define STRIPE_WIDTH (20)
#define CHECKERBOARD_WIDTH (20)

//
//Colors from RainBow wiki
//
#define COLOR_RED       (0xFFfb0200)
#define COLOR_ORANGE    (0xFFfd6802)
#define COLOR_YELLOW    (0xFFffef00)
#define COLOR_GREEN     (0xFF00ff03)
#define COLOR_BLUE      (0xFF0094fb)
#define COLOR_INDIGO    (0xFF4500f7)
#define COLOR_VIOLET    (0xFF9c00ff)

#define COLOR_GREY      (0xFFC0C0C0)
#define COLOR_BLACK     (0xFF000000)
#define COLOR_WHITE     (0xFFFFFFFF)

//
// List of supported notifications.
// In order you want them displayed.
//
DEVICE_STATE mSupportedNotifications[] = {
  (DEVICE_STATE)DEVICE_STATE_SECUREBOOT_OFF,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_0,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_1,
  (DEVICE_STATE)DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED,
  (DEVICE_STATE)DEVICE_STATE_SOURCE_DEBUG_ENABLED,
  (DEVICE_STATE)DEVICE_STATE_UNDEFINED,
  (DEVICE_STATE)DEVICE_STATE_MANUFACTURING_MODE,

  (DEVICE_STATE)DEVICE_STATE_MAX  //this needs to be the last one
};


/**
Helper debug method to print out what notifications are set
**/
VOID
PrintValues(DEVICE_STATE Notifications)
{
  DEBUG((DEBUG_INFO, "On Screen Notifications: \n"));
  if (Notifications & DEVICE_STATE_SECUREBOOT_OFF)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_SECUREBOOT_OFF\n"));
  }

  if (Notifications & DEVICE_STATE_PLATFORM_MODE_0)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_PLATFORM_MODE_0\n"));
  }

  if (Notifications & DEVICE_STATE_PLATFORM_MODE_1)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_PLATFORM_MODE_1\n"));
  }

  if (Notifications & DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_DEVELOPMENT_BUILD_ENABLED\n"));
  }

  if (Notifications & DEVICE_STATE_SOURCE_DEBUG_ENABLED)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_SOURCE_DEBUG_ENABLED\n"));
  }

  if (Notifications & DEVICE_STATE_MANUFACTURING_MODE)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_MANUFACTURING_MODE\n"));
  }

  if (Notifications & DEVICE_STATE_MAX)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_MAX\n"));
  }
}


/**
Function to Display all Active Device States

@param FrameBufferBase   - Address of point 0,0 in the frame buffer
@param PixelsPerScanLine - Number of pixels per scan line.
@param WidthInPixels     - Number of Columns in FrameBuffer
@param HeightInPixels    - Number of Rows in FrameBuffer
**/
VOID
EFIAPI
DisplayDeviceState(
IN  UINT8* FrameBufferBase,
IN  INT32  PixelsPerScanLine,
IN  INT32  WidthInPixels,
IN  INT32  HeightInPixels
)
{
  DEVICE_STATE Notifications = 0;
  DEVICE_STATE* SupportedNotification = mSupportedNotifications;
  POINT ul;
  INT32 SingleBannerHeight = ((HeightInPixels * HEIGHT_OF_SINGLE_BANNER) / 100);


  Notifications = GetDeviceState();
  PrintValues(Notifications);

  ul.X = 0;
  ul.Y = 0;

  while ((*SupportedNotification != DEVICE_STATE_MAX) && (Notifications > 0))
  {
    if (Notifications & *SupportedNotification)  //loop thru array of supported notifications
    {
      UI_STYLE_INFO si;
      si.Border.BorderWidth = 0;
      si.IconInfo.Width = 0;
      si.IconInfo.Height = 0;
      si.IconInfo.PixelData = NULL;

      if (*SupportedNotification & DEVICE_STATE_SECUREBOOT_OFF)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_RED;
        si.IconInfo.Width = SECURE_BOOT_UNLOCKED_BITMAP_WIDTH;
        si.IconInfo.Height = SECURE_BOOT_UNLOCKED_BITMAP_HEIGHT;
        si.IconInfo.Placement = MIDDLE_CENTER;
        si.IconInfo.PixelData = (UINT32*) &SecureBootUnlockedBitmap[0];

      }
      else if (*SupportedNotification & DEVICE_STATE_PLATFORM_MODE_0)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_ORANGE;
      }
      else if (*SupportedNotification & DEVICE_STATE_PLATFORM_MODE_1)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_YELLOW;
      }
      else if (*SupportedNotification & DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_GREEN;
      }
      else if (*SupportedNotification & DEVICE_STATE_SOURCE_DEBUG_ENABLED)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_BLUE;
      }
      else if (*SupportedNotification & DEVICE_STATE_UNDEFINED)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_INDIGO;
      }
      else if (*SupportedNotification & DEVICE_STATE_MANUFACTURING_MODE)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_VIOLET;
      }
      else
      {
        //
        // Catch any supported notification that doesn't have draw routine.
        // Generally this would mean developer forgot to update this part of the library to support
        //
        DEBUG((DEBUG_ERROR, "Notification 0x%X does not have code to support drawing.\n", *SupportedNotification));
        SupportedNotification++;
        continue;
      }

      UI_RECTANGLE* rect = new_UI_RECTANGLE(&ul, FrameBufferBase, PixelsPerScanLine, (UINT16)WidthInPixels, SingleBannerHeight, &si);
      DrawRect(rect);
      delete_UI_RECTANGLE(rect);
      ul.Y += SingleBannerHeight;
      Notifications -= *SupportedNotification;  //subtract notification so that we can break early.
    }  //close if notification is supported
    SupportedNotification++;
  }  //close while loop going thru each notification
}