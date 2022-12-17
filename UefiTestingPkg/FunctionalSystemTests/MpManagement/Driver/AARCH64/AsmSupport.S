// Assembly code support for MP management driver
//
// Copyright (c) Microsoft Corporation.
// Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include <Chipset/AArch64.h>
#include <AsmMacroIoLibV8.h>
#include <IndustryStandard/ArmStdSmc.h>

GCC_ASM_IMPORT(ApEntryPoint)
GCC_ASM_IMPORT(mCpuInfo)
GCC_ASM_IMPORT(gApStacksBase)
GCC_ASM_IMPORT(mNumCpus)
GCC_ASM_IMPORT(gApStackSize)

GCC_ASM_EXPORT(ReadEl0Stack)
GCC_ASM_EXPORT(AsmApEntryPoint)

.text

// UINTN
// ReadEl0Stack (
//   VOID
//   );
ASM_PFX(ReadEl0Stack):
  mrs     x0, sp_el0
  ret

// Entry-point for the AP
// VOID
// AsmApEntryPoint (
//   VOID
//   );
ASM_PFX(AsmApEntryPoint):
  mrs x0, mpidr_el1
  // Mask the non-affinity bits
  bic x0, x0, 0x00ff000000
  and x0, x0, 0xffffffffff
  ldr x1, mCpuInfo            // x1 = mCpuInfo
  mov x2, 0                   // x2 = processor index

// TODO: This is, again, reverse engineering to correlate the CPU index with the MPIDRs...
1:
  ldr x3, [x1]                // x3 = *x1
  cmp x0, x3                  // if mpidr_el1 != mCpuInfo[x].Mpidr then loop
  bne 1b
  add x1, x1, #40             // x1 = x1 + sizeof (ARM_CORE_INFO)
  add x2, x2, 1               // x2++
  ldr x3, mNumCpus
  ldr x3, [x3]
  cmp x2, x3                  // check if we've reached the end of mCpuInfo
  bge ProcessorNotFound

// Calculate stack address
  // x2 contains the index for the current processor plus 1
  ldr x0, gApStacksBase
  ldr x1, gApStackSize
  mul x3, x2, x1              // x3 = (ProcessorIndex + 1) * gApStackSize
  add sp, x0, x3              // sp = gApStacksBase + x3
  mov x29, xzr
  bl ApEntryPoint             // doesn't return

ProcessorNotFound:
// Turn off the processor
  b .
  MOV32 (w0, ARM_SMC_ID_PSCI_CPU_OFF)
  smc #0
