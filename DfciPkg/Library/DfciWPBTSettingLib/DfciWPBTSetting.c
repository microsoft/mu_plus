/** @file
DfciWPBTSetting.c

Library Instance for DXE to support getting, setting, defaults, and support the Dfci.WPBT.Enable setting.

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
#include <Settings/DfciPrivateSettings.h>

STATIC EFI_EVENT  mDfciWPBTSettingProviderSupportInstallEvent;
STATIC VOID      *mDfciWPBTSettingProviderSupportInstallEventRegistration = NULL;

typedef enum {
    ID_IS_BAD,
    ID_IS_WPBT_ENABLE,
}  ID_IS;

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
DfciWPBTSettingGetDefault (
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
DfciWPBTSettingGet (
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
ID_IS
IsIdSupported (DFCI_SETTING_ID_STRING Id)
{

    if (0 == AsciiStrnCmp (Id, DFCI_STD_SETTING_ID_V3_ENABLE_WPBT, DFCI_MAX_ID_LEN)) {
        return ID_IS_WPBT_ENABLE;
    } else {
        DEBUG((DEBUG_ERROR, "%a: Called with Invalid ID (%a)\n", __FUNCTION__, Id));
    }

    return ID_IS_BAD;
}

/**
 * Validate Nv Variable
 *
 * @param VariableName
 *
 * @return STATIC EFI_STATUS
 */
STATIC
EFI_STATUS
ValidateNvVariable (
    CHAR16 *VariableName
  )
{
    EFI_STATUS  Status;
    UINT32      Attributes = 0;
    UINTN       ValueSize = 0;
    VOID       *Value = NULL;


    Status = GetVariable3 (VariableName,
                          &gDfciSettingsGuid,
                          (VOID *) &Value,
                          &ValueSize,
                          &Attributes);

    if (!EFI_ERROR(Status)) {             // We have a variable
        FreePool (Value);
        if (DFCI_SETTINGS_ATTRIBUTES != Attributes) {  // Check if Attributes are wrong
            // Delete invalid URL variable
            Status = gRT->SetVariable (VariableName,
                                      &gDfciSettingsGuid,
                                       0,
                                       0,
                                       NULL);
            if (EFI_ERROR(Status)) {                   // What???
                DEBUG((DEBUG_ERROR, "%a: Unable to delete invalid variable %s\n", __FUNCTION__, VariableName));
            } else {
                DEBUG((DEBUG_INFO, "%a: Deleting invalid variable %s, with attributes %x\n", __FUNCTION__, VariableName, Attributes));
            }
        }
    } else {
        Status = EFI_SUCCESS;
    }

    return Status;
}

/**
 * Internal function used to initialize the non volatile variables.
 *
 * @param
 *
 * @return STATIC EFI_STATUS
 */
STATIC
EFI_STATUS
InitializeNvVariables (
    VOID
  )
{
    EFI_STATUS  Status;

    Status  = ValidateNvVariable (DFCI_SETTINGS_WPBT_NAME);

    return Status;
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
DfciWPBTSettingSet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN        UINTN                     ValueSize,
    IN  CONST VOID                     *Value,
    OUT       DFCI_SETTING_FLAGS       *Flags
  )
{
    UINT8           CurrentValue;
    UINTN           BufferSize;
    EFI_STATUS      Status;
    CHAR16         *VariableName;
    ID_IS           Id;

    if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (Flags == NULL) || (ValueSize != sizeof(UINT8))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    Id = IsIdSupported(This->Id);
    switch (Id) {
        case ID_IS_WPBT_ENABLE:
            VariableName = DFCI_SETTINGS_WPBT_NAME;
            break;

        default:
            DEBUG((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
            return EFI_UNSUPPORTED;
    }

    BufferSize = sizeof(CurrentValue);
    Status = DfciWPBTSettingGet (This, &BufferSize, &CurrentValue);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Error getting %s. Code=%r\n", __FUNCTION__, VariableName, Status));
        return Status;
    }


    if (CurrentValue == *((UINT8 *) Value)) {
       *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
        DEBUG((DEBUG_INFO, "Setting %s ignored, value didn't change\n", VariableName));
        return EFI_SUCCESS;
    }

    Status = gRT->SetVariable (VariableName,
                              &gDfciSettingsGuid,
                               DFCI_SETTINGS_ATTRIBUTES,
                               ValueSize,
                               (VOID *) Value);  // SetVariable should not touch *Value
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error setting variable %s.  Code = %r\n", VariableName, Status));
    } else {
        DEBUG((DEBUG_INFO, "Variable %s set Attributes=%x, Size=%d.\n", VariableName, DFCI_SETTINGS_ATTRIBUTES, ValueSize));
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
DfciWPBTSettingGet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN  OUT   UINTN                    *ValueSize,
    OUT       VOID                     *Value
  )
{
    ID_IS               Id;
    EFI_STATUS          Status;
    CHAR16             *VariableName;

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    Id = IsIdSupported(This->Id);
    switch (Id) {
        case ID_IS_WPBT_ENABLE:
            VariableName = DFCI_SETTINGS_WPBT_NAME;
            break;

        default:
            DEBUG((DEBUG_ERROR, "%a: Invalid id(%a).\n", __FUNCTION__, This->Id));
            return EFI_UNSUPPORTED;
            break;
    }

    Status = gRT->GetVariable (VariableName,
                              &gDfciSettingsGuid,
                               NULL,
                               ValueSize,
                               Value );
    if (EFI_NOT_FOUND == Status) {
        DEBUG((DEBUG_INFO, "%a - Variable %s not found. Getting default value.\n", __FUNCTION__, VariableName));
        Status = DfciWPBTSettingGetDefault (This, ValueSize, Value);
    }

    if (EFI_ERROR(Status)) {
        if (EFI_BUFFER_TOO_SMALL != Status) {
            DEBUG((DEBUG_ERROR, "%a - Error retrieving setting %s. Code=%r\n", __FUNCTION__, VariableName, Status));
        }
    } else {
        DEBUG((DEBUG_INFO, "%a - Setting %s retrieved.\n", __FUNCTION__ , VariableName));
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
DfciWPBTSettingGetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This,
    IN  OUT   UINTN                     *ValueSize,
    OUT       VOID                      *Value
  )
{
    ID_IS    Id;

    if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    Id = IsIdSupported(This->Id);
    if (Id == ID_IS_BAD) {
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
DfciWPBTSettingSetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This
  )
{
    DFCI_SETTING_FLAGS Flags = 0;
    EFI_STATUS         Status;
    UINT8              Value;
    UINTN              ValueSize;

    if (This == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    ValueSize = sizeof(Value);
    Status = DfciWPBTSettingGetDefault (This, &ValueSize, &Value);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return DfciWPBTSettingSet (This, ValueSize, &Value, &Flags);
}


//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//
DFCI_SETTING_PROVIDER mDfciWPBTSettingProviderTemplate = {
    DFCI_STD_SETTING_ID_V3_ENABLE_WPBT,
    DFCI_SETTING_TYPE_ENABLE,
    DFCI_SETTING_FLAGS_NO_PREBOOT_UI | DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED,
    DfciWPBTSettingSet,
    DfciWPBTSettingGet,
    DfciWPBTSettingGetDefault,
    DfciWPBTSettingSetDefault
};

/////---------------------Interface for Library  ---------------------//////

// NONE

/**
 * Library design is such that a dependency on gDfciSettingsProviderSupportProtocolGuid
 * is not desired.  So to resolve that a ProtocolNotify is used.
 *
 * This function gets triggered once on install and 2nd time when the Protocol gets installed.
 *
 * When the gDfciSettingsProviderSupportProtocolGuid protocol is available the function will
 * loop through all the Dfci settings (using PCD) and install the settings
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
DfciWPBTSettingProviderSupportProtocolNotify (
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

    Status = sp->RegisterProvider (sp, &mDfciWPBTSettingProviderTemplate);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Failed to Register %a.  Status = %r\n", mDfciWPBTSettingProviderTemplate.Id, Status));
    }

    //We got here, this means all protocols were installed and we didn't exit early.
    //close the event as we don't need to be signaled again. (shouldn't happen anyway)
    gBS->CloseEvent(Event);
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is needed for DfciSettingsManager support, and to publish the WPBT enabled
 * protocol if WBPT is enabled.
 *
 * The design is to have the PCD false for all modules except the 1 anonymously linked to the DfcSettingsManager.
 *
 * @param  ImageHandle   The firmware allocated handle for the EFI image.
 * @param  SystemTable   A pointer to the EFI System Table.
 *
 * @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
 *
 **/
EFI_STATUS
EFIAPI
DfciWPBTSettingConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;
    UINT8      Value;
    UINTN      ValueSize;

    if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {
        //Install callback on the SettingsManager gMsSystemSettingsProviderSupportProtocolGuid protocol
        mDfciWPBTSettingProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent (
            &gDfciSettingsProviderSupportProtocolGuid,
             TPL_CALLBACK,
             DfciWPBTSettingProviderSupportProtocolNotify,
             NULL,
            &mDfciWPBTSettingProviderSupportInstallEventRegistration
            );

        DEBUG((DEBUG_INFO, "%a: Event Registered.\n", __FUNCTION__));

        //Initialize nonvolatile variables
        Status = InitializeNvVariables ();
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Initialize Nv Var failed. %r.\n", __FUNCTION__, Status));
        }

        ValueSize = sizeof(Value);
        Status = DfciWPBTSettingGet (&mDfciWPBTSettingProviderTemplate, &ValueSize, &Value);

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: unable to get WPBT Enabled setting. %r.\n", __FUNCTION__, Status));
        } else {
            if (Value == 0x01) {
                Status = gBS->InstallProtocolInterface (&ImageHandle,
                                        &gDfciWBPTEnabledProtocolGuid,
                                         EFI_NATIVE_INTERFACE,
                                         NULL);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "%a: unable to install WBPT Enabled protocol. %r.\n", __FUNCTION__, Status));
                }
            }
        }
    }

    return EFI_SUCCESS;
}

