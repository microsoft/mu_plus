/** @file
This GraphicsConsoleHelper is only intended to be used by BDS to configure 
the console mode / graphic mode

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

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextOut.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/GraphicsConsoleHelperLib.h>

//
// Platform defined native resolution.
//
static UINT32 mNativeHorizontalResolution;
static UINT32 mNativeVerticalResolution;
static UINT32 mNativeTextModeColumn;
static UINT32 mNativeTextModeRow;

//
// Mode for PXE booting and other places that VGA is necessary.
//
static UINT32 mVgaTextModeColumn;
static UINT32 mVgaTextModeRow;
static UINT32 mVgaHorizontalResolution;
static UINT32 mVgaVerticalResolution;

static BOOLEAN mModeTableInitialized = FALSE;

/**
 * InitializeModeTable - 
 * @param
 *
 * @return VOID
 */
static VOID
InitializeModeTable(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop)
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info = NULL;
  UINT32 Indx;
  UINT32 MaxHRes = 800;
  UINTN Size = 0;

  if (Gop == NULL)
  {
    return;
  }

  if (!mModeTableInitialized)
  {
    mVgaTextModeColumn = 100; // Standard VGA resolution with
    mVgaTextModeRow = 31;     // EFI standard glyphs
    mVgaHorizontalResolution = 800;
    mVgaVerticalResolution = 600;

    mNativeTextModeColumn = 100; // Default to VGA resolution
    mNativeTextModeRow = 31;     // but set to highest native resolution.
    mNativeHorizontalResolution = 800;
    mNativeVerticalResolution = 600;

    if (Gop != NULL)
    {
      for (Indx = 0; Indx < Gop->Mode->MaxMode; Indx++)
      {
        Status = Gop->QueryMode(Gop, Indx, &Size, &Info);
        if (!EFI_ERROR(Status))
        {
          if (MaxHRes < Info->HorizontalResolution)
          {
            mNativeHorizontalResolution = Info->HorizontalResolution;
            mNativeVerticalResolution = Info->VerticalResolution;
            MaxHRes = Info->HorizontalResolution;
          }
          DEBUG((DEBUG_INFO, "Mode Info for Mode %d\n", Indx));
          DEBUG((DEBUG_INFO, "HRes: %d VRes: %d PPScanLine: %d \n", Info->HorizontalResolution, Info->VerticalResolution, Info->PixelsPerScanLine));
          FreePool(Info);

          mModeTableInitialized = TRUE;
        }
      }
      mNativeTextModeColumn = mNativeHorizontalResolution / EFI_GLYPH_WIDTH;
      mNativeTextModeRow = mNativeVerticalResolution / EFI_GLYPH_HEIGHT;
    }
  }
}

EFI_STATUS
EFIAPI
SetGraphicsConsoleMode(GRAPHICS_CONSOLE_MODE Mode)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL          *GraphicsOutput;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *SimpleTextOut;
  UINTN SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  UINT32 MaxGopMode;
  UINT32 MaxTextMode;
  UINT32 ModeNumber;
  UINT32 NewHorizontalResolution;
  UINT32 NewVerticalResolution;
  UINT32 NewColumns;
  UINT32 NewRows;
  UINTN HandleCount;
  EFI_HANDLE *HandleBuffer;
  EFI_STATUS Status;
  UINTN Index;
  UINTN CurrentColumn;
  UINTN CurrentRow;

    //
    // Get current video resolution and text mode
    //
    Status = gBS->HandleProtocol (
                    gST->ConsoleOutHandle,
                    &gEfiGraphicsOutputProtocolGuid,
                    (VOID**)&GraphicsOutput
                    );
    if (EFI_ERROR (Status)) {
        GraphicsOutput = NULL;
    }

  if (GraphicsOutput == NULL)
  {
    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol(
      gST->ConsoleOutHandle,
      &gEfiSimpleTextOutProtocolGuid,
      (VOID **)&SimpleTextOut);
  if (EFI_ERROR(Status))
  {
    return EFI_UNSUPPORTED;
  }

  InitializeModeTable(GraphicsOutput);
  MaxGopMode = 0;
  MaxTextMode = 0;

  if (Mode == GCM_LOW_RES)
  {
    //
    // The required resolution and text mode is setup mode.
    //
    NewHorizontalResolution = mVgaHorizontalResolution;
    NewVerticalResolution = mVgaVerticalResolution;
    NewColumns = mVgaTextModeColumn;
    NewRows = mVgaTextModeRow;
  }
  else if(Mode == GCM_NATIVE_RES)
  {
    //
    // The required resolution and text mode is boot mode.
    //
    NewHorizontalResolution = mNativeHorizontalResolution;
    NewVerticalResolution = mNativeVerticalResolution;
    NewColumns = mNativeTextModeColumn;
    NewRows = mNativeTextModeRow;
  }
  else
  {
    DEBUG((DEBUG_ERROR, "%a - Unsupported Graphics Console Request Mode 0x%X\n", __FUNCTION__, Mode));
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  MaxGopMode = GraphicsOutput->Mode->MaxMode;
  MaxTextMode = SimpleTextOut->Mode->MaxMode;

  //
  // 1. If current video resolution is same with required video resolution,
  //    video resolution need not be changed.
  //    1.1. If current text mode is same with required text mode, text mode need not be changed.
  //    1.2. If current text mode is different from required text mode, text mode need be changed.
  // 2. If current video resolution is different from required video resolution, we need restart whole console drivers.
  //
  for (ModeNumber = 0; ModeNumber < MaxGopMode; ModeNumber++)
  {
    Status = GraphicsOutput->QueryMode(
        GraphicsOutput,
        ModeNumber,
        &SizeOfInfo,
        &Info);
    if (!EFI_ERROR(Status))
    {
      if ((Info->HorizontalResolution == NewHorizontalResolution) &&
          (Info->VerticalResolution == NewVerticalResolution))
      {
        if ((GraphicsOutput->Mode->Info->HorizontalResolution == NewHorizontalResolution) &&
            (GraphicsOutput->Mode->Info->VerticalResolution == NewVerticalResolution))
        {
          //
          // Current resolution is same with required resolution, check if text mode need be set
          //
          Status = SimpleTextOut->QueryMode(SimpleTextOut, SimpleTextOut->Mode->Mode, &CurrentColumn, &CurrentRow);
          ASSERT_EFI_ERROR(Status);
          if (CurrentColumn == NewColumns && CurrentRow == NewRows)
          {
            //
            // If current text mode is same with required text mode. Do nothing
            //
            FreePool(Info);
            Status = EFI_SUCCESS;
            goto Exit;
          }
          else
          {
            //
            // If current text mode is different from requried text mode.  Set new video mode
            //
            for (Index = 0; Index < MaxTextMode; Index++)
            {
              Status = SimpleTextOut->QueryMode(SimpleTextOut, Index, &CurrentColumn, &CurrentRow);
              if (!EFI_ERROR(Status))
              {
                if ((CurrentColumn == NewColumns) && (CurrentRow == NewRows))
                {
                  //
                  // Required text mode is supported, set it.
                  //
                  Status = SimpleTextOut->SetMode(SimpleTextOut, Index);
                  ASSERT_EFI_ERROR(Status);
                  //
                  // Update text mode PCD.
                  //
                  Status = PcdSet32S(PcdConOutColumn, mVgaTextModeColumn);
                  ASSERT_EFI_ERROR(Status);
                  Status = PcdSet32S(PcdConOutRow, mVgaTextModeRow);
                  ASSERT_EFI_ERROR(Status);

                  FreePool(Info);
                  goto Exit;
                }
              }
            }
            if (Index == MaxTextMode)
            {
              //
              // If requrired text mode is not supported, return error.
              //
              FreePool(Info);
              Status = EFI_UNSUPPORTED;
              goto Exit;
            }
          }
        }
        else
        {
          //
          // If current video resolution is not same with the new one, set new video resolution.
          // In this case, the driver which produces simple text out need be restarted.
          //
          Status = GraphicsOutput->SetMode(GraphicsOutput, ModeNumber);
          if (!EFI_ERROR(Status))
          {
            FreePool(Info);
            break;
          }
        }
      }
      FreePool(Info);
    }
  }

  if (ModeNumber == MaxGopMode)
  {
    //
    // If the resolution is not supported, return error.
    //
    Status = EFI_UNSUPPORTED;
    goto Exit;
  }

  //
  // Set PCD to Inform GraphicsConsole to change video resolution.
  // Set PCD to Inform Consplitter to change text mode.
  //
  Status = PcdSet32S(PcdVideoHorizontalResolution, NewHorizontalResolution);
  ASSERT_EFI_ERROR(Status);
  Status = PcdSet32S(PcdVideoVerticalResolution, NewVerticalResolution);
  ASSERT_EFI_ERROR(Status);
  Status = PcdSet32S(PcdConOutColumn, NewColumns);
  ASSERT_EFI_ERROR(Status);
  Status = PcdSet32S(PcdConOutRow, NewRows);
  ASSERT_EFI_ERROR(Status);

  //
  // Video mode is changed, so restart graphics console driver and higher level driver.
  // Reconnect graphics console driver and higher level driver.
  // Locate all the handles with GOP protocol and reconnect it.
  //
  Status = gBS->LocateHandleBuffer(
      ByProtocol,
      &gEfiSimpleTextOutProtocolGuid,
      NULL,
      &HandleCount,
      &HandleBuffer);
  if (!EFI_ERROR(Status))
  {
    for (Index = 0; Index < HandleCount; Index++)
    {
      gBS->DisconnectController(HandleBuffer[Index], NULL, NULL);
    }
    for (Index = 0; Index < HandleCount; Index++)
    {
      gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
    }
    if (HandleBuffer != NULL)
    {
      FreePool(HandleBuffer);
    }
  }
  Status = EFI_SUCCESS;

Exit:
  return Status;
}
