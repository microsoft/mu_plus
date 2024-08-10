/**
  @file  MuArmGicExLib.c

  This library implements the extended interfaces for the Arm GIC.


**/

#include <Library/ArmLib.h>
#include <Library/ArmGicLib.h>
#include <Library/MuArmGicExLib.h>

#include "MuArmGicExLibInternal.h"

#define ISPENDR_ADDRESS(base, offset)  ((base) +\
          ARM_GICR_CTLR_FRAME_SIZE + ARM_GICR_ISPENDR + 4 * (offset))

#define ICPENDR_ADDRESS(base, offset)  ((base) +\
          ARM_GICR_CTLR_FRAME_SIZE + ARM_GICR_ICPENDR + 4 * (offset))

/**
 * Return the base address of the GIC redistributor for the current CPU
 *
 * @param Revision  GIC Revision. The GIC redistributor might have a different
 *                  granularity following the GIC revision.
 *
 * @retval Base address of the associated GIC Redistributor
 */
STATIC
UINTN
GicGetCpuRedistributorBase (
  IN UINTN                  GicRedistributorBase,
  IN ARM_GIC_ARCH_REVISION  Revision
  )
{
  UINTN   MpId;
  UINTN   CpuAffinity;
  UINTN   Affinity;
  UINTN   GicCpuRedistributorBase;
  UINT64  TypeRegister;

  MpId = ArmReadMpidr ();
  // Define CPU affinity as:
  // Affinity0[0:8], Affinity1[9:15], Affinity2[16:23], Affinity3[24:32]
  // whereas Affinity3 is defined at [32:39] in MPIDR
  CpuAffinity = (MpId & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((MpId & ARM_CORE_AFF3) >> 8);

  if (Revision < ARM_GIC_ARCH_REVISION_3) {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return 0;
  }

  GicCpuRedistributorBase = GicRedistributorBase;

  do {
    TypeRegister = MmioRead64 (GicCpuRedistributorBase + ARM_GICR_TYPER);
    Affinity     = ARM_GICR_TYPER_GET_AFFINITY (TypeRegister);
    if (Affinity == CpuAffinity) {
      return GicCpuRedistributorBase;
    }

    // Move to the next GIC Redistributor frame.
    // The GIC specification does not forbid a mixture of redistributors
    // with or without support for virtual LPIs, so we test Virtual LPIs
    // Support (VLPIS) bit for each frame to decide the granularity.
    // Note: The assumption here is that the redistributors are adjacent
    // for all CPUs. However this may not be the case for NUMA systems.
    GicCpuRedistributorBase += (((ARM_GICR_TYPER_VLPIS & TypeRegister) != 0)
                                ? GIC_V4_REDISTRIBUTOR_GRANULARITY
                                : GIC_V3_REDISTRIBUTOR_GRANULARITY);
  } while ((TypeRegister & ARM_GICR_TYPER_LAST) == 0);

  // The Redistributor has not been found for the current CPU
  ASSERT_EFI_ERROR (EFI_NOT_FOUND);
  return 0;
}

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
  )
{
  UINT32                 RegOffset;
  UINTN                  RegShift;
  ARM_GIC_ARCH_REVISION  Revision;
  UINTN                  GicCpuRedistributorBase;

  // Calculate enable register offset and bit position
  RegOffset = (UINT32)(Source / 32);
  RegShift  = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source))
  {
    // Write set-pending register
    MmioWrite32 (
      GicDistributorBase + ARM_GIC_ICDSPR + (4 * RegOffset),
      1 << RegShift
      );
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
                                GicRedistributorBase,
                                Revision
                                );
    if (GicCpuRedistributorBase == 0) {
      ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      return;
    }

    // Write set-enable register
    MmioWrite32 (
      ISPENDR_ADDRESS (GicCpuRedistributorBase, RegOffset),
      1 << RegShift
      );
  }
}

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
  )
{
  UINT32                 RegOffset;
  UINTN                  RegShift;
  ARM_GIC_ARCH_REVISION  Revision;
  UINTN                  GicCpuRedistributorBase;

  // Calculate enable register offset and bit position
  RegOffset = (UINT32)(Source / 32);
  RegShift  = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source))
  {
    // Write clear-enable register
    MmioWrite32 (
      GicDistributorBase + ARM_GIC_ICDICPR + (4 * RegOffset),
      1 << RegShift
      );
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
                                GicRedistributorBase,
                                Revision
                                );
    if (GicCpuRedistributorBase == 0) {
      return;
    }

    // Write clear-enable register
    MmioWrite32 (
      ICPENDR_ADDRESS (GicCpuRedistributorBase, RegOffset),
      1 << RegShift
      );
  }
}

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
  )
{
  UINT32                 RegOffset;
  UINTN                  RegShift;
  ARM_GIC_ARCH_REVISION  Revision;
  UINTN                  GicCpuRedistributorBase;
  UINT32                 Interrupts;

  // Calculate enable register offset and bit position
  RegOffset = (UINT32)(Source / 32);
  RegShift  = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source))
  {
    Interrupts = MmioRead32 (
                   GicDistributorBase + ARM_GIC_ICDSPR + (4 * RegOffset)
                   );
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
                                GicRedistributorBase,
                                Revision
                                );
    if (GicCpuRedistributorBase == 0) {
      return 0;
    }

    // Read set-enable register
    Interrupts = MmioRead32 (
                   ISPENDR_ADDRESS (GicCpuRedistributorBase, RegOffset)
                   );
  }

  return ((Interrupts & (1 << RegShift)) != 0);
}

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
  )
{
  ARM_GIC_ARCH_REVISION  Revision;
  UINT32                 ApplicableTargets;
  UINT32                 AFF3;
  UINT32                 AFF2;
  UINT32                 AFF1;
  UINT32                 AFF0;
  UINT32                 Irm;
  UINT64                 SGIValue;

  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    MmioWrite32 (
      GicDistributorBase + ARM_GIC_ICDSGIR,
      ((TargetListFilter & 0x3) << 24) |
      ((CPUTargetList & 0xFF) << 16)   |
      (SgiId & 0xF)
      );
  } else {
    // Below routine is adopted from gicv3_raise_secure_g0_sgi in TF-A

    /* Extract affinity fields from target */
    AFF0 = GET_MPIDR_AFF0 (CPUTargetList);
    AFF1 = GET_MPIDR_AFF1 (CPUTargetList);
    AFF2 = GET_MPIDR_AFF2 (CPUTargetList);
    AFF3 = GET_MPIDR_AFF3 (CPUTargetList);

    /*
     * Make target list from affinity 0, and ensure GICv3 SGI can target
     * this PE.
     */
    ApplicableTargets = (1 << AFF0);

    /*
     * Evaluate the filter to see if this is for the target or all others
     */
    Irm = (TargetListFilter == ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE) ? SGIR_IRM_TO_OTHERS : SGIR_IRM_TO_AFF;

    /* Raise SGI to PE specified by its affinity */
    SGIValue = GICV3_SGIR_VALUE (AFF3, AFF2, AFF1, SgiId, Irm, ApplicableTargets);

    /*
     * Ensure that any shared variable updates depending on out of band
     * interrupt trigger are observed before raising SGI.
     */
    ArmGicV3SendNsG1Sgi (SGIValue);
  }
}
