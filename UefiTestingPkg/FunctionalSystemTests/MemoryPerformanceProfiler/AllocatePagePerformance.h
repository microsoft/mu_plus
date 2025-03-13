#ifndef ALLOCATE_PAGE_PERFORMANCE_H
#define ALLOCATE_PAGE_PERFORMANCE_H

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PerformanceLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>


/**
 * @brief Executes the allocate Page performance profiles.
 *
 * This function executes the performance profiles for different memory allocation
 * sizes and prints the elapsed time for each profile.
 *
 * @return EFI_SUCCESS if all profiles were executed successfully, otherwise an error status.
 */
EFI_STATUS
EFIAPI
ExecuteAllocatePagePerformanceProfiles (
  VOID
  );

#endif // ALLOCATE_PAGE_PERFORMANCE_H