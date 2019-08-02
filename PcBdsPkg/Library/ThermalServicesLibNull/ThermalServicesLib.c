/** @file
This is the platform specific implementation of the Thermal Services Library

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ThermalServicesLib.h>

EFI_STATUS
EFIAPI
SystemThermalCheck (
  IN  THERMAL_CASE  Case,
  OUT BOOLEAN*      Good
)
{
  // return TRUE always, which means mitigation will never be invoked.
  *Good = TRUE;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SystemThermalMitigate (
  IN  THERMAL_CASE  Case,
  IN  UINT32        TimeoutPeriod
)
{
  // Should never be called because SystemThermalCheck always returns TRUE, so return Unsupported
  return EFI_UNSUPPORTED;
}

