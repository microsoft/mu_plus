/**@file

Library registers an interrupt handler which catches exceptions related to memory
protections and turns them off for the next boot.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>

#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryProtectionHobLib.h>
#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/ResetSystemLib.h>

/**
  Page Fault handler which turns off memory protections and does a warm reset.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
EFIAPI
MemoryProtectionExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  MEMORY_PROTECTION_OVERRIDE val = MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT;

  DEBUG((DEBUG_ERROR, "%a - ExceptionData: 0x%x - InterruptType: 0x%x\n", __FUNCTION__, SystemContext.SystemContextX64->ExceptionData, InterruptType));

  DumpCpuContext (
    InterruptType,
    SystemContext
    );

  MemoryProtectionExceptionOverrideWrite (val);

  ResetWarm ();
}

/**
  Registers MemoryProtectionExceptionHandler using the EFI_CPU_ARCH_PROTOCOL.

  @param  Event          The Event that is being processed, not used.
  @param  Context        Event Context, not used.

**/
VOID
EFIAPI
CpuArchRegisterMemoryProtectionExceptionHandler (
    IN  EFI_EVENT   Event,
    IN  VOID       *Context
    )
{
  EFI_STATUS             Status;
  EFI_CPU_ARCH_PROTOCOL  *mCpu = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID**) &mCpu
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Failed to Locate gEfiCpuArchProtocolGuid. \
      Memory protections cannot be turned off via Page Fault handler.\n",
      __FUNCTION__
      ));

    return;
  }

  Status = mCpu->RegisterInterruptHandler (
                   mCpu,
                   EXCEPT_IA32_PAGE_FAULT,
                   MemoryProtectionExceptionHandler
                   );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Failed to Register Exception Handler. Memory protections cannot be turned off via Page Fault handler.\n",
      __FUNCTION__
      ));
  }
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
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  CpuArchExHandlerCallBackEvent;
  VOID       *mCpuArchExHandlerRegistration = NULL;

  // Don't install exception handler if all memory mitigations are off
  if (!(gMPS.CpuStackGuard                ||
        HEAP_GUARD_ACTIVE                 ||
        HEAP_GUARD_PAGE_PROTECTION_ACTIVE ||
        HEAP_GUARD_POOL_PROTECTION_ACTIVE ||
        NX_PROTECTION_ACTIVE              ||
        IMAGE_PROTECTION_ACTIVE           ||
        NULL_POINTER_DETECTION_ACTIVE     ||
        gMPS.SetNxForStack)) {
    return EFI_SUCCESS;
  }

  Status = SystemTable->BootServices->CreateEvent (
                                        EVT_NOTIFY_SIGNAL,
                                        TPL_CALLBACK,
                                        CpuArchRegisterMemoryProtectionExceptionHandler,
                                        NULL,
                                        &CpuArchExHandlerCallBackEvent
                                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: - Failed to create CpuArch Notify Event. Memory protections cannot be turned off via Page Fault handler.\n",
      __FUNCTION__
      ));
  }

  // NOTE: Installing an exception handler before gEfiCpuArchProtocolGuid has been produced causes
  //       the default handler to be overwritten by the default handlers. Registering a protocol notify
  //       ensures the handler will be registered as soon as possible.
  SystemTable->BootServices->RegisterProtocolNotify (
                               &gEfiCpuArchProtocolGuid,
                               CpuArchExHandlerCallBackEvent,
                               &mCpuArchExHandlerRegistration
                               );

  return EFI_SUCCESS;
}