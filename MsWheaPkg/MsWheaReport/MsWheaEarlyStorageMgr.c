/** @file -- MsWheaEarlyStorageMgr.c

This source utilizes early storage APIs for MsWheaReport usage.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Pi/PiStatusCode.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PcdLib.h>
#include "MsWheaEarlyStorageMgr.h"

#ifdef INTERNAL_UNIT_TEST
#undef STATIC
#define STATIC    // Nothing...
#endif

#define MS_WHEA_EARLY_STORAGE_HEADER_SIZE     (sizeof(MS_WHEA_EARLY_STORAGE_HEADER))
#define MS_WHEA_EARLY_STORAGE_DATA_OFFSET     MS_WHEA_EARLY_STORAGE_HEADER_SIZE

/**

This is a helper function that returns the maximal capacity for header excluded data.

@retval Count    The maximum number of bytes that can be stored in the MS WHEA store.

**/
STATIC
UINT8
MsWheaESGetMaxDataCount (
  VOID
  )
{
  return (UINT8)((MsWheaEarlyStorageGetMaxSize() - (MS_WHEA_EARLY_STORAGE_DATA_OFFSET)) & 0xFF);
}

/**

This is a helper function that reads the early storage region with an offset of header size.

@param[in]  Ptr                       The pointer to hold intended read data
@param[in]  Size                      The size of intended read data
@param[in]  Offset                    The offset of read data, ranging from 0 to
                                      PcdMsWheaReportEarlyStorageCapacity - MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
MsWheaESReadData (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaESGetMaxDataCount()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = MsWheaEarlyStorageRead(Ptr, Size, MS_WHEA_EARLY_STORAGE_DATA_OFFSET + Offset);

Cleanup:
  return Status;
}


/**

This is a helper function that writes the early storage region with an offset of header size.

@param[in]  Ptr                       The pointer to hold intended written data
@param[in]  Size                      The size of intended written data
@param[in]  Offset                    The offset of written data, ranging from 0 to
                                      PcdMsWheaReportEarlyStorageCapacity - MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
MsWheaESWriteData (
  VOID                                *Ptr,
  UINT8                               Size,
  UINT8                               Offset
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaESGetMaxDataCount()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = MsWheaEarlyStorageWrite(Ptr, Size, MS_WHEA_EARLY_STORAGE_DATA_OFFSET + Offset);

Cleanup:
  return Status;
}

/**

This is a helper function that clears the early storage region with an offset of header size.

@param[in]  Size                      The size of intended clear data
@param[in]  Offset                    The offset of clear data, ranging from 0 to
                                      PcdMsWheaReportEarlyStorageCapacity - MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
MsWheaESClearData (
  UINT8 Size,
  UINT8 Offset
  )
{
  EFI_STATUS Status;
  if (Offset >= MsWheaESGetMaxDataCount()) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  Status = MsWheaEarlyStorageClear(Size, MS_WHEA_EARLY_STORAGE_DATA_OFFSET + Offset);

Cleanup:
  return Status;
}

/**

This routine dumps the contents of the early storage.

**/
VOID
EFIAPI
MsWheaESDump (
  VOID
  )
{
  UINT8   Data;
  UINT8  Index;
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, "CMOS MS WHEA Store..."));
  for (Index = 0; Index < MsWheaEarlyStorageGetMaxSize(); Index ++) {
    Status = MsWheaEarlyStorageRead(&Data, sizeof(Data), Index);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a: Reading Early Storage %d failed %r", __FUNCTION__, Index, Status));
      goto Cleanup;
    }

    if ((Index % 16) == 0) {
      DEBUG((DEBUG_INFO, "\n0x%02x: ", Index));
    }

    DEBUG((DEBUG_INFO, "%02x ", Data));
  }

Cleanup:
  DEBUG((DEBUG_INFO, "\n"));
  return;
}

/**

This routine clears all bytes in Data region

**/
STATIC
VOID
MsWheaESClearAllData (
  VOID
  )
{
  MsWheaESClearData(MsWheaESGetMaxDataCount(), 0);
}

/**

This routine reads the Early Storage MS WHEA store header.

@param Header                         Supplies a pointer that will hold the Early Storage header.

**/
STATIC
VOID
MsWheaESReadHeader (
  MS_WHEA_EARLY_STORAGE_HEADER        *Header
  )
{
  if (Header == NULL) {
    return;
  }

  MsWheaEarlyStorageRead(Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE, 0);
}

/**

This routine writes the passed Early Storage MS WHEA store header.

@param Header                         Supplies a pointer that holds the target Early Storage header.

**/
STATIC
VOID
MsWheaESWriteHeader (
  MS_WHEA_EARLY_STORAGE_HEADER        *Header
  )
{
  if (Header == NULL) {
    return;
  }

  MsWheaEarlyStorageWrite(Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE, 0);
}

/**

This routine checks the checksum of early storage region: starting from the signature of header to
the last byte of active range (excluding checksum field).

**/
STATIC
EFI_STATUS
MsWheaESChecksum16 (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header,
  UINT16                          *Checksum
  )
{
  UINT16      Data;
  UINT8       Index;
  UINT16      Sum;
  EFI_STATUS  Status;

  DEBUG((DEBUG_INFO, "%a Calculate sum...\n", __FUNCTION__));

  Status = EFI_SUCCESS;

  if ((Checksum == NULL) || (Header == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }
  else if ((Header->ActiveRange > MsWheaEarlyStorageGetMaxSize()) ||
           ((Header->ActiveRange & BIT0) != 0)) {
    Status = EFI_BAD_BUFFER_SIZE;
    goto Cleanup;
  }

  // Clear the checksum field for calculation then restore...
  *Checksum = Header->Checksum;
  Header->Checksum = 0;
  Sum = CalculateSum16 ((UINT16*)Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE);
  Header->Checksum = *Checksum;

  for (Index = 0; Index < Header->ActiveRange; Index += sizeof(Data)) {
    Status = MsWheaESReadData(&Data, sizeof(Data), Index);
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a: Reading Early Storage %d failed %r\n", __FUNCTION__, Index, Status));
      goto Cleanup;
    }
    Sum = Sum + Data;
  }

  *Checksum = (UINT16) (0x10000 - Sum);

Cleanup:
  return Status;
}

/**

This routine calculates the checksum of early storage region and update the range and checksum in
header accordingly.

**/
STATIC
VOID
MsWheaESContentChangeChecksumHelper (
  UINT16*         Buffer,
  UINTN           Length
  )
{
  UINT16    Sum16;
  MS_WHEA_EARLY_STORAGE_HEADER  Header;

  DEBUG((DEBUG_INFO, "Calculate sum content helper...\n"));

  MsWheaESReadHeader (&Header);
  Sum16 = (UINT16)(0x10000 - Header.Checksum);

  // Update length field of header, update checksum accordingly
  Sum16 = (Sum16 - (UINT16)(Header.ActiveRange & MAX_UINT16) - (UINT16)((Header.ActiveRange >> 16) & MAX_UINT16));
  Header.ActiveRange += (UINT32) Length;
  Sum16 = (Sum16 + (UINT16)(Header.ActiveRange & MAX_UINT16) + (UINT16)((Header.ActiveRange >> 16) & MAX_UINT16));

  // Calculate updated chunk only
  Sum16 = Sum16 + CalculateSum16 (Buffer, Length);
  Header.Checksum = (UINT16)(0x10000 - Sum16);

  MsWheaESWriteHeader (&Header);
}

/**

This routine calculates and updates the checksum based on the supplied header.

**/
STATIC
VOID
MsWheaESHeaderChangeChecksumHelper (
  MS_WHEA_EARLY_STORAGE_HEADER    *Header
  )
{
  UINT16    Checksum16;
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, "%a Calculate sum header helper...\n", __FUNCTION__));

  Status = MsWheaESChecksum16 (Header, &Checksum16);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "Checksum calculator failed - %r...", Status));
  }
  Header->Checksum = Checksum16;

  MsWheaESWriteHeader (Header);
}

/**

This routine returns a boolean indicating if the MS WHEA store is valid.

@retval TRUE if the MS WHEA store is valid, else FALSE.

**/
STATIC
BOOLEAN
MsWheaESRegionIsValid (
  OUT MS_WHEA_EARLY_STORAGE_HEADER *OutPutHeader OPTIONAL
  )
{
  BOOLEAN Valid;
  UINT16 Checksum16;
  EFI_STATUS Status;
  MS_WHEA_EARLY_STORAGE_HEADER Header;

  MsWheaESReadHeader(&Header);

  if (OutPutHeader != NULL) {
    CopyMem (OutPutHeader, &Header, sizeof(MS_WHEA_EARLY_STORAGE_HEADER));
  }

  Status = MsWheaESChecksum16 (&Header, &Checksum16);
  if (EFI_ERROR(Status)) {
    Valid = FALSE;
    DEBUG ((DEBUG_ERROR, "%a Checksum calculation failed %r\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  if ((Header.Signature != MS_WHEA_EARLY_STORAGE_SIGNATURE) ||
      (Checksum16 != Header.Checksum)) {
    Valid = FALSE;
    DEBUG ((DEBUG_ERROR, "%a Header verification failed: signature %08X, CS has %04X, expecting %04X\n",
                          __FUNCTION__,
                          Header.Signature,
                          Header.Checksum,
                          Checksum16));
    goto Cleanup;
  }
  else {
    DEBUG ((DEBUG_INFO, "%a Checksum all good\n", __FUNCTION__));
    Valid = TRUE;
  }

  Status = EFI_SUCCESS;

Cleanup:
  return Valid;
}

/**

This routine finds a contiguous memory that has default value of specified size in data region
from the MS WHEA store.

@param[in]  Size                      The size of intended clear data
@param[out] Offset                    The pointer to receive returned offset value, starting from
                                      Early MS_WHEA_EARLY_STORAGE_DATA_OFFSET

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_OUT_OF_RESOURCES          Null pointer or zero or over length request detected

**/
STATIC
EFI_STATUS
MsWheaESFindSlot (
  IN UINT8 Size,
  IN UINT8 *Offset
  )
{
  EFI_STATUS  Status = EFI_OUT_OF_RESOURCES;
  MS_WHEA_EARLY_STORAGE_HEADER Header;

  MsWheaESReadHeader(&Header);

  if (Header.ActiveRange + Size <= MsWheaESGetMaxDataCount()) {
    *Offset = (UINT8) Header.ActiveRange;
    Status = EFI_SUCCESS;
  }
  return Status;
}

/**

This routine initialized the Early Storage MS WHEA store.

**/
VOID
EFIAPI
MsWheaESInit (
  VOID
  )
{
  MS_WHEA_EARLY_STORAGE_HEADER  Header;

  // Check if the Early Storage has the signature and valid checksum.
  // If it does, the store has already been initialized and there's nothing more to do.
  if (MsWheaESRegionIsValid(&Header)) {
    goto Cleanup;
  }

  DEBUG((DEBUG_INFO, "%a: init early storage...\n", __FUNCTION__));

  // Clear the rest of the Early Storage store.
  MsWheaESClearAllData();

  // Zero all the fields in the
  SetMem(&Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE, 0);

  // Sign the header signature.
  Header.Signature = MS_WHEA_EARLY_STORAGE_SIGNATURE;
  Header.ActiveRange = 0;
  MsWheaESHeaderChangeChecksumHelper (&Header);

Cleanup:
  MsWheaESDump();
  return;
}

/**
This routine will extract necessary Rev 0 information from supplied metadata and store onto the next
contiguously available Early Storage data region

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See MsWheaESFindSlot and MsWheaESWriteData for more details
**/
STATIC
EFI_STATUS
MsWheaESV0InfoStore (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  UINT8                           Offset = 0;
  EFI_STATUS                      Status;
  MS_WHEA_EARLY_STORAGE_ENTRY_V0  WheaV0;

  Status = MsWheaESFindSlot(sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), &Offset);
  if (EFI_ERROR(Status) != FALSE) {
    goto Cleanup;
  }

  SetMem(&WheaV0, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), 0);

  WheaV0.Rev = MsWheaEntryMD->Rev;
  WheaV0.Phase = MsWheaEntryMD->Phase;
  WheaV0.ErrorStatusValue = MsWheaEntryMD->ErrorStatusValue;
  WheaV0.AdditionalInfo1 = MsWheaEntryMD->AdditionalInfo1;
  WheaV0.AdditionalInfo2 = MsWheaEntryMD->AdditionalInfo2;
  CopyMem(&WheaV0.PartitionID, &MsWheaEntryMD->IhvSharingGuid, sizeof(EFI_GUID));
  CopyMem(&WheaV0.ModuleID, &MsWheaEntryMD->ModuleID, sizeof(EFI_GUID));

  Status = MsWheaESWriteData(&WheaV0, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), Offset);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: Clear V0 Early Storage failed at %d %r\n", __FUNCTION__, Offset, Status));
    goto Cleanup;
  }

  MsWheaESContentChangeChecksumHelper((UINT16*)&WheaV0, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0));

Cleanup:
  return Status;
}

/**
This routine will read Rev 0 information from specified offset on Early Storage data region and fill in the
metadata based on read value

@param[out] MsWheaEntryMD             The pointer to reported MS WHEA error metadata
@param[in, out] Offset                The pointer to specified offset, will be updated to the end of
                                      processed data upon successful operation

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer detected
@retval Others                        See MsWheaESFindSlot and MsWheaESReadData for more
                                      details
**/
STATIC
EFI_STATUS
MsWheaESGetV0Info (
  OUT MS_WHEA_ERROR_ENTRY_MD          *MsWheaEntryMD,
  IN OUT UINT8                        *Offset
  )
{
  EFI_STATUS                      Status;
  MS_WHEA_EARLY_STORAGE_ENTRY_V0  WheaV0;

  if (MsWheaEntryMD == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(&WheaV0, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), 0);
  SetMem(MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);

  Status = MsWheaESReadData(&WheaV0, sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), *Offset);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: Read V0 Early Storage storage failed at %d %r\n", __FUNCTION__, Offset, Status));
    goto Cleanup;
  }

  MsWheaEntryMD->Rev = WheaV0.Rev;
  MsWheaEntryMD->Phase = WheaV0.Phase;
  MsWheaEntryMD->ErrorStatusValue = WheaV0.ErrorStatusValue;
  MsWheaEntryMD->ErrorSeverity = EFI_GENERIC_ERROR_FATAL;
  MsWheaEntryMD->AdditionalInfo1 = WheaV0.AdditionalInfo1;
  MsWheaEntryMD->AdditionalInfo2 = WheaV0.AdditionalInfo2;
  MsWheaEntryMD->PayloadSize = sizeof(MS_WHEA_ERROR_ENTRY_MD);
  CopyMem(&MsWheaEntryMD->IhvSharingGuid, &WheaV0.PartitionID, sizeof(EFI_GUID));
  CopyMem(&MsWheaEntryMD->ModuleID, &WheaV0.ModuleID, sizeof(EFI_GUID));

  Status = MsWheaESClearData(sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0), *Offset);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: Clear V0 Early Storage storage failed at %d %r\n", __FUNCTION__, Offset, Status));
    goto Cleanup;
  }
  *Offset = *Offset + sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_V0);

Cleanup:
  return Status;
}

/**

This routine will Set Early Storage header IsStorageFull flag and mark the system stage if the signature is
legit

@param[in] Phase                      Current environment stage by the time of report

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_NOT_FOUND                 Early Storage header region is not valid
@retval Others                        See MsWheaESFindSlot and MsWheaESReadData for more
                                      details

**/
STATIC
EFI_STATUS
MsWheaESSetHeaderFull (
  UINT8                         Phase,
  MS_WHEA_EARLY_STORAGE_HEADER  *Header
)
{
  EFI_STATUS                    Status;

  if (Header == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (Header->IsStorageFull != FALSE) {
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  // Set this field to any non-zero value;
  Header->IsStorageFull = 1;
  Header->FullPhase = Phase;
  MsWheaESHeaderChangeChecksumHelper (Header);

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**

This routine will read Early Storage header and fill out the Metadata if header indicates the Early Storage
is full

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_NOT_STARTED               Metadata is not filled as Early Storage is not full
@retval EFI_INVALID_PARAMETER         Null pointer detected
@retval EFI_NOT_FOUND                 Early Storage header region is not valid
@retval Others                        See MsWheaESFindSlot and MsWheaESReadData for more
                                      details

**/
STATIC
EFI_STATUS
MsWheaESCheckHeader (
  OUT MS_WHEA_ERROR_ENTRY_MD          *MsWheaEntryMD,
  MS_WHEA_EARLY_STORAGE_HEADER        *Header
)
{
  EFI_STATUS                    Status = EFI_SUCCESS;

  if (Header == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (Header->IsStorageFull == FALSE) {
    Status = EFI_NOT_STARTED;
    goto Cleanup;
  }

  if (MsWheaEntryMD == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem(MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD), 0);

  MsWheaEntryMD->Rev = MS_WHEA_REV_0;
  MsWheaEntryMD->Phase = Header->FullPhase;
  MsWheaEntryMD->ErrorSeverity = EFI_GENERIC_ERROR_RECOVERABLE;
  MsWheaEntryMD->ErrorStatusValue = MS_WHEA_ERROR_EARLY_STORAGE_STORE_FULL;
  MsWheaEntryMD->PayloadSize = sizeof(MS_WHEA_ERROR_ENTRY_MD);
  CopyMem(&MsWheaEntryMD->ModuleID, &gEfiCallerIdGuid, sizeof(EFI_GUID));

  // Reset the header full flag after processing
  Header->IsStorageFull = FALSE;
  MsWheaESHeaderChangeChecksumHelper(Header);

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

/**
This routine will store data onto Early Storage data region based on supplied MS WHEA metadata

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See MsWheaESFindSlot and MsWheaESWriteData for more
                                      details
**/
EFI_STATUS
EFIAPI
MsWheaESStoreEntry (
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  UINT8 Rev = 0;
  EFI_STATUS  Status = EFI_SUCCESS;
  MS_WHEA_EARLY_STORAGE_HEADER  Header;

  if (MsWheaEntryMD == NULL) {
    DEBUG((DEBUG_ERROR, "%a: input pointer cannot be null!\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // Make sure the Early Storage is valid.
  if (MsWheaESRegionIsValid(&Header) == FALSE) {
    DEBUG((DEBUG_ERROR, "%a: the Early Storage is not valid!\n", __FUNCTION__));
    Status = EFI_NOT_FOUND;
    goto Cleanup;
  }

  Rev = MsWheaEntryMD->Rev;
  switch (Rev) {
    case MS_WHEA_REV_0:
      // Store Rev0 structure
      Status = MsWheaESV0InfoStore(MsWheaEntryMD);
      break;
    default:
      // Any unsupported revisions are not stored
      Status = EFI_UNSUPPORTED;
      break;
  }

  if (Status == EFI_OUT_OF_RESOURCES) {
    // Early Storage is full, write the header error section
    DEBUG((DEBUG_WARN, "%a: the Early Storage is full at %d!\n", __FUNCTION__, Header.ActiveRange));
    MsWheaESSetHeaderFull(MsWheaEntryMD->Phase, &Header);
  }

Cleanup:
  return Status;
}

/**
This routine processes the stored errors on Early Storage data region

@param[in]  ReportFn                  Callback function when a MS WHEA metadata is ready to report

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null pointer detected
@retval Others                        See implementation specific functions and MS_WHEA_ERR_REPORT_PS_FN
                                      definition for more details
**/
EFI_STATUS
EFIAPI
MsWheaESProcess (
  IN MS_WHEA_ERR_REPORT_PS_FN         ReportFn
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  UINT8                   Index = 0;
  UINT8                   mRevInfo;
  MS_WHEA_ERROR_ENTRY_MD  MsWheaEntryMD;
  MS_WHEA_EARLY_STORAGE_HEADER  Header;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  if (ReportFn == NULL) {
    DEBUG((DEBUG_ERROR, "%a: Input function pointer cannot be null!\n", __FUNCTION__));
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // Make sure the Early Storage is valid.
  if (MsWheaESRegionIsValid(&Header) == FALSE) {
    DEBUG((DEBUG_WARN, "%a: the Early Storage is not valid!\n", __FUNCTION__));
    Status = EFI_NOT_FOUND;
    goto Cleanup;
  }

  // Check if there is indication of Early Storage full, report it if so
  Status = MsWheaESCheckHeader(&MsWheaEntryMD, &Header);
  if (EFI_ERROR(Status) == FALSE) {
    Status = ReportFn(&MsWheaEntryMD);
  }
  else {
    DEBUG((DEBUG_WARN, "%a: Early Storage header check status: %r\n", __FUNCTION__, Status));
  }

  // Go through normal entries
  while ((Header.ActiveRange >= sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON)) &&
         (Index <= (Header.ActiveRange - sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON)))) {

    Status = MsWheaESReadData(&mRevInfo,
                              sizeof(mRevInfo),
                              Index + OFFSET_OF(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON, Rev));
    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a: Early Storage storage read Index %d failed: %r\n", __FUNCTION__, Index, Status));
      Index += sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON);
      continue;
    }

    switch (mRevInfo) {
      case MS_WHEA_REV_0:
        Status = MsWheaESGetV0Info(&MsWheaEntryMD, &Index);
        if (EFI_ERROR(Status) == FALSE) {
          Status = ReportFn(&MsWheaEntryMD);
        }
        else {
          DEBUG((DEBUG_ERROR, "%a: V0 Early Storage storage process failed %r\n", __FUNCTION__, Status));
          Index += sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON);
        }
        break;
      default:
        Index += sizeof(MS_WHEA_EARLY_STORAGE_ENTRY_COMMON);
        break;
    }
  }

  // This is needed incase there is leftover garbage in default/failed cases
  MsWheaESClearAllData();

  // Zero all the fields in the
  SetMem(&Header, MS_WHEA_EARLY_STORAGE_HEADER_SIZE, 0);

  // Sign the header signature.
  Header.Signature = MS_WHEA_EARLY_STORAGE_SIGNATURE;
  Header.ActiveRange = 0;
  MsWheaESHeaderChangeChecksumHelper (&Header);

  Status = EFI_SUCCESS;

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
  return Status;
}
