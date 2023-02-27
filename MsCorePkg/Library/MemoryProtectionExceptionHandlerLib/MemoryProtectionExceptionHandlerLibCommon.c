/**@file

Library registers an interrupt handler which catches exceptions related to memory
protections and logs them in the platform's persistent storage.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>

#include <Pi/PiStatusCode.h>

#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeMemoryProtectionHobLib.h>
#include <Library/ExceptionPersistenceLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MsWheaEarlyStorageLib.h>
#include <Library/PeCoffGetEntryPointLib.h>

STATIC EFI_HANDLE  mImageHandle         = NULL;
STATIC UINTN       mMemProtExVector     = 0;
STATIC UINTN       mStackCookieExVector = 1;

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
  );

/**
  Stack cookie failure handler which does a warm reset if stack cookie protection is active.

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.

**/
VOID
EFIAPI
MemoryProtectionStackCookieFailureHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  if (gDxeMps.StackCookies == TRUE) {
    DEBUG ((DEBUG_ERROR, "Stack Cookie Exception!\n"));
    ExPersistClearExceptions ();
    ExPersistSetException (ExceptionPersistStackCookie);
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
CpuArchRegisterMemoryProtectionExceptionHandlers (
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
                   mMemProtExVector,
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
                   mStackCookieExVector,
                   MemoryProtectionStackCookieFailureHandler
                   );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: - Failed to Register Stack Cookie Failure Exception Handler.\n",
      __FUNCTION__
      ));
  }
}

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
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   CpuArchExHandlerCallBackEvent;
  VOID        *mCpuArchExHandlerRegistration = NULL;

  mImageHandle         = ImageHandle;
  mMemProtExVector     = MemProtExVector;
  mStackCookieExVector = StackCookieExVector;

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
                                        CpuArchRegisterMemoryProtectionExceptionHandlers,
                                        NULL,
                                        &CpuArchExHandlerCallBackEvent
                                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: - Failed to create CpuArch Notify Event. Memory protections cannot be turned off via Page Fault handler.\n",
      __FUNCTION__
      ));
    return EFI_ABORTED;
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
