

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

// Settings are a CHAR8 string
typedef CONST CHAR8 * DFCI_SETTING_ID_STRING;
// Maximum number of characters is 96 + a NULL character
#define DFCI_MAX_ID_SIZE (97)
#define DFCI_MAX_ID_LEN  (96)
typedef UINT32        DFCI_SETTING_ID_V1_ENUM;    // Only used for V1 support and Translate routines

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
  DFCI_SETTING_TYPE_USBPORTENUM,
  DFCI_SETTING_TYPE_STRING,             // CHAR8 string
  DFCI_SETTING_TYPE_BINARY,             // Opaque Binary Data
  DFCI_SETTING_TYPE_CERT                // Opaque Binary Data on write, Thumbprint text on read
} DFCI_SETTING_TYPE;

//
// Most of the settings types have a fixed length.  Limit the String and Binary
// settings types to a maximum size of 16KB
//
#define DFCI_SETTING_MAXIMUM_SIZE (1024 * 16)

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
