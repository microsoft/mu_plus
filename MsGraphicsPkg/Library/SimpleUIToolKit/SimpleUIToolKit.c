/** @file

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

EFI_HII_FONT_PROTOCOL               *mUITFont;
EFI_GRAPHICS_OUTPUT_PROTOCOL        *mUITGop;
MS_SIMPLE_WINDOW_MANAGER_PROTOCOL   *mUITSWM;
EFI_HANDLE                          mClientImageHandle;


EFI_STATUS
InitializeUIToolKit (IN EFI_HANDLE ImageHandle)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    DEBUG((DEBUG_INFO,"[SUIT] Initializing UI Toolkit for %g\n", &gEfiCallerIdGuid));

    // Save the client's image handle for later.
    //
    mClientImageHandle = ImageHandle;

    // Determine if the Font Protocol is available.
    //
    Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid,
                                  NULL,
                                  (VOID **)&mUITFont
                                 );

    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
    {
        mUITFont    = NULL;
        Status      = EFI_UNSUPPORTED;
        DEBUG((DEBUG_ERROR, "INFO [SUIT]: Failed to find Font protocol (%r).\n", Status));
        goto Exit;
    }

    // Determine if the GOP is available.
    //
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid,
                                  NULL,
                                  (VOID**)&mUITGop
                                 );

    if (EFI_ERROR (Status))
    {
        mUITGop     = NULL;
        Status      = EFI_UNSUPPORTED;
        DEBUG((DEBUG_ERROR, "INFO [SUIT]: Failed to find GOP (%r).\n", Status));
        goto Exit;
    }

    // Determine if the SWM is available.
    //
    Status = gBS->LocateProtocol (&gMsSWMProtocolGuid,
                                  NULL,
                                  (VOID **)&mUITSWM
                                 );

    if (EFI_ERROR (Status))
    {
        mUITSWM     = NULL;
        Status      = EFI_UNSUPPORTED;
        DEBUG((DEBUG_ERROR, "INFO [SUIT]: Failed to find SWM (%r).\n", Status));
        goto Exit;
    }

Exit:

    return Status;
}
