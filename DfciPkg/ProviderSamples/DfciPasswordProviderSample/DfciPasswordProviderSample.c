/**
 Library Instance for DXE to support getting, setting, defaults,
 and support SystemSettings for Tool/Application/Ui interface. 

The UEFI System Password set/delete interface  

Copyright (c) 2015 Microsoft Corporation. All rights reserved
**/

#include <PiDxe.h>
#include <DfciSystemSettingTypes.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DfciSettingsProvider.h>

EFI_EVENT  mPasswordProviderSupportInstallEvent;
VOID       *mPasswordProviderSupportInstallEventRegistration = NULL;

EFI_STATUS
EFIAPI
SamplePasswordGetDefault (
  IN  CONST DFCI_SETTING_PROVIDER* This,
  OUT UINT8* Value
  )
{

  EFI_STATUS Status;

  DEBUG((DEBUG_ERROR, __FUNCTION__ ": enter...\n"));

  if ((This == NULL) || (Value == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (This->Id != DFCI_SETTING_ID__PASSWORD) {
    DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (0x%X)\n", This->Id));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  *Value = FALSE;
  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

EFI_STATUS
EFIAPI
SamplePasswordGet (
  IN  CONST DFCI_SETTING_PROVIDER* This,
  OUT UINT8* Value
  )
{

  EFI_STATUS Status;

  DEBUG((DEBUG_ERROR, __FUNCTION__ ": enter...\n"));

  if ((This == NULL) || (Value == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (This->Id != DFCI_SETTING_ID__PASSWORD) {
    DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (0x%X)\n", This->Id));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  //
  // Get the password.
  //

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

EFI_STATUS
EFIAPI
SamplePasswordSet (
  IN  CONST DFCI_SETTING_PROVIDER* This,
  IN  CONST UINT8* Value,
  OUT DFCI_SETTING_FLAGS* Flags
  )
{

  EFI_STATUS Status;
    
  DEBUG((DEBUG_ERROR, __FUNCTION__ ": enter...\n"));

  if ((This == NULL) || (Flags == NULL) || (Value == NULL)) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  *Flags = 0;

  if (This->Id != DFCI_SETTING_ID__PASSWORD) {
    DEBUG((DEBUG_ERROR, "PasswordSet was called with incorrect Provider Id (0x%X)\n", This->Id));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  //
  // TODO: Set password.
  //

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

EFI_STATUS
EFIAPI
SamplePasswordSetDefault (
  IN CONST DFCI_SETTING_PROVIDER* This
  )
{

  EFI_STATUS Status;

  DEBUG((DEBUG_ERROR, __FUNCTION__ ": enter...\n"));

  if (This == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Cleanup;
  }

  if (This->Id != DFCI_SETTING_ID__PASSWORD) {
    DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (0x%X)\n", This->Id));
    Status = EFI_UNSUPPORTED;
    goto Cleanup;
  }

  //
  // Set the password to default value.
  //

  Status = EFI_SUCCESS;

Cleanup:
  return Status;
}

DFCI_SETTING_PROVIDER mSamplePasswordProvider = {
    DFCI_SETTING_ID__PASSWORD,
    DFCI_SETTING_TYPE_PASSWORD,
    DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED,
    SamplePasswordSet,
    SamplePasswordGet,
    SamplePasswordGetDefault,
    SamplePasswordSetDefault
};

/*
Library design is such that a dependency on gDfciSettingsProviderSupportProtocolGuid
is not desired.  So to resolve that a ProtocolNotify is used.

This function gets triggered once on install and 2nd time when the Protocol gets installed.

When the gDfciSettingsProviderSupportProtocolGuid protocol is available the function will
loop thru all supported device disablement supported features (using PCD) and install the settings

Context is NULL.
*/
VOID
EFIAPI
SamplePasswordProviderSupportProtocolNotify (
  IN  EFI_EVENT Event,
  IN  VOID* Context
  )
{

  STATIC UINT8 CallCount = 0;
  EFI_STATUS Status;
  DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL* SettingsProvider;

  DEBUG((DEBUG_ERROR, __FUNCTION__ ": enter...\n"));

  //
  // locate the settings provider protocol.
  //
  Status = gBS->LocateProtocol(&gDfciSettingsProviderSupportProtocolGuid, 
                               NULL, 
                               (VOID**)&SettingsProvider);

  if (EFI_ERROR(Status) != FALSE) {
    if ((CallCount++ != 0) || (Status != EFI_NOT_FOUND)) {
      DEBUG((DEBUG_ERROR, "%a() - Failed to locate gDfciSettingsProviderSupportProtocolGuid in notify.  Status = %r\n", __FUNCTION__, Status));
    }

    return;
  }

  //
  // Register this setting provider.
  //
  DEBUG((DEBUG_INFO, "Registering Password Setting Provider\n"));
  Status = SettingsProvider->RegisterProvider(SettingsProvider, 
                                              &mSamplePasswordProvider);

  if (EFI_ERROR(Status) != FALSE) {
    DEBUG((DEBUG_ERROR, "Failed to Register.  Status = %r\n", Status));
  }

  //
  // We got here, this means all protocols were installed and we didn't exit early. 
  //close the event as we dont' need to be signaled again. (shouldn't happen anyway)
  //
  gBS->CloseEvent(Event);
  return;
}

/**
The constructor function initializes the Lib for Dxe.

This constructor is only needed for SettingsManager support.  
The design is to have the PCD false fall all modules except the 1 that should support the SettingsManager.  Because this 
is a build time PCD

The constructor function publishes Performance and PerformanceEx protocol, allocates memory to log DXE performance
and merges PEI performance data to DXE performance log.
It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

@param  ImageHandle   The firmware allocated handle for the EFI image.
@param  SystemTable   A pointer to the EFI System Table.

@retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DfciPasswordProviderSampleLibConstructor (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE* SystemTable
  )
{

  //
  // If the settings manager is installed, register to be notified when the
  // settings manager installs the settings provider protocol.
  //
  if (FeaturePcdGet(PcdSettingsManagerInstallProvider) != FALSE) {
    mPasswordProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent(
                                             &gDfciSettingsProviderSupportProtocolGuid,
                                             TPL_CALLBACK,
                                             SamplePasswordProviderSupportProtocolNotify,
                                             NULL,
                                             &mPasswordProviderSupportInstallEventRegistration);

    DEBUG((DEBUG_INFO, "%a - Event Registered.\n", __FUNCTION__));
  }

  return EFI_SUCCESS;
}