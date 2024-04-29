;------------------------------------------------------------------------------
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;------------------------------------------------------------------------------

    EXPORT Asm_Read_ID_AA64MMFR1_EL1
    AREA |.text|, CODE, READONLY

;------------------------------------------------------------------------------
; Reads the ID_AA64MMFR1_EL1 special register.
;
; @retval The UINT64 value of the ID_AA64MMFR1_EL1 special register.
;
; UINT64
; Asm_Read_ID_AA64MMFR1_EL1 (
;   VOID
;   );
;------------------------------------------------------------------------------
Asm_Read_ID_AA64MMFR1_EL1
    MRS X0, ID_AA64MMFR1_EL1
    RET
    END
