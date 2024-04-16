/** @file
  This file include the platform specific event hooks which can be customized by IBV/OEM.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

/**
  The Hook Before End Of Dxe Event. it used for customized porting.
**/
VOID
BeforeEndOfDxeEventHook (
  VOID
  )
{
  return;
}

/**
  The Hook After End Of Dxe Event. it used for customized porting.
**/
VOID
AfterEndOfDxeEventHook (
  VOID
  )
{
  return;
}

/**
  The Hook Dispatch the deferred 3rd party images Event. it used for customized porting.
**/
VOID
AfterDispatchDeferredImagesHook (
  VOID
  )
{
  return;
}
