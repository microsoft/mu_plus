/** @file
ZeroTouchSettingsLib.h

Library provides a method for drivers to get Zero Touch information.

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

#ifndef __ZERO_TOUCH_SETTINGS_LIB_H__
#define __ZERO_TOUCH_SETTINGS_LIB_H__

/**
 * GetZeroTouchState
 *
 * Checks to see if Zero Touch has been opted out.
 *
 * @author miketur (7/18/2018)
 * @param
 *
 * @return BOOLEAN EFIAPI
 */
BOOLEAN
EFIAPI
GetZeroTouchInstallState (VOID);

/**
 * GetZeroTouchCertificate
 *
 * Checks if the user has opted out of Zero Touch enrollment. If
 * opted out, return EFI_NOT_FOUND to indicate no Certificate;
 *
 * Otherwize, the Zero Touch certificate is returned.
 *
 * @param Certificate
 * @param CertificateSize
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
GetZeroTouchCertificate(UINT8 **Certificate, UINTN *CertificateSize);

/**
Function to Set Zero Touch State.

Actually, only SetZeroTouchState(FALSE) is supported as the variable
is locked when created.  SetZeroTouchState(FALSE) will create the _ZT_CERT_INVALID
variable.  Once it is set, it can only be deleted if in MFG mode.

@retval: Success - Zero Touch is disabled
@retval: EFI_ERROR.  Error occurred.
**/
EFI_STATUS
EFIAPI
SetZeroTouchInstalled (VOID);

/**
Function to Set Zero Touch OptOut.

SetZeroTouchOptOut sets the _ZT_CERT_OPT_OUT variable. Once it is set, it can only be
deleted if in MFG mode.


@retval: Success - Zero Touch is disabled
@retval: EFI_ERROR.  Error occurred.
**/
EFI_STATUS
EFIAPI
SetZeroTouchOptOut (VOID);


#endif  // __ZERO_TOUCH_SETTINGS_LIB_H__
