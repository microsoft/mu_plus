/** @file NvidiaSupportDxe.c

    This is a driver for working around a specific defect in the Nvidia Graphics Output Protocol.

    For whatever reason, the Nvidia device is very slow to read from the frame buffer.  To get GOP
    performance, the Nvidia GOP caches the frame buffer.  When Bitlocker PIN prompt writes to the
    display using frame buffer writes, the Nvidia GOP doesn't know this happened. Later, when the
    OSK or mouse pointer cod uses GOP to capture the current display, the GOP read returns the
    stale cache data.  The effect is the mouse move deposit mouse pointer rectangles of the gray
    UEFI background, and not the blue Bitlocker menu.  The OSK will leave gray rectangles.

    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvidiaSupportDxe.h"

#define GOP_0    0
#define GOP_1    1
#define GOP_2    2
#define GOP_3    3
#define MAX_GOP  4

EFI_STATUS
GopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL,
  IN  UINTN                              Instance
  );

EFI_STATUS
EFIAPI
GopBlt0 (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = GopBlt (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta,
             GOP_0
             );

  return Status;
}

EFI_STATUS
EFIAPI
GopBlt1 (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = GopBlt (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta,
             GOP_1
             );

  return Status;
}

EFI_STATUS
EFIAPI
GopBlt2 (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = GopBlt (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta,
             GOP_2
             );

  return Status;
}

EFI_STATUS
EFIAPI
GopBlt3 (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = GopBlt (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta,
             GOP_3
             );

  return Status;
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL     gBlock1[MOUSE_POINTER_WIDTH_SMALL * MOUSE_POINTER_WIDTH_SMALL];
EFI_GRAPHICS_OUTPUT_BLT_PIXEL     gBlock2[MOUSE_POINTER_WIDTH_SMALL * MOUSE_POINTER_WIDTH_SMALL];
BOOLEAN                           gReadyToBootHasOccurred   = FALSE;
EFI_EVENT                         gGopCallBackEvent         = NULL;
EFI_EVENT                         gReadyToBootEvent         = NULL;
EFI_EVENT                         gTelemetryEvent           = NULL;
EFI_GUID                          *gGopOverrideProtocolGuid = NULL;
VOID                              *gGopRegistration         = NULL;
EFI_GRAPHICS_OUTPUT_PROTOCOL      *gGop[MAX_GOP]            = { NULL };
FB_CONFIGURE_INFO                 *gConfigure[MAX_GOP]      = { NULL };
EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT  gGopOldBlt[MAX_GOP]       = { NULL };
EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT  gGopBlt[MAX_GOP]          = {
  GopBlt0,
  GopBlt1,
  GopBlt2,
  GopBlt3,
};
//
// Debug Telemetry
//
UINTN   gTotalBlt;
UINTN   gBltToBuffer;
UINTN   gFrameBufferToBuffer;
UINT64  gTicksDecidingPath;
UINTN   gDecisions;

UINTN   gGopBigBltToBuffer;
UINT64  gGopTicsReadingBigBlt;
UINTN   gGopBigBltWidthSum;

UINTN   gFblBigBltToBuffer;
UINT64  gFblTicsReadingBigBlt;
UINTN   gFblBigBltWidthSum;

VOID
InitializeConfigure (
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop,
  UINTN                         Instance
  )
{
  UINTN              FrameBufferConfigureSize;
  FB_CONFIGURE_INFO  *FbInfo;
  EFI_STATUS         Status;

  Status = EFI_SUCCESS;

  if (gConfigure[Instance] != NULL) {
    //
    // Make sure the graphics mode is the same as the previous capture of the mode info.
    // If it is different, discard this Frame Buffer Configure element, and get a new one.
    //
    FbInfo = gConfigure[Instance];
    if ((FbInfo->Mode != Gop->Mode->Mode) ||
        (FbInfo->HorizontalResolution != Gop->Mode->Info->HorizontalResolution) ||
        (FbInfo->VerticalResolution != Gop->Mode->Info->VerticalResolution) ||
        (FbInfo->PixelsPerScanLine != Gop->Mode->Info->PixelsPerScanLine))
    {
      FreePool (FbInfo);
      gConfigure[Instance] = NULL;
      DEBUG ((DEBUG_WARN, "%a: Destroying old Frame Buffer Configure\n", __FUNCTION__));
    }
  }

  if (gConfigure[Instance] == NULL) {
    //
    // Capture the mode information.
    //
    FrameBufferConfigureSize = 0;
    Status                   = FrameBufferBltConfigure (
                                 (VOID *)(UINTN)Gop->Mode->FrameBufferBase,
                                 Gop->Mode->Info,
                                 NULL,
                                 &FrameBufferConfigureSize
                                 );

    if (Status == RETURN_BUFFER_TOO_SMALL) {
      FbInfo = AllocatePool (FrameBufferConfigureSize + sizeof (FB_CONFIGURE_INFO));
      if (FbInfo != NULL) {
        FbInfo->Mode                 = Gop->Mode->Mode;
        FbInfo->HorizontalResolution = Gop->Mode->Info->HorizontalResolution;
        FbInfo->VerticalResolution   = Gop->Mode->Info->VerticalResolution;
        FbInfo->PixelsPerScanLine    = Gop->Mode->Info->PixelsPerScanLine;
        Status                       = FrameBufferBltConfigure (
                                         (VOID *)(UINTN)Gop->Mode->FrameBufferBase,
                                         Gop->Mode->Info,
                                         (FRAME_BUFFER_CONFIGURE *)FbInfo->ConfigureBuffer,
                                         &FrameBufferConfigureSize
                                         );

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Error from FrameBufferLibBltConfigure. Code=%r\n", __FUNCTION__, Status));
          FreePool (FbInfo);
          FbInfo = NULL;
        }

        gConfigure[Instance] = FbInfo;
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Unable to allocate memory for Configure Buffer\n", __FUNCTION__));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "%a: Unexpected error from FrameBufferLibBltConfigure. Code=%r\n", __FUNCTION__, Status));
    }
  }

  if (gConfigure[Instance] == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialize FRAME_BUFFER_CONFIGURE\n", __FUNCTION__));
  }
}

/**
  Blt a rectangle of pixels on the graphics screen. Blt stands for BLock Transfer.

  @param  This         Protocol instance pointer.
  @param  BltBuffer    The data to transfer to the graphics screen.
                       Size is at least Width*Height*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL).
  @param  BltOperation The operation to perform when copying BltBuffer on to the graphics screen.
  @param  SourceX      The X coordinate of source for the BltOperation.
  @param  SourceY      The Y coordinate of source for the BltOperation.
  @param  DestinationX The X coordinate of destination for the BltOperation.
  @param  DestinationY The Y coordinate of destination for the BltOperation.
  @param  Width        The width of a rectangle in the blt rectangle in pixels.
  @param  Height       The height of a rectangle in the blt rectangle in pixels.
  @param  Delta        Not used for EfiBltVideoFill or the EfiBltVideoToVideo operation.
                       If a Delta of zero is used, the entire BltBuffer is being operated on.
                       If a sub-rectangle of the BltBuffer is being used then Delta
                       represents the number of bytes in a row of the BltBuffer.
  @param  Instance     Instance where to obtain old pointer.

  @retval EFI_SUCCESS           BltBuffer was drawn to the graphics screen.
  @retval EFI_INVALID_PARAMETER BltOperation is not valid.
  @retval EFI_DEVICE_ERROR      The device had an error and could not complete the request.

**/
EFI_STATUS
GopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer  OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta  OPTIONAL,
  IN  UINTN                              Instance
  )
{
  //
  // Problem statement.  For performance reasons, the NVidia creates a mirror buffer.  All Blt
  // operations xxToVideo are cached in the mirror buffer.  All xxxVideoToxx operations read
  // from the mirror buffer.
  //
  // This causes issues when Bitlocker prompts for the PIN as the OS writes directly to the
  // frame buffer, bypassing the mirror buffer.  When the OSK or Mouse try to capture the
  // current display, they read the last written UEFI screen, and not the current Bitlocker
  // screen.
  //
  // The work around is to read from the frame buffer directly using FrameBufferLib for all
  // mouse pointer reads.  For larger Blts, a sample Blt is read by both the first read has to be from the Framebuffer, but
  // there will be a performance issue if it is used for every read.  So, most of the
  // time.
  //
  UINT64      StartTime;
  UINT64      EndTime;
  EFI_STATUS  Status;
  BOOLEAN     UseFrameBuffer;

  UseFrameBuffer = FALSE;
  gTotalBlt++;

  InitializeConfigure (This, Instance);

  if (gConfigure[Instance] != NULL) {
    if ((gReadyToBootHasOccurred) && (BltOperation == EfiBltVideoToBltBuffer)) {
      if (gTelemetryEvent != NULL) {
        Status = gBS->SetTimer (
                        gTelemetryEvent,
                        TimerRelative,
                        TELEMETRY_DELAY
                        );

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Error %r setting telemetry timer.\n", __FUNCTION__, Status));
        }
      }

      if (Width <= MOUSE_POINTER_WIDTH_MEDIUM) {
        UseFrameBuffer = TRUE;
      } else {
        StartTime = GetPerformanceCounter ();
        Status    = (*gGopOldBlt[Instance])(
  This,
                      (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&gBlock1,
  EfiBltVideoToBltBuffer,
  SourceX,
  SourceY,
  0,
  0,
  MOUSE_POINTER_WIDTH_SMALL,
  MOUSE_POINTER_WIDTH_SMALL,
  0
  );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: Gop Blt Error. Code=%r\n", __FUNCTION__, Status));
        } else {
          Status = FrameBufferBlt (
                     (FRAME_BUFFER_CONFIGURE *)&gConfigure[Instance]->ConfigureBuffer,
                     (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&gBlock2,
                     EfiBltVideoToBltBuffer,
                     SourceX,
                     SourceY,
                     0,
                     0,
                     MOUSE_POINTER_WIDTH_SMALL,
                     MOUSE_POINTER_WIDTH_SMALL,
                     0
                     );
        }

        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "%a: FrameBufferLib Blt Error. Code=%r\n", __FUNCTION__, Status));
        } else {
          if (0 != CompareMem (&gBlock1, &gBlock2, sizeof (gBlock1))) {
            DEBUG ((DEBUG_WARN, "%a: Blt compare fail, using FrameBuffer read\n", __FUNCTION__));
            UseFrameBuffer = TRUE;
          }
        }

        EndTime             = GetPerformanceCounter ();
        gTicksDecidingPath += (EndTime - StartTime);
        gDecisions++;
      }
    }
  }

  if (UseFrameBuffer) {
    if (Width > (MOUSE_POINTER_WIDTH_MEDIUM * 2)) {
      gFblBigBltToBuffer++;
      gFblBigBltWidthSum += Width;
      StartTime           = GetPerformanceCounter ();
    }

    gFrameBufferToBuffer++;
    Status = FrameBufferBlt (
               (FRAME_BUFFER_CONFIGURE *)&gConfigure[Instance]->ConfigureBuffer,
               BltBuffer,
               BltOperation,
               SourceX,
               SourceY,
               DestinationX,
               DestinationY,
               Width,
               Height,
               Delta
               );

    if (Width > (MOUSE_POINTER_WIDTH_MEDIUM * 2)) {
      EndTime                = GetPerformanceCounter ();
      gFblTicsReadingBigBlt += (EndTime - StartTime);
    }
  } else {
    if (Width > (MOUSE_POINTER_WIDTH_MEDIUM * 2)) {
      gGopBigBltToBuffer++;
      gGopBigBltWidthSum += Width;
      StartTime           = GetPerformanceCounter ();
    }

    gBltToBuffer++;
    Status = (*gGopOldBlt[Instance])(
  This,
  BltBuffer,
  BltOperation,
  SourceX,
  SourceY,
  DestinationX,
  DestinationY,
  Width,
  Height,
  Delta
  );

    if (Width > (MOUSE_POINTER_WIDTH_MEDIUM * 2)) {
      EndTime                = GetPerformanceCounter ();
      gGopTicsReadingBigBlt += (EndTime - StartTime);
    }
  }

  return Status;
}

/**
   FormatTime

   Format time in nanoseconds as a reasonable value of nanoseconds (ns), microseconds(us),
   milliseconds(ms), or seconds(S), with 3 digits after the decimal point.  For example,
   "1.499 S" doesn't need ns resolution.

   The value is rounded to the printed range unless Value is really close to MAX_UINT64,
   which would cause an overflow.

   Since TimeMsg is in a static location, it is only valid until the next call to FormatTime.

   @param    Value           Time in nanoseconds

   @retval   TimeMsg         Pointer to a static location that receives the TimeMsg.
                             An error creating the TimeMsg sets TimeMsg to "".

 **/
CHAR8 *
FormatTime (
  UINT64  Value
  )
{
  STATIC  CHAR8  TempString[32];
  STATIC  CHAR8  *Units[] = { "us", "ms", "S" };
  UINTN          i;
  UINT64         Divisor;
  UINT64         Temp;
  EFI_STATUS     Status;

  TempString[0] = '\0';
  if (Value < 1000) {
    AsciiSPrint (TempString, sizeof (TempString), "%lu ns", Value);
  } else {
    if (Value < 1000000) {
      i       = 0;
      Divisor = 1;
    } else if (Value < 1000000000) {
      i       = 1;
      Divisor = 1000;
    } else {
      i       = 2;
      Divisor = 1000000;
    }

    Status = SafeUint64Add (Value, (Divisor / 2) - ((Divisor == 1) ? 0 : 1), &Temp);
    if (!EFI_ERROR (Status)) {
      Value = Temp;  // Only round up if there is no overflow.
    }

    Value /= Divisor;

    AsciiSPrint (TempString, sizeof (TempString), "%lu.%03u %a", Value / 1000, Value % 1000, Units[i]);
  }

  return (CHAR8 *)TempString;
}

/**
    Telemetry Event.  This routine is signaled 5 seconds after any BltToBuffer calls.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none

 **/
VOID
EFIAPI
OnTelemetryNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  DEBUG ((
    DEBUG_WARN,
    "Total Blts = %d, gBltToBuffer = %d, gFrameBufferToBuffer = %d, gDecisions = %d, TimeToDecide = %a\n",
    gTotalBlt,
    gBltToBuffer,
    gFrameBufferToBuffer,
    gDecisions,
    FormatTime ((gDecisions == 0) ? 0 : GetTimeInNanoSecond (gTicksDecidingPath / gDecisions))
    ));

  DEBUG ((
    DEBUG_WARN,
    "   Gop - Big Blts = %d, Avg Blt Size = %d, Avg Time To Read a big Blt = %a\n",
    gGopBigBltToBuffer,
    (gGopBigBltToBuffer == 0) ? 0 : gGopBigBltWidthSum / gGopBigBltToBuffer,
    FormatTime ((gGopBigBltToBuffer == 0) ? 0 : GetTimeInNanoSecond (gGopTicsReadingBigBlt / gGopBigBltToBuffer))
    ));

  DEBUG ((
    DEBUG_WARN,
    "   Fbl - Big Blts = %d, Avg Blt Size = %d, Avg Time To Read a big Blt = %a\n",
    gFblBigBltToBuffer,
    (gFblBigBltToBuffer == 0) ? 0 : gFblBigBltWidthSum / gFblBigBltToBuffer,
    FormatTime ((gFblBigBltToBuffer == 0) ? 0 : GetTimeInNanoSecond (gFblTicsReadingBigBlt / gFblBigBltToBuffer))
    ));
}

/**
    Indicate ReadyToBoot has occurred.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none

 **/
VOID
EFIAPI
OnReadyToBootNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gReadyToBootHasOccurred = TRUE;

  gBS->CloseEvent (Event);
}

/**
    Indicate ReadyToBoot has occurred.

    @param    Event           Not Used.
    @param    Context         Not Used.

   @retval   none

 **/
VOID
EFIAPI
OnGopProtocolInstallNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                         i;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  EFI_STATUS                    Status;

  DEBUG ((DEBUG_WARN, "%a: Entry...\n", __FUNCTION__));

  //
  // Get the next Gop, if any.  Process all that are
  // present, one at a time.
  //
  for ( ; ;) {
    //
    // Get the next Gop.
    //
    Status = gBS->LocateProtocol (
                    gGopOverrideProtocolGuid,
                    gGopRegistration,
                    (VOID **)&Gop
                    );
    //
    // If not found, or any other error, we're done
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    DEBUG ((DEBUG_WARN, "%a: processing Gop at %p\n", __FUNCTION__, Gop));

    for (i = 0; i < MAX_GOP; i++) {
      if (gGop[i] == NULL) {
        //
        // A NULL entry indicates we have not seen this interface before.  Remember
        // this interface, and hook the Blt routine.
        //
        gGop[i]       = Gop;
        gGopOldBlt[i] = Gop->Blt;
        Gop->Blt      = gGopBlt[i];
        DEBUG ((DEBUG_ERROR, "%a: Assigning %p to use Gop%d.\n", __FUNCTION__, Gop, i));
        break;
      } else if (gGop[i] == Gop) {
        // Ignore the same interface if seen multiple times
        DEBUG ((DEBUG_ERROR, "%a: %p already assigned to Gop%d.\n", __FUNCTION__, Gop, i));
        if (Gop->Blt != gGopBlt[i]) {
          DEBUG ((
            DEBUG_ERROR,
            "%a: Gop=%p Gop->Blt=%p, Should be %p for Gop%d.\n",
            __FUNCTION__,
            Gop,
            Gop->Blt,
            gGopBlt[i],
            i
            ));
        }

        break;
      }
    }

    if (i == MAX_GOP) {
      DEBUG ((DEBUG_ERROR, "%a: Too many Gop registrations.\n", __FUNCTION__));
      ASSERT (i < MAX_GOP);
    }
  }
}

/**
   ProcessGopRegistration

   Process registration of Gop protocol.

   @param    VOID

   @retval   EFI_SUCCESS     Registration successful
             error           Unsuccessful at registering for Gop protocol notifications

 **/
EFI_STATUS
ProcessGopRegistration (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Always register for file system notifications.  They may arrive at any time.
  //
  DEBUG ((DEBUG_WARN, "Registering for file systems notifications\n"));
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnGopProtocolInstallNotification,
                  NULL,
                  &gGopCallBackEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create callback event (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  Status = gBS->RegisterProtocolNotify (
                  gGopOverrideProtocolGuid,
                  gGopCallBackEvent,
                  &gGopRegistration
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to register for file system notifications (%r)\n", __FUNCTION__, Status));
    gBS->CloseEvent (gGopRegistration);
    goto Cleanup;
  }

  //
  // Process any existing driver GOP protocols that were present before the registration.
  //
  OnGopProtocolInstallNotification (gGopCallBackEvent, NULL);

Cleanup:
  return Status;
}

/**
    ProcessReadyToBootRegistration

    This function creates a group event handler for ReadyToBoot to start
    monitoring log file to media/


    @param    VOID

    @retval   EFI_SUCCESS     Registration successful
              error           Unsuccessful at registering for Ready To Boot notifications

  **/
EFI_STATUS
ProcessReadyToBootRegistration (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Register notify function for writing the log files.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnReadyToBootNotification,
                  gImageHandle,
                  &gEfiEventReadyToBootGuid,
                  &gReadyToBootEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for ReadyToBoot. Code = %r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
   ProcessTelemetryTimer

   Create a timer to put out debug messages during an idle time to not
   slow down the graphics operations.

   @param    VOID

   @retval   EFI_SUCCESS     Timer event created
             error           Unsuccessful at creating the timer event

 **/
EFI_STATUS
ProcessTelemetryTimer (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnTelemetryNotification,
                  NULL,
                  &gTelemetryEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Error %r creating telemetry timer\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
    Main entry point for this driver.

    @param    ImageHandle     Image handle of this driver.
    @param    SystemTable     Pointer to the system table.

    @retval   EFI_STATUS      Initialization was successful.
    @retval   Others          The operation failed.

**/
EFI_STATUS
EFIAPI
NvidiaSupportDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Surface devices use vendor GOP drivers that produce a private GUID when on the Custom boot
  // path. However, since the SinpleWindowManager isn't present on the Certified Path, the
  // driver will publish the standard GOP protocol.  At least, in theory.  The Nvidia GOP
  // chooses to always publish both with the same interface address.  Look for the MsGopOverride
  // protocol, and the correct protocol will be intercepted.
  //
  gGopOverrideProtocolGuid = PcdGetPtr (PcdMsGopOverrideProtocolGuid);

  //
  // NOTE:  DEBUG_WARN is used as DEBUG_INFO as FrameBufferBltLib produces a lot of
  //       DEBUG_INFO spew. DEBUG_INFO is turned off when building this module.
  //
  DEBUG ((
    DEBUG_WARN,
    "%a: entered. Registering for %g\n",
    __FUNCTION__,
    gGopOverrideProtocolGuid
    ));

  //
  // Step 1 - Register for ReadyToBoot
  //
  Status = ProcessReadyToBootRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 2 - Register for Gop Protocols
  //
  Status = ProcessGopRegistration ();
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Step 3 - Register for Debug Telemetry Events.
  //
  ProcessTelemetryTimer ();

 #if 0
  // Unit test for FormatTime routine
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 977LL, FormatTime (977LL), "977 ns"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1977LL, FormatTime (1977LL), "1.977 us"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1000977LL, FormatTime (1000977LL), "1.001 ms"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1999977LL, FormatTime (1999977LL), "2.000 ms"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1000999977LL, FormatTime (1000999977LL), "1.001 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1999999977LL, FormatTime (1999999977LL), "2.000 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1000999999977LL, FormatTime (1000999999977LL), "1001.000 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1999999999977LL, FormatTime (1999999999977LL), "2000.000 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1000999888666977LL, FormatTime (1000999888666977LL), "1000999.889 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1999999888666977LL, FormatTime (1999999888666977LL), "1999999.889 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1000555999888666977LL, FormatTime (1000555999888666977LL), "1000555999.889 S"));
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", 1999555999888666977LL, FormatTime (1999555999888666977LL), "1999555999.889 S"));
  // MAX_UINTN == 18446744073709551615
  DEBUG ((DEBUG_ERROR, "Test1: %lu, = %a s/b %a\n", MAX_UINTN, FormatTime (MAX_UINTN), "18446744073.709 S")); // Cannot round up
 #endif

Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Leaving, code = %r\n", __FUNCTION__, Status));

    if (gGopCallBackEvent != NULL) {
      gBS->CloseEvent (gGopCallBackEvent);
    }

    if (gReadyToBootEvent != NULL) {
      gBS->CloseEvent (gReadyToBootEvent);
      gReadyToBootHasOccurred = FALSE;
    }

    // All new activity is now stopped.  But, it is possible that the underlying Gop protocol
    // has been freed, and we would get an exception if we restored the Blt pointer.  So do two
    // things:
    //
    // 1. Leave the protocol alone.
    // 2. Return EFI_SUCCESS to leave his driver installed.
    //
  } else {
    DEBUG ((DEBUG_WARN, "%a: Leaving, code = %r\n", __FUNCTION__, Status));
  }

  // Always return EFI_SUCCESS.  This means any partial registration of functions
  // will still exist, reducing the complexity of the uninstall process after a partial
  // install.

  return EFI_SUCCESS;
}
