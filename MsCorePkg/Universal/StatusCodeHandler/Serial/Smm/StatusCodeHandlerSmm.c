/** @file
  Simple SerialLib based Status Code handler

  Copyright (c) 2018, Microsoft Corporation

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

  @retval EFI_SUCESS  Successfully registered

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

  mRscHandlerProtocol->Register(SerialStatusCode);

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
