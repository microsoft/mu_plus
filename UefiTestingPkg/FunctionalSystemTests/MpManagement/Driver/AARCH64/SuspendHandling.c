/** @file
  TODO: Populate this.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmSmcLib.h>

#include <IndustryStandard/ArmStdSmc.h>
#include <Pi/PiMultiPhase.h>
#include <Protocol/MpService.h>

#include "MpManagementInternal.h"

#define INTERRUPT_SOURCE_NUMBER     0x04

#define GICV3_MAX_SGI_TARGETS	16

#define MPIDR_AFFLVL_MASK	((UINT64)0xff)
#define MPIDR_AFF0_SHIFT	0
#define MPIDR_AFF1_SHIFT	8
#define MPIDR_AFF2_SHIFT	16
#define MPIDR_AFF3_SHIFT	32
#define MPIDR_AFFLVL0_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL1_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL2_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL3_VAL(mpidr) \
		(((mpidr) >> MPIDR_AFF3_SHIFT) & MPIDR_AFFLVL_MASK)

/* ICC SGI macros */
#define SGIR_TGT_MASK			((UINT64)0xffff)
#define SGIR_AFF1_SHIFT			16
#define SGIR_INTID_SHIFT		24
#define SGIR_INTID_MASK			((UINT64)0xf)
#define SGIR_AFF2_SHIFT			32
#define SGIR_IRM_SHIFT			40
#define SGIR_IRM_MASK			((UINT64)0x1)
#define SGIR_AFF3_SHIFT			48
#define SGIR_AFF_MASK			((UINT64)0xf)

#define SGIR_IRM_TO_AFF			(0)

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt)	\
	((((UINT64) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) |	\
	 (((UINT64) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) |	\
	 (((UINT64) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) |	\
	 (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) |		\
	 (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) |		\
	 ((_tgt) & SGIR_TGT_MASK))

void gicv3_raise_non_secure_g1_sgi(UINTN sgi_num, UINTN target)
{
	UINT32 tgt, aff3, aff2, aff1, aff0;
	UINT64 sgi_val;

	/* Extract affinity fields from target */
	aff0 = MPIDR_AFFLVL0_VAL(target);
	aff1 = MPIDR_AFFLVL1_VAL(target);
	aff2 = MPIDR_AFFLVL2_VAL(target);
	aff3 = MPIDR_AFFLVL3_VAL(target);

	/*
	 * Make target list from affinity 0, and ensure GICv3 SGI can target
	 * this PE.
	 */
	tgt = (1 << aff0);

	/* Raise SGI to PE specified by its affinity */
	sgi_val = GICV3_SGIR_VALUE(aff3, aff2, aff1, sgi_num, SGIR_IRM_TO_AFF,
			tgt);

	/*
	 * Ensure that any shared variable updates depending on out of band
	 * interrupt trigger are observed before raising SGI.
	 */
	// dsbishst();
	ArmGicV3SendNsG1Sgi(sgi_val);
	// isb();
}

EFI_STATUS
SetupInterruptStatus (
  IN  UINTN       CpuIndex
  )
{
  // Set binary point reg to 0x7 (no preemption)
  ArmGicV3SetBinaryPointer (0x7);

  // Set priority mask reg to 0xff to allow all priorities through
  ArmGicV3SetPriorityMask (0xff);

  // Enable gic cpu interface
  ArmGicV3EnableInterruptInterface ();

  ArmGicSetInterruptPriority (
    PcdGet64 (PcdGicDistributorBase),
    PcdGet64 (PcdGicRedistributorsBase),
    INTERRUPT_SOURCE_NUMBER,
    0x0
  );

  // Enable the intended interrupt source
  ArmGicEnableInterrupt ((UINT32)PcdGet64 (PcdGicDistributorBase), (UINT32)PcdGet64 (PcdGicRedistributorsBase), INTERRUPT_SOURCE_NUMBER);

  return EFI_SUCCESS;
}

EFI_STATUS
RestoreInterruptStatus (
  IN  UINTN       CpuIndex
  )
{
  // Disable gic cpu interface
  ArmGicV3DisableInterruptInterface ();

  // Disable the intended interrupt source
  ArmGicDisableInterrupt ((UINT32)PcdGet64 (PcdGicDistributorBase), (UINT32)PcdGet64 (PcdGicRedistributorsBase), INTERRUPT_SOURCE_NUMBER);

  return EFI_SUCCESS;
}

VOID
EFIAPI
CpuArchWakeFromSleep (
  UINTN   CpuIndex
  )
{
  // Sending SGI to the specified secondary CPU interfaces
  gicv3_raise_non_secure_g1_sgi (INTERRUPT_SOURCE_NUMBER, CpuIndex);
}

EFI_STATUS
EFIAPI
CpuArchHalt (
  VOID
  )
{
  ArmCallWFI ();

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CpuArchClockGate (
  IN UINTN         PowerLevel
  )
{
  ARM_SMC_ARGS  Args;
  EFI_STATUS    Status;

  Status = EFI_SUCCESS;

  /* Turn the AP on */
  if (sizeof (Args.Arg0) == sizeof (UINT32)) {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH32;
  } else {
    Args.Arg0 = ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64;
  }

  // Parameter for power_state
  Args.Arg1 = PowerLevel;
  // Parameter for entrypoint, only need for powerdown state
  Args.Arg2 = 0;
  // Parameter for context_id, only need for powerdown state
  Args.Arg3 = 0;

  ArmCallSmc (&Args);

  if (Args.Arg0 != ARM_SMC_PSCI_RET_SUCCESS) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

EFI_STATUS
EFIAPI
CpuArchSleep (
  IN UINTN         PowerLevel
  )
{
  return CpuArchClockGate (PowerLevel);
}
