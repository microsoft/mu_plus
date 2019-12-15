/** @file
Library used to display Device State on screen using color bars.

See DeviceStateLib for code that is related to getting and setting the device state.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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


//
//Colors
//
#define COLOR_RED       (0xFFfb0200)
#define COLOR_ORANGE    (0xFFfd6802)
#define COLOR_YELLOW    (0xFFffef00)
#define COLOR_GREEN     (0xFF00ff03)
#define COLOR_BLUE      (0xFF0094fb)
#define COLOR_INDIGO    (0xFF4500f7)
#define COLOR_VIOLET    (0xFF9c00ff)
#define COLOR_BROWN     (0xFF654321)

#define COLOR_GREY      (0xFFC0C0C0)
#define COLOR_DARK_GREY (0xFF404040)
#define COLOR_BLACK     (0xFF000000)
#define COLOR_WHITE     (0xFFFFFFFF)

//
// Dimensions
//
// This is width in pixels
#define FORWARD_STRIPE_WIDTH  (50)

//
// List of supported notifications.
// In order you want them displayed.
//
DEVICE_STATE mSupportedNotifications[] = {
  (DEVICE_STATE)DEVICE_STATE_SECUREBOOT_OFF,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_0,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_1,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_2,
  (DEVICE_STATE)DEVICE_STATE_PLATFORM_MODE_3,
  (DEVICE_STATE)DEVICE_STATE_DEVELOPMENT_BUILD_ENABLED,
  (DEVICE_STATE)DEVICE_STATE_SOURCE_DEBUG_ENABLED,
  (DEVICE_STATE)DEVICE_STATE_MANUFACTURING_MODE,
  (DEVICE_STATE)DEVICE_STATE_UNIT_TEST_MODE,

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

  if (Notifications & DEVICE_STATE_PLATFORM_MODE_2)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_PLATFORM_MODE_2\n"));
  }

  if (Notifications & DEVICE_STATE_PLATFORM_MODE_3)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_PLATFORM_MODE_3\n"));
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

  if (Notifications & DEVICE_STATE_UNIT_TEST_MODE)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_UNIT_TEST_MODE\n"));
  }

  if (Notifications & DEVICE_STATE_MAX)
  {
    DEBUG((DEBUG_INFO, "\tDEVICE_STATE_MAX\n"));
  }
}


/**
Given an array of blit data find the largest that fits the banner height

This is used to identify the correct icon for various screen resolutions.

@param Style         - UI_STYLE_INFO that will be modified if icon found
@param BannerHeight  - Height of a single banner
@param BannerWidth   - Width of a single banner
@param BlitArray     - array of struct BITMAPDATA holding blit data
@param ArrayLength   - number of items in BlitArray
@param IconPlacement - UI placement type for icon if found

@return Style Info struct will be updated if valid icon is found
**/
VOID
EFIAPI
PopulateIconData(
IN OUT   UI_STYLE_INFO         *Style,
IN       INT32                 BannerHeight,
IN       INT32                 BannerWidth,
IN       BITMAPDATA*           BlitArray[],
IN       UINT32                ArrayLength,
IN       UI_PLACEMENT          IconPlacement
)
{
  if(Style == NULL) {
    ASSERT(Style != NULL);
    return;
  }

  for(UINT32 index = 0; index < ArrayLength; index++) {
    DEBUG((DEBUG_VERBOSE, "Checking icon of size %u x %u to see if it fits\n", BlitArray[index]->Height, BlitArray[index]->Width));
    if((BlitArray[index]->Height <= BannerHeight) && (BlitArray[index]->Width <= BannerWidth)){
      DEBUG((DEBUG_VERBOSE, "Found fitting icon\n"));
      Style->IconInfo.Width = BlitArray[index]->Width;
      Style->IconInfo.Height = BlitArray[index]->Height;
      Style->IconInfo.Placement = IconPlacement;
      Style->IconInfo.PixelData = (UINT32*) &BlitArray[index]->BlitData[0];
      break;
    }
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
        if(sizeof(mUnlockBlitArray) > 0) {
          PopulateIconData(&si, SingleBannerHeight, WidthInPixels, mUnlockBlitArray, ARRAY_SIZE(mUnlockBlitArray), MIDDLE_CENTER);
        }
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
      else if (*SupportedNotification & DEVICE_STATE_PLATFORM_MODE_2)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_INDIGO;
      }
      else if (*SupportedNotification & DEVICE_STATE_PLATFORM_MODE_3)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_BROWN;
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
      else if (*SupportedNotification & DEVICE_STATE_MANUFACTURING_MODE)
      {
        si.FillType = FILL_SOLID;
        si.FillTypeInfo.SolidFill.FillColor = COLOR_VIOLET;
      }
      else if (*SupportedNotification & DEVICE_STATE_UNIT_TEST_MODE)
      {
        si.FillType = FILL_FORWARD_STRIPE;
        si.FillTypeInfo.StripeFill.Color1 = COLOR_DARK_GREY;
        si.FillTypeInfo.StripeFill.Color2 = COLOR_YELLOW;
        si.FillTypeInfo.StripeFill.StripeSize = FORWARD_STRIPE_WIDTH;
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