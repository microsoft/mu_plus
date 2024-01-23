;------------------------------------------------------------------------------
;
; Copyright (c) Microsoft Corporation. All rights reserved.
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Module Name:
;
;   RepLodsd.nasm
;
; Abstract:
;
;       Executes a rep lodsd (repeated load dword string instruction).  While there is no valid
;       data at the address, this allocates entries in the cache with the correct address tags.
;
;  @param  Address The pointer to the cache location.
;  @param  Length  The length in bytes of the region to load.
;
;  @return Value   Returns the next address to be touched (to verify the rep lodsd operation)
;
;------------------------------------------------------------------------------

    SECTION .text

;------------------------------------------------------------------------------
; UINT32 *
; EFIAPI
; AsmRepLodsd (
;   IN UINT32  *Address,      // rcx
;   IN UINTN   Length         // rdx
;   );
;------------------------------------------------------------------------------
global ASM_PFX(AsmRepLodsd)
ASM_PFX(AsmRepLodsd):
     push    rsi
     xchg    rcx, rdx
     mov     rsi, rdx
     shr     rcx, 2
     cld
     rep     lodsd
     mov     rax, rsi
     pop     rsi
    ret
