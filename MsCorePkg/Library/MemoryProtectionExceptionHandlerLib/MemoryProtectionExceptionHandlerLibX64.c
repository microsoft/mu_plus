/**@file

Library registers an interrupt handler which catches exceptions related to memory
protections and logs them in the platform's persistent storage.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>

#include <Pi/PiStatusCode.h>

#include <Protocol/DebugSupport.h>

#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/ExceptionPersistenceLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PcdLib.h>

#define IA32_PF_EC_ID  BIT4

/**
  Common constructor for this library.

  @param ImageHandle          Image handle this library.
  @param SystemTable          Pointer to SystemTable.
  @param MemProtExVector      Memory Protection Exception Vector.
  @param StackCookieExVector  Stack Cookie Exception Vector.

  @retval EFI_SUCCESS         Successfully registered CpuArchRegisterMemoryProtectionExceptionHandlers
  @retval EFI_ABORTED         Failed to register CpuArchRegisterMemoryProtectionExceptionHandlers

**/
EFI_STATUS
EFIAPI
MemoryProtectionExceptionHandlerCommonConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN  UINTN            MemProtExVector,
  IN  UINTN            StackCookieExVector
  );

/**
  Fault handler which logs exceptions in the platform specific early store and does a warm reset.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
EFIAPI
MemoryProtectionExceptionHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN    pointer;
  BOOLEAN  IgnoreNext = FALSE;

  if (!EFI_ERROR (ExPersistGetIgnoreNextPageFault (&IgnoreNext)) &&
      IgnoreNext &&
      (InterruptType == EXCEPT_IA32_PAGE_FAULT))
  {
    ExPersistClearIgnoreNextPageFault ();
    return;
  }

  DumpCpuContext (
    InterruptType,
    SystemContext
    );

  if (SystemContext.SystemContextX64 != NULL) {
    if ((InterruptType == EXCEPT_IA32_PAGE_FAULT) &&
        ((SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_ID) != 0))
    {
      // The RIP in SystemContext could not be used if it is page fault with I/D set.
      pointer = (UINTN)SystemContext.SystemContextX64->Rsp;
    } else {
      pointer = (UINTN)SystemContext.SystemContextX64->Rip;
    }

    MsWheaESAddRecordV0 (
      (EFI_COMPUTING_UNIT_MEMORY|EFI_CU_MEMORY_EC_UNCORRECTABLE),
      (UINT64)PeCoffSearchImageBase (pointer),
      SystemContext.SystemContextX64->Rip,
      NULL,
      NULL
      );
  } else {
    MsWheaESAddRecordV0 (
      (EFI_COMPUTING_UNIT_MEMORY|EFI_CU_MEMORY_EC_UNCORRECTABLE),
      SIGNATURE_64 ('M', 'E', 'M', ' ', 'P', 'R', 'O', 'T'),
      SIGNATURE_64 ('E', 'X', 'C', 'E', 'P', 'T', ' ', ' '),
      NULL,
      NULL
      );
  }

  if (EFI_ERROR (ExPersistSetException (ExceptionPersistPageFault))) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - Error mark exception occurred in platform early store\n",
      __FUNCTION__
      ));
  }

  ResetWarm ();
}

/**
  Main entry for this library.

  @param ImageHandle     Image handle this library.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCCESS

**/
EFI_STATUS
EFIAPI
MemoryProtectionExceptionHandlerConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  MemoryProtectionExceptionHandlerCommonConstructor (
    ImageHandle,
    SystemTable,
    EXCEPT_IA32_PAGE_FAULT,
    PcdGet8 (PcdStackCookieExceptionVector)
    );

  return EFI_SUCCESS;
}
