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

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/SerialPortLib.h>
#include <Library/DebugLib.h>
#include <Ppi/ReportStatusCodeHandler.h>
#include "../Common/SerialStatusCodeHandler.h"

/**
  Convert status code value and extended data to readable ASCII string, send string to serial I/O device.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or
                           software entity. This includes information about the class and
                           subclass that is used to classify the entity as well as an operation.
                           For progress codes, the operation is the current activity.
                           For error codes, it is the exception.For debug codes,it is not defined at this time.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. A system may contain multiple entities that match a class/subclass
                           pairing. The instance differentiates between them. An instance of 0 indicates
                           that instance information is unavailable, not meaningful, or not relevant.
                           Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      Status code reported to serial I/O successfully.

**/
EFI_STATUS
EFIAPI
SerialStatusCodePei
(
  IN CONST  EFI_PEI_SERVICES        **PeiServices,
  IN        EFI_STATUS_CODE_TYPE    Type,
  IN        EFI_STATUS_CODE_VALUE   Value,
  IN        UINT32                  Instance,
  IN CONST  EFI_GUID                *CallerId,
  IN CONST  EFI_STATUS_CODE_DATA    *Data
)
{
  return SerialStatusCode(Type, Value, Instance, CallerId, Data);
}

/**
  Status Code PEIM Entry point.
  
  Register this handler with the Pei Router

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCESS  Successfully registered

**/
EFI_STATUS
EFIAPI
PeiEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_RSC_HANDLER_PPI     *Ppi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiRscHandlerPpiGuid, 
             0,
             NULL,
             (VOID **) &Ppi
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

  Status = Ppi->Register (SerialStatusCodePei);
  ASSERT_EFI_ERROR (Status);
  
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
    SerialPortWrite(Buffer, NumberOfBytes);
}
