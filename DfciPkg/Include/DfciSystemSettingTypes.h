

#ifndef __DFCI_SETTING_TYPES_H__
#define __DFCI_SETTING_TYPES_H__

//
// Each System setting needs an ID.
// This ID is defined in a non-extensible way because to create
// a new setting requires an offline process involving tools team,
// core team, and platform team.
//
// Tools team must review and update XML files that drive the tools
// Core team must review and make sure core handles it and policy manager can support (possible add code to support)
// Platform team must review and add code to support the setting
//
//
typedef enum {
  DFCI_SETTING_ID__OWNER_KEY = 1,
  DFCI_SETTING_ID__USER_KEY  = 2,
  DFCI_SETTING_ID__USER1_KEY = 3,
  DFCI_SETTING_ID__USER2_KEY = 4,

  //This is used for permission only - Identity can perform recovery challenge/response operation
  DFCI_SETTING_ID__DFCI_RECOVERY = 25,

  DFCI_SETTING_ID__ASSET_TAG                  = 100,

  DFCI_SETTING_ID__SECURE_BOOT_KEYS_ENUM      = 200,

  DFCI_SETTING_ID__TPM_ENABLE                 = 300,
  DFCI_SETTING_ID__DOCKING_USB_PORT           = 301,
  DFCI_SETTING_ID__FRONT_CAMERA               = 302,
  DFCI_SETTING_ID__BLUETOOTH                  = 303,
  DFCI_SETTING_ID__REAR_CAMERA                = 304,
  DFCI_SETTING_ID__IR_CAMERA                  = 305,
  DFCI_SETTING_ID__ALL_CAMERAS                = 306,
  DFCI_SETTING_ID__WIFI_ONLY                  = 307,
  DFCI_SETTING_ID__WIFI_AND_BLUETOOTH         = 308,
  DFCI_SETTING_ID__WIRED_LAN                  = 309,
  DFCI_SETTING_ID__BLADE_USB_PORT             = 310,
  DFCI_SETTING_ID__ACCESSORY_RADIO_USB_PORT   = 311,
  DFCI_SETTING_ID__LTE_MODEM_USB_PORT         = 312,
  DFCI_SETTING_ID__WFOV_CAMERA                = 313,

  DFCI_SETTING_ID__ONBOARD_AUDIO              = 320,
  DFCI_SETTING_ID__MICRO_SDCARD               = 330,

  //
  //These are virtual mappings of external user accessable
  // standard usb ports.  Each
  // platform will map them differently.
  // Internal devices on USB should be disable using
  // friendly names (ie. Bluetooth).  DFCI OS tools will need
  // to handle naming them for each supported platform.
  //
  DFCI_SETTING_ID__USER_USB_PORT1             = 370,  //NO Preboot UI
  DFCI_SETTING_ID__USER_USB_PORT2,
  DFCI_SETTING_ID__USER_USB_PORT3,
  DFCI_SETTING_ID__USER_USB_PORT4,
  DFCI_SETTING_ID__USER_USB_PORT5,
  DFCI_SETTING_ID__USER_USB_PORT6,
  DFCI_SETTING_ID__USER_USB_PORT7,
  DFCI_SETTING_ID__USER_USB_PORT8,
  DFCI_SETTING_ID__USER_USB_PORT9,
  DFCI_SETTING_ID__USER_USB_PORT10,

  DFCI_SETTING_ID__IPV6                       = 400,
  DFCI_SETTING_ID__ALT_BOOT                   = 401,
  DFCI_SETTING_ID__BOOT_ORDER_LOCK            = 402,
  DFCI_SETTING_ID__ENABLE_USB_BOOT            = 403,
  DFCI_SETTING_ID__AUTO_POWERON_AFTER_LOSS    = 404,
  DFCI_SETTING_ID__DISABLE_BATTERY            = 405,
  DFCI_SETTING_ID__START_NETWORK              = 406,

  DFCI_SETTING_ID__TPM_ADMIN_CLEAR_PREAUTH    = 500, //NO Preboot UI
  DFCI_SETTING_ID__PASSWORD                   = 501,
  DFCI_SETTING_ID__SECURITY_PAGE_DISPLAY      = 600, // NO Preboot UI
  DFCI_SETTING_ID__DEVICES_PAGE_DISPLAY       = 601, // NO Preboot UI
  DFCI_SETTING_ID__BOOTMGR_PAGE_DISPLAY       = 602, // NO Preboot UI
  DFCI_SETTING_ID__SET_TIME_PAGE_DISPLAY      = 603, // NO Preboot UI

  // For VPRO/AMT
#ifdef VPRO_SUPPORT
  DFCI_SETTING_ID__VPRO                       = 700,
  DFCI_SETTING_ID__TXT                        = 701,
  DFCI_SETTING_ID__SGX                        = 702,
  DFCI_SETTING_ID__SEC_ERASE                  = 703,
  DFCI_SETTING_ID__AMT                        = 704,
  DFCI_SETTING_ID__CIRA                       = 705,
  DFCI_SETTING_ID__ASF                        = 706,
  DFCI_SETTING_ID__KVM                        = 707,
  DFCI_SETTING_ID__USB_R                      = 708,
  DFCI_SETTING_ID__USB_PROV                   = 709,
  DFCI_SETTING_ID__AMT_UNCONFIG               = 710,
#endif
  
  //
  //PERMISSION is based on these IDs and PERMISSION allows
  //PERMISSIONS for unknown IDs.  So to support error flows we need
  //1 ID value that is known as bad.
  //
  DFCI_SETTING_ID__MAX_AND_UNSUPPORTED        = 0xFFFFFFFF  //force to UINT32 type
} DFCI_SETTING_ID_ENUM;  //THIS IS a UINT32


//
// Each system setting needs a type.
//  This helps determine that VOID * Setting actual type and
//  memory allocation requirements.
//
typedef enum {
  DFCI_SETTING_TYPE_ENABLE,
  DFCI_SETTING_TYPE_ASSETTAG,
  DFCI_SETTING_TYPE_SECUREBOOTKEYENUM,
  DFCI_SETTING_TYPE_PASSWORD,
  DFCI_SETTING_TYPE_USBPORTENUM
} DFCI_SETTING_TYPE;

// type of the authentication token
typedef UINTN DFCI_AUTH_TOKEN;

//
// Flags are passed in and out of the setting set/get routines
// These flags allow the functions to return more detailed information
// that a consumer can use to create a better user experience.
//
typedef UINT64 DFCI_SETTING_FLAGS;
//
//Bitmask definitions for Flags
//
#define DFCI_SETTING_FLAGS_NONE                     (0x0000000000000000)

// OUT FLAGS are lower 32bits
#define DFCI_SETTING_FLAGS_OUT_REBOOT_REQUIRED      (0x0000000000000001)
#define DFCI_SETTING_FLAGS_OUT_LOCKED_UNTIL_REBOOT  (0x0000000000000002)
#define DFCI_SETTING_FLAGS_OUT_WRITE_ACCESS         (0x0000000000000100)
#define DFCI_SETTING_FLAGS_OUT_ALREADY_SET          (0x0000000080000000)

// IN FLAGS are middle - upper 16bits
#define DFCI_SETTING_FLAGS_IN_TEST_ONLY             (0x0000000100000000)

//STATIC Flags are in upper 16bits
#define DFCI_SETTING_FLAGS_NO_PREBOOT_UI            (0x0001000000000000)


// Auth defines and values
#define DFCI_AUTH_TOKEN_INVALID (0x0)

typedef enum {
  DFCI_IDENTITY_INVALID = 0x00,
  DFCI_IDENTITY_LOCAL = 0x01,
  DFCI_IDENTITY_SIGNER_USER  = 0x10,
  DFCI_IDENTITY_SIGNER_USER1 = 0x20,
  DFCI_IDENTITY_SIGNER_USER2 = 0x40,
  DFCI_IDENTITY_SIGNER_OWNER = 0x80,
  DFCI_MAX_IDENTITY = 0xFF  //Force size to UINT8
} DFCI_IDENTITY_ID;   //This must match the DFCI_PERMISSION_MASK values


#define IS_OWNER_IDENTITY_ENROLLED(mask) ((mask & DFCI_IDENTITY_SIGNER_OWNER) != 0)

//Define Permission Store instatiated in memory at runtime
#define DFCI_PERMISSION_MASK__ALL (DFCI_IDENTITY_LOCAL | DFCI_IDENTITY_SIGNER_USER |DFCI_IDENTITY_SIGNER_USER1 | DFCI_IDENTITY_SIGNER_USER2| DFCI_IDENTITY_SIGNER_OWNER)
#define DFCI_PERMISSION_MASK__DEFAULT (DFCI_IDENTITY_LOCAL)
#define DFCI_PERMISSION_MASK__NONE (0)

typedef UINT8 DFCI_PERMISSION_MASK; //currently only have 5 Identities so 8 bits is plenty


#endif  // __DFCI_SETTING_TYPES_H__
