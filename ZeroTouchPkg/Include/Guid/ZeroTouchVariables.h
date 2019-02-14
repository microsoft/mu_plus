/** @file
ZeroTouchVariables.h

The Variable used to store user opt out and Install Cert settings.

Copyright (c) 2018, Microsoft Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#ifndef __ZERO_TOUCH_VARIABLES_H__
#define __ZERO_TOUCH_VARIABLES_H__

/**
* The Zero Touch certificate is baked into the firmware volume by including something like
* this in the platformpkg.fdf:
*
*  FILE FREEFORM = PCD(gZeroTouchPkgTokenSpaceGuid.PcdZeroTouchCertificateFile) {
*      SECTION RAW = ZeroTouchPkg/Certs/ZeroTouch/ZTD_Leaf.cer
*  }
*
* This implementation counts on non standard variable locking. _ZT_CERT_OPT_IN requires
* requires LOCK_AT_READY_TO_BOOT
*
*/

#define ZERO_TOUCH_VARIABLE_OPT_IN_VAR_NAME   L"_ZTD_OPT_IN"
#define ZERO_TOUCH_VARIABLE_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define ZERO_TOUCH_VARIABLE_GUID  \
  { \
    0xbe023d3e, 0x5f0e, 0x4ce0, { 0x80, 0x5c, 0x06, 0xb7, 0x0a, 0xa2, 0x4f, 0xe7 } \
  }

extern EFI_GUID gZeroTouchVariableGuid;

#endif //  __ZERO_TOUCH_VARIABLES_H__


