/** @file AdvLoggerOsConnectorPrm.c

  This driver gives an interface to OS components to fetch the AdvancedLogger memory log.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <AdvancedLoggerInternal.h>

#include "AdvLoggerOsConnectorPrm.h"

typedef struct {
  VOID      *OutputBuffer;
  UINT32    *OutputBufferSize;
} ADVANCED_LOGGER_PRM_PARAMETER_BUFFER;

/**
    CheckAddress

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.
    LogBufferOffset, LogCurrentOffset, and LogBufferSize, could be written to by untrusted code.  Here, we check that
    the offsets are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          DataBuf     PRM static data buffer containing LoggerInfo pointer

    @return         BOOLEAN     TRUE - LoggerInfo passes security checks
    @return         BOOLEAN     FALSE- LoggerInfo failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  IN  ADV_LOGGER_PRM_DATA_BUFFER  *DataBuf
  )
{
  if ((DataBuf == NULL) || (DataBuf->LoggerInfo == NULL)) {
    return FALSE;
  }

  if (DataBuf->LoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  // most ValidateInfoBlocks check if LogBufferOffset == sizeof (*LoggerInfo)
  // we can't do that in the PRM because it is independently serviceable at OS runtime
  // we may be paired with a FW that has a different logger info structure size, so
  // we have to go based on what the boot time FW tells us

  if ((DataBuf->LoggerInfo->LogCurrentOffset > TOTAL_LOG_SIZE_WITH_ALI (DataBuf->LoggerInfo)) ||
      (DataBuf->LoggerInfo->LogCurrentOffset < DataBuf->LoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  // ensure that the passed in sizes match what we see in the advanced logger structure
  if ((DataBuf->ExpectedLogSize != DataBuf->LoggerInfo->LogBufferSize) ||
      (DataBuf->ExpectedHeaderSize != DataBuf->LoggerInfo->LogBufferOffset))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  The Advanced Logger Os Connector PRM handler.

  This handler reads the AdvancedLogger buffer and copies the data to the caller supplied buffer.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBuffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (AdvLoggerOsConnectorPrmHandler) {
  ADVANCED_LOGGER_PRM_PARAMETER_BUFFER  *ParamBuf;
  ADV_LOGGER_PRM_DATA_BUFFER            *DataBuf = NULL;

  if ((ParameterBuffer == NULL) || (ContextBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (ContextBuffer->StaticDataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify PRM data buffer signature is valid
  //
  if (
      (ContextBuffer->Signature != PRM_CONTEXT_BUFFER_SIGNATURE) ||
      (ContextBuffer->StaticDataBuffer->Header.Signature != PRM_DATA_BUFFER_HEADER_SIGNATURE))
  {
    return EFI_NOT_FOUND;
  }

  DataBuf = (ADV_LOGGER_PRM_DATA_BUFFER *)ContextBuffer->StaticDataBuffer->Data;

  if (!ValidateInfoBlock (DataBuf)) {
    return EFI_COMPROMISED_DATA;
  }

  ParamBuf = (ADVANCED_LOGGER_PRM_PARAMETER_BUFFER *)ParameterBuffer;

  if (ParamBuf->OutputBufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*(ParamBuf->OutputBufferSize) < DataBuf->ExpectedLogSize + DataBuf->ExpectedHeaderSize) {
    *ParamBuf->OutputBufferSize = DataBuf->ExpectedLogSize + DataBuf->ExpectedHeaderSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (ParamBuf->OutputBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The length to copy we take from the static data buffer, populated by the FW from a fixed at build PCD
  // We do this so we do not have to trust the writeable LoggerInfo structure. We confirm the header size and
  // data size against the structure's values to ensure they match
  //
  CopyMem (ParamBuf->OutputBuffer, (CONST VOID *)DataBuf->LoggerInfo, DataBuf->ExpectedLogSize + DataBuf->ExpectedHeaderSize);

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (ADVANCED_LOGGER_OS_CONNECTOR_PRM_HANDLER_GUID, AdvLoggerOsConnectorPrmHandler)
  );

/**
  Module entry point.

  @param[in]   ImageHandle     The image handle.
  @param[in]   SystemTable     A pointer to the system table.

  @retval  EFI_SUCCESS         This function always returns success.

**/
EFI_STATUS
EFIAPI
AdvLoggerOsConnectorPrmEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
