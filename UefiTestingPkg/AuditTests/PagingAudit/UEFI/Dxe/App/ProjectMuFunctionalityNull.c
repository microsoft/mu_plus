#include "DxePagingAuditTestApp.h"

/**
  Populates the heap guard protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
EFI_STATUS
PopulateHeapGuardDebugProtocol (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Populates the CPU MP debug protocol global

  @retval EFI_SUCCESS Protocol is already populated or was successfully populated
  @retval other       Return value of LocateProtocol
**/
EFI_STATUS
PopulateCpuMpDebugProtocol (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/*
  Writes the NULL page and stack information to the memory info database
 */
VOID
ProjectMuSpecialMemoryDump (
  VOID
  )
{
  return;
}

/**
  Populates the non protected image list global

  @retval EFI_SUCCESS            The non-protected image list was populated
  @retval EFI_INVALID_PARAMETER  The non-protected image list was already populated
  @retval other                  The non-protected image list was not populated
**/
EFI_STATUS
GetNonProtectedImageList (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Populates the special region array global
**/
EFI_STATUS
GetSpecialRegions (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Checks if the address is a guard page.

  @param[in] Address            Address to check

  @retval TRUE                  The address is a guard page
  @retval FALSE                 The address is not a guard page
**/
BOOLEAN
IsGuardPage (
  IN UINT64  Address
  )
{
  return FALSE;
}

/**
  In processors implementing the AMD64 architecture, SMBASE relocation is always supported.
  However, there are some virtual platforms that does not really support them. This PCD check
  is to allow these virtual platforms to skip the SMRR check.

  @retval TRUE                  Skip the SMRR check
  @retval FALSE                 Do not skip the SMRR check
**/
BOOLEAN
SkipSmrr (
  VOID
  )
{
  return FALSE;
}

/**
  Checks if a region is allowed to be read/write/execute based on the special region array
  and non protected image list

  @param[in] Address            Start address of the region
  @param[in] Length             Length of the region

  @retval TRUE                  The region is allowed to be read/write/execute
  @retval FALSE                 The region is not allowed to be read/write/execute
**/
BOOLEAN
CheckProjectMuRWXExemption (
  IN UINT64  Address,
  IN UINT64  Length
  )
{
  return FALSE;
}
