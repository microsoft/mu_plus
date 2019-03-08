/** @file
This BootGraphicsLib  is only intended to be used by BDS to draw
and the main boot graphics to the screen.

Implementation borrows from Edk2 BootLogoLib

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2018, Microsoft Corporation<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/BootLogo2.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BootGraphicsProviderLib.h>
#include <Library/DisplayDeviceStateLib.h>
#include <Library/BmpSupportLib.h>

#define MS_MAX_HEIGHT_PERCENTAGE 40 //40%
#define MS_MAX_WIDTH_PERCENTAGE 40  //40%

EFI_STATUS
EFIAPI
DisplayBootGraphic(
    BOOT_GRAPHIC Graphic)
{
  EFI_STATUS Status;
  UINTN Height;
  UINTN Width;
  UINT32 SizeOfX;
  UINT32 SizeOfY;
  INTN DestX;
  INTN DestY;
  UINT8 *ImageData;
  UINTN ImageSize;
  UINT32 Color;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINTN BltSize;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EDKII_BOOT_LOGO2_PROTOCOL *BootLogo2;
  BOOLEAN IsLandscape = FALSE;
  UINT8 SkipCounter;

  // Initialize pointers to prevent CleanUp failure
  ImageData = NULL;
  Blt = NULL;

  //
  // Try to open GOP first
  //
  Status = gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a - Failed to find GOP on ConsoleOutHandle. %r\n", __FUNCTION__, Status));
    //failed on console out.  Try globally within system
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&GraphicsOutput);
  }

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a - Failed to find GOP globally. %r\n", __FUNCTION__, Status));
    goto CleanUp;
  }

  //
  // Try to open Boot Logo 2 Protocol.
  //
  Status = gBS->LocateProtocol (&gEdkiiBootLogo2ProtocolGuid, NULL, (VOID **) &BootLogo2);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "%a - Failed to find BootLogo2 Protocol. %r\n", __FUNCTION__, Status));
    BootLogo2 = NULL;
  }

  //
  // Erase Cursor from screen
  //
  if (gST->ConOut != NULL) {
    gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  } else {
    DEBUG((DEBUG_WARN, "%a - ConOut is NULL, will not disable cursor\n", __FUNCTION__));
  }

  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;

  Blt = NULL;
  DestX = 0;
  DestY = 0;

  ///
  /// Print Mode information received from Graphics Output Protocol
  ///
  DEBUG((DEBUG_INFO, "MaxMode:0x%x \n", GraphicsOutput->Mode->MaxMode));
  DEBUG((DEBUG_INFO, "Mode:0x%x \n", GraphicsOutput->Mode->Mode));
  DEBUG((DEBUG_INFO, "SizeOfInfo:0x%x \n", GraphicsOutput->Mode->SizeOfInfo));
  DEBUG((DEBUG_INFO, "FrameBufferBase:0x%x \n", GraphicsOutput->Mode->FrameBufferBase));
  DEBUG((DEBUG_INFO, "FrameBufferSize:0x%x \n", GraphicsOutput->Mode->FrameBufferSize));
  DEBUG((DEBUG_INFO, "Version:0x%x \n", GraphicsOutput->Mode->Info->Version));
  DEBUG((DEBUG_INFO, "HorizontalResolution:0x%x \n", GraphicsOutput->Mode->Info->HorizontalResolution));
  DEBUG((DEBUG_INFO, "VerticalResolution:0x%x \n", GraphicsOutput->Mode->Info->VerticalResolution));
  DEBUG((DEBUG_INFO, "PixelFormat:0x%x \n", GraphicsOutput->Mode->Info->PixelFormat));
  DEBUG((DEBUG_INFO, "PixelsPerScanLine:0x%x \n", GraphicsOutput->Mode->Info->PixelsPerScanLine));

  //allow for custom bg color
  Color = GetBackgroundColor();

  // Color background when counter BackgroundColoringSkipCounter reaches 0

  SkipCounter = PcdGet8(PcdPostBackgroundColoringSkipCount);
  // Color background only when counter reaches 0;
  if (SkipCounter == 0) {
    Blt = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(&Color); // only pixel (0,0) is used for EfiBltVideoFill
    Status = GraphicsOutput->Blt(GraphicsOutput, Blt, EfiBltVideoFill,
                                 0, 0, 0, 0, SizeOfX, SizeOfY, 0);
    DEBUG((DEBUG_INFO, "Coloring Background to color 0x%x. Status: %r \n", Color, Status));
    Blt = NULL;
  }

  //decrement counter if not zero
  if (SkipCounter > 0) {
    SkipCounter--;
    Status = PcdSet8S (PcdPostBackgroundColoringSkipCount, SkipCounter);
  }

  //Draw our device state
  DisplayDeviceState(
        (UINT8*)((UINTN)GraphicsOutput->Mode->FrameBufferBase),
        GraphicsOutput->Mode->Info->PixelsPerScanLine,
        (INT32)SizeOfX,
        (INT32)SizeOfY
        );

  Status = GetBootGraphic(Graphic, &ImageSize, &ImageData);
  if(EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "GetPlatformBootGraphic Status: %r\n", Status));
    goto CleanUp;
  }

  //
  // Convert Bmp To Blt Buffer
  //
  Status = TranslateBmpToGopBlt(ImageData, ImageSize, &Blt, &BltSize, &Height, &Width);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to TranslateBmpToGopBlt In Logo.c %r\n", Status));
    goto CleanUp;
  }

  if (SizeOfX >= SizeOfY) {
    DEBUG((DEBUG_VERBOSE, "Landscape mode detected.\n"));
    IsLandscape = TRUE;
  }

  // If system logo it must meet size requirements.
  if(Graphic == BG_SYSTEM_LOGO) {
    //check if the image is appropriate size as per data defined in the windows engineering guide.
    if (Width > ((SizeOfX * MS_MAX_WIDTH_PERCENTAGE) / 100) || Height > ((SizeOfY * MS_MAX_HEIGHT_PERCENTAGE) / 100))
    {
      DEBUG((DEBUG_ERROR, "Logo dimensions are not according to Specification. Screen size is %d by %d, Logo size is %d by %d   \n", SizeOfX, SizeOfY, Width, Height));
      Status = EFI_INVALID_PARAMETER;
      goto CleanUp;
    }
  }

  DestX = (SizeOfX - Width) / 2;
  DestY = (SizeOfY - Height) / 2;

  //Blt to screen
  if ((DestX >= 0) && (DestY >= 0)) {
    Status = GraphicsOutput->Blt(GraphicsOutput, Blt, EfiBltBufferToVideo,
                                 0, 0, (UINTN)DestX, (UINTN)DestY, Width, Height,
                                 Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  }
  else
  {
    DEBUG((DEBUG_ERROR, "Something really wrong with logo size and orientation.  DestX = 0x%X DestY = 0x%X\n", DestX, DestY));
    goto CleanUp;
  }

  if(EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a - Gop->Blt Error %r\n", __FUNCTION__, Status));
  }

  if (!EFI_ERROR (Status)) {
    //
    // Attempt to register logo with Boot Logo 2 Protocol
    //
    if ((Graphic == BG_SYSTEM_LOGO) && (BootLogo2 != NULL))
    {
      Status = BootLogo2->SetBootLogo(BootLogo2, Blt, DestX, DestY, Width, Height);
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "%a - BootLogo2 Error %r\n", __FUNCTION__, Status));
      }
    }
    //
    // Status of this function is EFI_SUCCESS even if registration with Boot
    // Logo 2 Protocol fails.
    //
    Status = EFI_SUCCESS;
  }

CleanUp:
  if (Blt != NULL) {
    FreePool(Blt);
  }
  if (ImageData != NULL) {
    FreePool(ImageData);
  }
  return Status;
}