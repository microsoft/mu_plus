/** @file -- SmmPagingAuditApp.c
This user-facing application collects information from the SMM page tables and
writes it to files.

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

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/SmmCommunication.h>

#include <Register/Msr.h>

#include <Guid/DebugImageInfoTable.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../SmmPagingAuditCommon.h"


VOID      *mPiSmmCommonCommBufferAddress = NULL;
UINTN      mPiSmmCommonCommBufferSize;
UINTN      mWriteCount = 0;
CHAR8     *mWriteString;
CHAR8     *mBuffer;
CHAR16    *mLogFileName = L"MemoryInfo";


/**
 * @brief      Writes a buffer to file.
 *
 * @param      FileName     The name of the file being written to.
 * @param      Buffer       The buffer to write to file.
 * @param[in]  BufferSize   Size of the buffer.
 * @param[in]  WriteCount   Number to append to the end of the file.
 */
VOID
EFIAPI
WriteBufferToFile(
  IN     CHAR16*                    FileName,
  IN     VOID*                      Buffer,
  IN     UINTN                      BufferSize,
  IN     UINTN                      WriteCount
)
{
  UINTN StringSize = BufferSize;
  SHELL_FILE_HANDLE FileHandle;
  CHAR16 FileNameAndExt[MAX_STRING_SIZE];
  ZeroMem(FileNameAndExt, sizeof(CHAR16) * MAX_STRING_SIZE);
  UnicodeSPrint(FileNameAndExt, MAX_STRING_SIZE, L"%s%d.dat", FileName, WriteCount);
  ShellOpenFileByName(FileNameAndExt, &FileHandle, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
  ShellWriteFile(FileHandle, &StringSize, Buffer);
  ShellCloseFile(&FileHandle);
  ShellPrintEx(-1, -1, L"Wrote to file %s\n", FileNameAndExt);
}


/**
 * @brief      Takes the contents of mBuffer and concatonates it to mWriteString.
 */
VOID
EFIAPI
ConcatBufferOnWriteString()
{
  while ((strlen(mWriteString) + strlen(mBuffer)) >= MAX_STRING_SIZE)
  {
    WriteBufferToFile(mLogFileName, mWriteString, MAX_STRING_SIZE, (mWriteCount ++));
    ZeroMem(mWriteString, MAX_STRING_SIZE * sizeof(CHAR8));
  }
  AsciiStrCatS(mWriteString, MAX_STRING_SIZE, mBuffer);
  ZeroMem(mBuffer, MAX_STRING_SIZE * sizeof(CHAR8));
}


/**
 * @brief      Writes the name, base, and limit of each image in the image table to a file.
 */
VOID
EFIAPI
LoadedImageTableDump (
)
{
  EFI_STATUS                                   Status;
  EFI_DEBUG_IMAGE_INFO_TABLE_HEADER           *TableHeader;
  EFI_DEBUG_IMAGE_INFO                        *Table;
  EFI_LOADED_IMAGE_PROTOCOL                   *LoadedImageProtocolInstance;
  UINT64                                       ImageBase;
  UINT64                                       ImageSize;
  UINT64                                       Index;
  UINT32                                       TableSize;
  EFI_DEBUG_IMAGE_INFO_NORMAL                 *NormalImage;
  CHAR8                                       *PdbFileName;

  //
  // locate DebugImageInfoTable
  //
  Status = EfiGetSystemConfigurationTable (
             &gEfiDebugImageInfoTableGuid,
             (VOID**) &TableHeader
           );
  if (EFI_ERROR(Status))
  {
    return;
  }

  Table = TableHeader->EfiDebugImageInfoTable;
  TableSize = TableHeader->TableSize;

  DEBUG((DEBUG_VERBOSE, __FUNCTION__"\n\nLength %lx Start x0x%016lx\n\n",
         TableHeader->TableSize,
         Table));


  for (Index = 0; Index < TableSize; Index ++)
  {
    if (Table[Index].NormalImage == NULL)
    {
      continue;
    }

    NormalImage = Table[Index].NormalImage;
    LoadedImageProtocolInstance = NormalImage->LoadedImageProtocolInstance;
    ImageSize = LoadedImageProtocolInstance->ImageSize;
    ImageBase = (UINT64) LoadedImageProtocolInstance->ImageBase;

    if (ImageSize == 0)
    {
      // No need to register empty slots in the table as images.
      continue;
    }
    PdbFileName = PeCoffLoaderGetPdbPointer(LoadedImageProtocolInstance->ImageBase);

    AsciiSPrint(mBuffer, MAX_STRING_SIZE, "0x%lx,0x%lx,%a\n", ImageBase, ImageSize, PdbFileName);
    ConcatBufferOnWriteString();
  }
}


/**
 * @brief      Writes images from SMM debug info table to file.
 *
 * @param      SmmImages       Images from SMM.
 * @param[in]  SmmImagesCount  Number of images present.
 */
VOID
EFIAPI
SmmLoadedImageTableDump (
  IN     IMAGE_STRUCT       *SmmImages,
  IN     UINTN               SmmImagesCount
)
{
  UINT64         ImageBase;
  UINT64         ImageSize;
  CHAR8         *ImageName;
  UINT64         Index;

  DEBUG((DEBUG_VERBOSE, __FUNCTION__"\n\nLength %lx\n", SmmImagesCount));

  for (Index = 0; Index < SmmImagesCount; Index ++)
  {
    ImageSize = SmmImages[Index].ImageSize;
    ImageBase = SmmImages[Index].ImageBase;
    ImageName = SmmImages[Index].ImageName;

    if (ImageSize == 0)
    {
      DEBUG((DEBUG_ERROR, "0x%lx,0x%lx\n", ImageBase, ImageSize));
      continue;
    }

    AsciiSPrint(mBuffer, MAX_STRING_SIZE,
                "0x%lx,0x%lx,%a\n",
                ImageBase,
                ImageSize,
                ImageName);

    ConcatBufferOnWriteString();

  }
}



/**
 * @brief      Writes the UEFI memory map to file.
 */
VOID
EFIAPI
MemoryMapDumpHandler (
)
{

  EFI_STATUS                  Status;
  UINTN                       EfiMemoryMapSize;
  UINTN                       EfiMapKey;
  UINTN                       EfiDescriptorSize;
  UINT32                      EfiDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMap;
  EFI_MEMORY_DESCRIPTOR       *EfiMemoryMapEnd;
  EFI_MEMORY_DESCRIPTOR       *EfiMemNext;

  //
  // Get the EFI memory map.
  //
  EfiMemoryMapSize  = 0;
  EfiMemoryMap      = NULL;
  Status = gBS->GetMemoryMap (
             &EfiMemoryMapSize,
             EfiMemoryMap,
             &EfiMapKey,
             &EfiDescriptorSize,
             &EfiDescriptorVersion
           );
  //
  // Loop to allocate space for the memory map and then copy it in.
  //
  do {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR *) AllocatePool (EfiMemoryMapSize);
    ASSERT (EfiMemoryMap != NULL);
    Status = gBS->GetMemoryMap (
               &EfiMemoryMapSize,
               EfiMemoryMap,
               &EfiMapKey,
               &EfiDescriptorSize,
               &EfiDescriptorVersion
             );
    if (EFI_ERROR (Status))
    {
      FreePool (EfiMemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) EfiMemoryMap + EfiMemoryMapSize);
  EfiMemNext = EfiMemoryMap;

  while (EfiMemNext < EfiMemoryMapEnd)
  {
    AsciiSPrint(mBuffer, MAX_STRING_SIZE,
                "%lx,%lx,%lx,%lx,%lx\n",
                EfiMemNext->Type,
                EfiMemNext->PhysicalStart,
                EfiMemNext->VirtualStart,
                EfiMemNext->NumberOfPages,
                EfiMemNext->Attribute);

    ConcatBufferOnWriteString();
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR (EfiMemNext, EfiDescriptorSize);
  }

  if (EfiMemoryMap)
  {
    FreePool (EfiMemoryMap);
  }
}


EFI_STATUS
EFIAPI
TSEGDumpHandler (
)
{
  UINT64 SmrrBase = 0;
  UINT64 SmrrMask = 0;
  UINT32 SmmCodeSize = 0x1000000; // SMM might change size?
  SmrrBase = AsmReadMsr64(MSR_IA32_SMRR_PHYSBASE);
  SmrrMask = AsmReadMsr64(MSR_IA32_SMRR_PHYSMASK);

  DEBUG((DEBUG_ERROR, __FUNCTION__"TSEG base 0x%016lx mask: 0x%016x\n", SmrrBase , SmrrMask));

  // Writing this out in the format of a Memory Map entry (Type 16 will map to TSEG)
  AsciiSPrint(mBuffer, MAX_STRING_SIZE,
              "%lx,%lx,%lx,%lx,%lx\n",
              16,
              (SmrrBase & (SmrrMask & 0xFFFFF000)),
              0,
              EFI_SIZE_TO_PAGES(SmmCodeSize),
              0);

  ConcatBufferOnWriteString();

  return EFI_SUCCESS;
}

/**
  This helper function actually sends the requested communication
  to the SMM driver.

  @param[in]  RequestedFunction   The test function to request the SMM driver run.

  @retval     EFI_SUCCESS         Communication was successful.
  @retval     EFI_ABORTED         Some error occurred.

**/
STATIC
EFI_STATUS
SmmMemoryProtectionsDxeToSmmCommunicate (
)
{
  static EFI_SMM_COMMUNICATION_PROTOCOL   *SmmCommunication = NULL;
  EFI_STATUS                               Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  PAGE_TABLE_DUMP_COMM_BUFFER             *FirstCommBuffer;
  PAGE_TABLE_DUMP_COMM_BUFFER2            *SecondCommBuffer;
  CHAR16*                                  LogFileName4K = L"4K";
  CHAR16*                                  LogFileName2M = L"2M";
  CHAR16*                                  LogFileName1G = L"1G";
  UINTN                                    CommBufferSize;
  UINTN                                    Index;
  UINT64                                   RunNumber = 0;

  if (mPiSmmCommonCommBufferAddress == NULL)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" - Communication mBuffer not found!\n"));
    return EFI_ABORTED;
  }

  CommBufferSize = sizeof(PAGE_TABLE_DUMP_COMM_BUFFER) + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);
  if (CommBufferSize > mPiSmmCommonCommBufferSize)
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" - Communication mBuffer is too small\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Locate the protocol as needed.
  //
  if (!SmmCommunication)
  {
    Status = gBS->LocateProtocol(&gEfiSmmCommunicationProtocolGuid, NULL, (VOID**) &SmmCommunication);
    if (EFI_ERROR(Status))
    {
      return Status;
    }
  }

  do
  {
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER*) mPiSmmCommonCommBufferAddress;
    ZeroMem(CommHeader, CommBufferSize);
    CopyGuid(&CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid);
    CommHeader->MessageLength = sizeof(PAGE_TABLE_DUMP_COMM_BUFFER);
    FirstCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER*)CommHeader->Data;
    FirstCommBuffer->RunNumber = RunNumber;
    FirstCommBuffer->Full = FALSE;

    //
    // Signal trip to SMM
    //
    Status = SmmCommunication->Communicate(SmmCommunication,
                                           CommHeader,
                                           &CommBufferSize);

    //
    // Get the data out of the comm buffer.
    //
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER*) mPiSmmCommonCommBufferAddress;
    FirstCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER *) CommHeader->Data;


    //
    // Write data from the comm buffer to file.
    //
    WriteBufferToFile(LogFileName1G, FirstCommBuffer->Pte1G, sizeof(FirstCommBuffer->Pte1G), RunNumber);
    WriteBufferToFile(LogFileName2M, FirstCommBuffer->Pte2M, sizeof(FirstCommBuffer->Pte2M), RunNumber);
    WriteBufferToFile(LogFileName4K, FirstCommBuffer->Pte4K, sizeof(FirstCommBuffer->Pte4K), RunNumber);
    RunNumber ++;

  } while (FirstCommBuffer->Full);

  RunNumber = 0;
  CommBufferSize = sizeof(PAGE_TABLE_DUMP_COMM_BUFFER2) + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);

  do
  {
    //
    // Ready the comm buffer for our second round trip to SMM.
    //
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER*) mPiSmmCommonCommBufferAddress;
    ZeroMem(CommHeader, CommBufferSize);
    CopyGuid(&CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid);
    CommHeader->MessageLength = sizeof(PAGE_TABLE_DUMP_COMM_BUFFER2);
    SecondCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER2 *) CommHeader->Data;
    SecondCommBuffer->RunNumber = RunNumber;
    SecondCommBuffer->PdeCount = 0;
    SecondCommBuffer->SmmImageCount = 0;

    //
    // Signal SMM again.
    //
    Status = SmmCommunication->Communicate(SmmCommunication,
                                           CommHeader,
                                           &CommBufferSize);
    //
    // Get the data out of the comm buffer and write it to file.
    //
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER*) mPiSmmCommonCommBufferAddress;
    SecondCommBuffer = (PAGE_TABLE_DUMP_COMM_BUFFER2 *) CommHeader->Data;
    DEBUG((DEBUG_ERROR, __FUNCTION__" - Found 0x%lx page directories\n", SecondCommBuffer->PdeCount));
    for (Index = 0; Index < SecondCommBuffer->PdeCount; Index ++)
    {
      if (SecondCommBuffer->Pde[Index])
      {
        AsciiSPrint(
          mBuffer, MAX_STRING_SIZE, "0x%lx,0x%lx,PDE\n",
          SecondCommBuffer->Pde[Index], 512); // 512 is the size of a Page Directory
        ConcatBufferOnWriteString();
      }
    }

    SmmLoadedImageTableDump(SecondCommBuffer->SmmImages, SecondCommBuffer->SmmImageCount);

    AsciiSPrint(
      mBuffer, MAX_STRING_SIZE, "0x%lx,0x%lx,GDT\n0x%lx,0x%lx,IDT\n",
      SecondCommBuffer->Gdtr.Base, (UINT64) SecondCommBuffer->Gdtr.Limit,
      SecondCommBuffer->Idtr.Base, (UINT64) SecondCommBuffer->Idtr.Limit);
    ConcatBufferOnWriteString();
  } while ((SecondCommBuffer->SmmImageCount == BUFFER_SIZE_IMAGES) ||
    (SecondCommBuffer->PdeCount == BUFFER_SIZE_PDE));
  return EFI_SUCCESS;
} // SmmMemoryProtectionsDxeToSmmCommunicate()


/**
 * @brief      Locates and stores address of comm buffer.
 *
 * @return     EFI_ABORTED if buffer has already been located, error
 *             from getting system table, or success.
 */
EFI_STATUS
EFIAPI
LocateSmmCommonCommBuffer (
)
{
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *PiSmmCommunicationRegionTable;
  EFI_MEMORY_DESCRIPTOR                     *SmmCommMemRegion;
  UINTN                                      Index, BufferSize;
  EFI_STATUS                                 Status = EFI_ABORTED;
  UINTN DesiredBufferSize;

  if (mPiSmmCommonCommBufferAddress == NULL)
  {
    Status = EfiGetSystemConfigurationTable(&gEdkiiPiSmmCommunicationRegionTableGuid, (VOID**)&PiSmmCommunicationRegionTable);
    if (EFI_ERROR(Status))
    {
      return Status;
    }

    Status = EFI_BAD_BUFFER_SIZE;

    DesiredBufferSize = sizeof(PAGE_TABLE_DUMP_COMM_BUFFER) > sizeof(PAGE_TABLE_DUMP_COMM_BUFFER2) ? sizeof(PAGE_TABLE_DUMP_COMM_BUFFER) : sizeof(PAGE_TABLE_DUMP_COMM_BUFFER2);
    DEBUG((DEBUG_ERROR, __FUNCTION__" desired comm buffer size %ld\n", DesiredBufferSize));
    BufferSize = 0;
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)(PiSmmCommunicationRegionTable + 1);
    for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++)
    {
      if (SmmCommMemRegion->Type == EfiConventionalMemory)
      {
        BufferSize = EFI_PAGES_TO_SIZE((UINTN)SmmCommMemRegion->NumberOfPages);
        if (BufferSize >= (DesiredBufferSize + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data)))
        {
          Status = EFI_SUCCESS;
          break;
        }
      }
      SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)SmmCommMemRegion + PiSmmCommunicationRegionTable->DescriptorSize);
    }

    mPiSmmCommonCommBufferAddress = (VOID*)SmmCommMemRegion->PhysicalStart;
    mPiSmmCommonCommBufferSize = BufferSize;
  }

  return Status;
} // LocateSmmCommonCommBuffer()


/**
  SmmPagingAuditAppEntryPoint

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point executed successfully.
  @retval other           Some error occured when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingAuditAppEntryPoint (
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
)
{

  mWriteString = AllocatePool(MAX_STRING_SIZE * sizeof(CHAR8));
  mBuffer = AllocatePool(MAX_STRING_SIZE * sizeof(CHAR8));
  ZeroMem(mWriteString, MAX_STRING_SIZE * sizeof(CHAR8));
  ZeroMem(mBuffer, MAX_STRING_SIZE * sizeof(CHAR8));

  TSEGDumpHandler();
  MemoryMapDumpHandler();
  LoadedImageTableDump();

  if (EFI_ERROR(LocateSmmCommonCommBuffer()))
  {
    DEBUG((DEBUG_ERROR, __FUNCTION__" Comm buffer setup failed\n"));
    return EFI_ABORTED;
  }
  SmmMemoryProtectionsDxeToSmmCommunicate();

  WriteBufferToFile(mLogFileName, mWriteString, MAX_STRING_SIZE, (mWriteCount ++));
  FreePool(mWriteString);
  FreePool(mBuffer);
  DEBUG((DEBUG_ERROR, __FUNCTION__" the apps done! \n"));
  return EFI_SUCCESS;
} // SmmPagingAuditAppEntryPoint()
