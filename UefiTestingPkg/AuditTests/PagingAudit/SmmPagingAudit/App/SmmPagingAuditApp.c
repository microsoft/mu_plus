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
#include <Register/Cpuid.h>

#include <Guid/DebugImageInfoTable.h>
#include <Guid/MemoryAttributesTable.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

#include "../SmmPagingAuditCommon.h"


VOID      *mPiSmmCommonCommBufferAddress = NULL;
UINTN     mPiSmmCommonCommBufferSize;

CHAR8     *mMemoryInfoDatabaseBuffer = NULL;
UINTN     mMemoryInfoDatabaseSize = 0;
UINTN     mMemoryInfoDatabaseAllocSize = 0;
#define   MEM_INFO_DATABASE_REALLOC_CHUNK   0x1000
#define   MEM_INFO_DATABASE_MAX_STRING_SIZE 0x400     // Should be less than the realloc chunk.


/**
 * @brief      Writes a buffer to file.
 *
 * @param      FileName     The name of the file being written to.
 * @param      Buffer       The buffer to write to file.
 * @param[in]  BufferSize   Size of the buffer.
 * @param[in]  WriteCount   Number to append to the end of the file.
 */
STATIC
VOID
WriteBufferToFile (
  IN CONST CHAR16                   *FileName,
  IN       VOID                     *Buffer,
  IN       UINTN                    BufferSize
  )
{
  EFI_STATUS          Status;
  UINTN               StringSize = BufferSize;
  SHELL_FILE_HANDLE   FileHandle;
  CHAR16              FileNameAndExt[MAX_STRING_SIZE];

  // Calculate final file name.
  ZeroMem( FileNameAndExt, sizeof(CHAR16) * MAX_STRING_SIZE );
  UnicodeSPrint( FileNameAndExt, MAX_STRING_SIZE, L"%s.dat", FileName );

  // First, let's open the file if it exists so we can delete it...
  // This is the work around for truncation
  Status = ShellOpenFileByName( FileNameAndExt, &FileHandle,
                                (EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE),
                                0 );
  if (!EFI_ERROR( Status )) {
    // If file handle above was opened it will be closed by the delete.
    Status = ShellDeleteFile( &FileHandle );
    if (EFI_ERROR( Status )) {
      DEBUG(( DEBUG_ERROR, __FUNCTION__ " failed to delete file %r\n", Status ));
    }
  }

  // Open the file and write buffer contents.
  Status = ShellOpenFileByName( FileNameAndExt, &FileHandle,
                                (EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ),
                                0 );
  if (!EFI_ERROR( Status )) {
    ShellWriteFile( FileHandle, &StringSize, Buffer );
    ShellCloseFile( &FileHandle );

    ShellPrintEx( -1, -1, L"Wrote to file %s\n", FileNameAndExt );
  }
}


/**
  This helper function writes a string entry to the memory info database buffer.
  If string would exceed current buffer allocation, it will realloc.

  NOTE: The buffer tracks its size. It does not work with NULL terminators.

  @param[in]  DatabaseString    A pointer to a CHAR8 string that should be
                                added to the database.

  @retval     EFI_SUCCESS           String was successfully added.
  @retval     EFI_OUT_OF_RESOURCES  Buffer could not be grown to accomodate string.
                                    String has not been added.

**/
STATIC
EFI_STATUS
AppendToMemoryInfoDatabase (
  IN CONST CHAR8    *DatabaseString
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN         NewStringSize, NewDatabaseSize;
  CHAR8         *NewDatabaseBuffer;

  // If the incoming string is NULL or empty, get out of here.
  if (DatabaseString == NULL || DatabaseString[0] == '\0') {
    return EFI_SUCCESS;
  }

  // Determine the length of the incoming string.
  // NOTE: This size includes the NULL terminator.
  NewStringSize = AsciiStrnSizeS( DatabaseString, MEM_INFO_DATABASE_MAX_STRING_SIZE );
  NewStringSize = NewStringSize - sizeof(CHAR8);    // Remove NULL.

  // If we need more space, realloc now.
  // Subtract 1 because we only need a single NULL terminator.
  NewDatabaseSize = NewStringSize + mMemoryInfoDatabaseSize;
  if (NewDatabaseSize > mMemoryInfoDatabaseAllocSize) {
    NewDatabaseBuffer = ReallocatePool( mMemoryInfoDatabaseAllocSize,
                                        mMemoryInfoDatabaseAllocSize + MEM_INFO_DATABASE_REALLOC_CHUNK,
                                        mMemoryInfoDatabaseBuffer );
    // If we failed, don't change anything.
    if (NewDatabaseBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    }
    // Otherwise, updated the pointers and sizes.
    else {
      mMemoryInfoDatabaseBuffer = NewDatabaseBuffer;
      mMemoryInfoDatabaseAllocSize += MEM_INFO_DATABASE_REALLOC_CHUNK;
    }
  }

  // If we're still good, copy the new string to the end of
  // the buffer and update the size.
  if (!EFI_ERROR( Status )) {
    // Subtract 1 to remove the previous NULL terminator.
    CopyMem( &mMemoryInfoDatabaseBuffer[mMemoryInfoDatabaseSize], DatabaseString, NewStringSize );
    mMemoryInfoDatabaseSize = NewDatabaseSize;
  }

  return Status;
} // AppendToMemoryInfoDatabase()


/**
  This helper function will flush the MemoryInfoDatabase to its corresponding
  file and free all resources currently associated with it.

  @param[in]  FileName    Name of the file to be flushed to.

  @retval     EFI_SUCCESS     Database has been flushed to file.

**/
STATIC
EFI_STATUS
FlushAndClearMemoryInfoDatabase (
  IN CONST CHAR16     *FileName
  )
{
  // If we have database contents, flush them to the file.
  if (mMemoryInfoDatabaseSize > 0) {
    WriteBufferToFile( FileName, mMemoryInfoDatabaseBuffer, mMemoryInfoDatabaseSize );
  }

  // If we have a database, free it, and reset all counters.
  if (mMemoryInfoDatabaseBuffer != NULL) {
    FreePool( mMemoryInfoDatabaseBuffer );
    mMemoryInfoDatabaseBuffer = NULL;
  }
  mMemoryInfoDatabaseAllocSize = 0;
  mMemoryInfoDatabaseSize = 0;

  return EFI_SUCCESS;
} // FlushAndClearMemoryInfoDatabase()


/**
 * @brief      Writes the MemoryAttributesTable to a file.
 */
VOID
EFIAPI
MemoryAttributesTableDump (
  VOID
  )
{
  EFI_STATUS                      Status;
  EFI_MEMORY_ATTRIBUTES_TABLE     *MatMap;
  EFI_MEMORY_DESCRIPTOR           *Map;
  UINT64                          EntrySize;
  UINT64                          EntryCount;
  CHAR8                           *WriteString;
  CHAR8                           *Buffer;
  UINT64                          Index;
  UINTN                           BufferSize;
  UINTN                           FormattedStringSize;
  // NOTE: Important to use fixed-size formatters for pointer movement.
  CHAR8                           MatFormatString[] = "MAT,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n";
  CHAR8                           TempString[MAX_STRING_SIZE];

  //
  // First, we need to locate the MAT table.
  //
  Status = EfiGetSystemConfigurationTable( &gEfiMemoryAttributesTableGuid, &MatMap );

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Failed to retrieve MAT %r", Status));
    return;
  }

  // MAT should now be at the pointer.
  EntrySize    = MatMap->DescriptorSize;
  EntryCount   = MatMap->NumberOfEntries;
  Map          = (VOID*)((UINT8*)MatMap + sizeof( *MatMap ));


  //
  // Next, we need to allocate a buffer to hold all of the entries.
  // We'll be storing the data as fixed-length strings.
  //
  // Do a dummy format to determine the size of a string.
  // We're safe to use 0's, since the formatters are fixed-size.
  FormattedStringSize = AsciiSPrint( TempString, MAX_STRING_SIZE, MatFormatString, 0, 0, 0, 0, 0 );
  // Make sure to add space for the NULL terminator at the end.
  BufferSize = (EntryCount * FormattedStringSize) + sizeof(CHAR8);
  Buffer = AllocatePool(BufferSize);
  if (!Buffer) {
    DEBUG((DEBUG_ERROR, "Failed to allocate buffer for data dump!"));
    return;
  }


  //
  // Add all entries to the buffer.
  //
  WriteString = Buffer;
  for (Index = 0; Index < EntryCount; Index ++)
  {
    AsciiSPrint( WriteString,
                 FormattedStringSize+1,
                 MatFormatString,
                 Map->Type,
                 Map->PhysicalStart,
                 Map->VirtualStart,
                 Map->NumberOfPages,
                 Map->Attribute );

    WriteString += FormattedStringSize;
    Map = NEXT_MEMORY_DESCRIPTOR( Map, EntrySize );
  }


  //
  // Finally, write the strings to the dump file.
  //
  // NOTE: Don't need to save the NULL terminator.
  WriteBufferToFile( L"MAT", Buffer, BufferSize-1 );

  FreePool(Buffer);
}


/**
 * @brief      Writes the name, base, and limit of each image in the image table to a file.
 */
VOID
EFIAPI
LoadedImageTableDump (
  VOID
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
  CHAR8                                       TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // locate DebugImageInfoTable
  //
  Status = EfiGetSystemConfigurationTable( &gEfiDebugImageInfoTableGuid, (VOID**)&TableHeader );
  if (EFI_ERROR( Status )) {
    DEBUG(( DEBUG_ERROR, "Failed to retrieve loaded image table %r", Status ));
    return;
  }

  Table = TableHeader->EfiDebugImageInfoTable;
  TableSize = TableHeader->TableSize;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"\n\nLength %lx Start x0x%016lx\n\n", TableHeader->TableSize, Table ));

  for (Index = 0; Index < TableSize; Index ++) {
    if (Table[Index].NormalImage == NULL) {
      continue;
    }

    NormalImage = Table[Index].NormalImage;
    LoadedImageProtocolInstance = NormalImage->LoadedImageProtocolInstance;
    ImageSize = LoadedImageProtocolInstance->ImageSize;
    ImageBase = (UINT64) LoadedImageProtocolInstance->ImageBase;

    if (ImageSize == 0) {
      // No need to register empty slots in the table as images.
      continue;
    }
    PdbFileName = PeCoffLoaderGetPdbPointer( LoadedImageProtocolInstance->ImageBase );
    AsciiSPrint( TempString, MAX_STRING_SIZE,
                 "LoadedImage,0x%016lx,0x%016lx,%a\n",
                 ImageBase,
                 ImageSize,
                 PdbFileName );
    AppendToMemoryInfoDatabase( TempString );
  }
}


/**
  This helper function will call to the SMM agent to retrieve the entire contents of the
  SMM Loaded Image protocol list. It will then dump this data to the Memory Info Database.

  Will do nothing if all inputs are not provided.

  @param[in]  SmmCommunication    A pointer to the SmmCommunication protocol.
  @param[in]  CommBufferBase      A pointer to the base of the buffer that should be used
                                  for SMM communication.
  @param[in]  CommBufferSize      The size of the buffer.

**/
STATIC
VOID
SmmLoadedImageTableDump (
  IN EFI_SMM_COMMUNICATION_PROTOCOL     *SmmCommunication,
  IN VOID                               *CommBufferBase,
  IN UINTN                              CommBufferSize
  )
{
  EFI_STATUS                                Status;
  EFI_SMM_COMMUNICATE_HEADER                *CommHeader;
  SMM_PAGE_AUDIT_COMM_HEADER                *AuditCommHeader;
  SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER      *AuditCommData;
  UINTN                                     MinBufferSize, BufferSize;
  UINTN                                     Index;
  CHAR8                                     TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Bad parameters. This shouldn't happen.\n" ));
    return;
  }

  //
  // Prep the buffer for sending the required commands to SMM.
  //
  ZeroMem( CommBufferBase, CommBufferSize );
  CommHeader = CommBufferBase;
  AuditCommHeader = (VOID*)((UINTN)CommHeader + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data));
  AuditCommData = (VOID*)((UINTN)AuditCommHeader + sizeof(SMM_PAGE_AUDIT_COMM_HEADER));
  CopyGuid( &CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid );
  CommHeader->MessageLength = MinBufferSize - OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);

  AuditCommHeader->RequestType = SMM_PAGE_AUDIT_MISC_DATA_REQUEST;
  AuditCommHeader->RequestIndex = 0;

  //
  // Repeatedly call to SMM and copy the data, if present.
  //
  do
  {
    AuditCommData->HasMore = FALSE;
    BufferSize = CommBufferSize;

    //
    // Signal trip to SMM.
    //
    Status = SmmCommunication->Communicate( SmmCommunication,
                                            CommBufferBase,
                                            &BufferSize );

    //
    // Get the data out of the comm buffer.
    //
    for (Index = 0; Index < AuditCommData->SmmImageCount; Index++) {
      AsciiSPrint( &TempString[0], MAX_STRING_SIZE,
                   "SmmLoadedImage,0x%016lx,0x%016lx,%a\n",
                   AuditCommData->SmmImage[Index].ImageBase,
                   AuditCommData->SmmImage[Index].ImageSize,
                   &AuditCommData->SmmImage[Index].ImageName[0] );
      AppendToMemoryInfoDatabase( &TempString[0] );
    }

    AuditCommHeader->RequestIndex++;
  } while (AuditCommData->HasMore);

  return;
} // SmmLoadedImageTableDump()



/**
 * @brief      Writes the UEFI memory map to file.
 */
STATIC
VOID
MemoryMapDumpHandler (
  VOID
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
  CHAR8                       TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Get the EFI memory map.
  //
  EfiMemoryMapSize  = 0;
  EfiMemoryMap      = NULL;
  Status = gBS->GetMemoryMap( &EfiMemoryMapSize,
                              EfiMemoryMap,
                              &EfiMapKey,
                              &EfiDescriptorSize,
                              &EfiDescriptorVersion );
  //
  // Loop to allocate space for the memory map and then copy it in.
  //
  do {
    EfiMemoryMap = (EFI_MEMORY_DESCRIPTOR*)AllocateZeroPool( EfiMemoryMapSize );
    ASSERT( EfiMemoryMap != NULL );
    Status = gBS->GetMemoryMap( &EfiMemoryMapSize,
                                EfiMemoryMap,
                                &EfiMapKey,
                                &EfiDescriptorSize,
                                &EfiDescriptorVersion );
    if (EFI_ERROR( Status )) {
      FreePool( EfiMemoryMap );
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  EfiMemoryMapEnd = (EFI_MEMORY_DESCRIPTOR*)((UINT8*)EfiMemoryMap + EfiMemoryMapSize);
  EfiMemNext = EfiMemoryMap;

  while (EfiMemNext < EfiMemoryMapEnd) {
    AsciiSPrint( TempString, MAX_STRING_SIZE,
                 "MemoryMap,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
                 EfiMemNext->Type,
                 EfiMemNext->PhysicalStart,
                 EfiMemNext->VirtualStart,
                 EfiMemNext->NumberOfPages,
                 EfiMemNext->Attribute );
    AppendToMemoryInfoDatabase( TempString );
    EfiMemNext = NEXT_MEMORY_DESCRIPTOR( EfiMemNext, EfiDescriptorSize );
  }

  if (EfiMemoryMap) {
    FreePool( EfiMemoryMap );
  }
}


/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  Copied from UefiCpuPkg MtrrLib

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
STATIC
VOID
InitializeMtrrMask (
  OUT UINT64 *MtrrValidBitsMask,
  OUT UINT64 *MtrrValidAddressMask
  )
{
  UINT32                          MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;


  AsmCpuid( CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL );

  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid( CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL );
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  *MtrrValidBitsMask = LShiftU64( 1, VirPhyAddressSize.Bits.PhysicalAddressBits ) - 1;
  *MtrrValidAddressMask = *MtrrValidBitsMask & 0xfffffffffffff000ULL;
}


STATIC
EFI_STATUS
TSEGDumpHandler (
  VOID
  )
{
  UINT64      SmrrBase;
  UINT64      SmrrMask;
  UINT64      Length; 
  UINT64      MtrrValidBitsMask; 
  UINT64      MtrrValidAddressMask;  
  CHAR8       TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  MtrrValidBitsMask = 0;
  MtrrValidAddressMask = 0;

  InitializeMtrrMask( &MtrrValidBitsMask, &MtrrValidAddressMask );

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"MTRR valid bits 0x%016lx, address mask: 0x%016lx\n", MtrrValidBitsMask , MtrrValidAddressMask ));

  // This is a 64-bit read, but the SMRR registers bits 63:32 are reserved.
  SmrrBase = AsmReadMsr64( MSR_IA32_SMRR_PHYSBASE );
  SmrrMask = AsmReadMsr64( MSR_IA32_SMRR_PHYSMASK );
  // Extend the mask to account for the reserved bits.
  SmrrMask |= 0xffffffff00000000ULL;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"SMRR base 0x%016lx, mask: 0x%016lx\n", SmrrBase , SmrrMask ));

  // Extend the top bits of the mask to account for the reserved

  Length = ((~(SmrrMask & MtrrValidAddressMask)) & MtrrValidBitsMask) + 1;

  DEBUG(( DEBUG_VERBOSE, __FUNCTION__"Calculated length: 0x%016lx\n", Length ));

  // Writing this out in the format of a Memory Map entry (Type 16 will map to TSEG)
  AsciiSPrint( TempString, MAX_STRING_SIZE,
               "TSEG,0x%016lx,0x%016lx,0x%016lx,0x%016lx,0x%016lx\n",
               16,
               (SmrrBase & MtrrValidAddressMask),
               0,
               EFI_SIZE_TO_PAGES( Length ),
               0 );
  AppendToMemoryInfoDatabase( TempString );

  return EFI_SUCCESS;
}


/**
  This helper function will call to the SMM agent to retrieve the entire contents of the
  SMM Page Tables. It will then dump those tables to files differentiated by the
  page size (1G, 2M, 4K).

  Will do nothing if all inputs are not provided.

  @param[in]  SmmCommunication    A pointer to the SmmCommunication protocol.
  @param[in]  CommBufferBase      A pointer to the base of the buffer that should be used
                                  for SMM communication.
  @param[in]  CommBufferSize      The size of the buffer.

**/
STATIC
VOID
SmmPageTableEntriesDump (
  IN EFI_SMM_COMMUNICATION_PROTOCOL     *SmmCommunication,
  IN VOID                               *CommBufferBase,
  IN UINTN                              CommBufferSize
  )
{
  EFI_STATUS                                Status;
  EFI_SMM_COMMUNICATE_HEADER                *CommHeader;
  SMM_PAGE_AUDIT_COMM_HEADER                *AuditCommHeader;
  SMM_PAGE_AUDIT_TABLE_ENTRY_COMM_BUFFER    *AuditCommData;
  UINTN                                     MinBufferSize, BufferSize;
  UINTN                                     NewCount, NewSize;
  UINTN                                     Pte1GCount = 0;
  UINTN                                     Pte2MCount = 0;
  UINTN                                     Pte4KCount = 0;
  PAGE_TABLE_1G_ENTRY                       *Pte1GEntries = NULL;
  PAGE_TABLE_ENTRY                          *Pte2MEntries = NULL;
  PAGE_TABLE_4K_ENTRY                       *Pte4KEntries = NULL;

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_TABLE_ENTRY_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Bad parameters. This shouldn't happen.\n" ));
    return;
  }

  //
  // Prep the buffer for sending the required commands to SMM.
  //
  ZeroMem( CommBufferBase, CommBufferSize );
  CommHeader = CommBufferBase;
  AuditCommHeader = (VOID*)((UINTN)CommHeader + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data));
  AuditCommData = (VOID*)((UINTN)AuditCommHeader + sizeof(SMM_PAGE_AUDIT_COMM_HEADER));
  CopyGuid( &CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid );
  CommHeader->MessageLength = MinBufferSize - OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);

  AuditCommHeader->RequestType = SMM_PAGE_AUDIT_TABLE_REQUEST;
  AuditCommHeader->RequestIndex = 0;

  //
  // Repeatedly call to SMM and copy the data, if present.
  //
  do
  {
    AuditCommData->HasMore = FALSE;
    BufferSize = CommBufferSize;

    //
    // Signal trip to SMM
    //
    Status = SmmCommunication->Communicate( SmmCommunication,
                                            CommBufferBase,
                                            &BufferSize );

    //
    // Get the data out of the comm buffer.
    //
    if (AuditCommData->Pte1GCount > 0) {
      NewCount = Pte1GCount + AuditCommData->Pte1GCount;
      NewSize = NewCount * sizeof(PAGE_TABLE_1G_ENTRY);
      Pte1GEntries = ReallocatePool( Pte1GCount * sizeof(PAGE_TABLE_1G_ENTRY), NewSize, Pte1GEntries );
      if (Pte1GEntries == NULL) {
        goto Cleanup;
      }
      CopyMem( &Pte1GEntries[Pte1GCount], &AuditCommData->Pte1G[0], AuditCommData->Pte1GCount * sizeof(PAGE_TABLE_1G_ENTRY) );
      Pte1GCount = NewCount;
    }
    if (AuditCommData->Pte2MCount > 0) {
      NewCount = Pte2MCount + AuditCommData->Pte2MCount;
      NewSize = NewCount * sizeof(PAGE_TABLE_ENTRY);
      Pte2MEntries = ReallocatePool( Pte2MCount * sizeof(PAGE_TABLE_ENTRY), NewSize, Pte2MEntries );
      if (Pte2MEntries == NULL) {
        goto Cleanup;
      }
      CopyMem( &Pte2MEntries[Pte2MCount], &AuditCommData->Pte2M[0], AuditCommData->Pte2MCount * sizeof(PAGE_TABLE_ENTRY) );
      Pte2MCount = NewCount;
    }
    if (AuditCommData->Pte4KCount > 0) {
      NewCount = Pte4KCount + AuditCommData->Pte4KCount;
      NewSize = NewCount * sizeof(PAGE_TABLE_4K_ENTRY);
      Pte4KEntries = ReallocatePool( Pte4KCount * sizeof(PAGE_TABLE_4K_ENTRY), NewSize, Pte4KEntries );
      if (Pte4KEntries == NULL) {
        goto Cleanup;
      }
      CopyMem( &Pte4KEntries[Pte4KCount], &AuditCommData->Pte4K[0], AuditCommData->Pte4KCount * sizeof(PAGE_TABLE_4K_ENTRY) );
      Pte4KCount = NewCount;
    }

    AuditCommHeader->RequestIndex++;
  } while (AuditCommData->HasMore);

  //
  // Write data from the comm buffer to file.
  //
  WriteBufferToFile( L"1G", Pte1GEntries, Pte1GCount * sizeof(PAGE_TABLE_1G_ENTRY) );
  WriteBufferToFile( L"2M", Pte2MEntries, Pte2MCount * sizeof(PAGE_TABLE_ENTRY) );
  WriteBufferToFile( L"4K", Pte4KEntries, Pte4KCount * sizeof(PAGE_TABLE_4K_ENTRY) );

Cleanup:
  // Always put away your toys.
  if (Pte1GEntries != NULL) {
    FreePool( Pte1GEntries );
  }
  if (Pte2MEntries != NULL) {
    FreePool( Pte2MEntries );
  }
  if (Pte4KEntries != NULL) {
    FreePool( Pte4KEntries );
  }

  return;
} // SmmPageTableEntriesDump()


/**
  This helper function will call to the SMM agent to retrieve all the Page Table Directory entries.
  It will then dump those tables to a file.

  Will do nothing if all inputs are not provided.

  @param[in]  SmmCommunication    A pointer to the SmmCommunication protocol.
  @param[in]  CommBufferBase      A pointer to the base of the buffer that should be used
                                  for SMM communication.
  @param[in]  CommBufferSize      The size of the buffer.

**/
STATIC
VOID
SmmPdeEntriesDump (
  IN EFI_SMM_COMMUNICATION_PROTOCOL     *SmmCommunication,
  IN VOID                               *CommBufferBase,
  IN UINTN                              CommBufferSize
  )
{
  EFI_STATUS                                Status;
  EFI_SMM_COMMUNICATE_HEADER                *CommHeader;
  SMM_PAGE_AUDIT_COMM_HEADER                *AuditCommHeader;
  SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER      *AuditCommData;
  UINTN                                     MinBufferSize, BufferSize;
  UINTN                                     Index;
  CHAR8                                     TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - Bad parameters. This shouldn't happen.\n" ));
    return;
  }

  //
  // Prep the buffer for sending the required commands to SMM.
  //
  ZeroMem( CommBufferBase, CommBufferSize );
  CommHeader = CommBufferBase;
  AuditCommHeader = (VOID*)((UINTN)CommHeader + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data));
  AuditCommData = (VOID*)((UINTN)AuditCommHeader + sizeof(SMM_PAGE_AUDIT_COMM_HEADER));
  CopyGuid( &CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid );
  CommHeader->MessageLength = MinBufferSize - OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);

  AuditCommHeader->RequestType = SMM_PAGE_AUDIT_PDE_REQUEST;
  AuditCommHeader->RequestIndex = 0;

  //
  // Repeatedly call to SMM and copy the data, if present.
  //
  do
  {
    AuditCommData->HasMore = FALSE;
    BufferSize = CommBufferSize;

    //
    // Signal trip to SMM.
    //
    Status = SmmCommunication->Communicate( SmmCommunication,
                                            CommBufferBase,
                                            &BufferSize );

    //
    // Get the data out of the comm buffer.
    //
    for (Index = 0; Index < AuditCommData->PdeCount; Index++) {
      AsciiSPrint( &TempString[0], MAX_STRING_SIZE,
                   "PDE,0x%lx,0x%lx\n",
                   AuditCommData->Pde[Index],
                   512 ); // 512 is the size of a Page Directory
      AppendToMemoryInfoDatabase( &TempString[0] );
    }

    AuditCommHeader->RequestIndex++;
  } while (AuditCommData->HasMore);

  return;
} // SmmPdeEntriesDump()


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
  VOID
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_SMM_COMMUNICATION_PROTOCOL          *SmmCommunication = NULL;
  VOID                                    *CommBufferBase;
  EFI_SMM_COMMUNICATE_HEADER              *CommHeader;
  SMM_PAGE_AUDIT_COMM_HEADER              *AuditCommHeader;
  SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER    *AuditCommData;
  UINTN                                   MinBufferSize, BufferSize;
  CHAR8                                   TempString[MAX_STRING_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Make sure that we have access to a buffer that seems to be sufficient to do everything we need to do.
  //
  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG((DEBUG_ERROR, __FUNCTION__" - Communication mBuffer not found!\n"));
    return EFI_ABORTED;
  }
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) + sizeof(SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER);
  if (MinBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG((DEBUG_ERROR, __FUNCTION__" - Communication mBuffer is too small\n"));
    return EFI_BUFFER_TOO_SMALL;
  }
  CommBufferBase = mPiSmmCommonCommBufferAddress;

  //
  // Locate the protocol as needed.
  //
  if (SmmCommunication == NULL) {
    Status = gBS->LocateProtocol(&gEfiSmmCommunicationProtocolGuid, NULL, (VOID**) &SmmCommunication);
    if (EFI_ERROR( Status )) {
      return Status;
    }
  }

  //
  // Call all related handlers.
  //
  SmmPageTableEntriesDump( SmmCommunication, mPiSmmCommonCommBufferAddress, mPiSmmCommonCommBufferSize );
  SmmPdeEntriesDump( SmmCommunication, mPiSmmCommonCommBufferAddress, mPiSmmCommonCommBufferSize );
  SmmLoadedImageTableDump( SmmCommunication, mPiSmmCommonCommBufferAddress, mPiSmmCommonCommBufferSize );

  //
  // Prep the buffer for getting the last of the misc data.
  //
  ZeroMem( CommBufferBase, MinBufferSize );
  CommHeader = CommBufferBase;
  AuditCommHeader = (VOID*)((UINTN)CommHeader + OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data));
  AuditCommData = (VOID*)((UINTN)AuditCommHeader + sizeof(SMM_PAGE_AUDIT_COMM_HEADER));
  CopyGuid( &CommHeader->HeaderGuid, &gSmmPagingAuditSmiHandlerGuid );
  CommHeader->MessageLength = MinBufferSize - OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data);

  AuditCommHeader->RequestType = SMM_PAGE_AUDIT_MISC_DATA_REQUEST;
  AuditCommHeader->RequestIndex = 0;
  AuditCommData->HasMore = FALSE;
  BufferSize = MinBufferSize;

  //
  // Signal trip to SMM.
  //
  Status = SmmCommunication->Communicate( SmmCommunication,
                                          CommBufferBase,
                                          &BufferSize );

  //
  // Add remaining misc data to the database.
  //
  AsciiSPrint( &TempString[0], MAX_STRING_SIZE,
               "GDT,0x%016lx,0x%016lx\nIDT,0x%016lx,0x%016lx\n",
               AuditCommData->Gdtr.Base, (UINT64)AuditCommData->Gdtr.Limit,
               AuditCommData->Idtr.Base, (UINT64)AuditCommData->Idtr.Limit );
  AppendToMemoryInfoDatabase( &TempString[0] );

  //
  // Clean up the SMM cache.
  //
  AuditCommHeader->RequestType = SMM_PAGE_AUDIT_CLEAR_DATA_REQUEST;
  AuditCommHeader->RequestIndex = 0;
  BufferSize = MinBufferSize;
  Status = SmmCommunication->Communicate( SmmCommunication,
                                          CommBufferBase,
                                          &BufferSize );

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
  VOID
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

    DesiredBufferSize = sizeof(SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER);
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
  TSEGDumpHandler();
  MemoryMapDumpHandler();
  LoadedImageTableDump();
  MemoryAttributesTableDump();

  if (EFI_ERROR( LocateSmmCommonCommBuffer() )) {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" Comm buffer setup failed\n" ));
    return EFI_ABORTED;
  }
  SmmMemoryProtectionsDxeToSmmCommunicate();

  FlushAndClearMemoryInfoDatabase( L"MemoryInfoDatabase" );

  DEBUG(( DEBUG_INFO, __FUNCTION__" the app's done!\n" ));

  return EFI_SUCCESS;
} // SmmPagingAuditAppEntryPoint()
