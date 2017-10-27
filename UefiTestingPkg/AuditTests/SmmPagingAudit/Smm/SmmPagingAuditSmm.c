/** @file -- SmmPagingAuditSmm.c
This is the SMM portion of the SmmPagingAuditApp driver.
It copies valid entries from the page tables into the communication buffer.

Copyright (c) 2017, Microsoft Corporation

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

// MS_CHANGE - Entire file created.

#include <PiSmm.h>

#include <Library/SmmServicesTableLib.h>
#include <Library/SmmMemLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/SmmCommunication.h>
#include <Guid/DebugImageInfoTable.h>

#include "../SmmPagingAuditCommon.h"


VOID **mPdePointers = NULL;
INT64 mPageDirectoryCount = -1;

/**
 * @brief      Locates loaded image table and copies the image information into the comm buffer.
 *
 * @param      CommBuffer  Images will be copied here for use in the app.
 *
 * @return     EFI_SUCCESS or error from SmmLocateHandle
 */
EFI_STATUS
EFIAPI
SmmLoadedImageTableDump (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER2   *CommBuffer
)
{
  EFI_STATUS                  Status;
  UINTN                       NumHandles;
  UINTN                       HandleBufferSize;
  EFI_HANDLE                 *HandleBuffer;
  UINTN                       Index;
  UINTN                       BufferIndex = 0;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  UINT64                      ImageBase;
  UINTN                       ImageSize;
  CHAR8                      *ImageName;
  UINT64                      ImageNameSize;
  UINT64                      BufferMin;
  UINT64                      BufferMax;

  BufferMin = BUFFER_SIZE_IMAGES * CommBuffer->RunNumber;
  BufferMax = BUFFER_SIZE_IMAGES * (CommBuffer->RunNumber + 1) - 1;

  HandleBufferSize = 0;
  HandleBuffer = NULL;
  Status = gSmst->SmmLocateHandle(
             ByProtocol,
             &gEfiLoadedImageProtocolGuid,
             NULL,
             &HandleBufferSize,
             HandleBuffer
           );
  if (Status != EFI_BUFFER_TOO_SMALL)
  {
    return Status;
  }
  HandleBuffer = AllocateZeroPool (HandleBufferSize);
  if (HandleBuffer == NULL)
  {
    return EFI_BUFFER_TOO_SMALL;
  }
  Status = gSmst->SmmLocateHandle(
             ByProtocol,
             &gEfiLoadedImageProtocolGuid,
             NULL,
             &HandleBufferSize,
             HandleBuffer
           );
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  NumHandles = HandleBufferSize / sizeof(EFI_HANDLE);

  DEBUG((DEBUG_ERROR, "Copying images %d to %d to the comm buffer. There are %d handles total.\n", BufferMin, BufferMax, NumHandles));

  if (BufferMin > NumHandles)
  {
    // We already got all the handles, no need to loop through again.
    return EFI_SUCCESS;
  }
  for (Index = 0; Index < NumHandles; Index++)
  {
    Status = gSmst->SmmHandleProtocol(
               HandleBuffer[Index],
               &gEfiLoadedImageProtocolGuid,
               (VOID **)&LoadedImage
             );
    if (EFI_ERROR(Status))
    {
      continue;
    }
    if ((BufferIndex >= BufferMin) && (BufferIndex <= BufferMax))
    {
      ImageName = PeCoffLoaderGetPdbPointer(LoadedImage->ImageBase);
      ImageNameSize = AsciiStrLen(ImageName);
      ImageBase = (UINT64) LoadedImage->ImageBase;
      ImageSize = LoadedImage->ImageSize;

      CommBuffer->SmmImages[BufferIndex - BufferMin].ImageBase = ImageBase;
      CommBuffer->SmmImages[BufferIndex - BufferMin].ImageSize = ImageSize;
      CopyMem(&CommBuffer->SmmImages[BufferIndex - BufferMin].ImageName, ImageName, sizeof(CHAR8) *ImageNameSize );
      CommBuffer->SmmImageCount ++;
    }

    BufferIndex ++;
  }

  CommBuffer->SmmImageCount = BufferIndex;
  return EFI_SUCCESS;
}


/**
 * @brief      Copies IDTR into the comm buffer.
 *
 * @param      CommBuffer  The communications buffer.
 */
VOID
EFIAPI
IdtDumpHandler (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER2   *CommBuffer
)
{
  IA32_DESCRIPTOR            Idtr;
  AsmReadIdtr(&Idtr);

  CommBuffer->Idtr = Idtr;
}


/**
 * @brief      Copies GDTR into the comm buffer
 *
 * @param      CommBuffer  The communications buffer
 */
VOID
EFIAPI
GdtDumpHandler (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER2   *CommBuffer
)
{
  IA32_DESCRIPTOR            Gdtr;
  AsmReadGdtr(&Gdtr);

  CommBuffer->Gdtr = Gdtr;
}


/**
 * @brief      Copies all the page table entries that were marked as present.
 *
 * @param      CommBuffer   The "first trip" comm buffer
 * @param      CommBuffer2  The "second trip" comm buffer
 */
VOID
EFIAPI
PageTableDumpHandler (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER   *CommBuffer
)
{
  UINT64                           VA;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Work;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Pml4;
  PAGE_TABLE_1G_ENTRY             *Pte1G;
  PAGE_TABLE_ENTRY                *Pte2M;
  PAGE_TABLE_4K_ENTRY             *Pte4K;
  UINTN                            Index1;
  UINTN                            Index2;
  UINTN                            Index3;
  UINTN                            Index4;
  UINTN                            PdeCount = 0;
  UINTN                            Buffer1Min = 0;
  UINTN                            Buffer1Max = 0;
  UINTN                            Buffer2Min = 0;
  UINTN                            Buffer2Max = 0;
  UINTN                            Buffer3Min = 0;
  UINTN                            Buffer3Max = 0;
  UINTN                            BufferIndex1 = 0;
  UINTN                            BufferIndex2 = 0;
  UINTN                            BufferIndex3 = 0;
  UINTN                            NumPage4KNotPresent = 0;
  UINTN                            NumPage2MNotPresent = 0;
  UINTN                            NumPage1GNotPresent = 0;
  UINTN                            NumPage512GNotPresent = 0;

  //
  // Each run gets a slice of the page tables.
  // 1000 4K pages
  // 500 2M pages
  // 300 1G pages
  //
  Buffer1Min = BUFFER_SIZE_4K * CommBuffer->RunNumber;
  Buffer1Max = BUFFER_SIZE_4K * (CommBuffer->RunNumber + 1) - 1;
  Buffer2Min = BUFFER_SIZE_2M * CommBuffer->RunNumber;
  Buffer2Max = BUFFER_SIZE_2M * (CommBuffer->RunNumber + 1) - 1;
  Buffer3Min = BUFFER_SIZE_1G * CommBuffer->RunNumber;
  Buffer3Max = BUFFER_SIZE_1G * (CommBuffer->RunNumber + 1) - 1;

  DEBUG((DEBUG_ERROR, "Getting 4k from %ld to %ld\nGetting 2m from %ld to %ld\nGetting 1g from %ld to %ld\n",
        Buffer1Min,
        Buffer1Max,
        Buffer2Min,
        Buffer2Max,
        Buffer3Min,
        Buffer3Max));

  Pml4 = (PAGE_MAP_AND_DIRECTORY_POINTER *) AsmReadCr3 ();
  PdeCount ++;

  for (Index4 = 0x0; Index4 < 0x1FF; Index4 ++)
  {
    if (!Pml4[Index4].Bits.Present)
    {
      NumPage512GNotPresent++;
      continue;
    }
    Pte1G = (PAGE_TABLE_1G_ENTRY *) (Pml4[Index4].Bits.PageTableBaseAddress << 12);
    for (Index3 = 0x0;  Index3 < 0x1FF; Index3 ++ )
    {
      if (!Pte1G[Index3].Bits.Present)
      {
        NumPage1GNotPresent++;
        continue;
      }
      //
      // MustBe1 is the bit that indiacates whether the pointer is a directory
      // pointer or a page table entry.
      //
      if (!(Pte1G[Index3].Bits.MustBe1))
      {
        //
        // We have to cast 1G and 2M directories to this to
        // get all of their address bits.
        //
        Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte1G;
        Pte2M = (PAGE_TABLE_ENTRY *) (Work[Index3].Bits.PageTableBaseAddress << 12);
        PdeCount ++;
        for (Index2 = 0x0; Index2 < 0x1FF; Index2 ++ )
        {
          if (!Pte2M[Index2].Bits.Present)
          {
            NumPage2MNotPresent++;
            continue;
          }
          if (!(Pte2M[Index2].Bits.MustBe1))
          {
            Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte2M;
            Pte4K = (PAGE_TABLE_4K_ENTRY *) (Work[Index2].Bits.PageTableBaseAddress << 12);
            PdeCount ++;
            for (Index1 = 0x0; Index1 < 0x1FF; Index1 ++ )
            {
              if (!Pte4K[Index1].Bits.Present)
              {
                NumPage4KNotPresent++;
                continue;
              }
              VA = (Index4 << 39) + (Index3 << 30) + (Index2 << 21) + (Index1 << 12);
              if ((BufferIndex1 >= Buffer1Min) && (BufferIndex1 <= Buffer1Max))
              {
                CommBuffer->Pte4K[(BufferIndex1 - Buffer1Min)] = Pte4K[Index1];
              }
              else if ((BufferIndex1 > Buffer1Max))
              {
                CommBuffer->Full = TRUE;
              }
              BufferIndex1 ++;
            }
          }
          else
          {
            VA = (Index4 << 39) + (Index3 << 30) + (Index2 << 21);
            if ((BufferIndex2 >= Buffer2Min) && (BufferIndex2 <= Buffer2Max))
            {
              CommBuffer->Pte2M[(BufferIndex2 - Buffer2Min)] = Pte2M[Index2];
            }
            else if ((BufferIndex2 >= Buffer2Max))
            {
              CommBuffer->Full = TRUE;
            }
            BufferIndex2 ++;
          }
        }
      }
      else
      {
        VA = (Index4 << 39) + (Index3 << 30);
        if ((BufferIndex3 >= Buffer3Min) && (BufferIndex3 <= Buffer3Max))
        {
          CommBuffer->Pte1G[(BufferIndex3 - Buffer3Min)] = Pte1G[Index3];
        }
        else if (BufferIndex3 >= Buffer3Max)
        {
          CommBuffer->Full = TRUE;
        }
        BufferIndex3 ++;
      }
    }
  }
  mPageDirectoryCount = PdeCount;
  DEBUG((DEBUG_ERROR, "Pages used for Page Tables   = %d\n", PdeCount));
  DEBUG((DEBUG_ERROR, "Number of   4K Pages active  = %d - NotPresent = %d\n", BufferIndex1, NumPage4KNotPresent));
  DEBUG((DEBUG_ERROR, "Number of   2M Pages active  = %d - NotPresent = %d\n", BufferIndex2, NumPage2MNotPresent));
  DEBUG((DEBUG_ERROR, "Number of   1G Pages active  = %d - NotPresent = %d\n", BufferIndex3, NumPage1GNotPresent));
}


/**
 * @brief      Collects pointers to page directories.
 *
 */
VOID
EFIAPI
BuildPdeList (
)
{
  PAGE_MAP_AND_DIRECTORY_POINTER  *Work;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Pml4;
  PAGE_TABLE_1G_ENTRY             *Pte1G;
  PAGE_TABLE_ENTRY                *Pte2M;
  PAGE_TABLE_4K_ENTRY             *Pte4K;
  UINTN                            Index2;
  UINTN                            Index3;
  UINTN                            Index4;
  UINTN                            Index = 0;

  Pml4 = (PAGE_MAP_AND_DIRECTORY_POINTER *) AsmReadCr3 ();

  for (Index4 = 0x0; Index4 < 0x1FF; Index4 ++)
  {
    if (!Pml4[Index4].Bits.Present)
    {
      continue;
    }
    Pte1G = (PAGE_TABLE_1G_ENTRY *) (Pml4[Index4].Bits.PageTableBaseAddress << 12);
    mPdePointers[(Index ++)] = Pte1G;
    for (Index3 = 0x0;  Index3 < 0x1FF; Index3 ++ )
    {
      if (!Pte1G[Index3].Bits.Present)
      {
        continue;
      }
      //
      // MustBe1 is the bit that indiacates whether the pointer is a directory
      // pointer or a page table entry.
      //
      if (!(Pte1G[Index3].Bits.MustBe1))
      {
        //
        // We have to cast 1G and 2M directories to this to
        // get all of their address bits.
        //
        Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte1G;
        Pte2M = (PAGE_TABLE_ENTRY *) (Work[Index3].Bits.PageTableBaseAddress << 12);
        mPdePointers[(Index ++)] = Pte2M;
        for (Index2 = 0x0; Index2 < 0x1FF; Index2 ++ )
        {
          if (!Pte2M[Index2].Bits.Present)
          {
            continue;
          }
          if (!(Pte2M[Index2].Bits.MustBe1))
          {
            Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte2M;
            Pte4K = (PAGE_TABLE_4K_ENTRY *) (Work[Index2].Bits.PageTableBaseAddress << 12);
            mPdePointers[(Index ++)] = Pte4K;
          }
        }
      }
    }
  }
}


/**
 * @brief      Copies all the addresses of pages that are holding page directories.
 *
 * @param      CommBuffer   Comm buffer to copy results to.
 */
VOID
EFIAPI
GetPageDirectoryLocations (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER2   *CommBuffer
)
{
  UINT64                           VA;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Work;
  PAGE_MAP_AND_DIRECTORY_POINTER  *Pml4;
  PAGE_TABLE_1G_ENTRY             *Pte1G;
  PAGE_TABLE_ENTRY                *Pte2M;
  PAGE_TABLE_4K_ENTRY             *Pte4K;
  INTN                             Index;
  UINTN                            Index1;
  UINTN                            Index2;
  UINTN                            Index3;
  UINTN                            Index4;
  UINTN                            BufferMin;
  UINTN                            BufferMax;
  UINTN                            PdeBufferIndex = 0;

  Pml4 = (PAGE_MAP_AND_DIRECTORY_POINTER *) AsmReadCr3 ();

  BufferMin = BUFFER_SIZE_PDE * CommBuffer->RunNumber;
  BufferMax = BUFFER_SIZE_PDE * (CommBuffer->RunNumber + 1) - 1;


  if (((INT64) BufferMin) > mPageDirectoryCount)
  {
    // Nothing new to get.
    return;
  }

  for (Index4 = 0x0; Index4 < 0x1FF; Index4 ++)
  {
    if (!Pml4[Index4].Bits.Present)
    {
      continue;
    }
    Pte1G = (PAGE_TABLE_1G_ENTRY *) (Pml4[Index4].Bits.PageTableBaseAddress << 12);
    for (Index3 = 0x0;  Index3 < 0x1FF; Index3 ++ )
    {
      if (Pte1G[Index3].Bits.Present && (!Pte1G[Index3].Bits.MustBe1))
      {
        Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte1G;
        Pte2M = (PAGE_TABLE_ENTRY *) (Work[Index3].Bits.PageTableBaseAddress << 12);
        for (Index2 = 0x0; Index2 < 0x1FF; Index2 ++ )
        {
          if (Pte2M[Index2].Bits.Present && (!Pte2M[Index2].Bits.MustBe1))
          {
            Work = (PAGE_MAP_AND_DIRECTORY_POINTER *) Pte2M;
            Pte4K = (PAGE_TABLE_4K_ENTRY *) (Work[Index2].Bits.PageTableBaseAddress << 12);
            for (Index1 = 0x0; Index1 < 0x1FF; Index1 ++ )
            {
              VA = (Index4 << 39) + (Index3 << 30) + (Index2 << 21) + (Index1 << 12);
              for (Index = 0; Index < mPageDirectoryCount; Index ++)
              {
                if ((UINT64) mPdePointers[Index] == VA)
                {
                  if (PdeBufferIndex >= BufferMin && PdeBufferIndex <= BufferMax)
                  {
                    CommBuffer->Pde[PdeBufferIndex - BufferMin] = VA;
                    CommBuffer->PdeCount ++;
                  }
                  PdeBufferIndex ++;
                }
              }
            }
          }
          else
          {
            VA = (Index4 << 39) + (Index3 << 30) + (Index2 << 21);
            for (Index = 0; Index < mPageDirectoryCount; Index ++)
            {
              if ((UINT64) mPdePointers[Index] == VA)
              {
                if (PdeBufferIndex >= BufferMin && PdeBufferIndex <= BufferMax)
                {
                  CommBuffer->Pde[PdeBufferIndex - BufferMin] = VA;
                  CommBuffer->PdeCount ++;
                }
                PdeBufferIndex ++;
              }
            }
          }
        }
      }
      else
      {
        VA = (Index4 << 39) + (Index3 << 30);
        for (Index = 0; Index < mPageDirectoryCount; Index ++)
        {
          if ((UINT64) mPdePointers[Index] == VA)
          {
            if (PdeBufferIndex >= BufferMin && PdeBufferIndex <= BufferMax)
            {
              CommBuffer->Pde[PdeBufferIndex - BufferMin] = VA;
              CommBuffer->PdeCount ++;
            }
            PdeBufferIndex ++;
          }
        }
      }
    }
  }
}

/**
 * @brief      Copies all the addresses of pages that are holding page directories.
 *
 * @param      CommBuffer   Comm buffer to copy results to.
 */
VOID
EFIAPI
CopyPdes (
  IN OUT PAGE_TABLE_DUMP_COMM_BUFFER2   *CommBuffer
)
{
  UINTN                             Index;
  UINTN                            BufferMin;
  UINTN                            BufferMax;
  UINTN                            PageDirectoryCount = (UINT64) mPageDirectoryCount;

  BufferMin = BUFFER_SIZE_PDE * CommBuffer->RunNumber;
  BufferMax = BUFFER_SIZE_PDE * (CommBuffer->RunNumber + 1) - 1;

  DEBUG((DEBUG_ERROR, "Copying images %d to %d to the comm buffer. There are %d handles total.\n",
         BufferMin, BufferMax, PageDirectoryCount));

  for (Index = BufferMin; (Index <= BufferMax) && (Index < PageDirectoryCount); Index ++){
    CommBuffer->Pde[Index - BufferMin] = (UINT64) mPdePointers[Index];
  }
  CommBuffer->PdeCount = PageDirectoryCount;
}

/**
 * @brief      Dispatches tasks when called each (of 3) times by the app.
 *
 * @param[in]  DispatchHandle   The dispatch handle
 * @param      RegisterContext  The register context
 * @param      CommBuffer       The communications buffer
 * @param      CommBufferSize   The communications buffer size
 *
 * @return     EFI_ACCESS_DENIED if comm buffer is the wrong size, success otherwise.
 */
EFI_STATUS
EFIAPI
SmmPagingAuditHandler (
  IN     EFI_HANDLE                   DispatchHandle,
  IN     CONST VOID                   *RegisterContext,
  IN OUT VOID                         *CommBuffer,
  IN OUT UINTN                        *CommBufferSize
)
{
  UINTN                                    TempCommBufferSize;
  PAGE_TABLE_DUMP_COMM_BUFFER             *FirstCommBuffer = NULL;
  PAGE_TABLE_DUMP_COMM_BUFFER2            *SecondCommBuffer = NULL;
  static EFI_SMM_COMMUNICATION_PROTOCOL   *SmmCommunication = NULL;

  DEBUG(( DEBUG_ERROR, __FUNCTION__"()\n" ));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL)
  {
    return EFI_ACCESS_DENIED;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize == sizeof( PAGE_TABLE_DUMP_COMM_BUFFER ))
  {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" Getting page tables.\n" ));
    FirstCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER*)CommBuffer;
    PageTableDumpHandler(FirstCommBuffer);
  }
  else if (TempCommBufferSize == sizeof( PAGE_TABLE_DUMP_COMM_BUFFER2 ))
  {
    SecondCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER2*)CommBuffer;
    DEBUG((DEBUG_ERROR, __FUNCTION__" Getting misc info run #%d\n", SecondCommBuffer->RunNumber));
    if (SecondCommBuffer->RunNumber == 0)
    {
      IdtDumpHandler(SecondCommBuffer);
      GdtDumpHandler(SecondCommBuffer);
      mPdePointers = AllocateZeroPool(sizeof(VOID*) * mPageDirectoryCount);
      BuildPdeList();
    }
    CopyPdes(SecondCommBuffer);
    SmmLoadedImageTableDump(SecondCommBuffer);
  }
  else
  {
    return EFI_ACCESS_DENIED;
  }


  if (SecondCommBuffer && (BUFFER_SIZE_PDE < SecondCommBuffer->PdeCount))
  {
    //
    // Frees pool pinter addresses
    //
    FreePool(mPdePointers);
  }
  return EFI_SUCCESS;
}

/**
  The module Entry Point of the driver.

  @param[in]  ImageHandle    The firmware allocated handle for the EFI image.
  @param[in]  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingAuditSmmEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS                Status;
  EFI_HANDLE                DiscardedHandle;

  //
  // Register SMI handler.
  //
  DiscardedHandle = NULL;
  Status = gSmst->SmiHandlerRegister(
             SmmPagingAuditHandler,
             &gSmmPagingAuditSmiHandlerGuid,
             &DiscardedHandle);
  return Status;
}
