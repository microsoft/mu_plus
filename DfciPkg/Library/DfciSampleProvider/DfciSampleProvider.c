/** @file
DfciSampleProvider.c

Library Instance for DXE to support getting, setting, defaults, and support Dfci settings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Protocol/DfciSettingsProvider.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Settings/DfciSettings.h>

STATIC EFI_EVENT  mDfciSampleProviderProviderSupportInstallEvent;
STATIC VOID      *mDfciSampleProviderProviderSupportInstallEventRegistration = NULL;

// Sample provider STORE is a global variable.  It will not keep a setting across a restart, but
// the idea of this code is to highlight the DFCI mechanics involved with a settings provider

#define MY_SETTING_ID__SETTING1  "Oem.Setting1.Enable"

STATIC  UINT8  mMySettingStore;


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
DfciSampleProviderGetDefault (
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
DfciSampleProviderGet (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN  OUT   UINTN                  *ValueSize,
  OUT       VOID                   *Value
  );

/**
@param Id - Setting ID to check for support status
@retval TRUE - Supported
@retval FALSE - Not supported
**/
STATIC
BOOLEAN
IsIdSupported (DFCI_SETTING_ID_STRING Id)
{

    if (0 == AsciiStrnCmp (Id, MY_SETTING_ID__SETTING1, DFCI_MAX_ID_LEN)) {
        return TRUE;
    }

    return FALSE;
}

/**
 * Internal function used to initialize the non volatile storage.
 *
 * @param
 *
 * @return STATIC EFI_STATUS
 */
STATIC
EFI_STATUS
InitializeSettingStore (
    VOID
  )
{
    // This sample is just using a global variable as the settings storage.

    mMySettingStore = 1;

    return EFI_SUCCESS;
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
DfciSampleProviderSet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN        UINTN                     ValueSize,
    IN  CONST VOID                     *Value,
    OUT       DFCI_SETTING_FLAGS       *Flags
  )
{
    UINTN           BufferSize;
    EFI_STATUS      Status;
    UINT8           CurrentValue;
    UINT8           NewValue;

    if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (Flags == NULL) ||
        (ValueSize > DFCI_SETTING_MAXIMUM_SIZE) ||
        (ValueSize < sizeof(UINT8))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    if (!IsIdSupported(This->Id)) {
        DEBUG((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
        return EFI_UNSUPPORTED;
    }

    // Only use the first UINT8 of the value
    NewValue = *((UINT8 *) Value);
    BufferSize = sizeof(CurrentValue);
    Status = DfciSampleProviderGet (This, &BufferSize, &CurrentValue);

    if (Status != EFI_NOT_FOUND) {
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Error getting %a. Code=%r\n", __FUNCTION__, This->Id, Status));
            return Status;
        }

        if (NewValue == CurrentValue) {
                *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
                DEBUG((DEBUG_INFO, "Setting %a ignored, value didn't change\n", This->Id));
                return EFI_SUCCESS;
        }
    }

    mMySettingStore = NewValue;

    Status = EFI_SUCCESS;

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
DfciSampleProviderGet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN  OUT   UINTN                    *ValueSize,
    OUT       VOID                     *Value
  )
{
    EFI_STATUS          Status;

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    if (!IsIdSupported(This->Id)) {
        DEBUG((DEBUG_ERROR, "%a: Invalid id(%a).\n", __FUNCTION__, This->Id));
        return EFI_UNSUPPORTED;
    }

    if (*ValueSize < sizeof(UINT8)) {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    Status = EFI_SUCCESS;
    *ValueSize = sizeof(UINT8);
    *((UINT8 *) Value) = mMySettingStore;

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
DfciSampleProviderGetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This,
    IN  OUT   UINTN                     *ValueSize,
    OUT       VOID                      *Value
  )
{

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    if (!IsIdSupported(This->Id)) {
        return EFI_UNSUPPORTED;
    }

    if (*ValueSize < sizeof(UINT8)) {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    *ValueSize = sizeof(UINT8);
    *((UINT8 *)Value) = 1;  // Indicates Enabled default

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
DfciSampleProviderSetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This
  )
{
    DFCI_SETTING_FLAGS Flags;
    EFI_STATUS         Status;
    UINT8              Value;
    UINTN              ValueSize;

    if (This == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    ValueSize = sizeof(ValueSize);
    Status = DfciSampleProviderGetDefault (This, &ValueSize, &Value);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Flags = 0;
    return DfciSampleProviderSet (This, ValueSize, &Value, &Flags);
}

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//
// NO_PREBOOT_UI indicates there is no UI element for the user to change
// the value.  Therefore, set this setting to its default value on an UnEnroll
DFCI_SETTING_PROVIDER mDfciSampleProviderProviderSetting1 = {
    MY_SETTING_ID__SETTING1,
    DFCI_SETTING_TYPE_ENABLE,
    DFCI_SETTING_FLAGS_NO_PREBOOT_UI,   // NO UI element for user to change
    DfciSampleProviderSet,
    DfciSampleProviderGet,
    DfciSampleProviderGetDefault,
    DfciSampleProviderSetDefault
};

/////---------------------Interface for Library  ---------------------//////

// Here is where you would have private interfaces to get and or set a settings value

EFI_STATUS
OEM_GetSampleSetting1 (
  OUT UINT8    *LocalSetting
  ) {

    UINTN       LocalSettingSize;
    EFI_STATUS  Status;

    LocalSettingSize = sizeof (*LocalSetting);

    Status = DfciSampleProviderGet ( &mDfciSampleProviderProviderSetting1,
                                     &LocalSettingSize,
                                     &LocalSetting);

    return Status;
}


/**
 * Library design is such that a dependency on gDfciSettingsProviderSupportProtocolGuid
 * is not desired.  So to resolve that, a ProtocolNotify is used.
 *
 * This function gets triggered once on install and 2nd time when the Protocol gets installed.
 *
 * When the gDfciSettingsProviderSupportProtocolGuid protocol is available this function will
 * register the settings provided by this provider.
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
DfciSampleProviderProviderSupportProtocolNotify (
    IN  EFI_EVENT       Event,
    IN  VOID            *Context
  )
{
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

    Status = sp->RegisterProvider (sp, &mDfciSampleProviderProviderSetting1);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to Register %a.  Status = %r\n", mDfciSampleProviderProviderSetting1.Id, Status));
    }

    //We got here, this means all protocols were installed and we didn't exit early.
    //close the event as we don't need to be signaled again. (shouldn't happen anyway)
    gBS->CloseEvent(Event);
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is only needed for DfciSettingsManager support.
 * The design is to have the PCD false when linking for private access from all modules except
 * the 1 anonymously linked to the DfciSettingsManager.
 *
 * @param  ImageHandle   The firmware allocated handle for the EFI image.
 * @param  SystemTable   A pointer to the EFI System Table.
 *
 * @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
 *
 **/
EFI_STATUS
EFIAPI
DfciSampleProviderConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;

    if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {
        //Install callback on the SettingsManager gDfciSettingsProviderSupportProtocolGuid protocol
        mDfciSampleProviderProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent (
            &gDfciSettingsProviderSupportProtocolGuid,
             TPL_CALLBACK,
             DfciSampleProviderProviderSupportProtocolNotify,
             NULL,
            &mDfciSampleProviderProviderSupportInstallEventRegistration
            );

        DEBUG((DEBUG_INFO, "%a: Event Registered.\n", __FUNCTION__));

        //Initialize the settings store
        Status = InitializeSettingStore ();
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Initialize Store failed. %r.\n", __FUNCTION__, Status));
        }
    }
    return EFI_SUCCESS;
}

