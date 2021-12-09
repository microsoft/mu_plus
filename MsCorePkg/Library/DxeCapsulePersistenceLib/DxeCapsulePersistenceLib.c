/**
  This is a DXE version of the CapsulePersistenceLib.
  This implements the public interface of the library for DXE.
  It uses the disk to persist capsule images across reset
 
  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/CapsulePersistenceLib.h>

#include "CapsulePersistence.h"

/**
  Persists a Capsule across reset and adds it to the queue

  @param[in]      CapsuleHeader           EFI_CAPSULE_HEADER pointing to Capsule Image to persist.
  @param[out]     CapsuleIdentifier       Data for the capsule that has been added to the queue.
                                          This is optional, passing in a null is not a failure.

  @retval         EFI_SUCCESS             Capsule was successfully persisted.
  @retval         EFI_DEVICE_ERROR        Something went wrong while trying to persist the capsule.
  @retval         EFI_UNSUPPORTED         Capsule image is not supported by the firmware.
  @retval         EFI_INVALID_PARAMETER   CapsuleHeader was null
  @retval         EFI_NOT_FOUND           Storage medium was not found or other reason for generating not found.
  @retval         EFI_OUT_OF_RESOURCES    Not enough storage is available to hold the variable and its data.
  @retval         EFI_ALREADY_STARTED     The capsule ID has already been persisted.
**/
EFI_STATUS
EFIAPI
PersistCapsuleImageAcrossReset (
  IN  EFI_CAPSULE_HEADER           *CapsuleHeader,
  OUT CAPSULE_PERSISTED_IDENTIFIER *CapsuleIdentifier    OPTIONAL
  )
{
  return InternalPersistCapsuleImageAcrossReset (CapsuleHeader, CapsuleIdentifier);
}

/**
  Returns a pointer to a specific capsule.

  This method does not delete the capsule data from the medium of persistence.
  To delete the data, use DeletePersistedCapsuleById.

  This function expects that caller to allocate memory to hold the capsule data.

  @param[in]    CapsuleIdentifier     Pointer to the identifier for the capsule.
  @param[out]   CapsuleData           Pointer to a buffer to hold the capsules (OPTIONAL).
  @param[out]   CapsuleDataSize       On input, size of CapsuleData allocation.
                                      On output, the size of the persisted capsule.

  @retval       EFI_SUCCESS           Capsule was found and output data is valid.
  @retval       EFI_BUFFER_TOO_SMALL  CapsuleData buffer is too small to hold all the data.
  @retval       EFI_DEVICE_ERROR      Something went wrong while trying to retrive the capsule.

**/
EFI_STATUS
EFIAPI
GrabPersistedCapsuleByIdentifier (
  IN  CAPSULE_PERSISTED_IDENTIFIER *CapsuleIdentifier,
  OUT EFI_CAPSULE_HEADER           *CapsuleData OPTIONAL,
  OUT UINTN                        *CapsuleDataSize
  )
{
  if (CapsuleIdentifier == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  return InternalGetPersistedCapsuleData (CapsuleIdentifier->CapsuleId, CapsuleIdentifier->CapsuleHash, CapsuleData, CapsuleDataSize);
}

/**
  Deletes a capsule by id. This function does not check the hash of the file

  @param[in]  CapsuleId           The ID of the capsule to delete

  @retval     EFI_SUCCESS         Capsule was found and deleted.
  @retval     EFI_DEVICE_ERROR    Something went wrong while trying to delete the capsule.

**/
EFI_STATUS
EFIAPI
DeletePersistedCapsuleById (
  IN  UINT32  CapsuleId
 )
{
  return InternalDeletePersistedCapsuleData (CapsuleId);
}

/**
  Deletes all capsules stored on the medium of persistence.
  This returns success even if there are no capsules to delete.

  @retval   EFI_SUCCESS         Capsules were found and deleted.
  @retval   EFI_SUCCESS         No capsules are on the disk to delete.
  @retval   EFI_NOT_FOUND       We were unable to find the file system.
  @retval   EFI_DEVICE_ERROR    Something went wrong while trying to delete the capsules.

**/
EFI_STATUS
EFIAPI
DeleteAllPersistedCapsules (
  VOID
  )
{
  return InternalDeleteAllCapsulesOnFileSystem ();
}