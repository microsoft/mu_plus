/** @file -- UefiHardwareNxProtectionStub.h
Shared definition of the method between x64 and ARM.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MEMORY_PROTECTION_HARDWARE_NX_ENABLED_H_
#define _MEMORY_PROTECTION_HARDWARE_NX_ENABLED_H_

UNIT_TEST_STATUS
EFIAPI
UefiHardwareNxProtectionEnabled (
  IN UNIT_TEST_CONTEXT  Context
  );

#endif // _MEMORY_PROTECTION_HARDWARE_NX_ENABLED_H_
