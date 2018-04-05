/** @file
  Defines the System Setting Provider Support protocol.

  This protocol allows modules to register as setting providers.

Copyright (c) 2015, Microsoft Corporation. All rights reserved.<BR>

**/
#ifndef __DFCI_SETTING_PROVIDER_H__
#define __DFCI_SETTING_PROVIDER_H__


/**
Define the DFCI_SETTING_PROVIDER related structures
**/
typedef struct _DFCI_SETTING_PROVIDER DFCI_SETTING_PROVIDER;

/*
Set a single setting

@param This      Provider Setting
@param Value     a pointer to a datatype defined by the Type for this setting.
@param ValueSize Size of the data for this setting.
@param Flags     Informational Flags passed to the SET and/or Returned as a result of the set

@retval EFI_SUCCESS if setting could be set.  Check flags for other info (reset required, etc)
@retval Error - Setting not set.

*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PROVIDER_SET) (
  IN  CONST DFCI_SETTING_PROVIDER     *This,
  IN        UINTN                      ValueSize,
  IN  CONST VOID                      *Value,
  OUT DFCI_SETTING_FLAGS              *Flags
  );

/*
Get a single setting

@param This      Seting Provider
@param ValueSize IN=Size of location to store value
                 OUT=Size of value stored
@param Value     Output parameter for the setting value.
                 The type and size is based on the provider type
                 and must be allocated by the caller.
@retval EFI_SUCCESS If setting could be retrieved.
@retval EFI_BUFFER_TOO_SMALL if the ValueSize on input is too small
@retval ERROR       Error
*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PROVIDER_GET) (
  IN  CONST DFCI_SETTING_PROVIDER     *This,
  IN  OUT   UINTN                     *ValueSize,
  OUT VOID                            *Value
  );

/*
Get the default value of a single setting

@param This          Seting Provider
@param ValueSize     IN=Size of location to store value
                     OUT=Size of value stored
@param DefaultValue  Output parameter for the settings default value.
                     The type and size is based on the provider type
                     and must be allocated by the caller.

@retval EFI_SUCCESS  if the default could be returned.
@retval EFI_BUFFER_TOO_SMALL if the ValueSize on input is too small
@retval ERROR        Error
*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PROVIDER_GET_DEFAULT) (
  IN  CONST DFCI_SETTING_PROVIDER     *This,
  IN  OUT   UINTN                     *ValueSize,
  OUT VOID                            *DefaultValue
  );

/*
Set to default value

@param This          Seting Provider

@retval EFI_SUCCESS  default set
@retval ERROR        Error
*/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_PROVIDER_SET_DEFAULT) (
  IN  CONST DFCI_SETTING_PROVIDER     *This
  );


#pragma pack (push, 1)
struct _DFCI_SETTING_PROVIDER {
  DFCI_SETTING_ID_STRING                 Id;                 //Setting Id String
  DFCI_SETTING_TYPE                      Type;               //Enum setting type
  DFCI_SETTING_FLAGS                     Flags;              //Flag for this setting.
  DFCI_SETTING_PROVIDER_SET              SetSettingValue;    //Set the setting
  DFCI_SETTING_PROVIDER_GET              GetSettingValue;    //Get the setting
  DFCI_SETTING_PROVIDER_GET_DEFAULT      GetDefaultValue;    //Get the default value
  DFCI_SETTING_PROVIDER_SET_DEFAULT      SetDefaultValue;    //Set the setting to the default value
};
#pragma pack (pop)

////////////////////////// END DFCI_SETTING_PROVIDER ///////////////////////////////////////////


/**
Define the DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL related structures
**/
typedef struct  _DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL  DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL;

/**
Registers a Setting Provider with the System Settings module

@param  This                 Protocol instance pointer.
@param  Provider             Provider pointer to register

@retval EFI_SUCCESS          The provider registered.
@retval ERROR                The provider could not be registered.

**/
typedef
EFI_STATUS
(EFIAPI *DFCI_SETTING_REGISTER_PROVIDER) (
  IN DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL       *This,
  IN DFCI_SETTING_PROVIDER                        *Provider
  );


//
// DFCI SYSTEM SETTINGS PROVIDER SUPPORT protocol structure
//
#pragma pack (push, 1)
struct _DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL
{
  DFCI_SETTING_REGISTER_PROVIDER    RegisterProvider;
};
#pragma pack (pop)

extern EFI_GUID     gDfciSettingsProviderSupportProtocolGuid;

#endif      // __DFCI_SETTING_PROVIDER_H__
