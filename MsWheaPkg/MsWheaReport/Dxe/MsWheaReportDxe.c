/** @file -- MsWheaReportDxe.c

This Dxe driver will produce a RSC listener that listens to reported status codes.

Certains errors will be stored to flash upon reproting, under gEfiHardwareErrorVariableGuid 
with VarName "HwErrRecXXXX", where "XXXX" are hexadecimal digits;

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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

@retval EFI_SUCCESS                   Operation is successful
@retval EFI_ACCESS_DENIED             Exit boot has locked the report function
@retval EFI_OUT_OF_RESOURCES          List cannot make the space for requested error block payload
@retval EFI_INVALID_PARAMETER         Null pointer or zero length payload detected
**/
STATIC
EFI_STATUS
MsWheaReportHandlerDxe(
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));
  
  if (mExitBootHasOccurred != FALSE) {
    // This function is locked due to Exit Boot has occurred
    Status = EFI_ACCESS_DENIED;
    goto Cleanup;
  }

  // Input argument sanity check
  if (MsWheaEntryMD == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if ((mWriteArchAvailable != FALSE) && (mVarArchAvailable != FALSE)) {
    // Variable service is ready, store to HwErrRecXXXX
    Status = MsWheaReportHERAdd(MsWheaEntryMD);
  }
  else {
    // Add to linked list, similar to hob list
    Status = MsWheaAddReportEvent(&mMsWheaEntryList, MsWheaEntryMD);
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
  UINT8 CurrentPhase;

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

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

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
      Status = MsWheaReportHandlerDxe(MsWheaEntryMD);
      if (EFI_ERROR(Status) != FALSE) {
        DEBUG((DEBUG_ERROR, "%a: Hob entry process failed %r\n", __FUNCTION__, Status));
      }
    }
    else {
      DEBUG((DEBUG_ERROR, "%a: Bad entry: EntrySize: %08X, PayloadSize: %08X\n", __FUNCTION__, 
                                      EntrySize, 
                                      MsWheaEntryMD->PayloadSize));
    }

    GuidHob = GetNextGuidHob(&gMsWheaReportServiceGuid, GET_NEXT_HOB(GuidHob));
  }

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit...%r\n", __FUNCTION__, Status));
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

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  while (IsListEmpty(&mMsWheaEntryList) == FALSE) {
    HeadList = GetFirstNode(&mMsWheaEntryList);
    MsWheaListEntry = CR(HeadList, MS_WHEA_LIST_ENTRY, Link, MS_WHEA_LIST_ENTRY_SIGNATURE);

    if (MsWheaListEntry->PayloadPtr != NULL) {
      MsWheaReportEntry = MsWheaListEntry->PayloadPtr;
      MsWheaEntryMD = (MS_WHEA_ERROR_ENTRY_MD *) MsWheaReportEntry;
      
      Status = MsWheaReportHandlerDxe(MsWheaEntryMD);

      if (EFI_ERROR(Status) != FALSE) {
        DEBUG((DEBUG_ERROR, "%a: Linked list entry process failed %r\n", __FUNCTION__, Status));
      }
    }

    MsWheaDeleteReportEvent(&mMsWheaEntryList);
  }
  
  DEBUG((DEBUG_INFO, "%a: exit...\n", __FUNCTION__));
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
    DEBUG((DEBUG_WARN, "%a: CMOS entries process failed %r\n", __FUNCTION__, Status));
  }

  Status = MsWheaProcHob();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: Hob entries process failed %r\n", __FUNCTION__, Status));
  }

  Status = MsWheaProcList();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: List entries process failed %r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
Callback of exit boot event. This will unregister RSC handler in this module.

@param[in]  Event                     Event whose notification function is being invoked.
@param[in]  Context                   The pointer to the notification function's context, which is 
                                      implementation-dependent.
**/
STATIC
VOID
MsWheaReportDxeExitBoot (
  IN  EFI_EVENT                   Event,
  IN  VOID                        *Context
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  if (mExitBootHasOccurred != FALSE) {
    DEBUG((DEBUG_ERROR, "%a: Been here already...\n, __FUNCTION__"));
    Status = EFI_ACCESS_DENIED;
  }
  else {
    mExitBootHasOccurred = TRUE;
    Status = mRscHandlerProtocol->Unregister(MsWheaRscHandlerDxe);
    DEBUG((DEBUG_INFO, "%a: Protocol unregister result %r\n", __FUNCTION__, Status));
  }

  DEBUG((DEBUG_INFO, "%a: exit...%r\n", __FUNCTION__, Status));
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

  // collect all reported events during PEI and pre-DXE Runtime
  Status = MsWheaProcessPrevError();
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a processing hob list failed (%r)\n", __FUNCTION__, Status));
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

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  // init linked list, all fields should be 0
  InitializeListHead(&mMsWheaEntryList);
  
  // locate the RSC protocol
  Status = gBS->LocateProtocol(&gEfiRscHandlerProtocolGuid, NULL, (VOID**)&mRscHandlerProtocol);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a failed to register MsWhea report RSC handler (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // register for the RSC callback handler
  Status = mRscHandlerProtocol->Register(MsWheaRscHandlerDxe, (EFI_TPL) FixedPcdGet32(PcdMsWheaRSCHandlerTpl));
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a failed to register MsWhea report RSC handler (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  MsWheaRegisterCallbacks();

  // register for the exit boot event
  Status = gBS->CreateEventEx(EVT_NOTIFY_SIGNAL,
                              TPL_NOTIFY,
                              MsWheaReportDxeExitBoot,
                              NULL,
                              &gEfiEventExitBootServicesGuid,
                              &mExitBootServicesEvent);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a failed to register of MsWhea report exit boot (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
