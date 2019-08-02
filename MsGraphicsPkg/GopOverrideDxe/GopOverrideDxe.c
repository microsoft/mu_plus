/** @file

   Graphics (GOP) Override implementation

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>


//
// ****** Global variables ******
//

EFI_GUID                            *mMsGopOverrideProtocolGuid;
//EFI_GRAPHICS_OUTPUT_PROTOCOL        *mGop;
EFI_EVENT                           mGopRegisterEvent;
VOID                                *mGopRegistration;


/**
  GOP registration notification callback

  @param[in] Event      Event that signalled the callback.
  @param[in] Context    Pointer to an optional event contxt.

  @retval None.

**/
VOID
EFIAPI
GopRegisteredCallback (IN  EFI_EVENT    Event,
                       IN  VOID         *Context)
{
    EFI_STATUS                    Status = EFI_SUCCESS;
    EFI_HANDLE                    *Handles;
    UINTN                         HandleCount = 0;
    EFI_GRAPHICS_OUTPUT_PROTOCOL  *pGop;

    //
    // Find all the handles on which Graphics Output Protocol is installed (should be exactly one handle).
    //
    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
        );
    if (EFI_ERROR(Status) || HandleCount != 1) {
        DEBUG((DEBUG_ERROR,"ERROR [GOP]: Unable to locate one %g handle - code=%r - HandleCount=%d\n", &gEfiGraphicsOutputProtocolGuid, Status, HandleCount));
        goto Exit;
    }

    //
    // Get Graphics Output Protocol interface on this handle.
    //
    Status = gBS->HandleProtocol (
                    Handles[0],
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID **)&pGop);

    if (EFI_ERROR (Status)) {
        DEBUG((DEBUG_ERROR,"ERROR [GOP]: Unable to get %g protocol - code=%r\n", &gEfiGraphicsOutputProtocolGuid, Status));
        goto Exit;
    }

    //
    // Uninstall Graphics Output Protocol on this handle.
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (Handles[0],
                                                       &gEfiGraphicsOutputProtocolGuid,
                                                       (VOID *)pGop,
                                                       NULL
                                                      );
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"ERROR [GOP]: Unable to uninstall %g protocol - code=%r\n", &gEfiGraphicsOutputProtocolGuid, Status));
        goto Exit;
    }


    //
    // Now, install Graphics Output Override Protocol on this handle.
    //
    Status = gBS->InstallProtocolInterface (&Handles[0],
                                            mMsGopOverrideProtocolGuid,
                                            EFI_NATIVE_INTERFACE,
                                            (VOID *)pGop );
    if (EFI_ERROR (Status))
    {
        DEBUG((DEBUG_ERROR,"ERROR [GOP]: Unable to install %g protocol - code=%r\n", mMsGopOverrideProtocolGuid, Status));
        goto Exit;
    }

    //
    // On success, close the Graphics Output Protocol registration notification event.
    //
    if (mGopRegisterEvent != NULL)
    {
        Status = gBS->CloseEvent (mGopRegisterEvent);
        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR,"ERROR [GOP]: Unable to close GOP Override event - code=%r\n", Status));
            goto Exit;
        }
    }

Exit:
    if (Handles != NULL) {
        FreePool(Handles);
    }

    DEBUG((DEBUG_INFO, "INFO [GOP]: GopRegisteredCallback exit - code=%r\n", Status));

    return;
}


/**
  Main entry point for this driver.

  @param[in] ImageHandle    Image handle this driver.
  @param[in] SystemTable    Pointer to SystemTable.

  @retval EFI_SUCCESS           This function always complete successfully.
  @retval EFI_OUT_OF_RESOURCES  Insufficient resources to initialize.

**/
EFI_STATUS
EFIAPI
DriverInit (IN EFI_HANDLE         ImageHandle,
            IN EFI_SYSTEM_TABLE  *SystemTable)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    EFI_HANDLE     *Handles;
    UINTN           HandleCount = 0;

    mMsGopOverrideProtocolGuid = PcdGetPtr(PcdMsGopOverrideProtocolGuid);

    //
    // Find all the handles on which Graphics Output Protocol is installed (should be exactly one handle).
    //
    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
        );
    if (!EFI_ERROR(Status) && HandleCount == 1) {
        DEBUG((DEBUG_INFO, "[GOP Override]: 1 GOP handle located, not registering an event\n"));
        GopRegisteredCallback(NULL, NULL);
    } else {

        
        //
        // Graphics Output Protocol isn't availble now. Register for Graphics Output Protocol registration notifications.
        //
        Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL,
                                   TPL_NOTIFY,
                                   GopRegisteredCallback,
                                   NULL,
                                   &mGopRegisterEvent
                                  );

        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR, "ERROR [GOP]: Failed to create GOP registration event (%r).\r\n", Status));
            goto Exit;
        }

        Status = gBS->RegisterProtocolNotify (&gEfiGraphicsOutputProtocolGuid,
                                              mGopRegisterEvent,
                                              &mGopRegistration
                                             );

        if (EFI_ERROR (Status))
        {
            DEBUG((DEBUG_ERROR, "ERROR [GOP]: Failed to register for GOP registration notifications (%r).\r\n", Status));
            goto Exit;
        }
    }
Exit:
    DEBUG((DEBUG_INFO, "INFO [GOP]: DriverInit exit - code=%r\n", Status));

    return Status;
}


/**
  Driver Clean-Up

  @retval EFI_SUCCESS       This function always complete successfully.

**/
static EFI_STATUS
EFIAPI
DriverCleanUp (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;

    //
    // Close the Graphics Output Protocol registration notification event.
    //
    if (mGopRegisterEvent != NULL)
    {
        Status = gBS->CloseEvent (mGopRegisterEvent);
    }

    return Status;
}


/**
  Driver unload handler.

  @param[in] ImageHandle    Image handle this driver.

  @retval EFI_SUCCESS       This function always complete successfully.

**/
EFI_STATUS
EFIAPI
DriverUnload (IN EFI_HANDLE  ImageHandle)
{
    EFI_STATUS    Status = EFI_SUCCESS;

    Status = DriverCleanUp(ImageHandle);

    return Status;
}
