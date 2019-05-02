/**
Library Instance for DXE to support getting, setting, defaults, and support MsSystemSettings for Tool/Application/Ui interface.

The UEFI Core Boot Manger settings

Copyright (c) 2017 - 2018, Microsoft Corporation

All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/


#include <PiDxe.h>

#include <DfciSystemSettingTypes.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MsBootManagerSettingsLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <DfciSystemSettingTypes.h>
#include <MsBootManagerSettings.h>
#include <Protocol/DfciSettingsProvider.h>

#include <Settings/BootMenuSettings.h>

EFI_EVENT  mBootManagerSettingsProviderSupportInstallEvent;
VOID      *mBootManagerSettingsProviderSupportInstallEventRegistration = NULL;


/**
@param Id - Setting ID to check for support status
@retval TRUE - Supported
@retval FALSE - Not supported
**/
STATIC
BOOLEAN
IsIdSupported (DFCI_SETTING_ID_STRING Id)
{
    BOOLEAN Result = FALSE;


    if ((0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__IPV6, DFCI_MAX_ID_LEN)) ||
        (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ALT_BOOT, DFCI_MAX_ID_LEN)) ||
        (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__BOOT_ORDER_LOCK, DFCI_MAX_ID_LEN)) ||
        (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ENABLE_USB_BOOT, DFCI_MAX_ID_LEN)) ||
        (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__START_NETWORK, DFCI_MAX_ID_LEN))) {
        Result = TRUE;
    } else {
        Result = FALSE;
    }

    return Result;
}

/**
Internal function used to init the entire variable into flash.

Configure the Config variable so that on return it contains the same values

**/
STATIC
EFI_STATUS
InitializeNvVariable ()
{
    MS_BOOT_MANAGER_SETTINGS  Settings;
    EFI_STATUS                Status;
    UINT32                    Attributes = 0;
    UINTN                     BufferSize;

    //1. Read the variable from VarStore
    BufferSize = sizeof(Settings);
    Status = gRT->GetVariable(MS_BOOT_MANAGER_SETTINGS_NAME,
                             &gMsBootManagerSettingsGuid,
                             &Attributes,
                             &BufferSize,
                             &Settings );

    //2. Var Exists (check valid)
      //I   - Check Size
      //II  - Check attributes and confirm they are correct
      //III - Check Signature
    if (!EFI_ERROR(Status))
    {
        if (BufferSize != sizeof(Settings))
        {
            DEBUG((DEBUG_ERROR, "BootManager settings invalid size.\n"));
            Status = EFI_COMPROMISED_DATA;
        }
        else
        {

            if ((Settings.Signature == MS_BOOT_MANAGER_SETTINGS_SIGNATURE_OLD) &&
                (Attributes == (MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES | EFI_VARIABLE_RUNTIME_ACCESS)))
            {
                DEBUG((DEBUG_INFO, "BootManager Variable is being converted."));
                Settings.Signature = MS_BOOT_MANAGER_SETTINGS_SIGNATURE;
                Settings.BootOrderLock = FALSE; // Set default for first conversion.
                Settings.EnableUsbBoot = PcdGet8 (PcdEnableUsbBoot);
                Settings.StartNetwork  = PcdGet8 (PcdStartNetwork);
                Settings.Version = MS_BOOT_MANAGER_SETTINGS_VERSON3;
                //delete it first as it has RT set
                Status = gRT->SetVariable (MS_BOOT_MANAGER_SETTINGS_NAME,
                                          &gMsBootManagerSettingsGuid,
                                           0,
                                           0,
                                           NULL);
                if (EFI_ERROR(Status))
                {
                    DEBUG((DEBUG_INFO,"Error %r deleting old variable\n",Status));
                }

                Status = gRT->SetVariable (MS_BOOT_MANAGER_SETTINGS_NAME,
                                          &gMsBootManagerSettingsGuid,
                                           MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES,
                                           sizeof(Settings),
                                          &Settings);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "Unable to recreate BootManager settings Variable Code=%r\n",Status));
                    Status = EFI_SUCCESS;
                }
            }
            else
            {
                if (Attributes != MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES)
                {
                    DEBUG((DEBUG_ERROR, "BootManager settings Variable Attributes are invalid.\n"));
                    Status = EFI_COMPROMISED_DATA;
                }
                else if (Settings.Signature != MS_BOOT_MANAGER_SETTINGS_SIGNATURE)
                {
                    DEBUG((DEBUG_INFO, "BootManager Variable has corrupted signature."));
                    Status = EFI_COMPROMISED_DATA;
                }
                else if ((Settings.Version >= MS_BOOT_MANAGER_SETTINGS_VERSON1) &&
                         (Settings.Version < MS_BOOT_MANAGER_SETTINGS_VERSON3)) {
                    // Handle the case where systems have the new settings varirable, but don't have
                    // the correct value for USB Boot and, or StartNetworking
                    if (Settings.Version == MS_BOOT_MANAGER_SETTINGS_VERSON1) {
                        Settings.EnableUsbBoot  = PcdGet8(PcdEnableUsbBoot);
                    }
                    if (Settings.Version <= MS_BOOT_MANAGER_SETTINGS_VERSON2) {
                        Settings.StartNetwork = PcdGet8(PcdStartNetwork);
                    }
                    Settings.Version = MS_BOOT_MANAGER_SETTINGS_VERSON3;
                    Status = gRT->SetVariable (MS_BOOT_MANAGER_SETTINGS_NAME,
                                              &gMsBootManagerSettingsGuid,
                                               MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES,
                                               sizeof(Settings),
                                              &Settings);
                    if (EFI_ERROR(Status)) {
                        DEBUG((DEBUG_ERROR, "Unable to recreate BootManager settings Variable Code=%r\n",Status));
                        Status = EFI_SUCCESS;
                    }
                }
            }
        }
    }

    //3. Var Doesn't Exist or is not valid
      //I   - Load the defaults
      //II  - Set the attributes
      //III - Set it to var store
    if (EFI_ERROR(Status))
    {
        if (Status != EFI_NOT_FOUND)
        {
            //delete it first as it is corrupted or has RT set
            Status = gRT->SetVariable (MS_BOOT_MANAGER_SETTINGS_NAME,
                                      &gMsBootManagerSettingsGuid,
                                       0,
                                       0,
                                       NULL);
            if (EFI_ERROR(Status))
            {
                DEBUG((DEBUG_INFO,"Error %r deleting old variable\n",Status));
            }
        }

        ZeroMem (&Settings, sizeof(Settings));

        /* Set Default Values Here */
        Settings.Signature      = MS_BOOT_MANAGER_SETTINGS_SIGNATURE;
        Settings.IPv6           = PcdGet8 (PcdEnableIPv6Boot);
        Settings.AltBoot        = PcdGet8 (PcdEnableAltBoot);
        Settings.BootOrderLock  = PcdGet8 (PcdEnableBootOrderLock);
        Settings.EnableUsbBoot  = PcdGet8 (PcdEnableUsbBoot);
        Settings.StartNetwork   = PcdGet8 (PcdStartNetwork);

        Status = gRT->SetVariable (MS_BOOT_MANAGER_SETTINGS_NAME,
                                  &gMsBootManagerSettingsGuid,
                                   MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES,
                                   sizeof(Settings),
                                  &Settings);
    }

    //4. Configure the var lock protocol if needed

    //TODO


    return Status;
}

// Interface for Boot Manager

/**
 * GetBootManagerSettingDefault
 *
 * @param Id
 * @param Value
 *
 * @return EFI_STATUS
 */
EFI_STATUS
EFIAPI
GetBootManagerSettingDefault (
  IN DFCI_SETTING_ID_STRING    Id,
  OUT BOOLEAN *Value
)
{
    EFI_STATUS Status = EFI_SUCCESS;

    if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__IPV6, DFCI_MAX_ID_LEN)) {
        *Value =  PcdGet8 (PcdEnableIPv6Boot);
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ALT_BOOT, DFCI_MAX_ID_LEN)) {
        *Value  = PcdGet8 (PcdEnableAltBoot);
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__BOOT_ORDER_LOCK, DFCI_MAX_ID_LEN)) {
        *Value  = PcdGet8 (PcdEnableBootOrderLock);
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ENABLE_USB_BOOT, DFCI_MAX_ID_LEN)) {
        *Value  = PcdGet8 (PcdEnableUsbBoot);
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__START_NETWORK, DFCI_MAX_ID_LEN)) {
        *Value  = PcdGet8 (PcdStartNetwork);
    } else {
        DEBUG((DEBUG_ERROR, "%a - Called with Invalid ID (%a)\n", __FUNCTION__, Id));
        Status = EFI_UNSUPPORTED;
    }

    return Status;

}


/**
Function to Get a Boot Manager Setting.
If the setting has not be previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:      The MsSystemSettingsId for the setting
@param Value:   Ptr to a boolean value for the setting to be returned.  Enabled = True, Disabled = False


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetBootManagerSetting (
  IN  DFCI_SETTING_ID_STRING   Id,
  OUT BOOLEAN                 *Value
  )
{

    MS_BOOT_MANAGER_SETTINGS  Settings;
    EFI_STATUS                Status;
    UINT32                    Attributes;
    UINTN                     BufferSize;

    if (!IsIdSupported(Id))
    {
        DEBUG((DEBUG_ERROR, "%a - Called with Invalid ID (%a)\n", __FUNCTION__, Id));
        return EFI_UNSUPPORTED;
    }
    BufferSize = sizeof(Settings);
    Status = gRT->GetVariable(MS_BOOT_MANAGER_SETTINGS_NAME,
                             &gMsBootManagerSettingsGuid,
                             &Attributes,
                             &BufferSize,
                             &Settings );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_INFO, "%a - Error %r.  Returning Default.\n", __FUNCTION__, Status));
        return GetBootManagerSettingDefault(Id, Value);
    }

    if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__IPV6, DFCI_MAX_ID_LEN)) {
        *Value =  Settings.IPv6;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ALT_BOOT, DFCI_MAX_ID_LEN)) {
        *Value =  Settings.AltBoot;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__BOOT_ORDER_LOCK, DFCI_MAX_ID_LEN)) {
        *Value =  Settings.BootOrderLock;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ENABLE_USB_BOOT, DFCI_MAX_ID_LEN)) {
        *Value =  Settings.EnableUsbBoot;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__START_NETWORK, DFCI_MAX_ID_LEN)) {
        *Value =  Settings.StartNetwork;
    } else {
        // Cannot get here as checked above
    }

    return Status;
}

/**
Function to Set a Boot Manager Setting
@param Id:      The MsSystemSettingsId for the setting
@param Value:   The boolean value for the setting.  Enabled = True, Disabled = False
@param Flags:   The returning flags from setting the setting.  This can tell things like Reboot required.

@retval: Success - Setting was set.  Flags indicate any additional info
@retval: UNSUPPORTED or INVALID_PARAMETER.  Setting not set.  Flags not valid
@retval: Other EFI_ERROR.  Settings not set. Flags valid.
**/
EFI_STATUS
EFIAPI
SetBootManagerSetting (
  IN    DFCI_SETTING_ID_STRING  Id,
  IN    BOOLEAN                 Value,
  OUT   DFCI_SETTING_FLAGS     *Flags
  )
{

    MS_BOOT_MANAGER_SETTINGS  Settings;
    EFI_STATUS                Status;
    UINTN                     BufferSize;
    UINT32                    Attributes;
    BOOLEAN                   Changed = FALSE;

    if (Flags == NULL)
    {
        ASSERT(Flags != NULL);
        return EFI_INVALID_PARAMETER;
    }

    if (!IsIdSupported(Id))
    {
        DEBUG((DEBUG_ERROR, "%a - Called with Invalid ID (%a)\n", __FUNCTION__, Id));
        return EFI_UNSUPPORTED;
    }

    BufferSize = sizeof(Settings);
    Status = gRT->GetVariable(MS_BOOT_MANAGER_SETTINGS_NAME,
                             &gMsBootManagerSettingsGuid,
                             &Attributes,
                             &BufferSize,
                             &Settings );

    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_INFO, "%a - Error %r.  Can't set until initialized.\n", __FUNCTION__, Status));
        return Status;
    }

    if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__IPV6, DFCI_MAX_ID_LEN)) {
        Changed = Settings.IPv6 != Value;
        Settings.IPv6 = Value;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ALT_BOOT, DFCI_MAX_ID_LEN)) {
        Changed = Settings.AltBoot != Value;
        Settings.AltBoot = Value;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__BOOT_ORDER_LOCK, DFCI_MAX_ID_LEN)) {
        Changed = Settings.BootOrderLock != Value;
        Settings.BootOrderLock = Value;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__ENABLE_USB_BOOT, DFCI_MAX_ID_LEN)) {
        Changed = Settings.EnableUsbBoot != Value;
        Settings.EnableUsbBoot = Value;
    } else if (0 == AsciiStrnCmp (Id, DFCI_SETTING_ID__START_NETWORK, DFCI_MAX_ID_LEN)) {
        Changed = Settings.StartNetwork != Value;
        Settings.StartNetwork = Value;
    } else {
        // Cannot get here as checked above
    }

    if (Changed) {
        Status = gRT->SetVariable(MS_BOOT_MANAGER_SETTINGS_NAME,
                                 &gMsBootManagerSettingsGuid,
                                  MS_BOOT_MANAGER_SETTINGS_ATTRIBUTES,
                                  sizeof(Settings),
                                 &Settings);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "ERROR on SetVariable.  Code=%r\n", Status));
        }
    } else {
        *Flags = DFCI_SETTING_FLAGS_OUT_ALREADY_SET;
        Status = EFI_SUCCESS;
        DEBUG((DEBUG_INFO, "Setting %a ignored, value didn't change\n", Id));
    }
    return Status;
}



/////---------------------Interface for Settings Provider ---------------------//////

EFI_STATUS
EFIAPI
BootManagerSettingsSet (
  IN  CONST DFCI_SETTING_PROVIDER     *This,
  IN        UINTN                      ValueSize,
  IN  CONST VOID                      *Value,
  OUT DFCI_SETTING_FLAGS              *Flags
  )
{
  if ((This != NULL) && (Value != NULL) && (Flags != NULL) && (ValueSize == sizeof(BOOLEAN)))
  {
    return SetBootManagerSetting (This->Id, *((BOOLEAN*) Value), Flags);
  }
  return EFI_INVALID_PARAMETER;

}

EFI_STATUS
EFIAPI
BootManagerSettingsGet(
  IN CONST DFCI_SETTING_PROVIDER     *This,
  IN OUT   UINTN                     *ValueSize,
  OUT      VOID                      *Value
  )
{
  if ((This != NULL) && (Value != NULL) && (ValueSize != NULL) && (*ValueSize == sizeof(BOOLEAN)))
  {
    return GetBootManagerSetting(This->Id, Value);
  }
  return EFI_INVALID_PARAMETER;

}

//
// Gets the default value of setting
//
EFI_STATUS
EFIAPI
BootManagerSettingsGetDefault(
  IN CONST  DFCI_SETTING_PROVIDER     *This,
  IN OUT    UINTN                     *ValueSize,
  OUT       VOID                      *Value
  )
{
  if ((This != NULL) && (Value != NULL) && (ValueSize != NULL) && (*ValueSize == sizeof(BOOLEAN)))
  {
    return GetBootManagerSettingDefault(This->Id, Value);
  }
  return EFI_INVALID_PARAMETER;

}

//
// Set to the default value of setting
//
EFI_STATUS
EFIAPI
BootManagerSettingsSetDefault(
  IN  CONST DFCI_SETTING_PROVIDER     *This
  )
{
  BOOLEAN Value;
  EFI_STATUS Status;
  DFCI_SETTING_FLAGS Flags = 0;
  if (This != NULL)
  {
    Status = GetBootManagerSettingDefault(This->Id, &Value);
    if (!EFI_ERROR(Status))
    {
      return SetBootManagerSetting(This->Id, Value, &Flags);
    }
  }
  return EFI_INVALID_PARAMETER;

}

//
// Since ProviderSupport Registration copies the provider to its own
// allocated memory this code can use a single "template" and just change
// the id field as needed for registration.
//
DFCI_SETTING_PROVIDER mBootManagerProviderTemplate = {
  0,
  DFCI_SETTING_TYPE_ENABLE,
  DFCI_SETTING_FLAGS_NONE,
  BootManagerSettingsSet,
  BootManagerSettingsGet,
  BootManagerSettingsGetDefault,
  BootManagerSettingsSetDefault
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
BootManagerSettingsProviderSupportProtocolNotify(
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
        DEBUG((DEBUG_ERROR, "%a() - Failed to locate gDfciSettingsProviderSupportProtocolGuid in notify.  Status = %r\n", __FUNCTION__,  Status));
      }
      return;
    }

    //
    // Register items that are in the PREBOOT_UI
    //
    mBootManagerProviderTemplate.Id = DFCI_SETTING_ID__IPV6;
    Status = sp->RegisterProvider(sp, &mBootManagerProviderTemplate);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register IPV6.  Status = %r\n", Status));
    }
    mBootManagerProviderTemplate.Id = DFCI_SETTING_ID__ALT_BOOT;
    Status = sp->RegisterProvider(sp, &mBootManagerProviderTemplate);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register ALT_BOOT.  Status = %r\n", Status));
    }
    mBootManagerProviderTemplate.Id = DFCI_SETTING_ID__BOOT_ORDER_LOCK;
    Status = sp->RegisterProvider(sp, &mBootManagerProviderTemplate);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register BOOT_OREER_LOCK.  Status = %r\n", Status));
    }
    mBootManagerProviderTemplate.Id = DFCI_SETTING_ID__ENABLE_USB_BOOT;
    Status = sp->RegisterProvider(sp, &mBootManagerProviderTemplate);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register ENABLE_USB_BOOT.  Status = %r\n", Status));
    }

    //
    // Register items that are NOT in the PREBOOT_UI
    //
    mBootManagerProviderTemplate.Id = DFCI_SETTING_ID__START_NETWORK;
    mBootManagerProviderTemplate.Flags = DFCI_SETTING_FLAGS_NO_PREBOOT_UI;
    Status = sp->RegisterProvider(sp, &mBootManagerProviderTemplate);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_ERROR, "Failed to Register START_NETWORK.  Status = %r\n", Status));
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
MsBootManagerSettingsConstructor
(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
    EFI_STATUS Status = EFI_SUCCESS;
    if (FeaturePcdGet(PcdSettingsManagerInstallProvider))
    {
        //Install callback on the SettingsManager gDfciSettingsProviderSupportProtocolGuid protocol
        mBootManagerSettingsProviderSupportInstallEvent = EfiCreateProtocolNotifyEvent(
            &gDfciSettingsProviderSupportProtocolGuid,
             TPL_CALLBACK,
             BootManagerSettingsProviderSupportProtocolNotify,
             NULL,
            &mBootManagerSettingsProviderSupportInstallEventRegistration
            );

        DEBUG((DEBUG_INFO, "%a - Event Registered.\n", __FUNCTION__));

        //Init nv var
        Status = InitializeNvVariable();
        if (EFI_ERROR(Status))
        {
            DEBUG((DEBUG_ERROR, "%a - Initialize Nv Var failed. %r.\n", __FUNCTION__, Status));
        }
    }
    return EFI_SUCCESS;
}

