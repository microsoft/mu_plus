/** @file
DfciRecoveryLib.h

This library contains crypto support functions for the DFCI recovery feature.

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

#ifndef _DFCI_RECOVERY_LIB_H_
#define _DFCI_RECOVERY_LIB_H_

#define DFCI_RECOVERY_NONCE_SIZE            (512 / 8)   // 512-Bit Nonce
#define DFCI_RECOVERY_NONCE_KEY_SIZE        10          // Number of bytes at end of nonce that will be used for user auth.


typedef union
{
  UINT8 Bytes[DFCI_RECOVERY_NONCE_SIZE];
  struct {
    UINT8 Nonce[DFCI_RECOVERY_NONCE_SIZE - DFCI_RECOVERY_NONCE_KEY_SIZE];
    UINT8 Key[DFCI_RECOVERY_NONCE_KEY_SIZE];
  } Parts;
} DFCI_CHALLENGE_NONCE;

#pragma warning(disable: 4200) // zero-sized array

typedef CHAR8 DFCI_TARGET_MULTI_STRING;
#define DFCI_MULTI_STRING_MAX_SIZE 104

typedef struct _DFCI_RECOVERY_CHALLENGE
{
  UINTN                     SerialNumber;
  EFI_TIME                  Timestamp;
  DFCI_CHALLENGE_NONCE      Nonce;
  DFCI_TARGET_MULTI_STRING  MultiString[0];
} DFCI_RECOVERY_CHALLENGE;


/**
  This function will attempr to allocate and populate a buffer
  with a DFCI recovery challenge structure. If unsuccessful,
  will return an error and set pointer to NULL.

  @param[out] Challenge    Allocated buffer containing recovery challenge. NULL on error.
  @param[out] ChallengeSize Pointer to UINTN to receive the Size of Challenge object.

  @retval     EFI_SUCCESS           Challenge was successfully created and can be found in buffer.
  @retval     EFI_INVALID_PARAMETER Nuff said.
  @retval     EFI_NOT_FOUND         Could not locate a required protocol or resource.
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate a buffer for the challenge.
  @retval     Others                Error returned from LocateProtocol, GetVariable, GetTime, or GetRNG.

**/
EFI_STATUS
GetRecoveryChallenge (
  OUT DFCI_RECOVERY_CHALLENGE  **Challenge,
  OUT UINTN                     *ChallengeSize
  );


/**
  Take in a DER-encoded x509 cert buffer and a challenge
  and will attempt to encrypt it for transmission. Encrypted data
  buffer will be allocated and populated on success.

  @param[in]  Challenge     Pointer to an DFCI_RECOVERY_CHALLENGE to be encrypted.
  @param[in]  ChallengeSize Size of Challenge object.
  @param[in]  PublicKey     Pointer to a DER-encoded x509 cert.
  @param[in]  PublicKeySize Size of the x509 cert provided.
  @param[out] EncryptedData     Encrypted data buffer or NULL on error.
  @param[out] EncryptedDataSize Size of the encrypted data buffer or 0 on error.

  @retval     EFI_SUCCESS           Challenge was successfully encrypted and can be found in buffer.
  @retval     EFI_INVALID_PARAMETER Nuff said.
  @retval     EFI_NOT_FOUND         Could not locate the RNG protocol.
  @retval     EFI_ABORTED           Call to Pkcs1v2Encrypt() failed.
  @retval     Others                Error returned from LocateProtocol or GetRNG.

**/
EFI_STATUS
EncryptRecoveryChallenge (
  IN  DFCI_RECOVERY_CHALLENGE    *Challenge,
  IN  UINTN                       ChallengeSize,
  IN  CONST UINT8                *PublicKey,
  IN  UINTN                       PublicKeySize,
  OUT  UINT8                    **EncryptedData,
  OUT  UINTN                     *EncryptedDataSize
  );

#endif // _DFCI_RECOVERY_LIB_H_
