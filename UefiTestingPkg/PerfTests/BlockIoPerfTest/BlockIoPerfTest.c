/** @file -- BlockIoPerfTest.c
 * 
 * Do block io transfers and time them for performance evaluation.  

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
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/ShellLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TimerLib.h>
#include <Library/DevicePathLib.h>

#define MAX_SIZE_FOR_TEST (0x100000 * 20) 

#define ONE_MICROSECOND (1000)
#define ONE_MILLISECOND (1000 * ONE_MICROSECOND)
#define ONE_SECOND (1000 * ONE_MILLISECOND)

#define GET_SECONDS(a)        ((a) / ONE_SECOND)
#define GET_MILLISECONDS(a)   ((a) / ONE_MILLISECOND)
#define GET_MICROSECONDS(a)   ((a) / ONE_MICROSECOND)


VOID
PrintTimeFromNs(UINT64 TimeInNs)
{
  UINT64 Sec = 0;
  UINT64 Milli = 0;
  UINT64 Micro = 0;
  UINT64 Nano = 0;
  UINT64 RemainingTime = TimeInNs;

  if(RemainingTime > ONE_SECOND)  {
    Sec = GET_SECONDS(RemainingTime);
    RemainingTime -= (Sec * ONE_SECOND);
  }

  if(RemainingTime > ONE_MILLISECOND) {
    Milli = GET_MILLISECONDS(RemainingTime);
    RemainingTime -= (Milli * ONE_MILLISECOND);
  }

  if(RemainingTime > ONE_MICROSECOND) {
    Micro = GET_MICROSECONDS(RemainingTime);
    RemainingTime -= (Micro * ONE_MICROSECOND);
  }

  if(RemainingTime > 0) {
    Nano = RemainingTime;
  }

  if(Sec > 0) {
    Print(L"%d.%d%d%d seconds\n", Sec, Milli, Micro, Nano);
  } else if (Milli > 0) {
    Print(L"%d.%d%d milliseconds\n", Milli, Micro, Nano);
  } else if (Micro > 0) {
    Print(L"%d.%d microseconds\n", Micro, Nano);
  } else {
    Print(L"%d nanoseconds\n", Nano);
  }
}

VOID
TestBlockIo(EFI_BLOCK_IO_PROTOCOL *BlkIo)
{
  EFI_STATUS Status = EFI_SUCCESS;
  UINT32 MediaId;
  EFI_LBA Lba;
  UINT8 *Buffer = NULL;
  UINTN  ReadSizes[] = {0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x100000, MAX_SIZE_FOR_TEST}; 
  UINTN  Index;

  if(BlkIo == NULL) {
    Print(L"BlockIo is NULL\n");
    return;
  }
  
  Buffer = AllocatePages(EFI_SIZE_TO_PAGES(MAX_SIZE_FOR_TEST));
  if(Buffer == NULL)
  {
    Print(L"Failed to allocate memory");
    return;
  }

  MediaId = BlkIo->Media->MediaId;
  Print(L" Revision: 0x%lX\n WriteCaching: 0x%X\n BlockSize: 0x%X\n", 
        BlkIo->Revision, BlkIo->Media->WriteCaching, BlkIo->Media->BlockSize);
  Print(L" IoAlign: 0x%X\n", BlkIo->Media->IoAlign);

  for(Index =0; Index < ARRAY_SIZE(ReadSizes); Index++)
  {
    UINT64 Start = 0;
    UINT64 End = 0;
    Lba = 0;
    Print(L"Test %dKB\n", ReadSizes[Index]/1024);
    Start = GetPerformanceCounter();
    Status = BlkIo->ReadBlocks (BlkIo, MediaId, Lba, ReadSizes[Index], (VOID*)Buffer);
    End = GetPerformanceCounter();
    if(EFI_ERROR(Status))
    {
      Print(L"Error reading blocks.  Status = %r\n", Status);
    }
    PrintTimeFromNs(GetTimeInNanoSecond(End-Start));
    Print(L"\n\n");
  }

  FreePages(Buffer, EFI_SIZE_TO_PAGES(MAX_SIZE_FOR_TEST));
}

/**
  Test entry point.

  @param[in]  ImageHandle     The image handle.
  @param[in]  SystemTable     The system table.

  @retval EFI_SUCCESS            Command completed successfully.

**/
EFI_STATUS
EFIAPI
TestMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  UINTN                       BlockIoHandleCount;
  EFI_HANDLE                  *BlockIoBuffer = NULL;
  UINTN                       Index;
  EFI_DEVICE_PATH_PROTOCOL    *BlockIoDevicePath = NULL;
  CHAR16                      *DevicePathString = NULL;
  EFI_BLOCK_IO_PROTOCOL       *BlockIoProtocol = NULL;

  //
  // Initialize the shell lib (we must be in non-auto-init...)
  //  NOTE: This may not be necessary, but whatever.
  //
  Status = ShellInitialize();
  if (EFI_ERROR( Status ))
  {
    DEBUG((DEBUG_ERROR, "Failed to init Shell.  %r\n", Status));
    return Status;
  }

  //locate all handles with blockio
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &BlockIoHandleCount, &BlockIoBuffer);
  if (EFI_ERROR (Status) || BlockIoHandleCount == 0 || BlockIoBuffer == NULL) {
    //
    // If there was an error or there are no device handles that support
    // the BLOCK_IO Protocol, then return.
    //
    Print(L"No BlockIO in this system\n");
    return EFI_SUCCESS;
  }

  Print(L"Found %d BlockIO handles\n", BlockIoHandleCount);
  //
  // Loop through all the device handles that support the BLOCK_IO Protocol
  //
  for (Index = 0; Index < BlockIoHandleCount; Index++) {

    Status = gBS->HandleProtocol (BlockIoBuffer[Index], &gEfiDevicePathProtocolGuid, (VOID *) &BlockIoDevicePath);
    if (EFI_ERROR (Status) || BlockIoDevicePath == NULL) {
      Print(L"No Device Path Protocol for this block io\n");
    } else {
      DevicePathString = ConvertDevicePathToText(BlockIoDevicePath, TRUE, FALSE);
      if(DevicePathString != NULL) 
      {
        Print(L"DevicePath is %s\n", DevicePathString);
        FreePool(DevicePathString);
        DevicePathString = NULL;
      } else {
        Print(L"DevicePath to text was NULL\n");
      }  
    }
    Status = gBS->HandleProtocol (BlockIoBuffer[Index], &gEfiBlockIoProtocolGuid, (VOID *) &BlockIoProtocol);
    if (EFI_ERROR (Status) || BlockIoProtocol == NULL) {
      Print(L"BlockIoProtocol failed.  Can't test this one");
      Print(L"\n\n");
      continue;
    }
   
    TestBlockIo(BlockIoProtocol);
    Print(L"\n\n");


  } //end for looop

  if (BlockIoBuffer != NULL) {
      gBS->FreePool (BlockIoBuffer);
  }

  return Status;
}


