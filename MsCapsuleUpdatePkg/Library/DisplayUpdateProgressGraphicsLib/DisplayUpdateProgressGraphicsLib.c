/**

Copyright (c) 2016, Microsoft Corporation

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


#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/BootLogo2.h>

//Control Style  

//In % of Logo height
#define LOGO_BOTTOM_PADDING 20
#define PROGRESS_BLOCK_HEIGHT 10


#define PROGRESS_BAR_BG_COLOR 0xFFd0d0d0


EFI_GRAPHICS_OUTPUT_PROTOCOL* mGop = NULL;
UINTN mPreviousProgress = 100;  //set to 100 so it is reset on first call

//Positions in screen space coordinates
UINTN mStartX = 0;
UINTN mStartY = 0;

//size in pixels
UINTN mBlockWidth = 0;
UINTN mBlockHeight = 0;

EFI_GRAPHICS_OUTPUT_BLT_PIXEL *mBlockBitmap;            //Init on every new progress 100% -> to smaller value
EFI_GRAPHICS_OUTPUT_BLT_PIXEL *mProgressBarBackground;  //Init once 
EFI_GRAPHICS_OUTPUT_BLT_PIXEL mDefaultColor = { 0xFF, 0xFF, 0xFF, 0xFF }; //white

BOOLEAN mGraphicsGood = FALSE;

/*
  Internal function used to find the bounds of the white logo (on black or red background).
  These bounds are then computed to find the block size, 0%, 100%, etc.

*/
VOID
FindDim()
{
  EFI_STATUS Status;

  //Logo space
  INTN LogoX, LogoStartX, LogoEndX;
  INTN LogoY, LogoStartY, LogoEndY;

  UINTN OffsetX, OffsetY;  //Start of logo in screen coordinates.  Use this to convert from logo coordinates to screen coordinates
  UINTN width, height;     //size of logo in pixels

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Logo = NULL;
  EFI_BOOT_LOGO_PROTOCOL2 *BootLogoProt = NULL;  //get bounds of logo so we can minize our screen grab and looking for logo

  //check gop
  if (mGop == NULL)
  {
    DEBUG((DEBUG_ERROR, "No GOP found.  No progress bar support. \n"));
    return;
  }

  //get boot logo protocol so we know where on the screen to grab
  Status = gBS->LocateProtocol(&gEfiBootLogoProtocol2Guid, NULL, (VOID **)&BootLogoProt);
  if ((BootLogoProt == NULL) || (EFI_ERROR(Status)))
  {
    DEBUG((DEBUG_ERROR, "Failed to locate gEfiBootLogoProtocol2Guid.  No Progress bar support. \n", Status));
    return;
  }
  //get logo location and size
  Status = BootLogoProt->GetBootLogo(BootLogoProt, &OffsetX, &OffsetY, &width, &height);
  if (EFI_ERROR(Status))
  {
    DEBUG((DEBUG_ERROR, "Failed to Get Boot Logo Status = %r.  No Progress bar support. \n", Status));
    return;
  }

  //blt that into buffer
  Logo = AllocatePool(height * width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (Logo == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate memory for logo. No progress bar support. \n"));
    return;
  }
  Status = mGop->Blt(mGop, Logo, EfiBltVideoToBltBuffer, OffsetX, OffsetY, 0, 0, width, height, sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)* width);

  //within logo buffer find where the actual logo starts/ends Update the locals
  LogoEndX = 0;
  LogoEndY = 0;

  LogoStartX = width;  //set this up for loop exit condition
  //find left side of logo in logo coordinates
  for (LogoX = 0; LogoX < LogoStartX; LogoX++)
  {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *p = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION*)(Logo + LogoX);
    for (LogoY = 0; LogoY < (INTN)height; LogoY++)
    {
      if ((p->Raw & 0x0000FFFF) != 0x0)
      {
        LogoStartX = LogoX;
        LogoEndX = LogoX;  //for next loop will search from right side back to this column.  
        DEBUG((DEBUG_INFO, "StartX found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, p->Raw));
        break;
      }
      p = p + width;
    }
  }

  //find right side
  for (LogoX = width - 1; LogoX >= LogoEndX; LogoX--)
  {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *p = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION*)(Logo + LogoX);
    for (LogoY = 0; LogoY < (INTN)height; LogoY++)
    {
      if ((p->Raw & 0x0000FFFF) != 0x0)
      {
        LogoEndX = LogoX;
        DEBUG((DEBUG_INFO, "EndX found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, p->Raw));
        break;
      }
      p = p + width;
    }
  }

  //now we have startx and endx
  //lets figure out mBlockWidth and mStartX;

  mBlockWidth = ((LogoEndX - LogoStartX) + 99) / 100;  //make sure we get it all
  //adjust mStartX based on blockwidth so it is centered under logo in Screen coordinates
  mStartX = LogoStartX + OffsetX - (((mBlockWidth * 100) - (LogoEndX - LogoStartX)) / 2);
  DEBUG((DEBUG_INFO, "mBlockWidth set to 0x%X\n", mBlockWidth));
  DEBUG((DEBUG_INFO, "mStartX set to 0x%X\n", mStartX));


  LogoStartY = height; //set this up for loop exit condition
  //now we have to get the height of the logo; 
  for (LogoY = 0; LogoY < LogoStartY; LogoY++)
  {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *p = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION*)(Logo + (width * LogoY));
    for (LogoX = 0; LogoX < (INTN)width; LogoX++)
    {
      //not black or red
      if ((p->Raw & 0x0000FFFF) != 0x0)
      {
        LogoStartY = LogoY;
        LogoEndY = LogoY; //for next loop will search from bottom side back to this row.  
        DEBUG((DEBUG_INFO, "StartY found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, p->Raw));
        break;
      }
      p = p + 1;
    }
  }

  for (LogoY = height - 1; LogoY >= LogoEndY; LogoY--)
  {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION *p = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION*)(Logo + (width * LogoY));
    for (LogoX = 0; LogoX < (INTN)width; LogoX++)
    {
      if ((p->Raw & 0x0000FFFF) != 0x0)
      {
        LogoEndY = LogoY;
        DEBUG((DEBUG_INFO, "EndY found at (%d, %d) Color is: 0x%X \n", LogoX, LogoY, p->Raw));
        break;
      }
      p = p + 1;
    }
  }

  //figure out bottom padding (distance between logo bottom and progress bar)
  mStartY = (((LogoEndY - LogoStartY) *  LOGO_BOTTOM_PADDING) / 100) + LogoEndY + OffsetY;
  //Figure out block height 
  mBlockHeight = (((LogoEndY - LogoStartY) *  PROGRESS_BLOCK_HEIGHT) / 100);

  DEBUG((DEBUG_INFO, "mBlockHeight set to 0x%X\n", mBlockHeight));

  //done with logo
  FreePool(Logo);

  //create your progress bar background  (one time init)
  mProgressBarBackground = AllocatePool(mBlockWidth * 100 * mBlockHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (mProgressBarBackground == NULL)
  {
    DEBUG((DEBUG_ERROR, "Failed to allocate progress bar background\n"));
    return;
  }

  SetMem32(mProgressBarBackground, (mBlockWidth * 100 * mBlockHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)), PROGRESS_BAR_BG_COLOR);

  //Allocate our mBlockBitmap
  mBlockBitmap = AllocatePool(mBlockWidth * mBlockHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  if (mBlockBitmap == NULL)
  {
    FreePool(mProgressBarBackground);
    DEBUG((DEBUG_ERROR, "Failed to allocate block\n"));
    return;
  }

  //should also check screen width and height and make sure we fit. 
  if ((mBlockHeight > height) || (mBlockWidth > width) || (mBlockHeight < 1) || (mBlockWidth < 1))
  {
    DEBUG((DEBUG_ERROR, "CapsuleLib - Progress - Failed to get valid width and height.\n"));
    DEBUG((DEBUG_ERROR, "CapsuleLib - Progress - mBlockHeight: 0x%X  mBlockWidth: 0x%X.\n", mBlockHeight, mBlockWidth));
    FreePool(mProgressBarBackground);
    FreePool(mBlockBitmap);
  }
  else
  {
    mGraphicsGood = TRUE;
  }
}


/**
Function indicate the current completion progress of the firmware
update. Platform may override with its own specific function.

@param  Completion    A value between 0 and 100 indicating the current completion progress of the firmware update.  Zero resets everything.
@param  Color         Color of the progress bar.
                      This color will only be checked on the first call to progress per update.
                      If the color value is 0, the default color will be used.

@retval EFI_SUCESS    Progress displayed successfully.
**/
EFI_STATUS
EFIAPI
DisplayUpdateProgress(
  IN UINTN      Completion,
  IN UINT32     ColorVal
)
{
  EFI_STATUS                          Status;
  UINTN PreX;
  UINTN Index;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Color;

  Completion &= 0xFF;  //only the lower 8bits matter for percentage.  

  if (Completion == mPreviousProgress)
  {
    return EFI_SUCCESS;
  }

  //set gop if not already set.  1 time.
  if (mGop == NULL)
  {
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&mGop);
    if (EFI_ERROR(Status)) {
      mGop = NULL;
      DEBUG((DEBUG_ERROR, "Show Progress Function could not locate GOP.  Status = %r\n", Status));
      return EFI_NOT_READY;
    }

    FindDim();  //run once
  }

  //
  // Make sure we were able to get a valid start, end, and size info (find the Logo)
  //
  if (!mGraphicsGood)
  {
    DEBUG((DEBUG_INFO, "Graphics Not Good.  Not doing any onscreen visual display\n"));
    return EFI_SUCCESS;
  }

  //on first call of each progress session we do special init
  if (Completion < mPreviousProgress)
  {
    if (mPreviousProgress == 100)
    {
      //draw progress bar background
      mGop->Blt(mGop, mProgressBarBackground, EfiBltBufferToVideo, 0, 0, mStartX, mStartY, (mBlockWidth * 100), mBlockHeight, 0);  //no delta as we use who buffer. 

      Color.Raw = ColorVal;
      DEBUG((DEBUG_VERBOSE, "Color is 0x%X\n", Color.Raw));

      if (Color.Raw == 0x0)
      {
        Color.Pixel = mDefaultColor;
      }
      //Update block bitmap with correct color
      SetMem32(mBlockBitmap, (mBlockWidth * mBlockHeight * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL)), Color.Raw);

      mPreviousProgress = 0;  //clear previous
    }
    else
    {
      DEBUG((DEBUG_ERROR, "Completion (%d) should not be lesss than Previous (%d)!!!\n", Completion, mPreviousProgress));
      ASSERT(FALSE);
      return EFI_SUCCESS;  //keep the process moving on production units.  
    }
  }

  PreX = ((mPreviousProgress * mBlockWidth) + mStartX);
  for (Index = 0; Index < (Completion - mPreviousProgress); Index++)
  {
    //
    // Show progress by coloring new area
    //
    mGop->Blt(mGop,
      mBlockBitmap,
      EfiBltBufferToVideo,
      0,
      0,
      PreX,
      mStartY,
      mBlockWidth,
      mBlockHeight,
      0
    );
    PreX += mBlockWidth;
  }

  mPreviousProgress = Completion;

  return EFI_SUCCESS;
}