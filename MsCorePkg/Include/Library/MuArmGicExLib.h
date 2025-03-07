/**
  @file  MuArmGicExLib.h

  This file contains the extended definitions beyond ArmGicLib.

  It also defined a few interfaces to query and manipulate the GIC registers.

  Copyright (c) 2014, ARM Limited. All rights reserved.
  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MU_ARM_GIC_EX_LIB_H_
#define MU_ARM_GIC_EX_LIB_H_

#include <Library/ArmGicLib.h>

#define ARM_GICR_ISPENDR  0x0200        // Interrupt Set-Pending Registers
#define ARM_GICR_ICPENDR  0x0280        // Interrupt Clear-Pending Registers

/* ICC SGI macros */
#define SGIR_TGT_MASK     ((UINT64)0xffff)
#define SGIR_AFF1_SHIFT   16
#define SGIR_INTID_SHIFT  24
#define SGIR_INTID_MASK   ((UINT64)0xf)
#define SGIR_AFF2_SHIFT   32
#define SGIR_IRM_SHIFT    40
#define SGIR_IRM_MASK     ((UINT64)0x1)
#define SGIR_AFF3_SHIFT   48
#define SGIR_AFF_MASK     ((UINT64)0xff)

#define SGIR_IRM_TO_AFF     0
#define SGIR_IRM_TO_OTHERS  1

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt) \
  ((((UINT64) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) | \
   (((UINT64) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) | \
   (((UINT64) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) | \
   (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) | \
   (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) | \
   ((_tgt) & SGIR_TGT_MASK))

//
// GIC definitions
//
typedef enum {
  ARM_GIC_ARCH_REVISION_2,
  ARM_GIC_ARCH_REVISION_3
} ARM_GIC_ARCH_REVISION;

ARM_GIC_ARCH_REVISION
EFIAPI
ArmGicGetSupportedArchRevision (
  VOID
  );

/**
  Helper function to set the pending interrupt in the GIC.

  @param[in]  GicDistributorBase    The base address of the GIC Distributor.
  @param[in]  GicRedistributorBase  The base address of the GIC Redistributor.
  @param[in]  Source                The interrupt source number.
**/
VOID
EFIAPI
ArmGicSetPendingInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

/**
  Helper function to clear the pending interrupt in the GIC.

  @param[in]  GicDistributorBase    The base address of the GIC Distributor.
  @param[in]  GicRedistributorBase  The base address of the GIC Redistributor.
  @param[in]  Source                The interrupt source number.
 */
VOID
EFIAPI
ArmGicClearPendingInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

/**
  Helper function to check if the interrupt is pending in the GIC.

  @param[in]  GicDistributorBase    The base address of the GIC Distributor.
  @param[in]  GicRedistributorBase  The base address of the GIC Redistributor.
  @param[in]  Source                The interrupt source number.

  @retval TRUE  The interrupt is pending.
  @retval FALSE The interrupt is not pending.
**/
BOOLEAN
EFIAPI
ArmGicIsInterruptPending (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

/**
  Send a GIC SGI to a specific target. This function is available for GICv2 and GICv3/4.

  @param GicDistributorBase          The base address of the GIC Distributor.
  @param TargetListFilter            The target list filter.
  @param CPUTargetList               The CPU target list.
  @param SgiId                       The SGI ID.
**/
VOID
EFIAPI
ArmGicSendSgiToEx (
  IN  UINTN  GicDistributorBase,
  IN  UINT8  TargetListFilter,
  IN  UINTN  CPUTargetList,
  IN  UINT8  SgiId
  );

/**
  Send a GICv3 SGI to a specific target.

  @param SgiVal  The value to be written to the ICC_SGI1R_EL1 register.
**/
VOID
ArmGicV3SendNsG1Sgi (
  IN UINT64  SgiVal
  );

UINT32
EFIAPI
ArmGicV3GetControlSystemRegisterEnable (
  VOID
  );

UINT32
EFIAPI
ArmGicGetInterfaceIdentification (
  IN  UINTN  GicInterruptInterfaceBase
  );

// GIC Secure interfaces
VOID
EFIAPI
ArmGicSetupNonSecure (
  IN  UINTN  MpId,
  IN  UINTN  GicDistributorBase,
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicSetSecureInterrupts (
  IN  UINTN  GicDistributorBase,
  IN  UINTN  *GicSecureInterruptMask,
  IN  UINTN  GicSecureInterruptMaskSize
  );

VOID
EFIAPI
ArmGicEnableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicDisableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicEnableDistributor (
  IN  UINTN  GicDistributorBase
  );

VOID
EFIAPI
ArmGicDisableDistributor (
  IN  UINTN  GicDistributorBase
  );

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  UINTN  GicDistributorBase
  );

VOID
EFIAPI
ArmGicSendSgiTo (
  IN  UINTN  GicDistributorBase,
  IN  UINT8  TargetListFilter,
  IN  UINT8  CPUTargetList,
  IN  UINT8  SgiId
  );

/*
 * Acknowledge and return the value of the Interrupt Acknowledge Register
 *
 * InterruptId is returned separately from the register value because in
 * the GICv2 the register value contains the CpuId and InterruptId while
 * in the GICv3 the register value is only the InterruptId.
 *
 * @param GicInterruptInterfaceBase   Base Address of the GIC CPU Interface
 * @param InterruptId                 InterruptId read from the Interrupt
 *                                    Acknowledge Register
 *
 * @retval value returned by the Interrupt Acknowledge Register
 *
 */
UINTN
EFIAPI
ArmGicAcknowledgeInterrupt (
  IN  UINTN  GicInterruptInterfaceBase,
  OUT UINTN  *InterruptId
  );

VOID
EFIAPI
ArmGicEndOfInterrupt (
  IN  UINTN  GicInterruptInterfaceBase,
  IN UINTN   Source
  );

UINTN
EFIAPI
ArmGicSetPriorityMask (
  IN  UINTN  GicInterruptInterfaceBase,
  IN  INTN   PriorityMask
  );

VOID
EFIAPI
ArmGicSetInterruptPriority (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source,
  IN UINTN  Priority
  );

VOID
EFIAPI
ArmGicEnableInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

VOID
EFIAPI
ArmGicDisableInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

BOOLEAN
EFIAPI
ArmGicIsInterruptEnabled (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

// GIC revision 2 specific declarations

// Interrupts from 1020 to 1023 are considered as special interrupts
// (eg: spurious interrupts)
#define ARM_GIC_IS_SPECIAL_INTERRUPTS(Interrupt) \
            (((Interrupt) >= 1020) && ((Interrupt) <= 1023))

VOID
EFIAPI
ArmGicV2SetupNonSecure (
  IN  UINTN  MpId,
  IN  UINTN  GicDistributorBase,
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EnableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2DisableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN UINTN  GicInterruptInterfaceBase,
  IN UINTN  Source
  );

// GIC revision 3 specific declarations

#define ICC_SRE_EL2_SRE  (1 << 0)

#define ARM_GICD_IROUTER_IRM  BIT31

UINT32
EFIAPI
ArmGicV3GetControlSystemRegisterEnable (
  VOID
  );

VOID
EFIAPI
ArmGicV3SetControlSystemRegisterEnable (
  IN UINT32  ControlSystemRegisterEnable
  );

VOID
EFIAPI
ArmGicV3EnableInterruptInterface (
  VOID
  );

VOID
EFIAPI
ArmGicV3DisableInterruptInterface (
  VOID
  );

UINTN
EFIAPI
ArmGicV3AcknowledgeInterrupt (
  VOID
  );

VOID
EFIAPI
ArmGicV3EndOfInterrupt (
  IN UINTN  Source
  );

VOID
ArmGicV3SetBinaryPointer (
  IN UINTN  BinaryPoint
  );

VOID
ArmGicV3SetPriorityMask (
  IN UINTN  Priority
  );

#endif // MU_ARM_GIC_EX_LIB_H_
