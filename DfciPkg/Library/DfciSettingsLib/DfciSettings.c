/** @file
DfciSettings.c

Library Instance for DXE to support getting, setting, defaults, and support Dfci settings.

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

EFI_EVENT  mDfciSettingsProviderSupportInstallEvent;
VOID      *mDfciSettingsProviderSupportInstallEventRegistration = NULL;

typedef enum {
    ID_IS_BAD,
    ID_IS_RECOVERY_URL,
    ID_IS_BOOTSTRAP_URL,
    ID_IS_CERT,
    ID_IS_REGISTRATION_ID,
    ID_IS_TENANT_ID,
    ID_IS_FRIENDLY_NAME,
    ID_IS_TENANT_NAME,
}  ID_IS;

typedef struct  {
    DFCI_SETTING_ID_STRING                 Id;                 //Setting Id String
    DFCI_SETTING_TYPE                      Type;               //Enum setting type
    DFCI_SETTING_FLAGS                     Flags;              //Flag for this setting.
} PROVIDER_ENTRY;

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
EFI_STATUS
EFIAPI
DfciSettingsGetDefault (
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
EFI_STATUS
EFIAPI
DfciSettingsGet (
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

    if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__DFCI_RECOVERY_URL, DFCI_MAX_ID_LEN)) {
        return ID_IS_RECOVERY_URL;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__DFCI_BOOTSTRAP_URL, DFCI_MAX_ID_LEN)) {
        return ID_IS_BOOTSTRAP_URL;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__DFCI_HTTPS_CERT, DFCI_MAX_ID_LEN)) {
        return ID_IS_CERT;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__DFCI_REGISTRATION_ID, DFCI_MAX_ID_LEN)) {
        return ID_IS_REGISTRATION_ID;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__DFCI_TENANT_ID, DFCI_MAX_ID_LEN)) {
        return ID_IS_TENANT_ID;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__MDM_FRIENDLY_NAME, DFCI_MAX_ID_LEN)) {
        return ID_IS_FRIENDLY_NAME;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__MDM_TENANT_NAME, DFCI_MAX_ID_LEN)) {
        return ID_IS_TENANT_NAME;
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

    Status  = ValidateNvVariable (DFCI_SETTINGS_RECOVERY_URL_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_BOOTSTRAP_URL_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_HTTPS_CERT_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_REGISTRATION_ID_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_TENANT_ID_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_FRIENDLY_NAME);
    Status |= ValidateNvVariable (DFCI_SETTINGS_TENANT_NAME);

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
EFI_STATUS
EFIAPI
DfciSettingsSet (
    IN  CONST DFCI_SETTING_PROVIDER    *This,
    IN        UINTN                     ValueSize,
    IN  CONST VOID                     *Value,
    OUT       DFCI_SETTING_FLAGS       *Flags
  )
{
    VOID           *Buffer = NULL;
    UINTN           BufferSize;
    EFI_STATUS      Status;
    CHAR16         *VariableName;
    ID_IS           Id;

    if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (Flags == NULL) || (ValueSize > DFCI_SETTING_MAXIMUM_SIZE)) {
        DEBUG((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
        return EFI_INVALID_PARAMETER;
    }

    Id = IsIdSupported(This->Id);
    switch (Id) {
        case ID_IS_RECOVERY_URL:
            VariableName = DFCI_SETTINGS_RECOVERY_URL_NAME;
            break;

        case ID_IS_BOOTSTRAP_URL:
            VariableName = DFCI_SETTINGS_BOOTSTRAP_URL_NAME;
            break;

        case ID_IS_CERT:
             VariableName = DFCI_SETTINGS_HTTPS_CERT_NAME;
             break;

        case ID_IS_REGISTRATION_ID:
            VariableName = DFCI_SETTINGS_REGISTRATION_ID_NAME;
            break;

        case ID_IS_TENANT_ID:
            VariableName = DFCI_SETTINGS_TENANT_ID_NAME;
            break;

        case ID_IS_FRIENDLY_NAME:
            VariableName = DFCI_SETTINGS_FRIENDLY_NAME;
            break;

        case ID_IS_TENANT_NAME:
            VariableName = DFCI_SETTINGS_TENANT_NAME;
            break;

        default:
            DEBUG((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
            return EFI_UNSUPPORTED;
    }

    BufferSize = 0;
    Status = DfciSettingsGet (This, &BufferSize, NULL);

    if (Status != EFI_NOT_FOUND) {
        if (EFI_ERROR(Status) && (EFI_BUFFER_TOO_SMALL != Status)) {
            DEBUG((DEBUG_ERROR, "%a: Error getting %s. Code=%r\n", __FUNCTION__, VariableName, Status));
            return Status;
        }

        if ((BufferSize == 0) && (ValueSize == 0)) {
            *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
            DEBUG((DEBUG_INFO, "Setting %s ignored, sizes are 0\n", VariableName));
            return EFI_SUCCESS;
        }

        if ((ValueSize != 0) && (BufferSize == ValueSize)) {
            Buffer = AllocatePool (BufferSize);
            if (NULL == Buffer) {
                DEBUG((DEBUG_ERROR, "%a: Cannot allocate %d bytes.%r\n", __FUNCTION__, BufferSize));
                return EFI_OUT_OF_RESOURCES;
            }

            Status = gRT->GetVariable (VariableName,
                                      &gDfciSettingsGuid,
                                       NULL,
                                      &BufferSize,
                                       Buffer );
            if (EFI_ERROR(Status)) {
                FreePool (Buffer);
                DEBUG((DEBUG_ERROR, "%a: Error getting variable %s. Code=%r\n", __FUNCTION__, VariableName, Status));
                return Status;
            }

            if (0 == CompareMem(Buffer, Value, BufferSize)) {
                FreePool (Buffer);
                *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
                DEBUG((DEBUG_INFO, "Setting %s ignored, value didn't change\n", VariableName));
                return EFI_SUCCESS;
            }
            FreePool (Buffer);
        }
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
EFI_STATUS
EFIAPI
DfciSettingsGet (
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
        case ID_IS_RECOVERY_URL:
            VariableName = DFCI_SETTINGS_RECOVERY_URL_NAME;
            break;

        case ID_IS_BOOTSTRAP_URL:
            VariableName = DFCI_SETTINGS_BOOTSTRAP_URL_NAME;
            break;

        case ID_IS_CERT:
            VariableName = DFCI_SETTINGS_HTTPS_CERT_NAME;
            break;

        case ID_IS_REGISTRATION_ID:
            VariableName = DFCI_SETTINGS_REGISTRATION_ID_NAME;
            break;

        case ID_IS_TENANT_ID:
            VariableName = DFCI_SETTINGS_TENANT_ID_NAME;
            break;

        case ID_IS_FRIENDLY_NAME:
            VariableName = DFCI_SETTINGS_FRIENDLY_NAME;
            break
            ;
        case ID_IS_TENANT_NAME:
            VariableName = DFCI_SETTINGS_TENANT_NAME;
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
        Status = DfciSettingsGetDefault (This, ValueSize, Value);
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
EFI_STATUS
EFIAPI
DfciSettingsGetDefault (
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

    if (This->Type == DFCI_SETTING_TYPE_CERT) {
        *ValueSize = 0;  // Indicate no default
    } else if (This->Type == DFCI_SETTING_TYPE_ENABLE) {
        if (*ValueSize < sizeof(UINT8)) {
            *ValueSize = sizeof(UINT8);
            return EFI_BUFFER_TOO_SMALL;
        }
        *ValueSize = sizeof(UINT8);
        *((UINT8 *)Value) = 1;  // Indicates Enabled default
    } else {
        if (*ValueSize < sizeof(CHAR8)) {
            *ValueSize = sizeof(CHAR8);
            return EFI_BUFFER_TOO_SMALL;
        }
        *ValueSize = sizeof(CHAR8);
        *((CHAR8 *)Value) = '\0';
    }
    // DFCI Strings default to "", and CERTs default to not present.

    return EFI_SUCCESS;
}

/**
 * Settings Provider Set Default routine
 *
 * @param This
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
DfciSettingsSetDefault (
    IN  CONST DFCI_SETTING_PROVIDER     *This
  )
{
    DFCI_SETTING_FLAGS Flags = 0;
    EFI_STATUS         Status;
    CHAR8              Value;
    UINTN              ValueSize;

    if (This == NULL) {
        return EFI_INVALID_PARAMETER;
    }

    ValueSize = sizeof(ValueSize);
    Status = DfciSettingsGetDefault (This, &ValueSize, &Value);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    return DfciSettingsSet (This, ValueSize, &Value, &Flags);
}

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//                    mDfciSettingsProviders
STATIC PROVIDER_ENTRY mDfciSettingsProviders[] = {
    {
        DFCI_SETTING_ID__DFCI_RECOVERY_URL,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__DFCI_BOOTSTRAP_URL,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__DFCI_HTTPS_CERT,
        DFCI_SETTING_TYPE_CERT,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__DFCI_REGISTRATION_ID,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__DFCI_TENANT_ID,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__MDM_FRIENDLY_NAME,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    },
    {
        DFCI_SETTING_ID__MDM_TENANT_NAME,
        DFCI_SETTING_TYPE_STRING,
        DFCI_SETTING_FLAGS_NO_PREBOOT_UI
    }

};

#define PROVIDER_COUNT (sizeof(mDfciSettingsProviders)/sizeof(PROVIDER_ENTRY))

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//
DFCI_SETTING_PROVIDER mDfciSettingsProviderTemplate = {
    0,
    0,
    0,
    DfciSettingsSet,
    DfciSettingsGet,
    DfciSettingsGetDefault,
    DfciSettingsSetDefault
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
  )
{
    STATIC UINT8                            CallCount = 0;
    UINTN                                   i;
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

    for (i = 0; i < PROVIDER_COUNT; i++) {
        mDfciSettingsProviderTemplate.Id = mDfciSettingsProviders[i].Id;
        mDfciSettingsProviderTemplate.Type = mDfciSettingsProviders[i].Type;
        mDfciSettingsProviderTemplate.Flags = mDfciSettingsProviders[i].Flags;
        Status = sp->RegisterProvider (sp, &mDfciSettingsProviderTemplate);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Failed to Register %a.  Status = %r\n", mDfciSettingsProviderTemplate.Id, Status));
        }
    }

    //We got here, this means all protocols were installed and we didn't exit early.
    //close the event as we don't need to be signaled again. (shouldn't happen anyway)
    gBS->CloseEvent(Event);
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is only needed for DfciSettingsManager support.
 * The design is to have the PCD false for all modules except the 1 anonymously liked to the DfciettingsManager.
 *
 * @param  ImageHandle   The firmware allocated handle for the EFI image.
 * @param  SystemTable   A pointer to the EFI System Table.
 *
 * @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
 *
 **/
EFI_STATUS
EFIAPI
DfciSettingsConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status;

    if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {
        //Install callback on the SettingsManager gMsSystemSettingsProviderSupportProtocolGuid protocol
        mDfciSettingsProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent (
            &gDfciSettingsProviderSupportProtocolGuid,
             TPL_CALLBACK,
             DfciSettingsProviderSupportProtocolNotify,
             NULL,
            &mDfciSettingsProviderSupportInstallEventRegistration
            );

        DEBUG((DEBUG_INFO, "%a: Event Registered.\n", __FUNCTION__));

        //Init nv var
        Status = InitializeNvVariables ();
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Initialize Nv Var failed. %r.\n", __FUNCTION__, Status));
        }
    }
    return EFI_SUCCESS;
}

