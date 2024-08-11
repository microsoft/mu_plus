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

#endif // MU_ARM_GIC_EX_LIB_H_
