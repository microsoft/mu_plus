/** @file -- MsWheaReportPei.c

This Pei module will produce a RSC listener that listens to reported status codes.

Certains errors will be collected and added to Hob List and waiting to be collected
and/or stored during Dxe phase;

Copyright (c) 2018, Microsoft Corporation

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
@param[in]  PayloadPtr                The pointer to reported error block payload, the content will be copied
@param[in]  PayloadSize               The size of reported error block payload

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_OUT_OF_RESOURCES          List cannot make the space for requested error block payload
@retval EFI_INVALID_PARAMETER         Null pointer or zero length payload detected or length and header 
                                      length field exceeds variable limit
**/
STATIC
EFI_STATUS
MsWheaReportHandlerPei(
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN VOID                             *PayloadPtr,
  IN UINT32                           PayloadSize
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  UINT32                  Index;
  UINT32                  Size;
  VOID                    *MsWheaReportEntry = NULL;

  if ((PayloadPtr == NULL) || 
      (PayloadSize == 0) || 
      (MsWheaEntryMD == NULL) || 
      ((PayloadSize + 
        sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + 
        sizeof(EFI_ERROR_SECTION_DESCRIPTOR) + 
        sizeof(AUTHENTICATED_VARIABLE_HEADER) + 
        EFI_HW_ERR_REC_VAR_NAME_LEN * sizeof(CHAR16)) > 
        PcdGet32 (PcdMaxHardwareErrorVariableSize)) ) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // Add this block to the list
  Size =  sizeof(MS_WHEA_ERROR_ENTRY_MD) + PayloadSize;

  MsWheaReportEntry = BuildGuidHob(&gMsWheaReportServiceGuid, Size);
  if (MsWheaReportEntry == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Cleanup;
  }
  
  ZeroMem(MsWheaReportEntry, Size);

  Index = 0;
  CopyMem(&((UINT8*)MsWheaReportEntry)[Index], MsWheaEntryMD, sizeof(MS_WHEA_ERROR_ENTRY_MD));

  Index = sizeof(MS_WHEA_ERROR_ENTRY_MD);
  CopyMem(&((UINT8*)MsWheaReportEntry)[Index], PayloadPtr, PayloadSize);
  
  ((MS_WHEA_ERROR_ENTRY_MD*)MsWheaReportEntry)->PayloadSize = Size;

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

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  // Insert signature and clear the space, if not already clear
  MsWheaESInit();

  Status = PeiServicesLocatePpi(&gEfiPeiRscHandlerPpiGuid,
                                0,
                                NULL,
                                (VOID**)&RscHandlerPpi);
  
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": failed to locate PEI RSC Handler PPI (%r)\n", Status));
    goto Cleanup;
  }

  Status = RscHandlerPpi->Register(MsWheaRscHandlerPei);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": failed to register PEI RSC Handler PPI (%r)\n", Status));
  }

  if (PcdGetBool(PcdMsWheaReportTestEnable) != FALSE) {
    MsWheaInSituTest(MS_WHEA_PHASE_PEI);
  }

Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit (%r)\n", Status));
  return Status;
}
