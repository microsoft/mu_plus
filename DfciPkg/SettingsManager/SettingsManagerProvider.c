
#include "SettingsManager.h"
#include <Library/DfciPasswordLib.h>
#include <Library/DfciBaseStringLib.h>

#define DFCI_PASSWORD_STORE_SIZE sizeof(DFCI_PASSWORD_STORE)

LIST_ENTRY  mProviderList = INITIALIZE_LIST_HEAD_VARIABLE(mProviderList);  //linked list for the providers



/**
Helper function to return the string describing the type enum
**/
CHAR8*
ProviderTypeAsAscii(DFCI_SETTING_TYPE Type)
{
  switch(Type) {
    case DFCI_SETTING_TYPE_ENABLE:
      return "ENABLE/DISABLE TYPE";

    case DFCI_SETTING_TYPE_ASSETTAG:
      return "ASSET TAG TYPE";

    case DFCI_SETTING_TYPE_SECUREBOOTKEYENUM:
      return "SECURE BOOT KEY ENUM TYPE";

    case DFCI_SETTING_TYPE_PASSWORD:
        return "PASSWORD TYPE";

    case DFCI_SETTING_TYPE_USBPORTENUM:
      return "USB PORT STATE TYPE";
  }

  return "UNKNOWN TYPE";
}

/**
Helper function to set a setting based on ASCII input
**/
EFI_STATUS
EFIAPI
SetSettingFromAscii(
  IN CONST CHAR8*  Id,
  IN CONST CHAR8*  Value,
  IN CONST DFCI_AUTH_TOKEN *AuthToken,
  IN OUT DFCI_SETTING_FLAGS *Flags)
{
  DFCI_SETTING_PROVIDER *Provider = NULL;  //need provider to get type
  DFCI_SETTING_ID_ENUM IdNumber = DFCI_SETTING_ID__MAX_AND_UNSUPPORTED;

  if (Id == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Id is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_INFO, "%a - Id is %a\n", __FUNCTION__, Id));

  if (Value == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Value is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_INFO, "%a - Value is %a\n", __FUNCTION__, Value));

  if (AuthToken == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - AuthToken is NULL\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }
  DEBUG((DEBUG_INFO, "%a - AuthToken is 0x%X\n", __FUNCTION__, *AuthToken));

  //Convert ID to DFCI_SETTINGS_ID_ENUM (UINT64)
  IdNumber = (DFCI_SETTING_ID_ENUM) AsciiStrDecimalToUintn(Id);
  Provider = FindProviderById(IdNumber);
  if (Provider == NULL)
  {
    DEBUG((DEBUG_INFO, "%a - Provider for Id (%d) not found in system\n", __FUNCTION__, IdNumber));
    return EFI_NOT_FOUND;
  }
  
  return SetProviderValueFromAscii(Provider, Value, AuthToken, Flags);
}

/**
Helper function to set a setting based on ASCII input
**/
EFI_STATUS
EFIAPI
SetProviderValueFromAscii(
  IN CONST DFCI_SETTING_PROVIDER *Provider,
  IN CONST CHAR8* Value, 
  IN CONST DFCI_AUTH_TOKEN *AuthToken,
  IN OUT DFCI_SETTING_FLAGS *Flags
  )
{
  CONST VOID* SetValue = NULL;
  BOOLEAN            v = FALSE;
  UINT8              b = 0;  
  UINT8             *ByteArray;
  EFI_STATUS        Status;

  switch (Provider->Type)
  {
    /* Enable Type (Boolean)*/
  case DFCI_SETTING_TYPE_ENABLE:
    //convert to BOOLEAN
    
    if (AsciiStrCmp(Value, "Enabled") == 0)
    {
      v = TRUE;
      DEBUG((DEBUG_INFO, "Setting to Enabled\n"));
    }
    else if (AsciiStrCmp(Value, "Disabled") == 0)
    {
      v = FALSE;
      DEBUG((DEBUG_INFO, "Setting to Disabled\n"));
    }
    else
    {
      DEBUG((DEBUG_ERROR, "Invalid Settings Ascii Value for Type Eanble (%a)\n", Value));
      return EFI_INVALID_PARAMETER;
    }

    SetValue = &v;
    break;

  /* ASSET Tag Type (Ascii String)*/
  case DFCI_SETTING_TYPE_ASSETTAG:
    SetValue = &Value;
    DEBUG((DEBUG_INFO, "Setting Asset Tag to %a\n", Value));
    break;

  case DFCI_SETTING_TYPE_SECUREBOOTKEYENUM:
    if (AsciiStrCmp(Value, "MsOnly") == 0)
    {
      DEBUG((DEBUG_INFO, "Setting to MsOnly\n"));
      b = 0;
    }
    else if (AsciiStrCmp(Value, "MsPlus3rdParty") == 0)
    {
      DEBUG((DEBUG_INFO, "Setting to MsPlus3rdParty\n"));
      b = 1;
    }
    else if(AsciiStrCmp(Value, "None") == 0)
    {
      DEBUG((DEBUG_INFO, "Setting to None\n"));
      b = 2;
    }
    else
    {
      DEBUG((DEBUG_INFO, "Invalid Secure Boot Key Enum Setting. %a\n", Value));
      return EFI_INVALID_PARAMETER;
    }
    SetValue = &b;
    break;

  case DFCI_SETTING_TYPE_PASSWORD:  

      if ((HexLookUp(*(Value + (DFCI_PASSWORD_STORE_SIZE * 2))) != 0x0E) || (HexLookUp(*(Value + (DFCI_PASSWORD_STORE_SIZE * 2) + 1)) != 0x0B))
      {
          DEBUG((DEBUG_ERROR, "End Byte 'EB' is missing. Not a valid store format . %a\n", Value));
          return EFI_INVALID_PARAMETER;
      }

      ByteArray = (UINT8 *)AllocateZeroPool(DFCI_PASSWORD_STORE_SIZE);      

      Status = AsciitoHexByteArray(Value, ByteArray, DFCI_PASSWORD_STORE_SIZE);

      if (!EFI_ERROR(Status))
      {
          DEBUG((DEBUG_INFO, "Setting Password. %a\n", Value));
          SetValue = ByteArray;
      }
      else
      {
          DEBUG((DEBUG_ERROR, "Cannot set password. Invalid Character Present \n"));
          return EFI_INVALID_PARAMETER;
      }
      
      break;

  case DFCI_SETTING_TYPE_USBPORTENUM:
    if (AsciiStrCmp(Value, "UsbPortEnabled") == 0)
    {
      DEBUG((DEBUG_INFO, "Setting to Usb Port Enabled\n"));
      b = DfciUsbPortEnabled;
    }
    else if (AsciiStrCmp(Value, "UsbPortHwDisabled") == 0)
    {
      DEBUG((DEBUG_INFO, "Setting to Usb Port HW Disabled\n"));
      b = DfciUsbPortHwDisabled;
    }
    else
    {
      DEBUG((DEBUG_INFO, "Invalid or unsupported Usb Port Setting. %a\n", Value));
      return EFI_INVALID_PARAMETER;
    }
    SetValue = &b;
    break;

  default:
    DEBUG((DEBUG_ERROR, "Failed - SetProviderValueFromAscii for ID 0x%X Unsupported Type = 0x%X\n", Provider->Id, Provider->Type));
    return EFI_INVALID_PARAMETER;
  }
  return mSystemSettingAccessProtocol.Set(&mSystemSettingAccessProtocol, Provider->Id, AuthToken, Provider->Type, SetValue, Flags);
}

/**
Helper function to return a friendly name for the ID. 
Strings are static and should not be Freed
**/
CHAR8*
ProviderIdAsAscii(DFCI_SETTING_ID_ENUM Id)
{
  switch(Id){

  case DFCI_SETTING_ID__TPM_ENABLE:
    return "TPM Enable";

  case DFCI_SETTING_ID__TPM_ADMIN_CLEAR_PREAUTH:
    return "TPM Admin Clear Authorization";

  case DFCI_SETTING_ID__SECURE_BOOT_KEYS_ENUM:
    return "Secure Boot Keys Enum";

  case DFCI_SETTING_ID__ASSET_TAG:
    return "Asset Tag";

  case DFCI_SETTING_ID__DOCKING_USB_PORT:
    return "Docking Station USB Port Enable";

  case DFCI_SETTING_ID__BLADE_USB_PORT:
    return "Blade USB Port Enable";

  case DFCI_SETTING_ID__ACCESSORY_RADIO_USB_PORT:
    return "Accessory Radio Enable";

  case DFCI_SETTING_ID__LTE_MODEM_USB_PORT:
    return "LTE Modem Enable";

  case DFCI_SETTING_ID__FRONT_CAMERA:
    return "Front Camera Enable";

  case DFCI_SETTING_ID__REAR_CAMERA:
    return "Rear Camera Enable";

  case DFCI_SETTING_ID__IR_CAMERA:
    return "IR Camera Enable";

  case DFCI_SETTING_ID__WFOV_CAMERA:
    return "WFOV Camera Enable";

  case DFCI_SETTING_ID__ALL_CAMERAS:
    return "All Cameras Enable";

  case DFCI_SETTING_ID__WIFI_ONLY:
    return "Wifi Enable";

  case DFCI_SETTING_ID__WIFI_AND_BLUETOOTH:
    return "Wifi & Bluetooth Enable";

  case DFCI_SETTING_ID__WIRED_LAN:
    return "Wired LAN Enable";

  case DFCI_SETTING_ID__BLUETOOTH:
    return "Bluetooth Enable";

  case DFCI_SETTING_ID__ONBOARD_AUDIO:
    return "Onboard Audio Enable";
  
  case DFCI_SETTING_ID__MICRO_SDCARD:
    return "Micro SD-Card Enable";

  case DFCI_SETTING_ID__PASSWORD:
    return "System Password";
  }

  if ((Id >= DFCI_SETTING_ID__USER_USB_PORT1) &&
    (Id <= DFCI_SETTING_ID__USER_USB_PORT10))
  {
    return "User Accessable Usb Port State";
  }



  return "Unsupported Setting Id";
}


#define ENABLED_STRING_SIZE  (9)
#define ASSET_TAG_STRING_MAX_SIZE  (22)
#define SECURE_BOOT_ENUM_STRING_SIZE  (20)
#define SYSTEM_PASSWORD_STATE_STRING_SIZE  (30)
#define USB_PORT_STATE_STRING_SIZE (20)

/**
Helper function to Print out the Value as Ascii text. 
NOTE: -- This must match the XML format

Caller must free the return string if not null;

@param Provider : Pointer to provider instance the value should be printed for
@param Current  : TRUE if provider current value.  FALSE for provider default value.

@ret  String allocated with AllocatePool containing Ascii printable value.  
**/
CHAR8*
ProviderValueAsAscii(DFCI_SETTING_PROVIDER *Provider, BOOLEAN Current)
{

  EFI_STATUS Status;
  CHAR8* Value = NULL;
  BOOLEAN v = FALSE; //Boolean Types
  CHAR8* s = NULL;   //String types
  UINT8 b = 0xFF;    //Byte types (small enum)
  UINTN Length = 0;

  switch (Provider->Type)
  {
    case DFCI_SETTING_TYPE_ENABLE:
      if (Current)
      {
        Status = Provider->GetSettingValue(Provider, &v);
      }
      else 
      {
        Status = Provider->GetDefaultValue(Provider, &v);
      }
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "Failed - GetSettingValue for ID 0x%X Status = %r\n", Provider->Id, Status));
        break;
      }

      Value = AllocateZeroPool(ENABLED_STRING_SIZE);
      if (Value == NULL) {
        DEBUG((DEBUG_ERROR, "Failed - Couldn't allocate for string. \n"));
        break;
      }
      if (v) {
        AsciiStrCpyS(Value, ENABLED_STRING_SIZE , "Enabled");
      }
      else {
        AsciiStrCpyS(Value, ENABLED_STRING_SIZE, "Disabled");
      }
      break;

    case DFCI_SETTING_TYPE_ASSETTAG:
      if (Current)
      {
        Status = Provider->GetSettingValue(Provider, &s);
      }
      else
      {
        Status = Provider->GetDefaultValue(Provider, &s);
      }
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "Failed - GetSettingValue for ID 0x%X Status = %r\n", Provider->Id, Status));
        break;
      }
      Length = AsciiStrnLenS(s, ASSET_TAG_STRING_MAX_SIZE) + 1;  //max size of asset tag
      Value = AllocateZeroPool(Length);  //for null
      AsciiStrnCpyS(Value, Length, s, Length);
      break;

    case DFCI_SETTING_TYPE_SECUREBOOTKEYENUM:
      if (Current)
      {
        Status = Provider->GetSettingValue(Provider, &b);
      }
      else
      {
        Status = Provider->GetDefaultValue(Provider, &b);
      }
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "Failed - GetSettingValue for ID 0x%X Status = %r\n", Provider->Id, Status));
        break;
      }

      Value = AllocateZeroPool(SECURE_BOOT_ENUM_STRING_SIZE);
      if (Value == NULL) {
        DEBUG((DEBUG_ERROR, "Failed - Couldn't allocate for string. \n"));
        break;
      }
      if (b==0) {
        AsciiStrCpyS(Value, SECURE_BOOT_ENUM_STRING_SIZE, "MsOnly");
      }
      else if(b==1)
      {
        AsciiStrCpyS(Value, SECURE_BOOT_ENUM_STRING_SIZE, "MsPlus3rdParty");
      }
      else if (b == 3) //This is a special case.  Only supported as output.  
      {
        AsciiStrCpyS(Value, SECURE_BOOT_ENUM_STRING_SIZE, "Custom");
      }
      else
      {
        AsciiStrCpyS(Value, SECURE_BOOT_ENUM_STRING_SIZE, "None");
      }
      break;
    case DFCI_SETTING_TYPE_PASSWORD:
        if (Current)
        {
            Status = Provider->GetSettingValue(Provider, &v);
        }
        else
        {
            Status = Provider->GetDefaultValue(Provider, &v);
        }

        if (EFI_ERROR(Status))
        {
            DEBUG((DEBUG_ERROR, "Failed - GetSettingValue for ID 0x%X Status = %r\n", Provider->Id, Status));
            break;
        }
        Value = AllocateZeroPool(SYSTEM_PASSWORD_STATE_STRING_SIZE);
        if (Value == NULL) {
            DEBUG((DEBUG_ERROR, "Failed - Couldn't allocate for string. \n"));
            break;
        }
        if (v) {
            AsciiStrCpyS(Value, SYSTEM_PASSWORD_STATE_STRING_SIZE, "System Password Set");
        }
        else {
            AsciiStrCpyS(Value, SYSTEM_PASSWORD_STATE_STRING_SIZE, "No System Password");
        }
        break;

    case DFCI_SETTING_TYPE_USBPORTENUM:
      if (Current)
      {
        Status = Provider->GetSettingValue(Provider, &b);
      }
      else
      {
        Status = Provider->GetDefaultValue(Provider, &b);
      }
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "Failed - GetSettingValue for ID 0x%X Status = %r\n", Provider->Id, Status));
        break;
      }

      Value = AllocateZeroPool(USB_PORT_STATE_STRING_SIZE);
      if (Value == NULL) {
        DEBUG((DEBUG_ERROR, "Failed - Couldn't allocate for string. \n"));
        break;
      }
      if (b == DfciUsbPortHwDisabled) {
        AsciiStrCpyS(Value, USB_PORT_STATE_STRING_SIZE, "UsbPortHwDisabled");
      }
      else if (b == DfciUsbPortEnabled)
      {
        AsciiStrCpyS(Value, USB_PORT_STATE_STRING_SIZE, "UsbPortEnabled");
      }
      else
      {
        AsciiStrCpyS(Value, USB_PORT_STATE_STRING_SIZE, "UnsupportedValue");
      }
      break;

    default: 
      DEBUG((DEBUG_ERROR, "Failed - ProviderValueAsAscii for ID 0x%X Unsupported Type = 0x%X\n", Provider->Id, Provider->Type));
      break;
  }
  return Value;
}


/*
Helper function to print out one Setting Provider
*/
VOID
DebugPrintProviderEntry(DFCI_SETTING_PROVIDER *Provider)
{ 
  CHAR8 *Value = ProviderValueAsAscii(Provider, TRUE);
  CHAR8 *DefaultValue = ProviderValueAsAscii(Provider, FALSE);

  DEBUG((DEBUG_INFO, "Printing Provider @ 0x%X\n", (UINTN)Provider));
  DEBUG((DEBUG_INFO, "Id:            %d\n", Provider->Id));
  DEBUG((DEBUG_INFO, "Friendly Name: %a\n", ProviderIdAsAscii(Provider->Id)));
  DEBUG((DEBUG_INFO, "Type:          %a\n", ProviderTypeAsAscii(Provider->Type)));
  DEBUG((DEBUG_INFO, "Flags:         0x%X\n", Provider->Flags));
  DEBUG((DEBUG_INFO, "Current Value: %a\n", Value));
  DEBUG((DEBUG_INFO, "Default Value: %a\n", DefaultValue));

  if (DefaultValue != NULL)
  {
    FreePool(DefaultValue);
  }

  if (Value != NULL)
  {
    FreePool(Value);
  }
}

/*
Helper function to print out all Setting Providers currently registered
*/
VOID
DebugPrintProviderList()
{
  LIST_ENTRY* Link = NULL;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Prov = NULL;
  DEBUG((DEBUG_INFO, "-----------------------------------------------------\n"));
  DEBUG((DEBUG_INFO, "START PRINTING ALL REGISTERED SETTING PROVIDERS\n"));
  DEBUG((DEBUG_INFO, "-----------------------------------------------------\n"));
  
  for (Link = mProviderList.ForwardLink; Link != &mProviderList; Link = Link->ForwardLink) {
    //Convert Link Node into object stored
    Prov = CR(Link, DFCI_SETTING_PROVIDER_LIST_ENTRY, Link, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE);
    DebugPrintProviderEntry(&Prov->Provider);
  }
  DEBUG((DEBUG_INFO, "-----------------------------------------------------\n"));
  DEBUG((DEBUG_INFO, " END PRINTING ALL REGISTERED SETTING PROVIDERS\n"));
  DEBUG((DEBUG_INFO, "-----------------------------------------------------\n"));
}


/*
Function to find a setting provider given an ID
If it isn't found a NULL will be returned
*/
DFCI_SETTING_PROVIDER*
FindProviderById(DFCI_SETTING_ID_ENUM Id)
{
  LIST_ENTRY* Link = NULL;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Prov = NULL;

  for (Link = mProviderList.ForwardLink; Link != &mProviderList; Link = Link->ForwardLink) {
    //Convert Link Node into object stored
    Prov = CR(Link, DFCI_SETTING_PROVIDER_LIST_ENTRY, Link, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE);
    if (Prov->Provider.Id == Id)
    {
      DEBUG((DEBUG_VERBOSE, "FindProviderById - Found (%d)\n", Id));
      return &Prov->Provider;
    }
  }
  DEBUG((DEBUG_VERBOSE, "FindProviderById - Failed to find (%d)\n", Id));
  return NULL;
}



/**
Registers a Setting Provider with the System Settings module

@param  This                 Protocol instance pointer.
@param  Provider             Provider pointer to register

@retval EFI_SUCCESS          The provider registered.
@retval ERROR                The provider could not be registered.

**/
EFI_STATUS
EFIAPI
RegisterProvider(
IN DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL       *This,
IN DFCI_SETTING_PROVIDER                         *Provider
)
{
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Entry = NULL;
  DFCI_SETTING_PROVIDER *ExistingProvider = NULL;

  if (Provider == NULL)
  {
    DEBUG((DEBUG_ERROR, "Invalid Provider parameter\n"));
    return EFI_INVALID_PARAMETER;
  }


  DEBUG((DEBUG_INFO, "Registering Provider with ID 0x%X\n", Provider->Id));

  //check to make sure it doesn't already exist. 
  ExistingProvider = FindProviderById(Provider->Id);
  if (ExistingProvider != NULL)
  {
    DEBUG((DEBUG_ERROR, "Error - Can't register a provider more than once.  id(%d)\n", Provider->Id));
    ASSERT(ExistingProvider == NULL);
    return EFI_INVALID_PARAMETER;
  }

  //Check function pointers
  ASSERT(Provider->SetDefaultValue != NULL);
  ASSERT(Provider->GetDefaultValue != NULL);
  ASSERT(Provider->GetSettingValue != NULL);
  ASSERT(Provider->SetSettingValue != NULL);
  

  //Allocate memory for provider
  Entry = AllocateZeroPool(sizeof(DFCI_SETTING_PROVIDER_LIST_ENTRY));
  if (Entry == NULL)
  {
    DEBUG((DEBUG_ERROR, "RegisterProvider - Couldn't allocate memory for entry\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Signature = DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE;

  //Copy provider to new entry
  CopyMem(&Entry->Provider, Provider, sizeof(DFCI_SETTING_PROVIDER));

  //insert into list
  InsertTailList(&mProviderList, &Entry->Link);

  return EFI_SUCCESS;
}


/**
Function sets the providers to default for
any provider that contains the FilterFlag in its flags
**/
EFI_STATUS
EFIAPI
ResetAllProvidersToDefaultsWithMatchingFlags(
  DFCI_SETTING_FLAGS FilterFlag)
{
  EFI_STATUS Status = EFI_SUCCESS;
  LIST_ENTRY* Link = NULL;
  DFCI_SETTING_PROVIDER_LIST_ENTRY *Prov = NULL;


  for (Link = mProviderList.ForwardLink; Link != &mProviderList; Link = Link->ForwardLink) 
  {
    //Convert Link Node into object stored
    Prov = CR(Link, DFCI_SETTING_PROVIDER_LIST_ENTRY, Link, DFCI_SETTING_PROVIDER_LIST_ENTRY_SIGNATURE);
    if (Prov->Provider.Flags & FilterFlag)
    {
      DEBUG((DEBUG_INFO, "%a - Setting Provider %d to defaults as part of a Reset request. \n", __FUNCTION__, Prov->Provider.Id));
      Status = Prov->Provider.SetDefaultValue(&(Prov->Provider));
      if (EFI_ERROR(Status))
      {
        DEBUG((DEBUG_ERROR, "%a - Failed to Set Provider (%d) To Default Value. Status = %r\n", __FUNCTION__, Prov->Provider.Id, Status));
      }
    }
  }
  return EFI_SUCCESS;
} 

