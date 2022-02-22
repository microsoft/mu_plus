/** @file

  This PEIM publishes the Capsule PPI

  Caution: This module requires additional review when modified.
  This driver will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  Copyright (c) Microsoft Corporation. All Rights Reserved.

**/

#include <PiPei.h>
#include <Uefi/UefiSpec.h>

#include <Ppi/Capsule.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/CapsuleVendor.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

/**
  Capsule PPI service that gets called after memory is available. The
  capsule coalesce function, which must be called first, returns a base
  address and size, which can be anything actually. Once the memory init
  PEIM has discovered memory, then it should call this function and pass in
  the base address and size returned by the coalesce function. Then this
  function can create a capsule HOB and return.

  However, because this method of capsule update doesn't require coalescence,
  we always return EFI_SUCCESS here.

  @param[in]  PeiServices   standard pei services pointer
  @param[in]  CapsuleBase   address returned by the capsule coalesce function. Most
                            likely this will actually be a pointer to private data.
  @param[in]  CapsuleSize   value returned by the capsule coalesce function.

  @return     EFI_SUCCESS   Always return success.
**/
EFI_STATUS
EFIAPI
CreateState (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN VOID              *CapsuleBase,
  IN UINTN             CapsuleSize
  )
{
  DEBUG ((DEBUG_INFO, "[%a] - No work necessary for capsules on disk.\n", __FUNCTION__));
  return EFI_SUCCESS;
}

/**
  Determine if we're in capsule update boot mode.

  @param[in]  PeiServices     PEI services table

  @retval     EFI_SUCCESS     A capsule is available
  @retval     EFI_NOT_FOUND   No capsule detected

**/
EFI_STATUS
EFIAPI
CheckCapsuleUpdate (
  IN EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *VariablePpi;
  UINTN                            VarSize;

  Status =  PeiServicesLocatePpi (
              &gEfiPeiReadOnlyVariable2PpiGuid,
              0,
              NULL,
              (VOID **)&VariablePpi
              );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - failed to find variable PPI = %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (Status);
    return EFI_NOT_FOUND;
  }

  //
  // Look for item "1" in the CapsuleQueue to determine if any capsules are staged.
  //
  VarSize = 0;
  Status  = VariablePpi->GetVariable (VariablePpi, L"1", &gCapsuleQueueDataGuid, NULL, &VarSize, NULL);
  DEBUG ((DEBUG_INFO, "[%a] - Get Capsule Var Status: %r\n", __FUNCTION__, Status));
  if (Status == EFI_BUFFER_TOO_SMALL) {
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

/**
  Capsule PPI service to coalesce a fragmented capsule in memory. Because this method of capsule update doesn't
  require coalescence, we always return EFI_SUCCESS.

  @param PeiServices    General purpose services available to every PEIM.
  @param MemoryBase     Pointer to the base of a block of memory that we can walk
                        all over while trying to coalesce our buffers.
                        On output, this variable will hold the base address of
                        a coalesced capsule.
  @param MemorySize     Size of the memory region pointed to by MemoryBase.
                        On output, this variable will contain the size of the
                        coalesced capsule.

  @return EFI_SUCCESS   Always return success.
**/
EFI_STATUS
EFIAPI
CapsuleCoalesce (
  IN     EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID              **MemoryBase,
  IN OUT UINTN             *MemorySize
  )
{
  DEBUG ((DEBUG_INFO, "[%a] - No work necessary for capsules on disk.\n", __FUNCTION__));
  *MemorySize = 0;
  // we always return success because we don't need to coalesce
  return EFI_SUCCESS;
}

CONST EFI_PEI_CAPSULE_PPI  mCapsulePpi = {
  CapsuleCoalesce,
  CheckCapsuleUpdate,
  CreateState
};

CONST EFI_PEI_PPI_DESCRIPTOR  mUefiPpiListCapsule = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiCapsulePpiGuid,
  (EFI_PEI_CAPSULE_PPI *)&mCapsulePpi
};

/**
  Entry point function for the PEIM

  @param[in]  FileHandle              Handle of the file being invoked.
  @param[in]  PeiServices             Describes the list of possible PEI Services.

  @retval     EFI_SUCCESS             The interface was successfully installed.
  @retval     EFI_INVALID_PARAMETER   The PpiList pointer is NULL.
  @retval     EFI_INVALID_PARAMETER   Any of the PEI PPI descriptors in the list do not have the
                                      EFI_PEI_PPI_DESCRIPTOR_PPI bit set in the Flags field.
  @retval     EFI_OUT_OF_RESOURCES    There is no additional space in the PPI database.

**/
EFI_STATUS
EFIAPI
CapsulePeimEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  //
  // Produce the PPI
  //
  return PeiServicesInstallPpi (&mUefiPpiListCapsule);
}
