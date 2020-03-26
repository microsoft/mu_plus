/** @file -- MsWheaReportSmm.c

This Smm driver will produce a RSC listener that listens to reported status codes. The
service is intended for SMM environment and will only be available after
gEfiVariableWriteArchProtocolGuid is published.

Certain errors will be stored to flash upon reporting, under gEfiHardwareErrorVariableGuid
with VarName "HwErrRecXXXX", where "XXXX" are hexadecimal digits;

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/SmmVariable.h>
#include <Protocol/SmmReportStatusCodeHandler.h>

#include <Library/UefiLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include "MsWheaReportCommon.h"
#include "MsWheaReportHER.h"

EFI_SMM_VARIABLE_PROTOCOL  *mSmmVariable = NULL;

STATIC EFI_SMM_RSC_HANDLER_PROTOCOL   *mRscHandlerProtocol    = NULL;

/**

Returns the value of a variable. See definition of EFI_GET_VARIABLE in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaGetVariable (
  IN     CHAR16                      *VariableName,
  IN     EFI_GUID                    *VendorGuid,
  OUT    UINT32                      *Attributes,    OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  )
{
  EFI_STATUS Status;

  if ((mSmmVariable == NULL) || (mSmmVariable->SmmGetVariable == NULL)) {
    Status = EFI_NOT_READY;
    goto Cleanup;
  }

  Status = mSmmVariable->SmmGetVariable (VariableName,
                                         VendorGuid,
                                         Attributes,
                                         DataSize,
                                         Data);

Cleanup:
  return Status;
}

/**

Enumerates the current variable names. See definition of EFI_GET_NEXT_VARIABLE_NAME in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_GET_NEXT_VARIABLE_NAME for more details

**/
EFI_STATUS
EFIAPI
WheaGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
{
  EFI_STATUS Status;

  if ((mSmmVariable == NULL) || (mSmmVariable->SmmGetNextVariableName == NULL)) {
    Status = EFI_NOT_READY;
    goto Cleanup;
  }

  Status = mSmmVariable->SmmGetNextVariableName (VariableNameSize,
                                                 VariableName,
                                                 VendorGuid);

Cleanup:
  return Status;
}

/**
Sets the value of a variable. See definition of EFI_SET_VARIABLE in
Include/Uefi/UefiSpec.h.

@retval EFI_NOT_READY                 If requested service is not yet available
@retval Others                        See EFI_SET_VARIABLE for more details

**/
EFI_STATUS
EFIAPI
WheaSetVariable (
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  )
{
  EFI_STATUS Status;

  if ((mSmmVariable == NULL) || (mSmmVariable->SmmSetVariable == NULL)) {
    Status = EFI_NOT_READY;
    goto Cleanup;
  }

  Status = mSmmVariable->SmmSetVariable (VariableName,
                                         VendorGuid,
                                         Attributes,
                                         DataSize,
                                         Data);

Cleanup:
  return Status;
}

/**
Handler function that validates input arguments, and store on flash for OS to process.

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
EFIAPI
MsWheaReportHandlerSmm(
  IN MS_WHEA_ERROR_ENTRY_MD           *MsWheaEntryMD
  )
{
  EFI_STATUS Status;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  // Input argument sanity check
  if (MsWheaEntryMD == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  // Variable service is ready, store to HwErrRecXXXX
  Status = MsWheaReportHERAdd(MsWheaEntryMD);
  DEBUG((DEBUG_INFO, "%a: error record written to flash - %r\n", __FUNCTION__, Status));

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
EFIAPI
MsWheaRscHandlerSmm (
  IN EFI_STATUS_CODE_TYPE             CodeType,
  IN EFI_STATUS_CODE_VALUE            Value,
  IN UINT32                           Instance,
  IN EFI_GUID                         *CallerId,
  IN EFI_STATUS_CODE_DATA             *Data OPTIONAL
  )
{
  EFI_STATUS Status;

  Status = ReportHwErrRecRouter(CodeType,
                                Value,
                                Instance,
                                CallerId,
                                Data,
                                MS_WHEA_PHASE_SMM,
                                MsWheaReportHandlerSmm);
  return Status;
}

/**
Populates the current time for WHEA records

@param[in,out]  *CurrentTime              A pointer to an EFI_TIME variable which will contain the current time after
                                          this function executes

@retval          BOOLEAN                  True if *CurrentTime was populated.
                                          False otherwise.
**/
BOOLEAN
PopulateTime(EFI_TIME* CurrentTime)
{
  return FALSE;
}

/**
Gets the Record ID variable and increments it for WHEA records

@param[in,out]  *RecordID                   Pointer to a UINT64 which will contain the record ID to be put on the next WHEA Record
@param[in]      *RecordIDGuid               Pointer to guid used to get the record ID variable

@retval          EFI_SUCCESS                The firmware has successfully stored the variable and its data as
                                            defined by the Attributes.
@retval          EFI_INVALID_PARAMETER      An invalid combination of attribute bits, name, and GUID was supplied, or the
                                            DataSize exceeds the maximum allowed.
@retval          EFI_INVALID_PARAMETER      VariableName is an empty string.
@retval          EFI_OUT_OF_RESOURCES       Not enough storage is available to hold the variable and its data.
@retval          EFI_DEVICE_ERROR           The variable could not be retrieved due to a hardware error.
@retval          EFI_WRITE_PROTECTED        The variable in question is read-only.
@retval          EFI_WRITE_PROTECTED        The variable in question cannot be deleted.
@retval          EFI_SECURITY_VIOLATION     The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS being set,
                                            but the AuthInfo does NOT pass the validation check carried out by the firmware.
@retval          EFI_NOT_FOUND              The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
GetRecordID(UINT64* RecordID, EFI_GUID *RecordIDGuid)
{
  UINTN Size = sizeof(UINT64);

  //Get the last record ID number used
  if(EFI_ERROR(mSmmVariable->SmmGetVariable(L"RecordID",RecordIDGuid,NULL,&Size,RecordID))) {

    DEBUG ((DEBUG_INFO, "%a Record ID variable not retrieved, initializing to 0\n", __FUNCTION__));
    *RecordID = 0;

  }

  (*RecordID)++; //increment the record ID number

  //Set the variable so the next record uses a unique record ID
  return mSmmVariable->SmmSetVariable(L"RecordID",
                                      RecordIDGuid,
                                      EFI_VARIABLE_NON_VOLATILE |
                                      EFI_VARIABLE_BOOTSERVICE_ACCESS,
                                      Size,
                                      RecordID);
}

/**
Entry to MsWheaReportSmm, register RSC handler and callback functions

@param[in] ImageHandle                The image handle.
@param[in] SystemTable                The system table.

@retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MsWheaReportSmmEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  DEBUG((DEBUG_INFO, "%a: enter...\n", __FUNCTION__));

  // locate the RSC protocol
  Status = gSmst->SmmLocateProtocol(&gEfiSmmRscHandlerProtocolGuid, NULL, (VOID**)&mRscHandlerProtocol);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a failed to RSC handler protocol (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // register for the RSC callback handler
  Status = mRscHandlerProtocol->Register(MsWheaRscHandlerSmm);
  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "%a failed to register MsWhea report RSC handler (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

  // Locate global smm variable service. By depex, this should not fail
  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&mSmmVariable);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a failed to locate smm variable protocol (%r)\n", __FUNCTION__, Status));
    goto Cleanup;
  }

Cleanup:
  DEBUG((DEBUG_INFO, "%a: exit (%r)\n", __FUNCTION__, Status));
  return Status;
}
