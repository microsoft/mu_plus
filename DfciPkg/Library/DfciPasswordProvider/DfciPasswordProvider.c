/** @file DfciPasswordProvider.c

Library Instance for DXE to support getting, setting, defaults, and support
SystemSettings for the Admin password.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <DfciSystemSettingTypes.h>

#include <Protocol/DfciSettingsProvider.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PasswordStoreLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Settings/DfciSettings.h>
#include <Settings/DfciOemSample.h>

EFI_EVENT      mPasswordProviderSupportInstallEvent = NULL;;
VOID          *mPasswordProviderSupportInstallEventRegistration = NULL;

EFI_STATUS
EFIAPI
PasswordGetDefault(
IN  CONST DFCI_SETTING_PROVIDER     *This,
IN  OUT   UINTN                     *ValueSize,
OUT       UINT8                     *Value
)
{
    if ((This == NULL) || (This->Id == NULL) || (Value == NULL) || (ValueSize == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    if (*ValueSize < sizeof(UINT8))
    {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    if (0 != AsciiStrnCmp (This->Id, DFCI_OEM_SETTING_ID__PASSWORD, DFCI_MAX_ID_LEN))
    {
        DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (%a)\n", This->Id));
        return EFI_UNSUPPORTED;
    }

    *ValueSize = sizeof(UINT8);
    *Value = FALSE; // There is no system password set by default
    return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
PasswordGet(
IN CONST DFCI_SETTING_PROVIDER     *This,
IN OUT   UINTN                     *ValueSize,
OUT      UINT8                     *Value
)
{
    if ((This == NULL) || (Value == NULL) || (ValueSize == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    if (*ValueSize < sizeof(UINT8))
    {
        *ValueSize = sizeof(UINT8);
        return EFI_BUFFER_TOO_SMALL;
    }

    if (0 != AsciiStrnCmp (This->Id, DFCI_OEM_SETTING_ID__PASSWORD, DFCI_MAX_ID_LEN))
    {
        DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (0x%X)\n", This->Id));
        return EFI_UNSUPPORTED;
    }

    *ValueSize = sizeof(UINT8);
    *Value = PasswordStoreIsPasswordSet(); // get the current password state.

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PasswordSet (
IN  CONST DFCI_SETTING_PROVIDER     *This,
IN        UINTN                      ValueSize,
IN  CONST UINT8                     *Value,
OUT DFCI_SETTING_FLAGS              *Flags
)
{
    EFI_STATUS  Status = EFI_SUCCESS;

    if ((This == NULL) || (Flags == NULL) || (Value == NULL))
    {
        return EFI_INVALID_PARAMETER;
    }

    *Flags = 0;

    if (0 != AsciiStrnCmp (This->Id, DFCI_OEM_SETTING_ID__PASSWORD, DFCI_MAX_ID_LEN))
    {
        DEBUG((DEBUG_ERROR, "PasswordSet was called with incorrect Provider Id (%a)\n", This->Id));
        return EFI_UNSUPPORTED;
    }

    // Store the password
    // hash value which comes from xml or the CreatePasswordHash from password store.
    Status = PasswordStoreSetPassword(Value, ValueSize);

    if (!EFI_ERROR(Status)){
        *Flags |= DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED;
    }

    return Status;
}

EFI_STATUS
EFIAPI
PasswordSetDefault (
  IN  CONST DFCI_SETTING_PROVIDER     *This
  )
{
  if (This == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  if (0 != AsciiStrnCmp (This->Id, DFCI_OEM_SETTING_ID__PASSWORD, DFCI_MAX_ID_LEN))
  {
    DEBUG((DEBUG_ERROR, "PasswordProvider was called with incorrect Provider Id (%a)\n", This->Id));
    return EFI_UNSUPPORTED;
  }

  // The correct way to "delete" a password is to send the proper deleted password hash through
  // the Settings Access protocol Set function using a proper Auth Token.

  return EFI_ACCESS_DENIED;
}

DFCI_SETTING_PROVIDER mPasswordProvider = {
    DFCI_OEM_SETTING_ID__PASSWORD,
    DFCI_SETTING_TYPE_PASSWORD,
    DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED,
    (DFCI_SETTING_PROVIDER_SET) PasswordSet,
    (DFCI_SETTING_PROVIDER_GET) PasswordGet,
    (DFCI_SETTING_PROVIDER_GET_DEFAULT) PasswordGetDefault,
    (DFCI_SETTING_PROVIDER_SET_DEFAULT) PasswordSetDefault
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
PasswordProviderSupportProtocolNotify(
IN  EFI_EVENT       Event,
IN  VOID            *Context
)
{

    EFI_STATUS Status;
    DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL *sp;
    STATIC UINT8 CallCount = 0;

    //locate protocol
    Status = gBS->LocateProtocol(&gDfciSettingsProviderSupportProtocolGuid, NULL, (VOID**)&sp);
    if (EFI_ERROR(Status))
    {
      if ((CallCount++ != 0) || (Status != EFI_NOT_FOUND))
      {
        DEBUG((DEBUG_ERROR, "%a() - Failed to locate gDfciSettingsProviderSupportProtocolGuid in notify.  Status = %r\n", __FUNCTION__, Status));
      }
      return;
    }

    //call function
    DEBUG((DEBUG_INFO, "Registering Password Setting Provider\n"));
    Status = sp->RegisterProvider(sp, &mPasswordProvider);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register.  Status = %r\n", Status));
    }

    //We got here, this means all protocols were installed and we didn't exit early.
    //close the event as we dont' need to be signaled again. (shouldn't happen anyway)
    gBS->CloseEvent(Event);
}

/**
The constructor function initializes the Lib for Dxe.

This constructor is only needed for MsSettingsManager support.
The design is to have the PCD false fall all modules except the 1 that should support the MsSettingsManager.  Because this
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
DfciPasswordProviderConstructor
(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    if (FeaturePcdGet(PcdSettingsManagerInstallProvider))
    {
        //Install callback on the SettingsManager gDfciSettingsProviderSupportProtocolGuid protocol
        mPasswordProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent(
            &gDfciSettingsProviderSupportProtocolGuid,
            TPL_CALLBACK,
            PasswordProviderSupportProtocolNotify,
            NULL,
            &mPasswordProviderSupportInstallEventRegistration);

        DEBUG((DEBUG_INFO, "%a - Event Registered.\n", __FUNCTION__));
    }

    return EFI_SUCCESS;
}


/**
* Destructor for PasswordProvider to remove event notifications should other
* libraries or driver entry fail.
*
* @param  ImageHandle   The firmware allocated handle for the EFI image.
* @param  SystemTable   A pointer to the EFI System Table.
*
* @retval EFI_SUCCESS   The destructor always returns EFI_SUCCESS.
*
**/
EFI_STATUS
EFIAPI
DfciPasswordProviderDestructor(
    IN      EFI_HANDLE                ImageHandle,
    IN      EFI_SYSTEM_TABLE          *SystemTable
) {

    if (mPasswordProviderSupportInstallEvent != NULL) {
        gBS->CloseEvent (mPasswordProviderSupportInstallEvent);
    }

   return EFI_SUCCESS;
}