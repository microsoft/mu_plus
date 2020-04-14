/** @file
DfciSettings.c

Library Instance for DXE to support getting, setting, defaults, and support the
Device.CpuAndIoVirtualization.Enable setting.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Guid/DfciSettingsGuid.h>

#include <Protocol/DfciSettingsProvider.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Settings/DfciSettings.h>
#include <Settings/DfciOemSample.h>

EFI_EVENT  mDfciVirtualSettingsProviderSupportInstallEvent;
VOID      *mDfciVirtualSettingsProviderSupportInstallEventRegistration = NULL;

typedef enum {
    ID_IS_BAD,
    ID_IS_VIRTUALIZATION,
}  ID_IS;

// There are no setting to change the support for CPU and I/O virtualization

#define HARD_CODED_VIRTUALIZATION 1

// Forward declarations needed
/**
 * Settings Provider GetDefault routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsGetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This,
    IN  OUT   UINTN                     *ValueSize,
    OUT       VOID                      *Value
  );

/**
 * Settings Provider Get routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsGet (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN  OUT   UINTN                  *ValueSize,
  OUT       VOID                   *Value
  );

/**
@param Id - Setting ID to check for support status
@retval ID_IS_xx  - of the supported settings
@retval ID_IS_BAD - Not supported
**/
STATIC
ID_IS
IsIdSupported (
	IN  DFCI_SETTING_ID_STRING Id
  ) {

    if (0 == AsciiStrnCmp (Id, DFCI_OEM_SETTING_ID__ENABLE_VIRT_SETTINGS, DFCI_MAX_ID_LEN)) {
        return ID_IS_VIRTUALIZATION;
    } else {
        DEBUG((DEBUG_ERROR, "%a: Called with Invalid ID (%a)\n", __FUNCTION__, Id));
    }

    return ID_IS_BAD;
}

/////---------------------Interface for Settings Provider ---------------------//////

/**
 * Settings Provider Set Routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 * @param Flags
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsSet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN        UINTN                     ValueSize,
    IN  CONST VOID                     *Value,
    OUT       DFCI_SETTING_FLAGS       *Flags
  ) {

    EFI_STATUS      Status;
    ID_IS           Id;
    UINT8           NewValue;

    if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (Flags == NULL) || (ValueSize < sizeof(UINT8))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    NewValue = *((UINT8 *) Value);

    Id = IsIdSupported(This->Id);
    switch (Id) {
        case ID_IS_VIRTUALIZATION:
            if (NewValue != HARD_CODED_VIRTUALIZATION) {
                Status = EFI_UNSUPPORTED;
            } else {
                *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
                Status = EFI_SUCCESS;
            }
            break;

        default:
            DEBUG((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
            Status = EFI_UNSUPPORTED;
            break;
    }

    return Status;
}

/**
 * Settings Provider Get routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsGet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN  OUT   UINTN                    *ValueSize,
    OUT       VOID                     *Value
  ) {

    ID_IS               Id;
    EFI_STATUS          Status;
    UINT8              *CurrentValue;

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || (Value == NULL)) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    if (*ValueSize < sizeof(UINT8)) {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    CurrentValue = (UINT8 *) Value;

    Id = IsIdSupported(This->Id);
    Status = EFI_SUCCESS;

    switch (Id) {

        // Current setting is hard coded to Enabled.
        case ID_IS_VIRTUALIZATION:
            *CurrentValue = HARD_CODED_VIRTUALIZATION;
            *ValueSize = sizeof(UINT8);
            break;

        default:
            DEBUG((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
            Status = EFI_UNSUPPORTED;
            break;
    }

    return Status;
}

/**
 * Settings Provider GetDefault routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsGetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This,
    IN  OUT   UINTN                     *ValueSize,
    OUT       VOID                      *Value
  ) {

    ID_IS    Id;
    UINT8   *DefaultValue;

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || (Value == NULL)) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    if (*ValueSize < sizeof(UINT8)) {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    Id = IsIdSupported(This->Id);
    if (Id == ID_IS_BAD) {
        return EFI_UNSUPPORTED;
    }

    DefaultValue = (UINT8 *) Value;
    *ValueSize = sizeof(UINT8);
    *DefaultValue = HARD_CODED_VIRTUALIZATION;

    return EFI_SUCCESS;
}

/**
 * Settings Provider Set Default routine
 *
 * @param This
 *
 * @return EFI_STATUS EFIAPI
 */
STATIC
EFI_STATUS
EFIAPI
DfciVirtSettingsSetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This
  ) {

    DFCI_SETTING_FLAGS Flags = 0;
    EFI_STATUS         Status;
    UINT8              Value;
    UINTN              ValueSize;

    if (This == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    ValueSize = sizeof(ValueSize);
    Status = DfciVirtSettingsGetDefault (This, &ValueSize, &Value);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return DfciVirtSettingsSet (This, ValueSize, &Value, &Flags);
}

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//
STATIC DFCI_SETTING_PROVIDER mDfciCpuAndIoProviderTemplate = {
  0,
  0,
  0,
  DfciVirtSettingsSet,
  DfciVirtSettingsGet,
  DfciVirtSettingsGetDefault,
  DfciVirtSettingsSetDefault
};

/////---------------------Interface for Library  ---------------------//////

/**
Function to Get a Dfci Setting.
If the setting has not been previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:          The DFCI_SETTING_ID_ENUM of the Dfci
@param ValueSize:   IN=Size Of Buffer or 0 to get size, OUT=Size of returned Value
@param Value:       Ptr to a buffer for the setting to be returned.


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetVirtualizationSetting (
    IN      DFCI_SETTING_ID_STRING   Id,
    IN  OUT UINTN                   *ValueSize,
    OUT     VOID                    *Value
  ) {

    EFI_STATUS      Status;

    mDfciCpuAndIoProviderTemplate.Id = Id;
    Status = DfciVirtSettingsGet (&mDfciCpuAndIoProviderTemplate, ValueSize, Value);
    if (EFI_ERROR(Status) && (EFI_BUFFER_TOO_SMALL != Status)) {
        Status = DfciVirtSettingsGetDefault (&mDfciCpuAndIoProviderTemplate, ValueSize, Value);
    }
    return Status;
}

/**
 * Library design is such that a dependency on gDfciSettingsProviderSupportProtocolGuid
 * is not desired.  So to resolve that a ProtocolNotify is used.
 *
 * This function gets triggered once on install and 2nd time when the Protocol gets installed.
 *
 * When the gDfciSettingsProviderSupportProtocolGuid protocol is available the function will
 * loop thru all the Dfci settings (using PCD) and install the settings
 *
 * Context is NULL.
 *
 *
 * @param Event
 * @param Context
 *
 * @return VOID EFIAPI
 */
STATIC
VOID
EFIAPI
DfciSettingsProviderSupportProtocolNotify (
    IN  EFI_EVENT       Event,
    IN  VOID            *Context
  ) {

    STATIC UINT8                            CallCount = 0;
    DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL *sp;
    EFI_STATUS                              Status;

    //locate protocol
    Status = gBS->LocateProtocol (&gDfciSettingsProviderSupportProtocolGuid, NULL, (VOID**)&sp);
    if (EFI_ERROR(Status)) {
      if ((CallCount++ != 0) || (Status != EFI_NOT_FOUND)) {
        DEBUG((DEBUG_ERROR, "%a() - Failed to locate gDfciSettingsProviderSupportProtocolGuid in notify.  Status = %r\n", __FUNCTION__, Status));
      }
      return;
    }

    //
    // Register items that are NOT in the PREBOOT_UI
    //
    mDfciCpuAndIoProviderTemplate.Id = DFCI_OEM_SETTING_ID__ENABLE_VIRT_SETTINGS;
    mDfciCpuAndIoProviderTemplate.Type = DFCI_SETTING_TYPE_ENABLE;
    mDfciCpuAndIoProviderTemplate.Flags = DFCI_SETTING_FLAGS_NO_PREBOOT_UI;
    Status = sp->RegisterProvider (sp, &mDfciCpuAndIoProviderTemplate);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to Register Virtual Settings.  Status = %r\n", Status));
    }

    //We got here, this means all protocols were installed and we didn't exit early.
    //close the event as we dont' need to be signaled again. (shouldn't happen anyway)
    gBS->CloseEvent(Event);
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is only needed for DfciSettingsManager support.
 * The design is to have the PCD false for all modules except the 1 anonymously liked to the DfciSettingsManager.
 *
 * @param  ImageHandle   The firmware allocated handle for the EFI image.
 * @param  SystemTable   A pointer to the EFI System Table.
 *
 * @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
 *
 **/
EFI_STATUS
EFIAPI
DfciVirtualizationSettingsConstructor (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
  ) {

    if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {
        //Install callback on the SettingsManager gMsSystemSettingsProviderSupportProtocolGuid protocol
        mDfciVirtualSettingsProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent (
            &gDfciSettingsProviderSupportProtocolGuid,
             TPL_CALLBACK,
             DfciSettingsProviderSupportProtocolNotify,
             NULL,
            &mDfciVirtualSettingsProviderSupportInstallEventRegistration
            );

        DEBUG((DEBUG_INFO, "%a: Event Registered.\n", __FUNCTION__));
    }
    return EFI_SUCCESS;
}