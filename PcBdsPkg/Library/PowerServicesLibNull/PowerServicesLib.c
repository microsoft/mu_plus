/** @file
Null Power Services library class to support Platforms that dont have battery

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PowerServicesLib.h>

EFI_STATUS
EFIAPI
SystemPowerCheck (
  IN  POWER_CASE    Case,
  OUT BOOLEAN*      Good
)
{
  // For Platforms that do not have battery, always return EFI_SUCCESS
  *Good = TRUE;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SystemPowerMitigate (
  IN  POWER_CASE    Case
)
{
  // For Platforms that do not have battery, should never be called
  // (See SystemPowerCheck above), so return unsupported.

  return EFI_UNSUPPORTED;
}

