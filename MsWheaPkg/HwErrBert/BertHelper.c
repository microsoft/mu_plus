/** @file -- BertHelper 

Helper functions that take CPER record and publish each section to BERT table.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Guid/Cper.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Acpi.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Library/UefiBootServicesTableLib.h>
#include "BertHelper.h"

EFI_ACPI_TABLE_PROTOCOL              *mAcpiTableProtocol = NULL;

/**

Identify different sections in CPER and send each to AddCperSectionToBert.

@return FALSE if BootErrorRegion is out of space

**/
BOOLEAN
EFIAPI
BertAddAllCperSections (
  IN EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER *Bert,
  IN VOID                                        *ErrorData
) {
  EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr;
  EFI_ERROR_SECTION_DESCRIPTOR    *CperErrSecDscp;
  UINTN                            Index = 0;

  if (Bert == NULL || ErrorData == NULL) {
    DEBUG((DEBUG_ERROR, "%a - null parameter\n", __FUNCTION__));
    return FALSE;
  }

  CperHdr = (EFI_COMMON_ERROR_RECORD_HEADER*)ErrorData;
  CperErrSecDscp = (EFI_ERROR_SECTION_DESCRIPTOR *) (CperHdr + 1);

  do{
    if (!BertAddCperSection(Bert, CperHdr, CperErrSecDscp)) {
      // Boot Error Region is out of space!
      return FALSE;
    }
    CperErrSecDscp ++;
    Index ++;
    DEBUG((DEBUG_VERBOSE, "%a %d - Section %d of %d \n", __FUNCTION__, __LINE__, Index, CperHdr->SectionCount));
  } while(Index < CperHdr->SectionCount);

  return TRUE;
}


/**

Take CPER header and section descriptor,
use that to form a generic data entry,
then stuff that entry in BERT Boot Error Region.

**/
BOOLEAN
EFIAPI
BertAddCperSection (
  IN EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER *Bert,
  IN EFI_COMMON_ERROR_RECORD_HEADER              *CperHdr,
  IN EFI_ERROR_SECTION_DESCRIPTOR                *CperErrSecDscp
) {
    return BertErrorBlockAddErrorData (
      (VOID*)Bert->BootErrorRegion,
      Bert->BootErrorRegionLength,
      &CperErrSecDscp->SectionType,
      (VOID*) (((UINT8*) CperHdr) + (CperErrSecDscp->SectionOffset)),
      CperErrSecDscp->SectionLength,
      CperErrSecDscp->Severity,
      TRUE // correctable
    );
}


/**

Allocate and populate EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER
in the BERT_CONTEXT that was provided.

**/
VOID
EFIAPI
BertHeaderCreator (
  IN BERT_CONTEXT  *Context,
  IN UINT32        ErrorBlockSize
)
{
  if (Context == NULL) {
    return;
  }
  Context->BertHeader = AllocateZeroPool (sizeof(EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER));
  Context->Block = AllocateReservedZeroPool (ErrorBlockSize);
  Context->BlockSize = ErrorBlockSize;
  Context->BertHeader->Header.Signature = EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_SIGNATURE;
  Context->BertHeader->Header.Length = sizeof(EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER);
  Context->BertHeader->Header.Revision = EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_REVISION;
  Context->BertHeader->Header.OemTableId = PcdGet64(PcdAcpiDefaultOemTableId);
  Context->BertHeader->Header.CreatorId = PcdGet32(PcdAcpiDefaultCreatorId);
  Context->BertHeader->Header.CreatorRevision = PcdGet32(PcdAcpiDefaultOemRevision);
  CopyMem(Context->BertHeader->Header.OemId, PcdGetPtr(PcdAcpiDefaultOemId), sizeof(Context->BertHeader->Header.OemId));
  Context->BertHeader->BootErrorRegionLength = Context->BlockSize;
  Context->BertHeader->BootErrorRegion = (UINT64) Context->Block;
}


/**

Calculate BERT header checksum and publish BERT table via mAcpiTableProtocol

**/
EFI_STATUS
BertSetAcpiTable (
  IN BERT_CONTEXT *Context
)
{
  UINTN                                        AcpiTableHandle;
  EFI_STATUS                                   Status;
  EFI_ACPI_6_1_BOOT_ERROR_RECORD_TABLE_HEADER* Bert;
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Bert = Context->BertHeader;
  Bert->Header.Checksum = CalculateCheckSum8 ((UINT8*)(Bert), Bert->Header.Length);
  AcpiTableHandle = 0;

  Status = gBS->LocateProtocol (
                    &gEfiAcpiTableProtocolGuid,
                    NULL,
                    (VOID**)&mAcpiTableProtocol);

  if (!EFI_ERROR(Status)) {
    Status = mAcpiTableProtocol->InstallAcpiTable (
                                   mAcpiTableProtocol,
                                   Bert,
                                   Bert->Header.Length,
                                   &AcpiTableHandle);
  }
  return Status;
}

/**

Create initial block in BERTs Boot Error Region.

**/
EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE*
BertErrorBlockInitial (
  VOID   *Block,
  UINT32 Severity
)
{
  EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE* BlockHeader = Block;
  BlockHeader->BlockStatus = (EFI_ACPI_6_1_ERROR_BLOCK_STATUS) {0, 0, 0, 0, 0};
  BlockHeader->RawDataOffset = 0;
  BlockHeader->RawDataLength = 0;
  BlockHeader->DataLength = 0;
  BlockHeader->ErrorSeverity = Severity;
  return BlockHeader;
}


/**

Add Error Block to BERT table's Boot Error Region

@return FALSE if ExpectedNewDataLength > MaxBlockLength; TRUE otherwise

**/
BOOLEAN
BertErrorBlockAddErrorData (
  IN VOID                  *ErrorBlock,
  IN UINT32                MaxBlockLength,
  IN EFI_GUID              *Guid,
  IN VOID                  *GenericErrorData,
  IN UINT32                SizeOfGenericErrorData,
  IN UINT32                ErrorSeverity,
  IN BOOLEAN               Correctable
)
{
  VOID                                              *GenericErrorDataFollowEntry;
  EFI_ACPI_6_1_ERROR_BLOCK_STATUS                   *BlockStatus;
  EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE       *BlockHeader;
  EFI_ACPI_6_1_GENERIC_ERROR_DATA_ENTRY_STRUCTURE   *Entry;

  if (ErrorBlock == NULL || GenericErrorData == NULL) {
    DEBUG((DEBUG_ERROR, "%a - %d: Invalid Param \n", __FUNCTION__, __LINE__));
    return FALSE;
  }
  DEBUG((DEBUG_VERBOSE, "%a - %d: Dumping GenericErrorData contents: \n", __FUNCTION__, __LINE__));
  DEBUG_BUFFER(DEBUG_VERBOSE, GenericErrorData, SizeOfGenericErrorData, DEBUG_DM_PRINT_ADDRESS | DEBUG_DM_PRINT_ASCII);

  // Setup BERT structures
  BlockHeader = ErrorBlock;
  BlockStatus = &BlockHeader->BlockStatus;

  // Calculate length of BERT error region (including new entry)
  UINT32 ExpectedNewDataLength = BlockHeader->DataLength +
                                 sizeof(EFI_ACPI_6_1_GENERIC_ERROR_DATA_ENTRY_STRUCTURE) +
                                 SizeOfGenericErrorData;
  // Fail if we don't have room
  if (sizeof(EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE) + ExpectedNewDataLength > MaxBlockLength) {
    return FALSE;
  }

  // Set BlockStatus Correctable/Uncorrectable fields
  if (Correctable == TRUE) {
    if (BlockStatus->CorrectableErrorValid == 0) {
      BlockStatus->CorrectableErrorValid = 1;
    } else {
      BlockStatus->MultipleCorrectableErrors = 1;
    }
  } else {
    if (BlockStatus->UncorrectableErrorValid == 0) {
      BlockStatus->UncorrectableErrorValid = 1;
    } else {
      BlockStatus->MultipleUncorrectableErrors = 1;
    }
  }

  // Setup Generic Error Data Entry with the values that were passed in
  BlockStatus->ErrorDataEntryCount++;
  Entry = (EFI_ACPI_6_1_GENERIC_ERROR_DATA_ENTRY_STRUCTURE*)(((UINT8*)ErrorBlock) +
           sizeof(EFI_ACPI_6_1_GENERIC_ERROR_STATUS_STRUCTURE) +
           BlockHeader->DataLength);
  
  // Setup Entry header
  gBS->SetMem (Entry, sizeof(EFI_ACPI_6_1_GENERIC_ERROR_DATA_ENTRY_STRUCTURE), 0);
  gBS->CopyMem (&Entry->SectionType, Guid, sizeof(EFI_GUID));
  Entry->ErrorSeverity = ErrorSeverity;
  Entry->Revision = EFI_ACPI_6_1_GENERIC_ERROR_DATA_ENTRY_REVISION;
  Entry->ErrorDataLength = SizeOfGenericErrorData;

  // Copy data right after header
  GenericErrorDataFollowEntry = (VOID*)(Entry + 1);
  gBS->CopyMem (
         GenericErrorDataFollowEntry,
         GenericErrorData,
         SizeOfGenericErrorData);
  
  // Setup the header with the new size
  BlockHeader->DataLength = ExpectedNewDataLength;

  return TRUE;
}