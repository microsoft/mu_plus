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
#include <Protocol/MemoryProtectionNonstopMode.h>

#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/ExceptionPersistenceLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

#define IA32_PF_EC_ID  BIT4
#define EXCEPT_I2C     0x2c

STATIC EFI_HANDLE  mImageHandle = NULL;

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
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  UINTN                                    pointer;
  MEMORY_PROTECTION_NONSTOP_MODE_PROTOCOL  *NonstopModeProtocol;
  BOOLEAN                                  IgnoreNext = FALSE;
  EFI_STATUS                               Status;

  if (!EFI_ERROR (ExPersistGetIgnoreNextPageFault (&IgnoreNext)) &&
      IgnoreNext &&
      (InterruptType == EXCEPT_IA32_PAGE_FAULT))
  {
    ExPersistClearIgnoreNextPageFault ();
    Status = gBS->LocateProtocol (&gMemoryProtectionNonstopModeProtocolGuid, NULL, (VOID **)&NonstopModeProtocol);
    if (!EFI_ERROR (Status)) {
      Status = NonstopModeProtocol->ClearPageFault (InterruptType, SystemContext);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a - Error Clearing Page Fault\n", __FUNCTION__));
      } else {
        DEBUG ((DEBUG_INFO, "%a - Page Fault Cleared\n", __FUNCTION__));
      }

      return;
    }
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
  I2C handler which does a warm reset if stack cookie protection is active.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
EFIAPI
MemoryProtectionI2CHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if (gDxeMps.StackCookies == TRUE) {
    DEBUG ((DEBUG_ERROR, "Stack Cookie Exception!\n"));
    ExPersistClearExceptions();
    ExPersistSetException (ExceptionPersistI2C);
    ResetWarm ();
  } else {
    return;
  }
}

/**
  Registers MemoryProtectionExceptionHandler using the EFI_CPU_ARCH_PROTOCOL.

  @param  Event          The Event that is being processed, not used.
  @param  Context        Event Context, not used.

**/
VOID
EFIAPI
CpuArchRegisterMemoryProtectionExceptionHandler (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS             Status;
  EFI_CPU_ARCH_PROTOCOL  *mCpu = NULL;

  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID **)&mCpu
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
      "%a: - Failed to Register Exception Handler. Page faults won't be logged via ExceptionPersistenceLib.\n",
      __FUNCTION__
      ));
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &mImageHandle,
                    &gMemoryProtectionExceptionHandlerGuid,
                    NULL,
                    NULL
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: - Exception handler registered, but NULL protocol installation failed.\n",
        __FUNCTION__
        ));
    }
  }

  Status = mCpu->RegisterInterruptHandler (
                   mCpu,
                   EXCEPT_I2C,
                   MemoryProtectionI2CHandler
                   );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Failed to Register I2C Exception Handler.\n",
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   CpuArchExHandlerCallBackEvent;
  VOID        *mCpuArchExHandlerRegistration = NULL;

  mImageHandle = ImageHandle;

  // Don't install exception handler if all memory mitigations are off
  if (!((gDxeMps.HeapGuardPolicy.Data && (gDxeMps.HeapGuardPageType.Data || gDxeMps.HeapGuardPoolType.Data)) ||
        gDxeMps.NxProtectionPolicy.Data          ||
        gDxeMps.ImageProtectionPolicy.Data       ||
        gDxeMps.NullPointerDetectionPolicy.Data))
  {
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
