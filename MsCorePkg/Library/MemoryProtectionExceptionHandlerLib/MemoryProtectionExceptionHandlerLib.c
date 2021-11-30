/**@file

Library registers an interrupt handler which catches exceptions related to memory
protections and turns them off for the next boot.

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
#include <Library/MemoryProtectionHobLib.h>
#include <Library/MemoryProtectionExceptionLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#define IA32_PF_EC_ID   BIT4

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
  MEMORY_PROTECTION_OVERRIDE val;
  UINTN pointer;
  
  val = MEM_PROT_VALID_BIT | MEM_PROT_EX_HIT_BIT;

  DumpCpuContext (
    InterruptType,
    SystemContext
  );

  if (SystemContext.SystemContextX64 != NULL) {

    if ((InterruptType == EXCEPT_IA32_PAGE_FAULT) &&
        ((SystemContext.SystemContextX64->ExceptionData & IA32_PF_EC_ID) != 0)) {
      // The RIP in SystemContext could not be used if it is page fault with I/D set.
      pointer = (UINTN) SystemContext.SystemContextX64->Rsp;
    } else {
      pointer = (UINTN) SystemContext.SystemContextX64->Rip;
    }

    MsWheaESAddRecordV0 (
      EFI_ERROR_MAJOR | EFI_SW_EC_IA32_PAGE_FAULT,
      (UINT64) PeCoffSearchImageBase (pointer),
      SystemContext.SystemContextX64->Rip,
      NULL,
      NULL
      );
  } else {
    MsWheaESAddRecordV0 (
      EFI_ERROR_MAJOR | EFI_SW_EC_IA32_PAGE_FAULT,
      SIGNATURE_64('M', 'E', 'M', ' ', 'P', 'R', 'O', 'T'),
      SIGNATURE_64('E', 'X', 'C', 'E', 'P', 'T', ' ', ' '),
      NULL,
      NULL
      );
  }

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
  // if (!(gMPS.CpuStackGuard                    ||
  //       (gMPS.HeapGuardPolicy.Data && (gMPS.HeapGuardPageType.Data || gMPS.HeapGuardPoolType.Data)) ||
  //       gMPS.DxeNxProtectionPolicy.Data       ||
  //       gMPS.ImageProtectionPolicy.Data       ||
  //       gMPS.NullPointerDetectionPolicy.Data)) {
  if (TRUE) {  
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