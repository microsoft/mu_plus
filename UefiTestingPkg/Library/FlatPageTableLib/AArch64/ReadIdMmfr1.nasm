;------------------------------------------------------------------------------
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

    AREA .text, CODE, READONLY
    EXPORT Asm_Read_ID_AA64MMFR1_EL1

Asm_Read_ID_AA64MMFR1_EL1
    MRS X0, ID_AA64MMFR1_EL1
    RET
    END
