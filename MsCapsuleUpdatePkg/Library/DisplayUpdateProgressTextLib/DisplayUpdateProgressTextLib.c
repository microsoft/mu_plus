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


#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>

#include <Protocol/GraphicsOutput.h>

//Control Style  

UINTN mPreviousProgress = 100;  //set to 100 so it is reset on first call

EFI_GRAPHICS_OUTPUT_BLT_PIXEL mDefaultColor = { 0xFF, 0xFF, 0xFF, 0xFF }; //white


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
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Color;

  Completion &= 0xFF;  //only the lower 8bits matter for percentage.  

  if (Completion == mPreviousProgress)
  {
    return EFI_SUCCESS;
  }

  //on first call of each progress session we do special init
  if (Completion < mPreviousProgress)
  {
    if (mPreviousProgress == 100)
    {
      Color.Raw = ColorVal;
      DEBUG((DEBUG_VERBOSE, "Color is 0x%X\n", Color.Raw));

      if (Color.Raw == 0x0)
      {
        Color.Pixel = mDefaultColor;
      }

      mPreviousProgress = 0;  //clear previous
    }
    else
    {
      DEBUG((DEBUG_ERROR, "Completion (%d) should not be lesss than Previous (%d)!!!\n", Completion, mPreviousProgress));
      ASSERT(FALSE);
      return EFI_SUCCESS;  //keep the process moving on production units.  
    }
  }

  Print(L"Update Progress - %d%%\n", Completion);

  mPreviousProgress = Completion;

  return EFI_SUCCESS;
}