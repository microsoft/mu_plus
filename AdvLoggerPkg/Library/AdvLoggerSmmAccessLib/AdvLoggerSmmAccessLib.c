/** @file
  SMM GetVariable Implementation to retrieve the AdvLogger memory.

  Copyright (c) Microsoft Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <AdvancedLoggerInternal.h>

#include <Guid/SmmVariableCommon.h>

#include <Protocol/AdvancedLogger.h>
#include <AdvancedLoggerInternalProtocol.h>

#include <Library/AdvLoggerAccessLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiBootServicesTableLib.h>

STATIC ADVANCED_LOGGER_INFO  *mLoggerInfo        = NULL;
STATIC UINT32                mBufferSize         = 0;
STATIC EFI_PHYSICAL_ADDRESS  mMaxAddress         = 0;
STATIC UINTN                 mLoggerTransferSize = 0;
extern UINTN                 mVariableBufferPayloadSize;

/**
    CheckAddress

    The address of the ADVANCE_LOGGER_INFO block pointer is captured before END_OF_DXE.  The
    pointers LogBuffer and LogCurrent, and LogBufferSize, could be written to by untrusted code.  Here, we check that
    the pointers are within the allocated mLoggerInfo space, and that LogBufferSize, which is used in multiple places
    to see if a new message will fit into the log buffer, is valid.

    @param          NONE

    @return         BOOLEAN     TRUE - mInforBlock passes security checks
    @return         BOOLEAN     FALSE- mInforBlock failed security checks

**/
STATIC
BOOLEAN
ValidateInfoBlock (
  VOID
  )
{
  if (mLoggerInfo == NULL) {
    return FALSE;
  }

  if (mLoggerInfo->Signature != ADVANCED_LOGGER_SIGNATURE) {
    return FALSE;
  }

  if (mLoggerInfo->LogBuffer != (PA_FROM_PTR (mLoggerInfo + 1))) {
    return FALSE;
  }

  if ((mLoggerInfo->LogCurrent > mMaxAddress) ||
      (mLoggerInfo->LogCurrent < mLoggerInfo->LogBuffer))
  {
    return FALSE;
  }

  if (mBufferSize == 0) {
    mBufferSize = mLoggerInfo->LogBufferSize;
  } else {
    if (mLoggerInfo->LogBufferSize != mBufferSize) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
    AdvLoggerInit - Obtain the address of the logger info block.

    @param          NONE

    @return         None
**/
VOID
AdvLoggerAccessInit (
  VOID
  )
{
  ADVANCED_LOGGER_PROTOCOL  *LoggerProtocol;
  UINTN                     TempSize;
  EFI_STATUS                Status;

  if (gBS == NULL) {
    return;
  }

  //
  // PcdMaxVariableSize include the variable header and the variable name size.
  //
  // The code requires the mLoggerTransferSize to be consistent, and not dependent
  // on the variable name size.  The requirement is due the variable name is the
  // block number of the data.  If we cannot compute a common size, then turn
  // off AdvLoggerAccess.
  //
  // Round down to the next lower 1KB boundary below PcdMaxVariableSize.
  //
  Status = SafeUintnSub (PcdGet32 (PcdMaxVariableSize), 1023, &TempSize);
  if (EFI_ERROR (Status)) {
    mLoggerTransferSize = 0;
  } else {
    mLoggerTransferSize = (TempSize / 1024) * 1024;
  }

  //
  // Locate the Logger Information block.
  //
  Status = gBS->LocateProtocol (
                  &gAdvancedLoggerProtocolGuid,
                  NULL,
                  (VOID **)&LoggerProtocol
                  );
  if (!EFI_ERROR (Status)) {
    mLoggerInfo = LOGGER_INFO_FROM_PROTOCOL (LoggerProtocol);
    if (mLoggerInfo != NULL) {
      mMaxAddress = mLoggerInfo->LogBuffer + mLoggerInfo->LogBufferSize;
    }

    if (!ValidateInfoBlock ()) {
      mLoggerInfo = NULL;
    }
  }

  //
  // If mLoggerInfo is NULL at this point, there is no Advanced Logger.
  //

  DEBUG ((DEBUG_INFO, "%a: LoggerInfo=%p, code=%r\n", __FUNCTION__, mLoggerInfo, Status));
}

/**

  This code clears the AdvLogger private storage. This hook only supports
  the operation below:

  Variable name: ADV_LOGGER_CLEAR_VAR
  Variable size: 0
  Variable attribute: BS + RT
  Variable data: NULL
  Operation environment: After Runtime Only.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param[in]  VariableName       A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid         A unique identifier for the vendor.
  @param[in]  Attributes         Attributes bitmask to set for the variable.
  @param[in]  DataSize           The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE or
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data               The contents for the variable.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Found the specified data.
  @return EFI_NOT_FOUND             Not found, or end of logger buffer.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.
  @return EFI_UNSUPPORTED           Advanced Logger is not present.

**/
EFI_STATUS
AdvLoggerAccessSetVariable (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_PHYSICAL_ADDRESS  CurrentBuffer;
  EFI_PHYSICAL_ADDRESS  NewBuffer;
  EFI_PHYSICAL_ADDRESS  OldValue;

  if ((!ValidateInfoBlock ()) || (mLoggerTransferSize == 0)) {
    return EFI_UNSUPPORTED;
  }

  if ((VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (StrCmp (VariableName, ADV_LOGGER_CLEAR_VAR) ||
      (DataSize != 0) || (Data != NULL) ||
      (Attributes != EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)) {
    // Only supports ADV_LOGGER_CLEAR_VAR delete
    return EFI_ACCESS_DENIED;
  }

  if ((mLoggerInfo == NULL) || (!mLoggerInfo->AtRuntime)) {
    // Only supports runtime request for now
    return EFI_ACCESS_DENIED;
  }

  // Simply clear the cursor to the beginning.
  do {
    CurrentBuffer = mLoggerInfo->LogCurrent;

    NewBuffer = mLoggerInfo->LogBuffer;
    OldValue  = InterlockedCompareExchange64 (
                  (UINT64 *)&mLoggerInfo->LogCurrent,
                  (UINT64)CurrentBuffer,
                  (UINT64)NewBuffer
                  );
  } while (OldValue != CurrentBuffer);

  DEBUG ((DEBUG_INFO, "%a Advanced logger buffer cleared.\n", __func__));

  return EFI_SUCCESS;
}

/**

  This code accesses the AdvLogger private storage.

  The variable names are:

  V[decimal digits)]

  V0 - returns the first mLoggerTransferSize bytes of data.
  V1 - returns the second mLoggerTransferSize bytes of data.
  V2 ...
  ...

  Vxx - returns the last few bytes of the log
  Vxx+1 = returns EFI_NOT_FOUND.



  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Found the specified data.
  @return EFI_NOT_FOUND             Not found, or end of logger buffer.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.
  @return EFI_UNSUPPORTED           Advanced Logger is not present.

**/
EFI_STATUS
AdvLoggerAccessGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  CHAR16      *EndPointer;
  EFI_STATUS  Status;
  UINTN       BlockNumber;
  UINT8       *LogBufferStart;
  UINT8       *LogBufferEnd;
  UINTN       LogBufferSize;

  if ((!ValidateInfoBlock ()) || (mLoggerTransferSize == 0)) {
    return EFI_UNSUPPORTED;
  }

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] != L'V') {
    return EFI_NOT_FOUND;
  }

  Status = StrDecimalToUintnS (
             &VariableName[1],
             &EndPointer,
             &BlockNumber
             );

  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*EndPointer != L'\0') {
    return EFI_INVALID_PARAMETER;
  }

  LogBufferStart  = (UINT8 *)mLoggerInfo;
  LogBufferEnd    = (UINT8 *)PTR_FROM_PA (mLoggerInfo->LogCurrent);
  LogBufferStart += (BlockNumber * mLoggerTransferSize);

  if (LogBufferStart >= LogBufferEnd) {
    return EFI_NOT_FOUND;
  }

  LogBufferSize = MIN (((UINTN)(LogBufferEnd - LogBufferStart)), mLoggerTransferSize);
  if (Attributes != NULL) {
    *Attributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  }

  if (LogBufferSize > *DataSize) {
    *DataSize = LogBufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *DataSize = LogBufferSize;
  CopyMem (Data, LogBufferStart, LogBufferSize);

  return EFI_SUCCESS;
}

/**
    AdvLoggerAccessAtRuntime - Exit Boot Services.

    @param          NONE

    @return         NONE
**/
VOID
AdvLoggerAccessAtRuntime (
  VOID
  )
{
  if (mLoggerInfo != NULL) {
    mLoggerInfo->AtRuntime = TRUE;
  }
}

/**
    AdvLoggerAccessGoneVirtual - Virtual Address Change.

    @param          NONE

    @return         NONE
**/
VOID
AdvLoggerAccessGoneVirtual (
  VOID
  )
{
  if (mLoggerInfo != NULL) {
    mLoggerInfo->GoneVirtual = TRUE;
  }
}
