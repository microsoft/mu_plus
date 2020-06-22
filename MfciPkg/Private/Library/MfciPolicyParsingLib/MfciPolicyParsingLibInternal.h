/** @file
  Private header defining the structure of the binary MFCI
  Policy packet and internal helper funcation declarations

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/


#ifndef _MFCI_POLICY_LIB_INTERNAL_H
#define _MFCI_POLICY_LIB_INTERNAL_H


#pragma pack(1)  

////
// The binary policy blob is laid out as:
//
// UINT16   FormatVersion;
// UINT32   PolicyVersion;
// GUID     PolicyPublisher;
// UINT16   Reserved1Count; // 0
//          Reserved1[Reserved1Count]  // not present
// UINT32   OptionFlags;    // 0
// UINT16   Reserved2Count; // 0
// UINT16   RulesCount;
//          Reserved2[Reserved2Count]  // not present
// // RULE  Rules[RulesCount];
// // BYTE  ValueTable[];
////

typedef struct _MfciPolicyBlob {
   UINT16   FormatVersion;
   UINT32   PolicyVersion;
   GUID PolicyPublisher;
   UINT16   Reserved1Count; // 0
   // Reserved1[Reserved1Count] // not present
   UINT32   OptionFlags;    // 0
   UINT16   Reserved2Count; // 0
   UINT16   RulesCount;
   // Reserved2[Reserved2Count] // Not present
   // Rules[RulesCount]
} MfciPolicyBlob, *pMfciPolicyBlob;

typedef struct _POLICY_VALUE_HEADER
{
    UINT16 Type;
} POLICY_VALUE_HEADER;


typedef struct _POLICY_VALUE_QWORD
{
    POLICY_VALUE_HEADER Header;
    UINT64 Value;
} POLICY_VALUE_QWORD;


typedef struct _POLICY_STRING
{
    UINT16 StringLength; // Length of String in bytes, excluding any null-terminator.
    CHAR16 String[];     // May or may not be NULL terminated !!!
} POLICY_STRING;


typedef struct _POLICY_VALUE_STRING
{
    POLICY_VALUE_HEADER Header;
    POLICY_STRING String;
} POLICY_VALUE_STRING;


//
// This structure defines how a rule is laid out in the policy blob.
//
typedef struct _RULE
{
    UINT32 RootKey;

    // Offsets within the value table to the key path and value.
    UINT32 OffsetToSubKeyName;
    UINT32 OffsetToValueName;

    UINT32 OffsetToValue; // Offset within the value table to the value.
} RULE;


typedef enum _POLICY_VALUE_TYPE
{
    POLICY_VALUE_TYPE_STRING = 0,
    POLICY_VALUE_TYPE_QWORD = 5
} POLICY_VALUE_TYPE;

#pragma pack()  


//
// Minimum size of a policy blob.
//
#define POLICY_BLOB_MIN_SIZE \
            (sizeof(UINT16) + sizeof(UINT32) + sizeof(GUID) + (3 * sizeof(UINT16)) + sizeof(UINT32))

// 64kB is way more then we ever dreamed
#define POLICY_BLOB_MAX_SIZE (1 << 15)
#define POLICY_STRING_MAX_LENGTH (1 << 8) // 512 CHAR16's (including NULL), 1024 bytes
#define POLICY_NAME_SEPARATOR L'\\'

#define POLICY_FORMAT_VERSION 2
#define POLICY_VERSION 1

// ('5AE6F808-8384-4EB9-A23A-0CCC1093E3DD')  # Do NOT change
#define POLICY_PUBLISHER_GUID \
  { \
    0x5AE6F808, 0x8384, 0x4EB9, { 0xA2, 0x3A, 0x0C, 0xCC, 0x10, 0x93, 0xE3, 0xDD } \
  }

CONST GUID gPolicyPublisherGuid = POLICY_PUBLISHER_GUID;

//
#define UEFI_POLICIES_ROOT_KEY    0xEF100000
#define MFCI_POLICY_SUB_KEY_NAME    L"UEFI"

#define MFCI_POLICY_VALUE_DEFINED_MASK             0x00000000FFFFFFFF
#define MFCI_POLICY_VALUE_OEM_MASK                 0xFFFFFFFF00000000
#define MFCI_POLICY_VALUE_ACTIONS_MASK             0x0000FFFF0000FFFF
#define MFCI_POLICY_VALUE_STATES_MASK              0xFFFF0000FFFF0000

#define MFCI_POLICY_VALUE_ACTION_SECUREBOOT_CLEAR  0x0000000000000001
#define MFCI_POLICY_VALUE_ACTION_TPM_CLEAR         0x0000000000000002

#define MFCI_POLICY_VALUE_STATE_DISABLE_SPI_LOCK   0x0000000000010000

#define MFCI_POLICY_VALUE_INVALID ((CONST UINT64)0x6464646464646464)


EFI_STATUS
EFIAPI
ValidateSignature (
   IN CONST UINT8   *SignedPolicy,
      UINTN          SignedPolicySize,
   IN CONST UINT8   *TrustAnchorCert,
   IN UINTN          TrustAnchorCertSize,
   IN CONST CHAR8   *EKU
 );

EFI_STATUS
EFIAPI
SanityCheckSignedPolicy (
   IN  CONST VOID   *SignedPolicy,
       UINTN         SignedPolicySize
 );


EFI_STATUS
EFIAPI
SanityCheckPolicy (
   IN  CONST MfciPolicyBlob *Policy,
       UINTN               PolicySize
 );

RULE*
EFIAPI
PolicyFindMatchingRule (
    IN CONST VOID *policy,
    IN UINT32 RootKey,
    IN CHAR16 *SubKey,
    IN CHAR16 *ValueName
);

UINT32
EFIAPI
CalculateSizeOfValueTableEntry(
    IN CONST POLICY_VALUE_HEADER *ValueHeader
);

#endif // _MFCI_POLICY_LIB_INTERNAL_H
