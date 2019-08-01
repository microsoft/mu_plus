/*++ @file

This file defines the variables used by the USB and PXE Boot Detection feature.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __MS_ALT_BOOT_VARIABLES_INC__
#define __MS_ALT_BOOT_VARIABLES_INC__

/**
  This variable is a flag to track whether or not a system has been
  booted from USB or PXE
**/
#define kszAltBootFlagVariableName  (L"AltBootFlag")


/**
 Namespace GUID for the Boot Type Tracking feature.
 {26D75FF0-D5CD-49EC-8092-F8EC4D18EF33}
**/
extern EFI_GUID gAltBootGuid;



#endif // __MS_ALT_BOOT_VARIABLES_INC__