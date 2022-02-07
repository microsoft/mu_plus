/** @file -- MsWheaReportMm.h

Internal header file for MsWheaReport drivers under MM environment.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MS_WHEA_REPORT_MM_H_
#define _MS_WHEA_REPORT_MM_H_

/**
  Common entry to MsWheaReportMm, register RSC handler and callback functions

  @retval Status                        From internal routine or boot object, should not fail
**/
EFI_STATUS
EFIAPI
MsWheaReportCommonEntry (
  VOID
  );

#endif // _MS_WHEA_REPORT_MM_H_
