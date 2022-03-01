/**@file

NULL implementation of library which provides access to the memory
protection setting which may exist in the platform-specific early store
due to a memory related exception being triggered.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/MemoryProtectionExceptionLib.h>

/**
  Checks if an exception was hit on a previous boot.

  @param[out]  ExceptionOccurred  Boolean TRUE if an exception occurred, FALSE otherwise.
                                  ExceptionOccurred IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             ExceptionOccurred contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExGetExceptionOccurred (
  OUT   BOOLEAN  *ExceptionOccurred
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets memory protection exception value in platform-specific persistent storage to indicate that an
  exception has occurred.

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExSetExceptionOccurred (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Clears from the platform-specific persistent storage the memory protection exception value indicating that
  a memory violation exception occurred on a previous boot.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearExceptionOccurred (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Checks if the exception handler should ignore the next memory guard violation exception.

  @param[out]  IgnoreNextException  Boolean TRUE if next exception should be ignored, FALSE otherwise.
                                    IgnoreNextException IS NOT updated if the function returns an error.

  @retval EFI_SUCCESS             IgnoreNextException contains the result of the check
  @retval EFI_INVALID_PARAMETER   Unable to validate persistent storage contents
  @retval EFI_UNSUPPORTED         Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExGetIgnoreNextException (
  OUT BOOLEAN  *IgnoreNextException
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets memory protection exception value in platform-specific persistent storage to indicate that the
  next memory guard violation exception should be ignored, meaning when the exception occurs, the call
  to MemProtExGetExceptionOccurred() will not reflect that an exception occurred the previous boot
  (assuming that MemProtExGetExceptionOccurred() would have returned FALSE prior to the last exception).

  @retval EFI_SUCCESS       Value set
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExSetIgnoreNextException (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Clears from the platform-specific persistent storage the memory protection exception value indicating that the
  next memory guard violation exception should be ignored.

  @retval EFI_SUCCESS       Value cleared
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearIgnoreNextException (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Clears all memory protection exception values from the platform-specific persistent storage.

  @retval EFI_SUCCESS       Successfully cleared the exception bytes
  @retval EFI_UNSUPPORTED   Platform-specific persistent storage is unresponsive or NULL implementation called

**/
EFI_STATUS
EFIAPI
MemProtExClearAll (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
