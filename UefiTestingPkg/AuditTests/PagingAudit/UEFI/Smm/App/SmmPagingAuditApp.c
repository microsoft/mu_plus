/** @file -- SmmPagingAuditApp.c
This user-facing application collects information from the SMM page tables and
writes it to files.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

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

#include "../../PagingAuditCommon.h"
#include "../SmmPagingAuditCommon.h"


VOID      *mPiSmmCommonCommBufferAddress = NULL;
UINTN     mPiSmmCommonCommBufferSize;

CHAR8     *mMemoryInfoDatabaseBuffer = NULL;
UINTN     mMemoryInfoDatabaseSize = 0;
UINTN     mMemoryInfoDatabaseAllocSize = 0;

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

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_MISC_DATA_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, "%a - Bad parameters. This shouldn't happen.\n", __FUNCTION__ ));
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

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_TABLE_ENTRY_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, "%a - Bad parameters. This shouldn't happen.\n", __FUNCTION__ ));
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
    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "%a - SmmCommunication errored - %r.\n", __FUNCTION__, Status));
      goto Cleanup;
    }
    //
    // Get the data out of the comm buffer.
    //
    if (AuditCommData->Pte1GCount > 0) {
      NewCount = Pte1GCount + AuditCommData->Pte1GCount;
      NewSize = NewCount * sizeof(PAGE_TABLE_1G_ENTRY);
      Pte1GEntries = ReallocatePool( Pte1GCount * sizeof(PAGE_TABLE_1G_ENTRY), NewSize, Pte1GEntries );
      if (Pte1GEntries == NULL) {
        DEBUG(( DEBUG_ERROR, "%a - 1G entries not allocated.\n", __FUNCTION__ ));
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
        DEBUG(( DEBUG_ERROR, "%a - 2M entries not allocated.\n", __FUNCTION__ ));
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
        DEBUG(( DEBUG_ERROR, "%a - 4K entries not allocated.\n", __FUNCTION__ ));
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

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  //
  // Check to make sure we have what we need.
  //
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) +
                  sizeof(SMM_PAGE_AUDIT_COMM_HEADER) +
                  sizeof(SMM_PAGE_AUDIT_PDE_ENTRY_COMM_BUFFER);
  if (SmmCommunication == NULL || CommBufferBase == NULL || CommBufferSize < MinBufferSize) {
    DEBUG(( DEBUG_ERROR, "%a - Bad parameters. This shouldn't happen.\n", __FUNCTION__ ));
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

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__ ));

  //
  // Make sure that we have access to a buffer that seems to be sufficient to do everything we need to do.
  //
  if (mPiSmmCommonCommBufferAddress == NULL) {
    DEBUG((DEBUG_ERROR, "%a - Communication mBuffer not found!\n", __FUNCTION__ ));
    return EFI_ABORTED;
  }
  MinBufferSize = OFFSET_OF(EFI_SMM_COMMUNICATE_HEADER, Data) + sizeof(SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER);
  if (MinBufferSize > mPiSmmCommonCommBufferSize) {
    DEBUG((DEBUG_ERROR, "%a - Communication mBuffer is too small\n", __FUNCTION__ ));
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
      DEBUG((DEBUG_ERROR, "%a Failed to get system configuration table %r\n", __FUNCTION__, Status));
      return Status;
    }

    Status = EFI_BAD_BUFFER_SIZE;

    DesiredBufferSize = sizeof(SMM_PAGE_AUDIT_UNIFIED_COMM_BUFFER);
    DEBUG((DEBUG_ERROR, "%a desired comm buffer size %ld\n", __FUNCTION__, DesiredBufferSize));
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
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmPagingAuditAppEntryPoint (
  IN     EFI_HANDLE         ImageHandle,
  IN     EFI_SYSTEM_TABLE  *SystemTable
)
{
  DumpProcessorSpecificHandlers();
  MemoryMapDumpHandler();
  LoadedImageTableDump();
  MemoryAttributesTableDump();

  if (EFI_ERROR( LocateSmmCommonCommBuffer() )) {
    DEBUG(( DEBUG_ERROR, "%a Comm buffer setup failed\n", __FUNCTION__ ));
    return EFI_ABORTED;
  }
  SmmMemoryProtectionsDxeToSmmCommunicate();

  FlushAndClearMemoryInfoDatabase( L"MemoryInfoDatabase" );

  DEBUG(( DEBUG_INFO, "%a the app's done!\n", __FUNCTION__ ));

  return EFI_SUCCESS;
} // SmmPagingAuditAppEntryPoint()
