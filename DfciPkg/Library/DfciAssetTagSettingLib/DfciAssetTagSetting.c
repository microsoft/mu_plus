/** @file
DfciAssetTagSetting.c

Library Instance for DXE to support getting, setting, and defaults for the Dfci3.AssetTag.String setting.

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

STATIC EFI_EVENT  mDfciAssetTagSettingProviderSupportInstallEvent;
STATIC VOID       *mDfciAssetTagSettingProviderSupportInstallEventRegistration = NULL;

typedef enum {
  ID_IS_BAD = 0,
  ID_IS_ASSET_TAG_STRING,
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
DfciAssetTagSettingGetDefault (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN  OUT   UINTN                  *ValueSize,
  OUT       VOID                   *Value
  );

/**
 * Settings Provider Get routine
 *
 * @param This
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS
 *         EFI_SUCCESS           - Returns value
 *         EFI_INVALID_PARAMETER - Bad parameters
 *         EFI_BUFFER_TOO_SMALL  - Size of new Value is larger than ValueSize
 *         EFI_UNSUPPORTED       - "This" points to an Id that is not supported
 */
STATIC
EFI_STATUS
EFIAPI
DfciAssetTagSettingGet (
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
IsIdSupported (
  DFCI_SETTING_ID_STRING  Id
  )
{
  if (0 == AsciiStrnCmp (Id, DFCI_STD_SETTING_ID_V3_ASSET_TAG, DFCI_MAX_ID_LEN)) {
    return ID_IS_ASSET_TAG_STRING;
  } else {
    DEBUG ((DEBUG_ERROR, "%a: Called with Invalid ID (%a)\n", __FUNCTION__, Id));
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
  CHAR16  *VariableName
  )
{
  EFI_STATUS  Status;
  UINT32      Attributes = 0;
  UINTN       ValueSize  = 0;
  VOID        *Value     = NULL;

  Status = GetVariable3 (
             VariableName,
             &gDfciSettingsGuid,
             (VOID *)&Value,
             &ValueSize,
             &Attributes
             );

  if (!EFI_ERROR (Status)) {
    // We have a variable
    FreePool (Value);
    if (DFCI_SETTINGS_ATTRIBUTES != Attributes) {
      // Check if Attributes are wrong
      // Delete invalid URL variable
      Status = gRT->SetVariable (
                      VariableName,
                      &gDfciSettingsGuid,
                      0,
                      0,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        // What???
        DEBUG ((DEBUG_ERROR, "%a: Unable to delete invalid variable %s\n", __FUNCTION__, VariableName));
      } else {
        DEBUG ((DEBUG_INFO, "%a: Deleting invalid variable %s, with attributes %x\n", __FUNCTION__, VariableName, Attributes));
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

  Status = ValidateNvVariable (DFCI_SETTINGS_ASSET_TAG_NAME);

  return Status;
}

/**
 * Validate the new settings value
 *
 * @param   Value
 * @param   ValueSize
 *
 * @return  EFI_SUCCESS            - Value has been validated
 * @return  EFI_INVALID_PARAMETER  - Value is not valid
 */
EFI_STATUS
ValidateAssetTagValue (
  IN CONST CHAR8  *Value,
  IN       UINTN  ValueSize
  )
{
  UINTN        i;
  VOID         *Ptr;
  CONST CHAR8  *ValidChars;
  UINTN        ValidCharsLen;
  UINTN        ValidSize;

  ValidChars    = (CONST CHAR8 *)FixedPcdGetPtr (PcdDfciAssetTagChars);
  ValidCharsLen = AsciiStrLen (ValidChars);
  ValidSize     = (UINTN)FixedPcdGet16 (PcdDfciAssetTagLen) + sizeof (CHAR8);

  if (ValueSize > ValidSize) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr = ScanMem8 (Value, ValueSize, 0x00);
  if (Ptr == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Not a NULL terminated string.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //  Ptr should point to the first NULL in Value.  It must be the last character
  //  of value.
  if (((UINT8 *)Ptr - (UINT8 *)Value) != (INTN)(ValueSize - 1)) {
    DEBUG ((DEBUG_ERROR, "%a: NULL not last character in string.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  for (i = 0; i < (ValueSize - 1); i++) {
    Ptr = ScanMem8 (ValidChars, ValidCharsLen, ((UINT8 *)Value)[i]);
    if (Ptr == NULL) {
      DEBUG ((DEBUG_ERROR, "ValidCharsLen=%d, BadIndex=%d\n", ValidCharsLen, i));
      DEBUG ((DEBUG_ERROR, "ValidChars=%a\n", ValidChars));
      DEBUG ((DEBUG_ERROR, "%a: Invalid ASSET_TAG %a\n", __FUNCTION__, Value));
      return EFI_INVALID_PARAMETER;
    }
  }

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
DfciAssetTagSettingSet (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN        UINTN                  ValueSize,
  IN  CONST VOID                   *Value,
  OUT       DFCI_SETTING_FLAGS     *Flags
  )
{
  VOID        *Buffer = NULL;
  UINTN       BufferSize;
  ID_IS       Id;
  EFI_STATUS  Status;
  CHAR16      *VariableName;

  if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (Flags == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateAssetTagValue ((CONST CHAR8 *)Value, ValueSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Id = IsIdSupported (This->Id);
  switch (Id) {
    case ID_IS_ASSET_TAG_STRING:
      VariableName = DFCI_SETTINGS_ASSET_TAG_NAME;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid id(%s).\n", __FUNCTION__, This->Id));
      return EFI_UNSUPPORTED;
  }

  BufferSize = 0;
  Status     = DfciAssetTagSettingGet (This, &BufferSize, NULL);

  if (Status != EFI_NOT_FOUND) {
    if (EFI_ERROR (Status) && (EFI_BUFFER_TOO_SMALL != Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Error getting %s. Code=%r\n", __FUNCTION__, VariableName, Status));
      return Status;
    }

    if ((BufferSize == 0) && (ValueSize == 0)) {
      *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
      DEBUG ((DEBUG_INFO, "Setting %s ignored, sizes are 0\n", VariableName));
      return EFI_SUCCESS;
    }

    if ((ValueSize != 0) && (BufferSize == ValueSize)) {
      Buffer = AllocatePool (BufferSize);
      if (NULL == Buffer) {
        DEBUG ((DEBUG_ERROR, "%a: Cannot allocate %d bytes.\n", __FUNCTION__, BufferSize));
        return EFI_OUT_OF_RESOURCES;
      }

      Status = gRT->GetVariable (
                      VariableName,
                      &gDfciSettingsGuid,
                      NULL,
                      &BufferSize,
                      Buffer
                      );
      if (EFI_ERROR (Status)) {
        FreePool (Buffer);
        DEBUG ((DEBUG_ERROR, "%a: Error getting variable %s. Code=%r\n", __FUNCTION__, VariableName, Status));
        return Status;
      }

      if (0 == CompareMem (Buffer, Value, BufferSize)) {
        FreePool (Buffer);
        *Flags |= DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
        DEBUG ((DEBUG_INFO, "Setting %s ignored, value didn't change\n", VariableName));
        return EFI_SUCCESS;
      }

      FreePool (Buffer);
    }
  }

  Status = gRT->SetVariable (
                  VariableName,
                  &gDfciSettingsGuid,
                  DFCI_SETTINGS_ATTRIBUTES,
                  ValueSize,
                  (VOID *)Value
                  );                             // SetVariable should not touch *Value
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error setting variable %s.  Code = %r\n", VariableName, Status));
  } else {
    DEBUG ((DEBUG_INFO, "Variable %s set Attributes=%x, Size=%d.\n", VariableName, DFCI_SETTINGS_ATTRIBUTES, ValueSize));
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
 * @return EFI_STATUS
 *         EFI_SUCCESS           - Returns value
 *         EFI_INVALID_PARAMETER - Bad parameters
 *         EFI_BUFFER_TOO_SMALL  - Size of new Value is larger than ValueSize
 *         EFI_UNSUPPORTED       - "This" points to an Id that is not supported
 */
STATIC
EFI_STATUS
EFIAPI
DfciAssetTagSettingGet (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN  OUT   UINTN                  *ValueSize,
  OUT       VOID                   *Value
  )
{
  ID_IS       Id;
  EFI_STATUS  Status;
  CHAR16      *VariableName;

  if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Id = IsIdSupported (This->Id);
  switch (Id) {
    case ID_IS_ASSET_TAG_STRING:
      VariableName = DFCI_SETTINGS_ASSET_TAG_NAME;
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a: Invalid id(%a).\n", __FUNCTION__, This->Id));
      return EFI_UNSUPPORTED;
      break;
  }

  Status = gRT->GetVariable (
                  VariableName,
                  &gDfciSettingsGuid,
                  NULL,
                  ValueSize,
                  Value
                  );
  if (EFI_NOT_FOUND == Status) {
    DEBUG ((DEBUG_INFO, "%a - Variable %s not found. Getting default value.\n", __FUNCTION__, VariableName));
    Status = DfciAssetTagSettingGetDefault (This, ValueSize, Value);
  }

  if (EFI_ERROR (Status)) {
    if (EFI_BUFFER_TOO_SMALL != Status) {
      DEBUG ((DEBUG_ERROR, "%a - Error retrieving setting %s. Code=%r\n", __FUNCTION__, VariableName, Status));
    }
  } else {
    DEBUG ((DEBUG_INFO, "%a - Setting %s retrieved.\n", __FUNCTION__, VariableName));
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
DfciAssetTagSettingGetDefault (
  IN  CONST DFCI_SETTING_PROVIDER  *This,
  IN  OUT   UINTN                  *ValueSize,
  OUT       VOID                   *Value
  )
{
  ID_IS  Id;

  if ((This == NULL) || (This->Id == NULL) || (ValueSize == NULL) || ((Value == NULL) && (*ValueSize != 0))) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter.\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  Id = IsIdSupported (This->Id);
  if (Id == ID_IS_BAD) {
    return EFI_UNSUPPORTED;
  }

  if (*ValueSize < sizeof (CHAR8)) {
    *ValueSize = sizeof (CHAR8);
    return EFI_BUFFER_TOO_SMALL;
  }

  *ValueSize        = sizeof (CHAR8);
  *((CHAR8 *)Value) = '\0';    // Indicates NULL string default

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
DfciAssetTagSettingSetDefault (
  IN  CONST DFCI_SETTING_PROVIDER  *This
  )
{
  DFCI_SETTING_FLAGS  Flags = 0;
  EFI_STATUS          Status;
  UINT8               Value;
  UINTN               ValueSize;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ValueSize = sizeof (Value);
  Status    = DfciAssetTagSettingGetDefault (This, &ValueSize, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DfciAssetTagSettingSet (This, ValueSize, &Value, &Flags);
}

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id, type, and flags field as needed for registration.
//
DFCI_SETTING_PROVIDER  mDfciAssetTagSettingProviderTemplate = {
  DFCI_STD_SETTING_ID_V3_ASSET_TAG,
  DFCI_SETTING_TYPE_STRING,
  0,
  DfciAssetTagSettingSet,
  DfciAssetTagSettingGet,
  DfciAssetTagSettingGetDefault,
  DfciAssetTagSettingSetDefault
};

/////---------------------Interface for Library  ---------------------//////

/**
 * Settings Provider AssetTagGet routine for pre SettingsManager access.
 *
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS
 *         EFI_SUCCESS           - Returns value
 *         EFI_INVALID_PARAMETER - Bad parameters
 *         EFI_BUFFER_TOO_SMALL  - Size of new Value is larger than ValueSize
 */
EFI_STATUS
EFIAPI
DfciGetAssetTag (
  IN  OUT   UINTN  *ValueSize,
  OUT       VOID   *Value
  )
{
  EFI_STATUS  Status;

  Status = DfciAssetTagSettingGet (
             &mDfciAssetTagSettingProviderTemplate,
             ValueSize,
             Value
             );
  return Status;
}

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
DfciAssetTagSettingProviderSupportProtocolNotify (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  STATIC UINT8                            CallCount = 0;
  DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL  *sp;
  EFI_STATUS                              Status;

  // locate protocol
  Status = gBS->LocateProtocol (&gDfciSettingsProviderSupportProtocolGuid, NULL, (VOID **)&sp);
  if (EFI_ERROR (Status)) {
    if ((CallCount++ != 0) || (Status != EFI_NOT_FOUND)) {
      DEBUG ((DEBUG_ERROR, "%a() - Failed to locate gDfciSettingsProviderSupportProtocolGuid in notify.  Status = %r\n", __FUNCTION__, Status));
    }

    return;
  }

  Status = sp->RegisterProvider (sp, &mDfciAssetTagSettingProviderTemplate);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to Register %a.  Status = %r\n", mDfciAssetTagSettingProviderTemplate.Id, Status));
  }

  // We got here, this means all protocols were installed and we didn't exit early.
  // close the event as we don't need to be signaled again. (shouldn't happen anyway)
  gBS->CloseEvent (Event);
}

/**
 * The constructor function initializes the Lib for Dxe.
 *
 * This constructor is needed for DfciSettingsManager support of the Asset Tag.
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
DfciAssetTagSettingConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (FeaturePcdGet (PcdSettingsManagerInstallProvider)) {
    // Install callback on the SettingsManager gMsSystemSettingsProviderSupportProtocolGuid protocol
    mDfciAssetTagSettingProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent (
                                                        &gDfciSettingsProviderSupportProtocolGuid,
                                                        TPL_CALLBACK,
                                                        DfciAssetTagSettingProviderSupportProtocolNotify,
                                                        NULL,
                                                        &mDfciAssetTagSettingProviderSupportInstallEventRegistration
                                                        );

    DEBUG ((DEBUG_INFO, "%a: Event Registered.\n", __FUNCTION__));
  }

  // Initialize nonvolatile variables
  Status = InitializeNvVariables ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Initialize Nv Var failed. %r.\n", __FUNCTION__, Status));
  }

  return EFI_SUCCESS;
}
