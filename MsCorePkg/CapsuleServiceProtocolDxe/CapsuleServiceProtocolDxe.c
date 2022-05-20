/** @file
  Capsule Service Protocol Driver produces a protocol with two functions: UpdateCapsule()
  and QueryCapsuleCapabilities().

  This protocol is consumed by CapsuleRuntimeDxe before ExitBootServices.
  Once ExitBootServices is triggered, this protocol is no longer used.

  Copyright (c) Microsoft Corporation. All rights reserved.

**/

#include <Uefi.h>

#include <Guid/CapsuleVendor.h>
#include <Guid/FmpCapsule.h>
#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/ResetUtilityLib.h>

#include <Library/IsCapsuleSupportedLib.h>
#include <Library/CapsulePersistenceLib.h>
#include <Library/QueueLib.h>

#include <Protocol/CapsuleService.h>

UINT32   mMaxSizePopulateCapsule    = 0;
UINT32   mMaxSizeNonPopulateCapsule = 0;
BOOLEAN  mFirstPersistence          = TRUE;

/**
  Saves a capsule image to disk and adds the ID to the queue

  @param[in] CapsuleHeader        A pointer to the capsule header which we want to save

  @retval EFI_SUCCESS             Capsule image is persisted and added to the queue
  @retval EFI_INVALID_PARAMETER   CapsuleHeader was NULL
  @retval Other                   Something went wrong

*/
STATIC
EFI_STATUS
PersistCapsuleImageAcrossResetAndAddToQueue (
  IN EFI_CAPSULE_HEADER  *CapsuleHeader
  )
{
  EFI_STATUS                    Status;
  CAPSULE_PERSISTED_IDENTIFIER  CapsuleId;

  Status = EFI_SUCCESS;

  // If this is the first persistence, we should delete everything on the disk
  if (mFirstPersistence) {
    DEBUG ((DEBUG_INFO, "[%a] - removing all items from queue since this is the first capsule\n", __FUNCTION__));
    Status = DeleteAllPersistedCapsules ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - failed to remove items from the disk = %r\n", __FUNCTION__, Status));
      return Status;
    }
  }

  // Clear the queue
  Status = EFI_SUCCESS;
  while (mFirstPersistence && Status == EFI_SUCCESS) {
    Status = QueuePopItem (&gCapsuleQueueDataGuid, NULL, NULL);
  }

  mFirstPersistence = FALSE;

  // Save capsule image to the EFI partition
  Status = PersistCapsuleImageAcrossReset (CapsuleHeader, &CapsuleId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - failed to persist the capsule to disk = %r\n", __FUNCTION__, Status));
    return Status;
  }

  // Add to queue
  Status = QueueAddItem (&gCapsuleQueueDataGuid, &CapsuleId, sizeof (CapsuleId));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - failed to add the capsule to queue = %r\n", __FUNCTION__, Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "[%a] Queued Capsule with ID: %d, Hash 0x%lx\n",
    __FUNCTION__,
    CapsuleId.CapsuleId,
    CapsuleId.CapsuleHash
    ));

  return Status;
}

/**
  Returns if the capsule can be supported via UpdateCapsule().

  @param[in]  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                    being passed into update capsule.
  @param[in]  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                    CapsuleHeaderArray.
  @param[out] MaximumCapsuleSize    On output the maximum size that UpdateCapsule() can
                                    support as an argument to UpdateCapsule() via
                                    CapsuleHeaderArray and ScatterGatherList.
  @param[out] ResetType             Returns the type of reset required for the capsule update.

  @retval     EFI_SUCCESS           Valid values returned.
  @retval     EFI_UNSUPPORTED       The capsule image is not supported on this platform, and
                                    MaximumCapsuleSize and ResetType are undefined.
  @retval     EFI_INVALID_PARAMETER MaximumCapsuleSize is NULL, or ResetTyep is NULL,
                                    Or CapsuleCount is Zero, or CapsuleImage is not valid.

**/
EFI_STATUS
EFIAPI
QueryCapsuleCapabilitiesService (
  IN  EFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN  UINTN               CapsuleCount,
  OUT UINT64              *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE      *ResetType
  )
{
  EFI_STATUS          Status;
  UINTN               ArrayNumber;
  EFI_CAPSULE_HEADER  *CapsuleHeader;
  BOOLEAN             NeedReset;

  // CapsuleCount can't be less than one.
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  // Check whether input parameter is valid
  if ((MaximumCapsuleSize == NULL) || (ResetType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleHeaderArray == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CapsuleHeader = NULL;
  NeedReset     = FALSE;

  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];

    // A capsule which has the CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    if ((CapsuleHeader->Flags &
         (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)) ==
        CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE)
    {
      return EFI_INVALID_PARAMETER;
    }

    // A capsule which has the CAPSULE_FLAGS_INITIATE_RESET flag must have
    // CAPSULE_FLAGS_PERSIST_ACROSS_RESET set in its header as well.
    if ((CapsuleHeader->Flags &
         (CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_INITIATE_RESET)) ==
        CAPSULE_FLAGS_INITIATE_RESET)
    {
      return EFI_INVALID_PARAMETER;
    }

    // Check the FMP capsule flag
    if (  CompareGuid (&CapsuleHeader->CapsuleGuid, &gEfiFmpCapsuleGuid)
       && ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) != 0))
    {
      return EFI_INVALID_PARAMETER;
    }

    // Check Capsule image without populate flag is supported by firmware
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE) == 0) {
      Status = IsCapsuleImageSupported (CapsuleHeader);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  // Find out if there is any capsule defined to persist across system reset.
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) != 0) {
      NeedReset = TRUE;
      break;
    }
  }

  // Set the ResetType and MaxSize based on if we need a reset or not
  if (NeedReset) {
    *ResetType          = EfiResetWarm;
    *MaximumCapsuleSize = (UINT64)mMaxSizePopulateCapsule;
  } else {
    // For non-reset capsule image.
    *ResetType          = EfiResetCold;
    *MaximumCapsuleSize = (UINT64)mMaxSizeNonPopulateCapsule;
  }

  return EFI_SUCCESS;
}

/**
  Passes capsules to the firmware with both virtual and physical mapping. Depending on the intended
  consumption, the firmware may process the capsule immediately. If the payload should persist
  across a system reset, the reset value returned from EFI_QueryCapsuleCapabilities must
  be passed into ResetSystem() and will cause the capsule to be processed by the firmware as
  part of the reset process.

  @param[in]  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                    being passed into update capsule.
  @param[in]  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                    CapsuleHeaderArray.
  @param[in]  ScatterGatherList     Physical pointer to a set of
                                    EFI_CAPSULE_BLOCK_DESCRIPTOR that describes the
                                    location in physical memory of a set of capsules.

  @retval     EFI_SUCCESS           Valid capsule was passed. If
                                    CAPSULE_FLAGS_PERSIST_ACROSS_RESET is not set, the
                                    capsule has been successfully processed by the firmware.
  @retval     EFI_DEVICE_ERROR      The capsule update was started, but failed due to a device error.
  @retval     EFI_INVALID_PARAMETER CapsuleSize is NULL, or an incompatible set of flags were
                                    set in the capsule header.
  @retval     EFI_INVALID_PARAMETER CapsuleCount is Zero.
  @retval     EFI_INVALID_PARAMETER For across reset capsule image, ScatterGatherList is NULL.
  @retval     EFI_UNSUPPORTED       CapsuleImage is not recognized by the firmware.
  @retval     EFI_OUT_OF_RESOURCES  When ExitBootServices() has been previously called this error indicates the capsule
                                    is compatible with this platform but is not capable of being submitted or processed
                                    in runtime. The caller may resubmit the capsule prior to ExitBootServices().
  @retval     EFI_OUT_OF_RESOURCES  When ExitBootServices() has not been previously called then this error indicates
                                    the capsule is compatible with this platform but there are insufficient resources
                                    to process.

**/
EFI_STATUS
EFIAPI
UpdateCapsuleService (
  IN EFI_CAPSULE_HEADER    **CapsuleHeaderArray,
  IN UINTN                 CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS  ScatterGatherList OPTIONAL
  )
{
  UINTN               ArrayNumber;
  EFI_STATUS          Status;
  EFI_CAPSULE_HEADER  *CapsuleHeader;
  BOOLEAN             InitiateReset;
  UINT64              MaximumCapsuleSize;
  EFI_RESET_TYPE      ResetType;

  Status = EFI_SUCCESS;

  // CapsuleCount can't be less than one.
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleHeaderArray == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InitiateReset = FALSE;
  CapsuleHeader = NULL;

  // First query if we support all these capsules
  Status = QueryCapsuleCapabilitiesService (CapsuleHeaderArray, CapsuleCount, &MaximumCapsuleSize, &ResetType);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Walk through all capsules, record whether there is a capsule needs reset
  // or initiate reset. And then process capsules which has no reset flag directly.
  for (ArrayNumber = 0; ArrayNumber < CapsuleCount; ArrayNumber++) {
    CapsuleHeader = CapsuleHeaderArray[ArrayNumber];

    // Here should be in the boot-time for non-reset capsule image
    // Platform specific update for the non-reset capsule image.
    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_PERSIST_ACROSS_RESET) == 0) {
      return EFI_OUT_OF_RESOURCES;
    }

    if ((CapsuleHeader->Flags & CAPSULE_FLAGS_INITIATE_RESET) != 0) {
      InitiateReset = TRUE;
    }

    // Try to persist the capsule so we can get it on the next boot
    Status = PersistCapsuleImageAcrossResetAndAddToQueue (CapsuleHeader);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a]: Failed to Stage Capsule %d. Status = %r\n", __FUNCTION__, ArrayNumber, Status));
      break;
    }
  }

  //
  // ScatterGatherList is only referenced if the capsules are defined to persist across
  // system reset.
  //
  if (ScatterGatherList == (EFI_PHYSICAL_ADDRESS)(UINTN)NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (InitiateReset) {
    // Firmware that encounters a capsule which has the CAPSULE_FLAGS_INITIATE_RESET Flag set in its header
    // will initiate a reset of the platform which is compatible with the passed-in capsule request and will
    // not return back to the caller.
    // TODO: do a cold reset here or a different type so memory isn't preserved?
    ResetSystemWithSubtype (EfiResetWarm, &gCapsuleArmedResetGuid);
    // we shouldn't reach this point
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  return Status;
}

CONST CAPSULE_SERVICE_PROTOCOL  mCapsuleServiceProtocol = {
  UpdateCapsuleService,
  QueryCapsuleCapabilitiesService
};

/**
  Main entry for this driver.

  @param[in]  ImageHandle     Image handle this driver.
  @param[in]  SystemTable     Pointer to the SystemTable.

  @retval     EFI_SUCCESS    Successfully initialized.
  @retval     Other          Something went wrong

**/
EFI_STATUS
EFIAPI
CapsuleServiceInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mMaxSizePopulateCapsule    = PcdGet32 (PcdMaxSizePopulateCapsule);
  mMaxSizeNonPopulateCapsule = PcdGet32 (PcdMaxSizeNonPopulateCapsule);

  // Install the CapsuleService Protocol
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gCapsuleServiceProtocolGuid,
                  (CAPSULE_SERVICE_PROTOCOL *)&mCapsuleServiceProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
