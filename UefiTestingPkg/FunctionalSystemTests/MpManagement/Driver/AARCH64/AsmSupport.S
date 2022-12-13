// Assembly code support for MP management driver
//
// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

#include <Chipset/AArch64.h>

GCC_ASM_EXPORT(ReadEl0Stack)

.text

// UINTN
// ReadEl0Stack (
//   VOID
//   );
ASM_PFX(ReadEl0Stack):
  mrs     x0, sp_el0
  ret
