/** @file -- MsWheaReportTraditional.c

This traditional MM driver will produce a RSC listener that listens to reported status
codes. The service is intended for MM environment and will only be available after
gEfiVariableWriteArchProtocolGuid is published.

Certain errors will be stored to flash upon reporting, under gEfiHardwareErrorVariableGuid
with VarName "HwErrRecXXXX", where "XXXX" are hexadecimal digits;

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include "MsWheaReportMm.h"

/**
  Entry to MsWheaReportSmm, register RSC handler and callback functions

  @param[in] ImageHandle                The image handle.
  @param[in] SystemTable                The system table.

  @retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MsWheaReportTraditionalEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  return MsWheaReportCommonEntry ();
}

