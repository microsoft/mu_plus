/** @file
  Simple SerialLib based Status Code handler

Copyright (C) Microsoft Corporation. All rights reserved.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiSmm.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SmmReportStatusCodeHandler.h>
#include <Library/SmmServicesTableLib.h>
#include "../Common/SerialStatusCodeHandler.h"

EFI_SMM_RSC_HANDLER_PROTOCOL  *mRscHandlerProtocol = NULL;

/**
  Status Code SMM Entry point.

  Register this handler with the SMM Router

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS  Successfully registered

**/
EFI_STATUS
EFIAPI
SmmEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
)
{
  EFI_STATUS                Status;

  Status = gSmst->SmmLocateProtocol(
                    &gEfiSmmRscHandlerProtocolGuid,
                    NULL,
                    (VOID **)&mRscHandlerProtocol
                    );
  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
    return Status;
  }


  Status = SerialPortInitialize();
  if (EFI_ERROR(Status))
  {
    ASSERT_EFI_ERROR(Status);
    return Status;
  }

  mRscHandlerProtocol->Register((EFI_SMM_RSC_HANDLER_CALLBACK)SerialStatusCode);

  return EFI_SUCCESS;
}

/**
  This routine writes a status code string to the correct place.

  @retval Buffer        Supplies a buffer holding the string to output.
  @retval NumberOfBytes Supplies a value that indicates how many bytes are in
                        the buffer.

**/
VOID
EFIAPI
WriteStatusCode (
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  //
  // Send the print string to a Serial Port
  //
  SerialPortWrite (Buffer, NumberOfBytes);
}
