/** @file
ZeroTouchVariables.h

The Variable used to store user opt out and Install Cert settings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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

#define ZERO_TOUCH_VARIABLE_OPT_IN_VAR_NAME  L"_ZTD_OPT_IN"
#define ZERO_TOUCH_VARIABLE_ATTRIBUTES       (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE )

#define MAX_ALLOWABLE_ZERO_TOUCH_VAR_SIZE  (1024 * 24)  // 24kb

#define ZERO_TOUCH_VARIABLE_GUID  \
  { \
    0xbe023d3e, 0x5f0e, 0x4ce0, { 0x80, 0x5c, 0x06, 0xb7, 0x0a, 0xa2, 0x4f, 0xe7 } \
  }

extern EFI_GUID  gZeroTouchVariableGuid;

#endif //  __ZERO_TOUCH_VARIABLES_H__
