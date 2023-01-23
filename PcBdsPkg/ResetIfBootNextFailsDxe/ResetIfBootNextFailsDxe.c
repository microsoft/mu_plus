/** @file

  A DXE driver that will reset the system if Boot Next fails.

  This can be useful if the platform would like to perform earlier boot
  steps in PEI and DXE differently if a Boot Next option is not present.

  Copyright (c) Microsoft Corporation. All rights reserved.

**/

#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Guid/GlobalVariable.h>

#include <Protocol/ReportStatusCodeHandler.h>

#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC UINTN                     mSavedBootNextOption              = MAX_UINTN;
STATIC EFI_RSC_HANDLER_PROTOCOL  *mReportStatusCodeHandlerProtocol = NULL;

/**
  Called when an OS loader option fails.

  This function will attempt to reset the system the option that failed was the BootNext option.

  @param[in]    Data    The Data from the Report Status Code.

**/
VOID
ProcessLoadOptionFailure (
  IN EFI_STATUS_CODE_DATA  *Data
  )
{
  //
  // There is no header definition for the ExtendedData.
  // BmBoot.c, in MdeModulePkg/Library/UefiBootManagerLib/, defines the data
  // as a 2 entry array of UINTN.
  //
  // The first one is a device path pointer, and the second is the status.
  //
  EFI_STATUS  Status;
  UINT16      BootCurrent;
  UINTN       Size;

  if (mSavedBootNextOption == MAX_UINTN) {
    // BootNext was not set on this boot. Nothing to do in this library.
    return;
  }

  // BootNext should have been cleared by BDS by the point a boot option is called and fails.
  Size   = sizeof (UINT16);
  Status = gRT->GetVariable (EFI_BOOT_NEXT_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &Size, NULL);
  ASSERT (Status == EFI_NOT_FOUND);

  // BootCurrent should have been set by BDS to the boot option that is called and fails.
  Size        = sizeof (BootCurrent);
  BootCurrent = MAX_UINT16;
  Status      = gRT->GetVariable (EFI_BOOT_CURRENT_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &Size, &BootCurrent);
  ASSERT (Status == EFI_SUCCESS);

  // Only reset if the current boot option is the BootNext option.
  if (mSavedBootNextOption == BootCurrent) {
    DEBUG ((DEBUG_INFO, "[%a] - Attempting to reset due to Boot Next boot option failure.\n", __FUNCTION__));
    ResetWarm ();
  }
}

/**
  Process Report Status code looking for the Boot Manager progress codes.

  @param    CodeType     Type of Report Status Code.
  @param    Value        Class, SubClass, and operation.
  @param    Instance     Provider Instance.
  @param    CallerId     Callers module id.  Not Used.
  @param    Data         Data specific to CodeType and Code.

  @retval   EFI_SUCCESS

  This may be called for multiple ReadyToBoot notifications, so the Event is not closed.

**/
EFI_STATUS
EFIAPI
RscHandlerCallback (
  IN EFI_STATUS_CODE_TYPE   CodeType,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN EFI_GUID               *CallerId,
  IN EFI_STATUS_CODE_DATA   *Data
  )
{
  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE) &&
      (((Value & EFI_STATUS_CODE_CLASS_MASK)|(Value & EFI_STATUS_CODE_SUBCLASS_MASK)) == (EFI_SOFTWARE | EFI_SOFTWARE_DXE_BS_DRIVER)) &&
      (((Value & EFI_STATUS_CODE_OPERATION_MASK) == EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED)))
  {
    DEBUG ((DEBUG_VERBOSE, "[%a] - Checking boot option failure reported from module {%g}.\n", __FUNCTION__, CallerId));
    ProcessLoadOptionFailure (Data);
  }

  return EFI_SUCCESS;
}

/**
  Unregister the ReportStatusCode handler.

  @param[in]    Event      The Exit Boot Services event.
  @param[in]    Context    A pointer to the context structure associated with the notification function.

**/
VOID
EFIAPI
OnExitBootServicesNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (mReportStatusCodeHandlerProtocol != NULL) {
    Status = mReportStatusCodeHandlerProtocol->Unregister (RscHandlerCallback);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] - Unable to unregister RscHandler - %r\n", __FUNCTION__, Status));
    }

    mReportStatusCodeHandlerProtocol = NULL;
  }
}

/**
  ProcessReportStatusCodeHandlerRegistration

  This function registers for a ReportStatusCode callback

  @retval   EFI_SUCCESS  Event created
  @retval   Others       Event registration failed

**/
EFI_STATUS
ProcessReportStatusCodeHandlerRegistration (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mReportStatusCodeHandlerProtocol != NULL) {
    Status =  mReportStatusCodeHandlerProtocol->Register (RscHandlerCallback, TPL_CALLBACK);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a] Error registering RscHandler - %r\n", __FUNCTION__, Status));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "[%a] Report Status Code Handler protocol NULL.\n", __FUNCTION__));
    Status = EFI_NOT_FOUND;
  }

  return Status;
}

/**
  Creates a callback to unregister the ReportStatusCodeHandler.

  @retval  EFI_SUCCESS    Registration succeeded
  @retval  Others         An error code returned from the CreateEventEx() call

**/
EFI_STATUS
ProcessExitBootServicesRegistration (
  VOID
  )
{
  EFI_EVENT   InitEvent;
  EFI_STATUS  Status;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnExitBootServicesNotification,
                  gImageHandle,
                  &gEfiEventExitBootServicesGuid,
                  &InitEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Create Event failed for ExitBootServices - %r\n", __FUNCTION__, Status));
  }

  return Status;
}

/**
  Caches the BootNext option local to this driver.

  The option value is saved when this function is called so it is available in case the
  driver needs to refer to the value after the actual BootNext UEFI variable is deleted
  when code in the module is called back in the future.

*/
VOID
CacheBootNextOption (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT16      BootNext;

  Size   = sizeof (BootNext);
  Status = gRT->GetVariable (EFI_BOOT_NEXT_VARIABLE_NAME, &gEfiGlobalVariableGuid, NULL, &Size, &BootNext);
  if (Status == EFI_SUCCESS) {
    mSavedBootNextOption = BootNext;
  }
}

/**
  Driver entry point.

  @param[in]    ImageHandle   The image handle.
  @param[in]    SystemTable   A pointer to the System Table.

  @retval  EFI_SUCCESS        The entry point executed successfully.
  @retval  EFI_NOT_FOUND      The Report Status Code Handler protocol could not be found.

*/
EFI_STATUS
EFIAPI
DxeResetIfBootNextFailsEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (
                  &gEfiRscHandlerProtocolGuid,
                  NULL,
                  (VOID **)&mReportStatusCodeHandlerProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a] - Error locating RscHandler Protocol - %r\n", __FUNCTION__, Status));
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  Status = ProcessReportStatusCodeHandlerRegistration ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = ProcessExitBootServicesRegistration ();
    ASSERT_EFI_ERROR (Status);

    CacheBootNextOption ();
  }

  return EFI_SUCCESS;
}
