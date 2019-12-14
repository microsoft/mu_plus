/** @file -- MsWheaReportPei.c

This Pei module will produce a RSC listener that listens to reported status codes.

Certain errors will be collected and added to Hob List and waiting to be collected
and/or stored during Dxe phase;

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PeiServicesLib.h>
#include <Guid/VariableFormat.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Ppi/ReportStatusCodeHandler.h>
#include "MsWheaEarlyStorageMgr.h"
#include "MsWheaReportCommon.h"

/**
Handler function that validates input arguments, and create a hob list entry for this input for later process.

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_OUT_OF_RESOURCES          List cannot make the space for requested error block payload
@retval EFI_INVALID_PARAMETER         Null pointer or zero length payload detected or length and header
                                      length field exceeds variable limit
**/
STATIC
EFI_STATUS
MsWheaReportHandlerPei(
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  UINT32                  Index;
  UINT32                  Size;
  VOID                    *MsWheaReportEntry = NULL;

  if ((MsWheaEntryMD == NULL) ||
      ((sizeof(EFI_COMMON_ERROR_RECORD_HEADER) +
        sizeof(EFI_ERROR_SECTION_DESCRIPTOR) +
        sizeof(AUTHENTICATED_VARIABLE_HEADER) +
        EFI_HW_ERR_REC_VAR_NAME_LEN * sizeof(CHAR16)) >
        PcdGet32 (PcdMaxHardwareErrorVariableSize)) ) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // Add this block to the list
  Size =  sizeof(MS_WHEA_ERROR_ENTRY_MD);

  MsWheaReportEntry = BuildGuidHob(&gMsWheaReportServiceGuid, Size);
  if (MsWheaReportEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }

  ZeroMem(MsWheaReportEntry, Size);

  Index = 0;
  CopyMem(&((UINT8*)MsWheaReportEntry)[Index], MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD));

  Index += sizeof(MS_WHEA_ERROR_ENTRY_MD);

  ((MS_WHEA_ERROR_ENTRY_MD*)MsWheaReportEntry)->PayloadSize = Index;

Cleanup:
  return Status;
}

/**
Added module phase information and route reported status code value and extended data to ReportHwErrRecRouter
for further processing.

@param[in]  PeiServices               Pointer to PEI services table.
@param[in]  CodeType                  Indicates the type of status code being reported.
@param[in]  Value                     Describes the current status of a hardware or software entity. This
                                      includes information about the class and subclass that is used to
                                      classify the entity as well as an operation.
@param[in]  Instance                  The enumeration of a hardware or software entity within the system.
                                      Valid instance numbers start with 1.
@param[in]  CallerId                  This optional parameter may be used to identify the caller. This
                                      parameter allows the status code driver to apply different rules to
                                      different callers.
@param[in]  Data                      This optional parameter may be used to pass additional data.

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        Any other error that rises from Variable Services, Boot Services,
                                      Runtime Services, etc.
**/
STATIC
EFI_STATUS
MsWheaRscHandlerPei (
  IN CONST EFI_PEI_SERVICES**         PeiServices,
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN CONST EFI_GUID                   *CallerId,
  IN CONST EFI_STATUS_CODE_DATA       *Data OPTIONAL
  )
{
  return ReportHwErrRecRouter(CodeType,
                              Value,
                              Instance,
                              CallerId,
                              Data,
                              MS_WHEA_PHASE_PEI,
                              MsWheaReportHandlerPei);
}

//A do-nothing function so MsWHeaReportCommon.c doesn't encounter an error when calling PopulateTime() during Pei phase.
//Dxe phase drivers actually populate time, see PopulateTime() in MsWheaReportDxe.c
BOOLEAN
PopulateTime(EFI_TIME* CurrentTime)
{
  return FALSE;
}

//A do-nothing function so MsWHeaReportCommon.c doesn't encounter an error when calling GetRecordID() during Pei phase.
//Dxe phase drivers actually populate time, see GetRecordID() in MsWheaReportDxe.c
EFI_STATUS
GetRecordID(UINT64* RecordID,
            EFI_GUID *RecordIDGuid OPTIONAL
            )
{
  return EFI_UNSUPPORTED;
}

/**
Entry to MsWheaReportPei.

@param FileHandle                     The image handle.
@param PeiServices                    The PEI services table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MsWheaReportPeiEntry (
  IN EFI_PEI_FILE_HANDLE              FileHandle,
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{

  EFI_STATUS                Status = EFI_SUCCESS;
  EFI_PEI_RSC_HANDLER_PPI   *RscHandlerPpi;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  // Insert signature and clear the space, if not already clear
  MsWheaESInit();

  Status = PeiServicesLocatePpi(&gEfiPeiRscHandlerPpiGuid,
                                0,
                                NULL,
                                (VOID**)&RscHandlerPpi);

  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: failed to locate PEI RSC Handler PPI (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  Status = RscHandlerPpi->Register(MsWheaRscHandlerPei);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: failed to register PEI RSC Handler PPI (%r)\n", __FUNCTION__, Status));
  }

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
