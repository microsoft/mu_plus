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
* The Zero Touch certificate is baked into the library.  _ZT_CERT_OPT_OUT, if present,
* indicates that the user has opted out of Zero Touch.
*
* This implementation counts on non standard variable locking. _ZT_CERT_OPT_OUT requires
* LOCK_ON_CREATE and LOCK_AT_END_OF_DXE, and _ZT_CERT_INSTALL requires LOCK_AT_END_OF_DXE.
* Both require theses variable to be unlocked in manufacturing mode.
*
* _ZT_CERT_OPT_OUT is lock on create.  Once set, Mfg mode is needed to clear
*
* On every boot:
*
* 1. Delete _ZT_CERT_OPT_OUT. This will only succeed if in Mfg mode
* 2. At ReadyToBoot, if _ZT_CERT_INSTALL is NOT_FOUND, install _ZT_CERT_INSTALL
*    with a value = 1. This will only succeed if in Mfg mode.
* 3. IdentityAndAuth manager will install the ZT Certificate into DFCI if
*    _ZT_CERT_INSTALL has a value of 1.  After installing the ZT Certificate,
*    IdentityAndAuth manager will set the value of _ZT_CERT_INSTALL to 0;
*
* Library access will only return the cert if the certificate is installable according
* to the state of these variables.
*
*/

#define ZERO_TOUCH_VARIABLE_OPT_OUT_VAR_NAME   L"_ZT_CERT_OPT_OUT"
#define ZERO_TOUCH_VARIABLE_INSTALL_VAR_NAME   L"_ZT_CERT_INSTALL"
#define ZERO_TOUCH_VARIABLE_ATTRIBUTES (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define ZERO_TOUCH_VARIABLE_GUID  \
  { \
    0xbe023d3e, 0x5f0e, 0x4ce0, { 0x80, 0x5c, 0x06, 0xb7, 0x0a, 0xa2, 0x4f, 0xe7 } \
  }

extern EFI_GUID gZeroTouchVariableGuid;

#endif //  __ZERO_TOUCH_VARIABLES_H__


