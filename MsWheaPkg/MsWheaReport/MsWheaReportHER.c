/** @file -- MsWheaReportHER.c

This source implements backend routines to support storing persistent hardware
error records in UEFI.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>

#include "MsWheaReportHER.h"

//
// This library was designed with advanced unit-test features.
// This define handles the configuration.
#ifdef INTERNAL_UNIT_TEST
  #undef STATIC
#define STATIC    // Nothing...
#endif

/**
This routine will fill out the CPER header for caller.

Zeroed: Flags, PersistenceInfo;

@param[in]  MsWheaEntryMD             Pointer to the internal WHEA structure that will be used
                                      to populate the structure.
@param[in]  TotalSize                 Total size of the entire record.
@param[out] CperHdr                   Supplies a pointer to CPER header structure

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateCperHdrDefaultMin (
  CONST IN MS_WHEA_ERROR_ENTRY_MD     *MsWheaEntryMD,
  IN  UINT32                          TotalSize,
  OUT EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  EFI_TIME    CurrentTime;
  UINT64      RecordID;

  if (CperHdr == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem (CperHdr, sizeof (EFI_COMMON_ERROR_RECORD_HEADER), 0);

  CperHdr->SignatureStart = EFI_ERROR_RECORD_SIGNATURE_START;
  CperHdr->Revision       = EFI_ERROR_RECORD_REVISION;
  CperHdr->SignatureEnd   = EFI_ERROR_RECORD_SIGNATURE_END;
  CperHdr->SectionCount   = ((MS_WHEA_ERROR_EXTRA_SECTION_DATA *)MsWheaEntryMD->ExtraSection == NULL) ? 1 : 2;
  CperHdr->ErrorSeverity  = MsWheaEntryMD->ErrorSeverity;
  CperHdr->ValidationBits = EFI_ERROR_RECORD_HEADER_PLATFORM_ID_VALID;
  CperHdr->RecordLength   = TotalSize;

  if (PopulateTime (&CurrentTime)) {
    CperHdr->ValidationBits   |= (EFI_ERROR_RECORD_HEADER_TIME_STAMP_VALID);
    CperHdr->TimeStamp.Seconds = DecimalToBcd8 (CurrentTime.Second);
    CperHdr->TimeStamp.Minutes = DecimalToBcd8 (CurrentTime.Minute);
    CperHdr->TimeStamp.Hours   = DecimalToBcd8 (CurrentTime.Hour);
    CperHdr->TimeStamp.Day     = DecimalToBcd8 (CurrentTime.Day);
    CperHdr->TimeStamp.Month   = DecimalToBcd8 (CurrentTime.Month);
    CperHdr->TimeStamp.Year    = DecimalToBcd8 (CurrentTime.Year % 100);
    CperHdr->TimeStamp.Century = DecimalToBcd8 ((CurrentTime.Year / 100 + 1) % 100); // should not lose data
    if (MsWheaEntryMD->Phase == MS_WHEA_PHASE_DXE_VAR) {
      CperHdr->TimeStamp.Flag = BIT0;
    }
  } else {
    CperHdr->ValidationBits &= (~EFI_ERROR_RECORD_HEADER_TIME_STAMP_VALID);
  }

  CopyMem (&CperHdr->PlatformID, (EFI_GUID *)PcdGetPtr (PcdDeviceIdentifierGuid), sizeof (EFI_GUID));
  if (!IsZeroBuffer (&MsWheaEntryMD->IhvSharingGuid, sizeof (EFI_GUID))) {
    CperHdr->ValidationBits |= EFI_ERROR_RECORD_HEADER_PARTITION_ID_VALID;
  }

  CopyGuid (&CperHdr->PartitionID, &MsWheaEntryMD->IhvSharingGuid);

  // Default to MS WHEA Service guid
  CopyMem (&CperHdr->CreatorID, &gMsWheaReportServiceGuid, sizeof (EFI_GUID));

  // Default to Boot Error
  CopyMem (&CperHdr->NotificationType, &gEfiEventNotificationTypeBootGuid, sizeof (EFI_GUID));

  if (EFI_ERROR (GetRecordID (&RecordID))) {
    DEBUG ((DEBUG_INFO, "%a - RECORD ID NOT UPDATED\n", __FUNCTION__));
  }

  // Even if the record id was not updated, the value is either 0 or the previously incremented value
  CperHdr->RecordID = RecordID;
  CperHdr->Flags   |= EFI_HW_ERROR_FLAGS_PREVERR;
  // CperHdr->PersistenceInfo = 0;// Untouched.
  // SetMem(&CperHdr->Resv1, sizeof(CperHdr->Resv1), 0); // Reserved field, should be 0.

Cleanup:
  return Status;
}

/**
This routine will fill out the CPER Section Descriptor for caller.

Zeroed: SectionFlags, FruId, FruString;

@param[in]  MsWheaEntryMD             Pointer to the internal WHEA structure that will be used
                                      to populate the structure.
@param[out] CperErrSecDscp            Supplies a pointer to CPER header structure

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateCperErrSecDscpDefaultMin (
  CONST IN MS_WHEA_ERROR_ENTRY_MD   *MsWheaEntryMD,
  IN  UINT32                        Offset,
  OUT EFI_ERROR_SECTION_DESCRIPTOR  *CperErrSecDscp
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if (CperErrSecDscp == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem (CperErrSecDscp, sizeof (EFI_ERROR_SECTION_DESCRIPTOR), 0);

  CperErrSecDscp->SectionOffset = Offset;
  CperErrSecDscp->SectionLength = sizeof (MU_TELEMETRY_CPER_SECTION_DATA);
  CperErrSecDscp->Revision      = MS_WHEA_SECTION_REVISION;
  // CperErrSecDscp->SecValidMask = 0;

  // CperErrSecDscp->Resv1 = 0; // Reserved field, should be 0.
  // CperErrSecDscp->SectionFlags = 0; // Untouched.

  // Default to Mu telemetry error data
  CopyMem (&CperErrSecDscp->SectionType, &gMuTelemetrySectionTypeGuid, sizeof (EFI_GUID));

  // SetMem(&CperErrSecDscp->FruId, sizeof(CperErrSecDscp->FruId), 0); // Untouched.
  CperErrSecDscp->Severity = MsWheaEntryMD->ErrorSeverity;
  // SetMem(CperErrSecDscp->FruString, sizeof(CperErrSecDscp->FruString), 0); // Untouched.

Cleanup:
  return Status;
}

/**
This routine will fill out the CPER Section Descriptor for caller.

Zeroed: SectionFlags, FruId, FruString;

@param[in]  MsWheaEntryMD             Pointer to the internal WHEA structure that will be used
                                      to populate the structure.
@param[out] CperErrSecDscp            Supplies a pointer to CPER header structure

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateCperErrExtraSecDscpDefaultMin (
  CONST IN MS_WHEA_ERROR_ENTRY_MD   *MsWheaEntryMD,
  IN  UINT32                        Offset,
  OUT EFI_ERROR_SECTION_DESCRIPTOR  *CperErrSecDscp
  )
{
  EFI_STATUS                        Status = EFI_SUCCESS;
  MS_WHEA_ERROR_EXTRA_SECTION_DATA  *ExtraSectionPtr;

  ExtraSectionPtr = (MS_WHEA_ERROR_EXTRA_SECTION_DATA *)MsWheaEntryMD->ExtraSection;

  if ((ExtraSectionPtr == NULL) || (CperErrSecDscp == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem (CperErrSecDscp, sizeof (EFI_ERROR_SECTION_DESCRIPTOR), 0);

  CperErrSecDscp->SectionOffset = Offset;
  CperErrSecDscp->SectionLength = ExtraSectionPtr->DataSize;
  CperErrSecDscp->Revision      = MS_WHEA_SECTION_REVISION;
  // CperErrSecDscp->SecValidMask = 0;

  // CperErrSecDscp->Resv1 = 0; // Reserved field, should be 0.
  // CperErrSecDscp->SectionFlags = 0; // Untouched.

  // Default to Mu telemetry error data
  CopyMem (&CperErrSecDscp->SectionType, &ExtraSectionPtr->SectionGuid, sizeof (EFI_GUID));

  // SetMem(&CperErrSecDscp->FruId, sizeof(CperErrSecDscp->FruId), 0); // Untouched.
  CperErrSecDscp->Severity = MsWheaEntryMD->ErrorSeverity;
  // SetMem(CperErrSecDscp->FruString, sizeof(CperErrSecDscp->FruString), 0); // Untouched.

Cleanup:
  return Status;
}

/**
This routine will fill out the Mu Telemetry Error Data structure for caller.

@param[in]  MsWheaEntryMD             Internal telemetry metadata collected during error report
@param[out] MuTelemetryData           Supplies a pointer to Mu Telemetry Error Data structure

@retval EFI_SUCCESS                   The operation completed successfully
@retval EFI_INVALID_PARAMETER         Any required input pointer is NULL
**/
STATIC
EFI_STATUS
CreateMuTelemetryData (
  CONST IN MS_WHEA_ERROR_ENTRY_MD     *MsWheaEntryMD,
  OUT MU_TELEMETRY_CPER_SECTION_DATA  *MuTelemetryData
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if ((MuTelemetryData == NULL) ||
      (MsWheaEntryMD == NULL))
  {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  SetMem (MuTelemetryData, sizeof (MU_TELEMETRY_CPER_SECTION_DATA), 0);

  MuTelemetryData->ComponentID      = MsWheaEntryMD->ModuleID;
  MuTelemetryData->SubComponentID   = MsWheaEntryMD->LibraryID;
  MuTelemetryData->ErrorStatusValue = MsWheaEntryMD->ErrorStatusValue;
  MuTelemetryData->AdditionalInfo1  = MsWheaEntryMD->AdditionalInfo1;
  MuTelemetryData->AdditionalInfo2  = MsWheaEntryMD->AdditionalInfo2;

Cleanup:
  return Status;
}

/**

This routine accepts the pointer to the MS WHEA entry metadata, payload and payload length, allocate and
fill (AnF) in the buffer with supplied information. Then return the pointer of processed buffer.

Note: Caller is responsible for freeing the valid returned buffer!!!

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
@param[out]  PayloadSize              The pointer to payload length, this will be updated to the totally
                                      allocated/operated size if operation succeeded

@retval NULL if any errors, otherwise filled and allocated buffer will be returned.

**/
STATIC
VOID *
MsWheaAnFBuffer (
  CONST IN MS_WHEA_ERROR_ENTRY_MD  *MsWheaEntryMD,
  IN OUT UINT32                    *PayloadSize
  )
{
  EFI_STATUS  Status  = EFI_SUCCESS;
  UINT8       *Buffer = NULL;
  UINT32      BufferIndex;
  UINT32      TotalSize;

  EFI_COMMON_ERROR_RECORD_HEADER    *CperHdr;
  EFI_ERROR_SECTION_DESCRIPTOR      *CperErrSecDscp;
  MU_TELEMETRY_CPER_SECTION_DATA    *MuTelemetryData;
  EFI_ERROR_SECTION_DESCRIPTOR      *CperErrExtraSecDscp;
  UINT8                             *ExtraSectionData;
  MS_WHEA_ERROR_EXTRA_SECTION_DATA  *ExtraSectionPtr;

  DEBUG ((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  if ((MsWheaEntryMD == NULL) || (PayloadSize == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  CperErrExtraSecDscp = NULL;
  ExtraSectionData    = NULL;

  TotalSize = sizeof (EFI_COMMON_ERROR_RECORD_HEADER) +
              sizeof (EFI_ERROR_SECTION_DESCRIPTOR) +
              sizeof (MU_TELEMETRY_CPER_SECTION_DATA);
  ExtraSectionPtr = (MS_WHEA_ERROR_EXTRA_SECTION_DATA *)MsWheaEntryMD->ExtraSection;
  if (ExtraSectionPtr != NULL) {
    TotalSize += sizeof (EFI_ERROR_SECTION_DESCRIPTOR) +
                 ExtraSectionPtr->DataSize;
  }

  Buffer = AllocateZeroPool (TotalSize);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  // Grab pointers to each header
  CperHdr     = (EFI_COMMON_ERROR_RECORD_HEADER *)&Buffer[0];
  BufferIndex = sizeof (EFI_COMMON_ERROR_RECORD_HEADER);

  CperErrSecDscp = (EFI_ERROR_SECTION_DESCRIPTOR *)&Buffer[BufferIndex];
  BufferIndex   += sizeof (EFI_ERROR_SECTION_DESCRIPTOR);

  if (ExtraSectionPtr != NULL) {
    CperErrExtraSecDscp = (EFI_ERROR_SECTION_DESCRIPTOR *)&Buffer[BufferIndex];
    BufferIndex        += sizeof (EFI_ERROR_SECTION_DESCRIPTOR);
  }

  MuTelemetryData = (MU_TELEMETRY_CPER_SECTION_DATA *)&Buffer[BufferIndex];
  BufferIndex    += sizeof (MU_TELEMETRY_CPER_SECTION_DATA);

  if (ExtraSectionPtr != NULL) {
    ExtraSectionData = (UINT8 *)&Buffer[BufferIndex];
  }

  // Fill out error type based headers according to UEFI Spec...
  CreateCperHdrDefaultMin (MsWheaEntryMD, TotalSize, CperHdr);
  // Add all section descriptors.
  CreateCperErrSecDscpDefaultMin (MsWheaEntryMD, (UINT32)((UINTN)MuTelemetryData - (UINTN)CperHdr), CperErrSecDscp);
  if (ExtraSectionPtr != NULL) {
    CreateCperErrExtraSecDscpDefaultMin (MsWheaEntryMD, (UINT32)((UINTN)ExtraSectionData - (UINTN)CperHdr), CperErrExtraSecDscp);
  }

  // Add all section data.
  CreateMuTelemetryData (MsWheaEntryMD, MuTelemetryData);
  if (ExtraSectionPtr != NULL) {
    CopyMem (
      ExtraSectionData,
      &ExtraSectionPtr->Data,
      ExtraSectionPtr->DataSize
      );
  }

  // Update PayloadSize as the recorded error has Headers and Payload merged
  *PayloadSize = TotalSize;

Cleanup:
  DEBUG ((DEBUG_INFO, "%a: exit %r...\n", __FUNCTION__, Status));
  return (VOID *)Buffer;
}

/**

This routine accepts the pointer to a UINT16 number. It will iterate through each HwErrRecXXXX and stops
after PcdVariableHardwareMaxCount iterations or spotted a slot that returns EFI_NOT_FOUND.

@param[out]  next                       The pointer to output result holder

@retval EFI_SUCCESS                     Entry addition is successful.
@retval EFI_INVALID_PARAMETER           Input pointer is NULL.
@retval EFI_OUT_OF_RESOURCES            No available slot for HwErrRec.
@retval Others                          See GetVariable for more details

**/
STATIC
EFI_STATUS
MsWheaFindNextAvailableSlot (
  OUT UINT16  *next
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      Index  = 0;
  UINTN       Size   = 0;
  CHAR16      VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];

  if (next == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  for (Index = 0; Index <= PcdGet16 (PcdVariableHardwareMaxCount); Index++) {
    Size = 0;
    UnicodeSPrint (VarName, sizeof (VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, (UINT16)(Index & MAX_UINT16));
    Status = WheaGetVariable (
               VarName,
               &gEfiHardwareErrorVariableGuid,
               NULL,
               &Size,
               NULL
               );
    if (Status == EFI_NOT_FOUND) {
      break;
    }
  }

  // Translate result corresponds to this specific function
  if (Status == EFI_NOT_FOUND) {
    *next  = (UINT16)(Index & MAX_UINT16);
    Status = EFI_SUCCESS;
  } else if (Status == EFI_SUCCESS) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    // Do Nothing, pass on the error
  }

Cleanup:
  return Status;
}

/**

Clear all the HwErrRec entries on flash.

@retval EFI_SUCCESS                     Entry addition is successful.
@retval Others                          See GetVariable/SetVariable for more details

**/
EFI_STATUS
EFIAPI
MsWheaClearAllEntries (
  VOID
  )
{
  CHAR16      VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  EFI_STATUS  Status = EFI_SUCCESS;
  UINT32      Attributes;
  CHAR16      *Name = NULL;
  UINTN       NameSize;
  UINTN       NewNameSize;
  EFI_GUID    Guid;

  DEBUG ((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  NameSize = sizeof (CHAR16);
  Name     = AllocateZeroPool (NameSize);

  while (TRUE) {
    // Get the next name out of the system
    NewNameSize = NameSize;
    Status      = WheaGetNextVariableName (&NewNameSize, Name, &Guid);

    // Make sure the variable has enough room for the name
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool (NameSize, NewNameSize, Name);
      if (Name == NULL) {
        // Being out of memory is bad
        ASSERT (FALSE);
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      Status   = WheaGetNextVariableName (&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      // Out of variables, lets get out of here!
      break;
    } else if (EFI_ERROR (Status)) {
      // Something is wrong...
      DEBUG ((DEBUG_ERROR, "%a %d get next variable name failed - %r\n", __FUNCTION__, __LINE__, Status));
      break;
    } else if (!CompareGuid (&Guid, &gEfiHardwareErrorVariableGuid)) {
      // If the GUID doesn't match, it isn't what we're looking for
      continue;
    }

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    DEBUG ((DEBUG_VERBOSE, "Attributes for variable %s: %x\n", VarName, Attributes));

    if (PcdGetBool (PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = WheaSetVariable (VarName, &gEfiHardwareErrorVariableGuid, Attributes, 0, NULL);

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a Clear HwErrRec has an issue - %r\n", __FUNCTION__, Status));
      break;
    }
  }

  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND)) {
    Status = EFI_SUCCESS;
  }

  if (Name) {
    FreePool (Name);
  }

  DEBUG ((DEBUG_ERROR, "%a exit...\n", __FUNCTION__));

  return Status;
}

/**

This routine accepts the pointer to the MS WHEA entry metadata, error specific data payload and its size
then store on variable storage as HwErrRec awaiting to be picked up by OS (Refer to UEFI Spec 2.7A)

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Entry addition is successful.
@retval EFI_INVALID_PARAMETER         Input has NULL pointer as input.
@retval EFI_OUT_OF_RESOURCES          Not enough space for the requested space.

**/
EFI_STATUS
EFIAPI
MsWheaReportHERAdd (
  IN MS_WHEA_ERROR_ENTRY_MD  *MsWheaEntryMD
  )
{
  EFI_STATUS  Status  = EFI_SUCCESS;
  UINT16      Index   = 0;
  VOID        *Buffer = NULL;
  UINT32      Size    = 0;
  CHAR16      VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  UINT32      Attributes;

  // 1. Find an available variable name for next write
  Status = MsWheaFindNextAvailableSlot (&Index);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Find the next available slot failed (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // 2. Fill out headers
  Buffer = MsWheaAnFBuffer (MsWheaEntryMD, &Size);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Buffer allocate and fill failed (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  } else if (Size == 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "%a: Buffer filling returned 0 length...\n", __FUNCTION__));
    goto Cleanup;
  } else if (Size > PcdGet32 (PcdMaxHardwareErrorVariableSize)) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "%a: Buffer was bigger than we allow... %x > %x\n",
      __FUNCTION__,
      Size,
      PcdGet32 (PcdMaxHardwareErrorVariableSize)
      ));
    ASSERT (TRUE);
    goto Cleanup;
  }

  // 3. Save the record to flash
  UnicodeSPrint (VarName, sizeof (VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, Index);

  Attributes = EFI_VARIABLE_NON_VOLATILE |
               EFI_VARIABLE_BOOTSERVICE_ACCESS |
               EFI_VARIABLE_RUNTIME_ACCESS;

  if (PcdGetBool (PcdVariableHardwareErrorRecordAttributeSupported)) {
    Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
  }

  DEBUG ((DEBUG_VERBOSE, "Attributes for variable %s: %x\n", VarName, Attributes));

  Status = WheaSetVariable (VarName, &gEfiHardwareErrorVariableGuid, Attributes, Size, Buffer);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Write size of %d at index %04X failed with (%r)\n", __FUNCTION__, Size, Index, Status));
  } else {
    DEBUG ((DEBUG_INFO, "%a: Write size of %d at index %04X succeeded\n", __FUNCTION__, Size, Index));
  }

Cleanup:
  if (Buffer) {
    FreePool (Buffer);
  }

  DEBUG ((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
