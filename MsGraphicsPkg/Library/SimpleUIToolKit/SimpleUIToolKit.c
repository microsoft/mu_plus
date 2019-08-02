/** @file

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
