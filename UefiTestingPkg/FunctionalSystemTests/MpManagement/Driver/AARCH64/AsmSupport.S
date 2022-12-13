//
// Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>
// Portion of Copyright (c) 2014 NVIDIA Corporation. All rights reserved.<BR>
// Copyright (c) 2016 HP Development Company, L.P.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#include <Chipset/AArch64.h>

GCC_ASM_EXPORT(ReadEl0Stack)

.text

ASM_PFX(ReadEl0Stack):
  mrs     x0, sp_el0
  ret
