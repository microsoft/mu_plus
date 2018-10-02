/** @file -- MsWheaReportDxe.c

This Dxe driver will produce a RSC listener that listens to reported status codes.

Certains errors will be stored to flash upon reproting, under gEfiHardwareErrorVariableGuid 
with VarName "HwErrRecXXXX", where "XXXX" are hexadecimal digits;

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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Protocol/ReportStatusCodeHandler.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include "MsWheaReportCommon.h"
#include "MsWheaReportHER.h"
#include "MsWheaEarlyStorageMgr.h"
#include "MsWheaReportList.h"

STATIC EFI_RSC_HANDLER_PROTOCOL       *mRscHandlerProtocol    = NULL;

STATIC EFI_EVENT                      mWriteArchAvailEvent    = NULL;
STATIC EFI_EVENT                      mVarArchAvailEvent      = NULL;
STATIC EFI_EVENT                      mExitBootServicesEvent  = NULL;

STATIC BOOLEAN                        mWriteArchAvailable     = FALSE;
STATIC BOOLEAN                        mVarArchAvailable       = FALSE;
STATIC BOOLEAN                        mExitBootHasOccurred    = FALSE;

STATIC LIST_ENTRY                     mMsWheaEntryList;

/**
Handler function that validates input arguments, and store on flash/CMOS for OS to process.

Note: It is the reporter's responsibility to make sure the format of each blob is compliant
with specifications. Malformed data will fail the entire reporting.

@param[in]  MsWheaEntryMD             The pointer to reported MS WHEA error metadata
@param[in]  PayloadPtr                The pointer to reported error block payload, the content will be copied
@param[in]  PayloadSize               The size of reported error block payload

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_ACCESS_DENIED             Exit boot has locked the report function
@retval EFI_OUT_OF_RESOURCES          List cannot make the space for requested error block payload
@retval EFI_INVALID_PARAMETER         Null pointer or zero length payload detected
**/
STATIC
EFI_STATUS
MsWheaReportHandlerDxe(
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD,
  IN VOID                             *PayloadPtr,
  IN UINT32                           PayloadSize
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));
  
  if (mExitBootHasOccurred != FALSE) {
    // This function is locked due to Exit Boot has occurred
    Status = EFI_ACCESS_DENIED;
    goto Cleanup;
  }

  // Input argument sanity check
  if ((PayloadPtr == NULL) || 
      (PayloadSize == 0) || 
      (MsWheaEntryMD == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if ((mWriteArchAvailable != FALSE) && (mVarArchAvailable != FALSE)) {
    // Variable service is ready, store to HwErrRecXXXX
    Status = MsWheaReportHERAdd(MsWheaEntryMD, PayloadPtr, PayloadSize);
  }
  else {
    // Add to linked list, similar to hob list
    Status = MsWheaAddReportEvent(&mMsWheaEntryList, MsWheaEntryMD, PayloadPtr, PayloadSize);
  }

Cleanup:
  return Status;
}

/**
Added module phase information and route reported status code value and extended data to ReportHwErrRecRouter
for further processing.

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
MsWheaRscHandlerDxe (
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN EFI_GUID                         *CallerId,
  IN EFI_STATUS_CODE_DATA             *Data OPTIONAL
  )
{
  MS_WHEA_ERROR_PHASE CurrentPhase;

  if ((mWriteArchAvailable != FALSE) && (mVarArchAvailable != FALSE)) {
    CurrentPhase = MS_WHEA_PHASE_DXE_RUNTIME;
  }
  else {
    CurrentPhase = MS_WHEA_PHASE_DXE;
  }

  return ReportHwErrRecRouter(CodeType,
                              Value,
                              Instance,
                              CallerId,
                              Data,
                              CurrentPhase,
                              MsWheaReportHandlerDxe);
}

/**
This routine processes the reported errors during PEI phase through hob list

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See MsWheaReportHandlerDxe function for more details
**/
STATIC
EFI_STATUS
MsWheaProcHob (
  VOID
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  UINT16                  EntrySize = 0;
  VOID                    *GuidHob = NULL;
  VOID                    *MsWheaReportEntry = NULL;
  MS_WHEA_ERROR_ENTRY_MD  *MsWheaEntryMD = NULL;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  GuidHob = GetFirstGuidHob(&gMsWheaReportServiceGuid);
  if (GuidHob == NULL) {
    // This means no reported errors during Pei phase, job done!
    Status = EFI_SUCCESS;
    goto Cleanup;
  }

  while (GuidHob != NULL) {
    MsWheaReportEntry = GET_GUID_HOB_DATA(GuidHob);
    EntrySize = GET_GUID_HOB_DATA_SIZE(GuidHob);
    MsWheaEntryMD = (MS_WHEA_ERROR_ENTRY_MD *) MsWheaReportEntry;
    if ((EntrySize != 0) && (EntrySize >= MsWheaEntryMD->PayloadSize)) {
      // Report this entry
      Status = MsWheaReportHandlerDxe(MsWheaEntryMD, 
                                      MsWheaEntryMD + 1, 
                                      MsWheaEntryMD->PayloadSize - sizeof(MS_WHEA_ERROR_ENTRY_MD));
      if (EFI_ERROR(Status) != FALSE) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ ": Hob entry process failed %r\n", Status));
      }
    }
    else {
      DEBUG((DEBUG_ERROR, __FUNCTION__": Bad entry: EntrySize: %08X, PayloadSize: %08X\n", 
                                      EntrySize, 
                                      MsWheaEntryMD->PayloadSize));
    }

    GuidHob = GetNextGuidHob(&gMsWheaReportServiceGuid, GET_NEXT_HOB(GuidHob));
  }

Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...%r\n", Status));
  return Status;
}

/**
This routine processes the reported errors during Dxe phase through linked list

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_INVALID_PARAMETER         Null head entry pointer detected
@retval Others                        See MsWheaReportHandlerDxe function for more details
**/
STATIC
EFI_STATUS
MsWheaProcList (
  VOID
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;
  VOID                    *MsWheaReportEntry = NULL;
  MS_WHEA_ERROR_ENTRY_MD  *MsWheaEntryMD = NULL;
  MS_WHEA_LIST_ENTRY      *MsWheaListEntry = NULL;
  LIST_ENTRY              *HeadList = NULL;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  while (IsListEmpty(&mMsWheaEntryList) == FALSE) {
    HeadList = GetFirstNode(&mMsWheaEntryList);
    MsWheaListEntry = CR(HeadList, MS_WHEA_LIST_ENTRY, Link, MS_WHEA_LIST_ENTRY_SIGNATURE);

    if (MsWheaListEntry->PayloadPtr != NULL) {
      MsWheaReportEntry = MsWheaListEntry->PayloadPtr;
      MsWheaEntryMD = (MS_WHEA_ERROR_ENTRY_MD *) MsWheaReportEntry;
      
      Status = MsWheaReportHandlerDxe(MsWheaEntryMD, 
                                      MsWheaEntryMD + 1, 
                                      MsWheaEntryMD->PayloadSize - sizeof(MS_WHEA_ERROR_ENTRY_MD));

      if (EFI_ERROR(Status) != FALSE) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ ": Linked list entry process failed %r\n", Status));
      }
    }

    MsWheaDeleteReportEvent(&mMsWheaEntryList);
  }
  
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...\n"));
  return Status;
}

/**
Process all previously reported status errors during PEI/early Dxe/previous boots 

@retval EFI_SUCCESS                   Operation is successful
@retval Others                        See each individual function for more details
**/
STATIC
EFI_STATUS
MsWheaProcessPrevError (
  VOID
  )
{
  EFI_STATUS Status;
  
  Status = MsWheaESProcess(MsWheaReportHandlerDxe);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": CMOS entries process failed %r\n", Status));
  }

  Status = MsWheaProcHob();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": Hob entries process failed %r\n", Status));
  }

  Status = MsWheaProcList();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": List entries process failed %r\n", Status));
  }

  return Status;
}

/**
Callback of exit boot event. This will unregister RSC handler in this module. 
**/
STATIC
VOID
MsWheaReportDxeExitBoot (
  IN  EFI_EVENT                   Event,
  IN  VOID                        *Context
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  if (mExitBootHasOccurred != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ ": Been here already...\n"));
    Status = EFI_ACCESS_DENIED;
  }
  else {
    mExitBootHasOccurred = TRUE;
    Status = mRscHandlerProtocol->Unregister(MsWheaRscHandlerDxe);
    DEBUG((DEBUG_INFO, __FUNCTION__ ": Protocol unregister result %r\n", Status));
  }

  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit...%r\n", Status));
}

/**
Register Exit Boot callback and process previous errors when variable service is ready

@param[in]  Event                 Event whose notification function is being invoked.
@param[in]  Context               The pointer to the notification function's context, which is 
                                  implementation-dependent.
**/
STATIC
VOID
MsWheaArchCallback (
  IN  EFI_EVENT             Event,
  IN  VOID                  *Context
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  if ((Event == mWriteArchAvailEvent) && (mWriteArchAvailable == FALSE)) {
    mWriteArchAvailable = TRUE;
  } 
  else if ((Event == mVarArchAvailEvent) && (mVarArchAvailable == FALSE)) {
    mVarArchAvailable = TRUE;
  }
  else {
    // Unrecognized event or all available already, do nothing
    goto Cleanup;
  }

  if ((mVarArchAvailable == FALSE) || (mWriteArchAvailable == FALSE)) {
    // Some protocol(s) not ready
    goto Cleanup;
  }

  // register for the exit boot event
  Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                              TPL_NOTIFY,
                              MsWheaReportDxeExitBoot,
                              NULL,
                              &gEfiEventExitBootServicesGuid,
                              &mExitBootServicesEvent);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to register of MsWhea report exit boot (%r)\n", Status));
    goto Cleanup;
  }

  // collect all reported events during PEI and pre-DXE Runtime
  Status = MsWheaProcessPrevError();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " processing hob list failed (%r)\n", Status));
  }

  if (PcdGetBool(PcdMsWheaReportTestEnable) != FALSE) {
    MsWheaInSituTest(MS_WHEA_PHASE_DXE_RUNTIME);
  }

Cleanup:
  return;
}

/**
Register Write Archetecture and Variable Archetecture callbacks

@param[in]  Event                     Event whose notification function is being invoked.
@param[in]  Context                   The pointer to the notification function's context, which is 
                                      implementation-dependent.
**/
STATIC
VOID
MsWheaRegisterCallbacks (
  VOID
  )
{
  VOID *Registration; // Just a dummy, not used

  // register for Write Archetecture Protocol Callback
  mWriteArchAvailEvent = EfiCreateProtocolNotifyEvent(&gEfiVariableWriteArchProtocolGuid, 
                                                    TPL_CALLBACK, 
                                                    MsWheaArchCallback, 
                                                    NULL, 
                                                    &Registration);

  // register for Variable Archetecture Protocol Callback
  mVarArchAvailEvent = EfiCreateProtocolNotifyEvent(&gEfiVariableArchProtocolGuid, 
                                                    TPL_CALLBACK, 
                                                    MsWheaArchCallback, 
                                                    NULL, 
                                                    &Registration);
}

/**
Entry to MsWheaReportDxe, register RSC handler and callback functions

@param[in] ImageHandle                The image handle.
@param[in] SystemTable                The system table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MsWheaReportDxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  ) 
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG((DEBUG_INFO, __FUNCTION__ ": enter...\n"));

  // init linked list, all fields should be 0
  InitializeListHead(&mMsWheaEntryList);
  
  // locate the RSC protocol
  Status = gBS->LocateProtocol(&gEfiRscHandlerProtocolGuid, NULL, (VOID**)&mRscHandlerProtocol);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to register MsWhea report RSC handler (%r)\n", Status));
    goto Cleanup;
  }

  // register for the RSC callback handler
  Status = mRscHandlerProtocol->Register(MsWheaRscHandlerDxe, TPL_HIGH_LEVEL);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, __FUNCTION__ " failed to register MsWhea report RSC handler (%r)\n", Status));
    goto Cleanup;
  }
  
  
  if (PcdGetBool(PcdMsWheaReportTestEnable) != FALSE) {
    MsWheaInSituTest(MS_WHEA_PHASE_DXE);
  }

  MsWheaRegisterCallbacks();

Cleanup:
  DEBUG((DEBUG_INFO, __FUNCTION__ ": exit (%r)\n", Status));
  return Status;
}
