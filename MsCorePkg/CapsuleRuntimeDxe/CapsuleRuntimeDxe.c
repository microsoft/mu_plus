/** @file
  Capsule Runtime Driver produces two UEFI capsule runtime services.
  (UpdateCapsule, QueryCapsuleCapabilities)
  It installs the Capsule Architectural Protocol defined in PI1.0a to signify
  the capsule runtime services are ready.

  Copyright (c) Microsoft Corporation. All rights reserved.

**/

#include <Uefi.h>

#include <Protocol/Capsule.h>
#include <Guid/CapsuleVendor.h>
#include <Guid/FmpCapsule.h>
#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/SecurityLockAuditLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/CapsuleService.h>

// Handle for the installation of Capsule Architecture Protocol.
EFI_HANDLE  mCapsuleArchProtocolHandle = NULL;

// Support locking capsule interface.
BOOLEAN  mAfterLocked = FALSE;

// Pointer to the capsule service protocol
// We don't need to convert this since we lock at EBS
CAPSULE_SERVICE_PROTOCOL  *mCapsuleServiceProtocol = NULL;

/**
  Checks if the capsule can be supported via UpdateCapsule().

  @param[in, out]   Protocol              Pointer to a capsule service protocol pointer

  @retval           EFI_SUCCESS           Valid values returned.
  @retval           EFI_UNSUPPORTED       The capsule interface is locked
  @retval           EFI_INVALID_PARAMETER Protocol or mAfterLocked is NULL,
**/
STATIC
EFI_STATUS
GetCapsuleServiceProtocol (
  IN OUT CAPSULE_SERVICE_PROTOCOL  **Protocol
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;

  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mAfterLocked) {
    return EFI_UNSUPPORTED;
  }

  if (mCapsuleServiceProtocol == NULL) {
    // request the protocol if we haven't cached this

    Status = gBS->LocateProtocol (
                    &gCapsuleServiceProtocolGuid,
                    NULL,
                    (VOID **)&mCapsuleServiceProtocol
                    );
  }

  if (!EFI_ERROR (Status)) {
    *Protocol = mCapsuleServiceProtocol;
  }

  return Status;
}

/**
  Checks if the capsule can be supported via UpdateCapsule().

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
  EFI_STATUS                Status;
  CAPSULE_SERVICE_PROTOCOL  *ProtocolHandle;

  // CapsuleCount can't be less than one.
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if we've already locked
  if (mAfterLocked) {
    return EFI_UNSUPPORTED;
  }

  // Find the protocol and query it
  ProtocolHandle = NULL;
  Status         = GetCapsuleServiceProtocol (&ProtocolHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to find Capsule Service DXE protocol  = %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (ProtocolHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  return ProtocolHandle->QueryCapsuleCapabilities (
                           CapsuleHeaderArray,
                           CapsuleCount,
                           MaximumCapsuleSize,
                           ResetType
                           );
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

  @retval EFI_SUCCESS               Valid capsule was passed. If CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set,
                                    the capsule has been successfully processed by the firmware.
  @retval EFI_DEVICE_ERROR          The capsule update was started, but failed due to a device error.
  @retval EFI_INVALID_PARAMETER     CapsuleSize is NULL, or an incompatible set of flags were
                                    set in the capsule header.
  @retval EFI_INVALID_PARAMETER     CapsuleCount is Zero.
  @retval EFI_INVALID_PARAMETER     For across reset capsule image, ScatterGatherList is NULL.
  @retval EFI_UNSUPPORTED           CapsuleImage is not recognized by the firmware.
  @retval EFI_OUT_OF_RESOURCES      When ExitBootServices() has been previously called this error indicates the capsule
                                    is compatible with this platform but is not capable of being submitted or processed
                                    in runtime. The caller may resubmit the capsule prior to ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES      When ExitBootServices() has not been previously called then this error indicates
                                    the capsule is compatible with this platform but there are insufficient resources to process.

**/
EFI_STATUS
EFIAPI
UpdateCapsuleService (
  IN EFI_CAPSULE_HEADER    **CapsuleHeaderArray,
  IN UINTN                 CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS  ScatterGatherList OPTIONAL
  )
{
  EFI_STATUS                Status;
  CAPSULE_SERVICE_PROTOCOL  *ProtocolHandle;

  // CapsuleCount can't be less than one.
  if (CapsuleCount < 1) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if we've already locked
  if (mAfterLocked) {
    return EFI_UNSUPPORTED;
  }

  // Find the protocol and query it
  ProtocolHandle = NULL;
  Status         = GetCapsuleServiceProtocol (&ProtocolHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Failed to find Capsule Service DXE protocol  = %r\n", __FUNCTION__, Status));
    return Status;
  }

  if (ProtocolHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  return ProtocolHandle->UpdateCapsule (
                           CapsuleHeaderArray,
                           CapsuleCount,
                           ScatterGatherList
                           );
}

/**
  LockCapsuleInterface Event handler locks the capsule interface so no input is accepted.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context

**/
VOID
EFIAPI
LockCapsuleInterface (
  IN      EFI_EVENT  Event,
  IN      VOID       *Context
  )
{
  mAfterLocked            = TRUE;
  mCapsuleServiceProtocol = NULL;
  SECURITY_LOCK_REPORT_EVENT ("Lock Capsule Interface", SOFTWARE_LOCK);
  DEBUG ((DEBUG_INFO, "Capsule Interface Locked!!\n"));
}

/**
  Main entry for this driver.

  @param[in]  ImageHandle    Image handle this driver.
  @param[in]  SystemTable    Pointer to the SystemTable.

  @retval     EFI_SUCCESS    Successfully initialized capsule services.
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
  EFI_EVENT   LockEvent;

  // Install capsule runtime services into UEFI runtime service tables.
  gRT->UpdateCapsule            = UpdateCapsuleService;
  gRT->QueryCapsuleCapabilities = QueryCapsuleCapabilitiesService;

  // Install the Capsule Architectural Protocol on a new handle
  // to signify the capsule runtime services are ready.
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCapsuleArchProtocolHandle,
                  &gEfiCapsuleArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Add lock support
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  LockCapsuleInterface,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &LockEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
