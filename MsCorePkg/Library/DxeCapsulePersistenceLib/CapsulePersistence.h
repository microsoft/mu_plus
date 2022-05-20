/**
  An interface for persisting Capsules across reset.
  This file is concerned with disk manipulation and file management.

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DXE_CAPSULE_PERSISTENCE_H_
#define _DXE_CAPSULE_PERSISTENCE_H_

#include <Protocol/SimpleFileSystem.h>

/**
  This persists a capsule across reset but does not add it to the queue
  It is saved to the disk.

  @param[in]  CapsuleHeader         Points to a capsule header.
  @param[out] CapsuleIdentifier     The ID and hash of the capsule (optional)

  @retval     EFI_SUCCESS           Capsule successfully persisted to disk
  @retval     EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval     EFI_INVALID_PARAMETER CapsuleHeader was null
  @retval     EFI_NOT_FOUND         Disk was not found or other reason
  @retval     EFI_DEVICE_ERROR      Something went wrong persisting the capsule
  @retval     Other                 Something else went wrong persisting the capsule
**/
EFI_STATUS
InternalPersistCapsuleImageAcrossReset (
  IN  EFI_CAPSULE_HEADER            *CapsuleHeader,
  OUT CAPSULE_PERSISTED_IDENTIFIER  *CapsuleIdentifier OPTIONAL
  );

/**
  Allows the system to request a capsule by ID from the persisted medium

  If the persisted capsule by a given ID is not present, the pointer data will not be changed.
  The persisted capsule will not be cleared from the disk until explicitly told to do so.

  IMPORTANT: Caller is responsible for allocating/freeing the returned data (CapsuleData)

  @param[in]    CapsuleId             The ID of the capsule that is going to be loaded
  @param[in]    CapsuleHash           The hash of the capsule this is going to be loaded
                                      This will be verified when the capsule is loaded
  @param[out]   CapsuleData           A double pointer that will point to the capsule
                                      This value will not be changed on non EFI_SUCCESS
                                      return values (OPTIONAL).
  @param[out]   CapsuleDataSize       The size of the capsule data buffer

  @retval       EFI_SUCCESS           Capsules were de-persisted, and output data is valid.
  @retval       EFI_INVALID_PARAMETER CapsuleData was NULL or the hash didn't match
  @retval       EFI_OUT_OF_RESOURCES  Failed to allocate a buffer
  @retval       EFI_BUFFER_TOO_SMALL  Buffer is too small
  @retval       EFI_NOT_FOUND         The capsule file was not found
  @retval       EFI_DEVICE_ERROR      Something went wrong while trying to retrieve the capsule.

**/
EFI_STATUS
InternalGetPersistedCapsuleData (
  IN UINT32               CapsuleId,
  IN UINT64               CapsuleHash,
  OUT EFI_CAPSULE_HEADER  *CapsuleData OPTIONAL,
  OUT UINTN               *CapsuleDataSize
  );

/**
  Deletes the data for a given capsule.

  @param[in]  CapsuleId        The ID of the capsule that is going to be loaded

  @retval     EFI_SUCCESS      Capsules were de-persisted, and output data is valid.
  @retval     EFI_NOT_FOUND    The capsule file was not found
  @retval     EFI_DEVICE_ERROR Something went wrong while trying to retrieve the capsule.

**/
EFI_STATUS
InternalDeletePersistedCapsuleData (
  IN UINT32  CapsuleId
  );

/**
  Removes the Capsules folder and its contents.

  @retval     EFI_SUCCESS       Stale capsules removed.
  @retval     Other             Something went wrong during capsule removal.

**/
EFI_STATUS
InternalDeleteAllCapsulesOnFileSystem (
  VOID
  );

#endif
