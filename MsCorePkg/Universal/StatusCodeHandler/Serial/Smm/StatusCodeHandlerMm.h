/** @file
  Internal header file for simple SerialLib based MM Status Code handler

Copyright (C) Microsoft Corporation. All rights reserved.

  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _STATUS_CODE_HANDLER_MM_H_
#define _STATUS_CODE_HANDLER_MM_H_

/**
  Status Code MM Common Entry point.

  Register this handler with the MM Router

  @retval EFI_SUCCESS  Successfully registered

**/
EFI_STATUS
MmEntry (
  VOID
);

#endif // _STATUS_CODE_HANDLER_MM_H_
