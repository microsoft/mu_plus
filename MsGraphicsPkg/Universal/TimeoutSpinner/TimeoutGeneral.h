/** @file TimeoutGeneral.h

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __TIMEOUT_GENERAL_H__
#define __TIMEOUT_GENERAL_H__

/**
  InitializeGeneralSpinner

  Initialize General Spinner

  @param  Spc       - Spinner Containter

  @return EFI_STATUS - Spinner initialized
          other      - Spinner not initialized
**/
EFI_STATUS
InitializeGeneralSpinner (
  IN SPINNER_CONTAINER  *Spc
  );

#endif // __TIMEOUT_GENERAL_H__
