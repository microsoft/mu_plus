/** @file
DfciSettingsProvider.h

Defines the System Setting Provider Support protocol.

This protocol allows modules to register as setting providers.

Copyright (c) 2018, Microsoft Corporation

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
