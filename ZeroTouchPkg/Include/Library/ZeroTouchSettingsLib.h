/** @file
ZeroTouchSettingsLib.h

Library provides a method for drivers to get Zero Touch information.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
GetZeroTouchCertificate (
  UINT8  **Certificate,
  UINTN  *CertificateSize
  );

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
GetZeroTouchState (
  VOID
  );

/**
Function to Set Zero Touch State.

@param[in]  NewState

@retval: Success                 ZERO_TOUCH_STATE has been set
@retval: EFI_INVALID_PARAMETER.  ZERO_TOUCH_INACTIVE as NewState.
**/
EFI_STATUS
EFIAPI
SetZeroTouchState (
  IN  ZERO_TOUCH_STATE  NewState
  );

#endif // __ZERO_TOUCH_SETTINGS_LIB_H__
