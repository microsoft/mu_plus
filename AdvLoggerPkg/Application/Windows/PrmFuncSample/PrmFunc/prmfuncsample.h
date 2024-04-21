/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    prmfuncsample.h

Environment:

    Kernel mode

--*/

#if !defined (_PRMFUNCTEST_H_)
#define _PRMFUNCTEST_H_

#include <ntddk.h>
#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <initguid.h>
#include <prminterface.h>

#define PRMFUNCTEST_POOL_TAG  (ULONG) 'fmrP'
// this is because of a bug in some versions of Windows where the EfiStatus gets truncated to a UINT32
#define EFI_BUFFER_TOO_SMALL_TRUNCATED  0x5
#define EFI_BUFFER_TOO_SMALL            0x8000000000000005

DEFINE_GUID (
  GUID_ADVLOGGER_PRM_HANDLER,
  0x0f8aef11,
  0x77b8,
  0x4d7f,
  0x84,
  0xcc,
  0xfe,
  0x0c,
  0xce,
  0x64,
  0xac,
  0x14
  );

#define PRM_PARAMETER_BUFFER_SIZE  308

typedef struct _PRM_TEST_PARAMETERS {
  GUID     Guid;
  UCHAR    ParameterBuffer[PRM_PARAMETER_BUFFER_SIZE];
} PRM_TEST_PARAMETERS, *PPRM_TEST_PARAMETERS;

typedef struct _PRM_DIRECT_CALL_PARAMETERS {
  GUID     Guid;
  UCHAR    ParameterBuffer[PRM_PARAMETER_BUFFER_SIZE];
} PRM_DIRECT_CALL_PARAMETERS, *PPRM_DIRECT_CALL_PARAMETERS;

typedef struct _PRM_TEST_RESULT {
  NTSTATUS    Status;
  ULONG64     EfiStatus;
  UCHAR       Buffer[PRM_PARAMETER_BUFFER_SIZE];
} PRM_TEST_RESULT, *PPRM_TEST_RESULT;

typedef struct {
  VOID      *OutputBuffer;
  UINT32    *OutputBufferSize;
} ADVANCED_LOGGER_PRM_PARAMETER_BUFFER;

#endif
