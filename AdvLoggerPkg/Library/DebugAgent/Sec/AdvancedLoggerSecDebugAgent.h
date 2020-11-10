/** @file
    Advanced Logger Common function declaration


    Copyright (C) Microsoft Corporation. All rights reserved.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ADVANCED_LOGGER_SEC_DEBUGAGENT_H__
#define __ADVANCED_LOGGER_SEC_DEBUGAGENT_H__

/**
  AllocateRamForSEC

  For Intel, use Cache as RAM space for the AdvancedLogger memory buffer.

  This function is used to set up the premem Advanced Logger buffer.

  Returns:

    TRUE - Cache as RAM enabled at memory requested.

**/
EFI_PHYSICAL_ADDRESS
EFIAPI
AllocateRamForSEC (
    EFI_PHYSICAL_ADDRESS LogAddress,
    UINTN                LogSize
  );


/**
  FreeRamForSEC

  Used to dispose of the pre-mem log buffer.

  Disable the Cache as RAM space that was allocated for the AdvancedLogger.

**/
VOID
EFIAPI
FreeRamForSEC (
    EFI_PHYSICAL_ADDRESS LogAddress
  );



/**
  Executes a rep lodsd (repeated load dword string instruction).  While there is no valid
  data at the address, this allocates entries in the cache with the correct address tags.

  @param  Address The pointer to the cache location.
  @param  Length  The length in bytes of the region to load.

  @return Value   Returns the next address to be touched (to verify the rep lodsd operation).

**/
UINT32 *
EFIAPI
AsmRepLodsd (
  IN UINT32  *Address,
  IN UINT32   Length
  );

#endif  // __ADVANCED_LOGGER_SEC_DEBUGAGENT_H__
