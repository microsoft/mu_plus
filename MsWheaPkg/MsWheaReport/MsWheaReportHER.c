/** @file -- MsWheaReportHER.c

This source implements backend routines to support storing persistent hardware
error records in UEFI.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PrintLib.h>
#include "MsWheaReportHER.h"

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
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN OUT UINT32                       *PayloadSize
  )
{
  EFI_STATUS                      Status = EFI_SUCCESS;
  UINT8                           *Buffer = NULL;
  UINT32                          BufferIndex = 0;
  UINT32                          ErrorPayloadSize = 0;

  EFI_COMMON_ERROR_RECORD_HEADER  *CperHdr;
  EFI_ERROR_SECTION_DESCRIPTOR    *CperErrSecDscp;
  MU_TELEMETRY_CPER_SECTION_DATA  *MuTelemetryData;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  if ((MsWheaEntryMD == NULL) || (PayloadSize == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  ErrorPayloadSize = sizeof(MU_TELEMETRY_CPER_SECTION_DATA);

  Buffer = AllocateZeroPool(sizeof(EFI_COMMON_ERROR_RECORD_HEADER) +
                            sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
                            ErrorPayloadSize);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  // Grab pointers to each header
  CperHdr = (EFI_COMMON_ERROR_RECORD_HEADER*)&Buffer[BufferIndex];
  BufferIndex += sizeof(EFI_COMMON_ERROR_RECORD_HEADER);

  CperErrSecDscp = (EFI_ERROR_SECTION_DESCRIPTOR*)&Buffer[BufferIndex];
  BufferIndex += sizeof(EFI_ERROR_SECTION_DESCRIPTOR);

  MuTelemetryData = (MU_TELEMETRY_CPER_SECTION_DATA*)&Buffer[BufferIndex];
  BufferIndex += sizeof(MU_TELEMETRY_CPER_SECTION_DATA);

  // Fill out error type based headers according to UEFI Spec...
  CreateHeadersDefault(CperHdr,
                      CperErrSecDscp,
                      MuTelemetryData,
                      MsWheaEntryMD,
                      ErrorPayloadSize);

  // Update PayloadSize as the recorded error has Headers and Payload merged
  *PayloadSize = BufferIndex;

 Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit %r...\n", __FUNCTION__, Status));
  return (VOID*) Buffer;
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
  OUT UINT16      *next
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  UINT32            Index = 0;
  UINTN             Size = 0;
  CHAR16            VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];

  if (next == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  for (Index = 0; Index <= PcdGet16(PcdVariableHardwareMaxCount); Index++) {
    Size = 0;
    UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, (UINT16)(Index & MAX_UINT16));
    Status = WheaGetVariable (VarName,
                              &gEfiHardwareErrorVariableGuid,
                              NULL,
                              &Size,
                              NULL);
    if (Status == EFI_NOT_FOUND) {
      break;
    }
  }
  // Translate result corresponds to this specific function
  if (Status == EFI_NOT_FOUND) {
    *next = (UINT16)(Index & MAX_UINT16);
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
  CHAR16                VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  EFI_STATUS            Status = EFI_SUCCESS;
  UINT32                Attributes;
  CHAR16               *Name = NULL;
  UINTN                 NameSize;
  UINTN                 NewNameSize;
  EFI_GUID              Guid;

  DEBUG((DEBUG_ERROR, "%a enter\n", __FUNCTION__));

  NameSize = sizeof(CHAR16);
  Name = AllocateZeroPool(NameSize);

  while (TRUE) {
    // Get the next name out of the system
    NewNameSize = NameSize;
    Status = WheaGetNextVariableName(&NewNameSize, Name, &Guid);

    // Make sure the variable has enough room for the name
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Name = ReallocatePool(NameSize, NewNameSize, Name);
      if (Name == NULL) {
        // Being out of memory is bad
        ASSERT(FALSE);
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }
      Status = WheaGetNextVariableName(&NewNameSize, Name, &Guid);
      NameSize = NewNameSize;
    }

    if (Status == EFI_NOT_FOUND) {
      // Out of variables, lets get out of here!
      break;
    }
    else if (EFI_ERROR(Status)) {
      // Something is wrong...
      DEBUG((DEBUG_ERROR, "%a %d get next variable name failed - %r\n", __FUNCTION__, __LINE__, Status));
      break;
    }
    else if (!CompareGuid(&Guid, &gEfiHardwareErrorVariableGuid)) {
      // If the GUID doesn't match, it isn't what we're looking for
      continue;
    }

    Attributes = EFI_VARIABLE_NON_VOLATILE |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS;

    DEBUG((DEBUG_VERBOSE, "Attributes for variable %s: %x\n", VarName, Attributes));

    if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
      Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    Status = WheaSetVariable(VarName, &gEfiHardwareErrorVariableGuid, Attributes, 0, NULL);

    if (EFI_ERROR(Status) != FALSE) {
      DEBUG((DEBUG_ERROR, "%a Clear HwErrRec has an issue - %r\n", __FUNCTION__, Status));
      break;
    }
  }

  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND)) {
    Status = EFI_SUCCESS;
  }

  if (Name) {
    FreePool(Name);
  }

  DEBUG((DEBUG_ERROR, "%a exit...\n", __FUNCTION__));

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
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  EFI_STATUS        Status = EFI_SUCCESS;
  UINT16            Index = 0;
  VOID              *Buffer = NULL;
  UINT32            Size = 0;
  CHAR16            VarName[EFI_HW_ERR_REC_VAR_NAME_LEN];
  UINT32            Attributes;

  // 1. Find an available variable name for next write
  Status = MsWheaFindNextAvailableSlot(&Index);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Find the next available slot failed (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // 2. Fill out headers
  Buffer = MsWheaAnFBuffer(MsWheaEntryMD, &Size);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG((DEBUG_ERROR, "%a: Buffer allocate and fill failed (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }
  else if (Size == 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((DEBUG_ERROR, "%a: Buffer filling returned 0 length...\n", __FUNCTION__));
    goto Cleanup;
  }
  else if (Size > PcdGet32(PcdMaxHardwareErrorVariableSize)) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG((DEBUG_ERROR, "%a: Buffer was bigger than we allow... %x > %x\n",
           __FUNCTION__, Size, PcdGet32(PcdMaxHardwareErrorVariableSize)));
    ASSERT(TRUE);
    goto Cleanup;
  }

  // 3. Save the record to flash
  UnicodeSPrint(VarName, sizeof(VarName), L"%s%04X", EFI_HW_ERR_REC_VAR_NAME, Index);

  Attributes = EFI_VARIABLE_NON_VOLATILE |
               EFI_VARIABLE_BOOTSERVICE_ACCESS |
               EFI_VARIABLE_RUNTIME_ACCESS;

  if (PcdGetBool(PcdVariableHardwareErrorRecordAttributeSupported)) {
    Attributes |= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
  }

  DEBUG((DEBUG_VERBOSE, "Attributes for variable %s: %x\n", VarName, Attributes));

  Status = WheaSetVariable(VarName, &gEfiHardwareErrorVariableGuid, Attributes, Size, Buffer);

  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Write size of %d at index %04X errored with (%r)\n", __FUNCTION__, Size, Index, Status));
  } else {
    DEBUG((DEBUG_INFO, "%a: Write size of %d at index %04X succeeded\n", __FUNCTION__, Size, Index));
  }

Cleanup:
  if (Buffer) {
    FreePool(Buffer);
  }
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
