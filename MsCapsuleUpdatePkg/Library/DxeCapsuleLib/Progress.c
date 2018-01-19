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


#include "DxeCapsuleLibInternal.h"
#include <Library/DisplayUpdateProgressLib.h>

/**
Function indicate the current completion progress of the firmware
update. Platform may override with own specific progress function.

@param  Completion    A value between 0 and 100 indicating the current completion progress of the firmware update.  Zero resets everything.

                      *****Unique to this implemention*****
                      The percentage is the lowest 8 bits.
                      The next 24 bits is the color to use.  This color will only be checked on the first call to progress per update.
                      If the color value is 0 the default color will be used.

@retval EFI_SUCESS    Input capsule is a correct FMP capsule.
**/
EFI_STATUS
EFIAPI
Update_Image_Progress(
  IN UINTN Completion
)
{
  EFI_STATUS                     Status;
  UINT32                         Color;

  Color = (UINT32)((Completion >> 8) & 0xFFFFFF);
  Completion &= 0xFF;  //only the lower 8bits matter for percentage.  

  DEBUG((DEBUG_INFO, "Update Progress - %d%%\n", Completion));

  if (Completion > 100) {
    return EFI_INVALID_PARAMETER;
  }

  //PET WATCHDOG
  //cancel watchdog
  gBS->SetWatchdogTimer(0x0000, 0x0000, 0x0000, NULL);
  if (Completion != 100)
  {
    //start watchdog
    gBS->SetWatchdogTimer((UINTN)PcdGet8(PcdCapsuleUpdateWatchdogTimeInSeconds), 0x0000, 0x00, NULL);
  }

  Status = DisplayUpdateProgress(Completion, Color);

  return Status;
}