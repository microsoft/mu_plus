/** @file

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Register/ArchitecturalMsr.h>
#include <Library/UnitTestLib.h>
#include <Library/ResetSystemLib.h>
#include <Protocol/Cpu.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Resets the system on interrupt

  @param  InterruptType    Defines the type of interrupt or exception that
                           occurred on the processor.This parameter is processor architecture specific.
  @param  SystemContext    A pointer to the processor context when
                           the interrupt occurred on the processor.
**/
VOID
EFIAPI
InterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  // Avoid using runtime services to reset the system because doing so will raise the TPL level
  // when it is already on TPL_HIGH. HwResetSystemLib is used here instead to perform a bare-metal reset
  // and sidestep this issue.
  ResetWarm ();
}

/**
  Registers the interrupt handler which will be invoked when a page fault occurs.

  @retval     EFI_SUCCESS         Interrupt handler successfully installed
  @retval     EFI_UNSUPPORTED     Installing the interrupt handler is not supported.
  @retval     others              Error occurred during installation.
**/
EFI_STATUS
EFIAPI
RegisterMemoryProtectionTestAppInterruptHandler (
  VOID
  )
{
  EFI_CPU_ARCH_PROTOCOL  *CpuProtocol = NULL;
  EFI_STATUS             Status;

  // Find the CPU Architecture Protocol
  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&CpuProtocol);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to locate gEfiCpuArchProtocolGuid. Status = %r\n", Status));
    return EFI_INVALID_PARAMETER;
  }

  // Uninstall the existing page fault handler
  CpuProtocol->RegisterInterruptHandler (CpuProtocol, EXCEPT_IA32_PAGE_FAULT, NULL);

  return CpuProtocol->RegisterInterruptHandler (CpuProtocol, EXCEPT_IA32_PAGE_FAULT, InterruptHandler);
}

/**
  Checks if the hardware NX protection is enabled.

  @param[in]  Context   The unit test context.

  @retval     UNIT_TEST_PASSED    Hardware NX protection is enabled.
  @retval     others              Hardware NX protection is not enabled.
**/
UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  MSR_IA32_EFER_REGISTER  Efer;

  Efer.Uint64 = AsmReadMsr64 (MSR_IA32_EFER);
  if (Efer.Bits.NXE == 1) {
    return UNIT_TEST_PASSED;
  }

  UT_LOG_WARNING ("Efer set as 0x%x\n", Efer);
  return UNIT_TEST_ERROR_TEST_FAILED;
}
