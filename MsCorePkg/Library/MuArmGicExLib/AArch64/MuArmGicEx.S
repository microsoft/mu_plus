#
#  Copyright (c) 2014, ARM Limited. All rights reserved.
#  Copyright (c) Microsoft Corporation.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

#include <AsmMacroIoLibV8.h>

#if !defined(__clang__)

//
// Clang versions before v3.6 do not support the GNU extension that allows
// system registers outside of the IMPLEMENTATION DEFINED range to be specified
// using the generic notation below. However, clang knows these registers by
// their architectural names, so it has no need for these aliases anyway.
//
#define ICC_SGI1R_EL1           S3_0_C12_C11_5

#endif

// VOID
// ArmGicV3SendNsG1Sgi (
//  IN UINT64          SgiVal
//  );
ASM_FUNC(ArmGicV3SendNsG1Sgi)
        dsb     ishst
        msr     ICC_SGI1R_EL1, x0
        isb
        ret
