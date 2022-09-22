/** @file
Library used to display things on the Frame buffer by memory copying

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Ppi/Graphics.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

/***
 Get Graphics Information
*/
EFI_STATUS
EFIAPI
GetGraphicsInfo (
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  **Mode
  )
{
  EFI_STATUS  Status;

  //
  // Locate the PEI Graphics information.
  //
  Status = PeiServicesLocatePpi (&gEfiPeiGraphicsPpiGuid, 0, NULL, (VOID *)Mode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate Pei Graphics Information. Code=%r\n", __FUNCTION__, Status));
    *Mode = NULL;
  }

  return Status;
}
