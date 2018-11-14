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

typedef enum {
    ZERO_TOUCH_INACTIVE,
    ZERO_TOUCH_OPT_IN,
    ZERO_TOUCH_OPT_OUT
} ZERO_TOUCH_STATE;

/**
 * GetZeroTouchCertificate
 *
 * Returns the built in ZeroTouch certificate.
 *
 * @param Certificate
 * @param CertificateSize
 *
 * @return EFI_STATUS EFIAPI
 *
 */
EFI_STATUS
EFIAPI
GetZeroTouchCertificate(UINT8 **Certificate, UINTN *CertificateSize);

/**
 * Function to Get Zero Touch State.
 *
 * @retval: ZERO_TOUCH_INACTIVE   User has never selected a state.
 * @retval: ZERO_TOUCH_OPT_IN     User has selected Opt In.
 * @retval: ZERO_TOUCH_OPT_OUT    User has selected Opt Out.
 *
 **/
ZERO_TOUCH_STATE
EFIAPI
GetZeroTouchState (VOID);

/**
Function to Set Zero Touch State.

@param[in]  NewState

@retval: Success                 ZERO_TOUCH_STATE has been set
@retval: EFI_INVALID_PARAMETER.  ZERO_TOUCH_INACTIVE as NewState.
**/
EFI_STATUS
EFIAPI
SetZeroTouchState (
    IN  ZERO_TOUCH_STATE NewState
  );

#endif  // __ZERO_TOUCH_SETTINGS_LIB_H__