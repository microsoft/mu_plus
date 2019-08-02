/** @file
DfciRecoveryLib.c

This library contains crypto support functions for the DFCI
recovery feature.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciRecoveryLib.h>
#include <Library/MemoryAllocationLib.h>            // AllocatePool
#include <Library/UefiBootServicesTableLib.h>       // gBS
#include <Library/UefiRuntimeServicesTableLib.h>    // gRT
#include <Library/BaseCryptLib.h>                   // Pkcs1v2Encrypt
#include <Library/DfciDeviceIdSupportLib.h>         // Provide device info for recovery operator

#include <Protocol/Rng.h>               // EFI_RNG_PROTOCOL

#define   RANDOM_SEED_BUFFER_SIZE   64  // We'll seed with 64 bytes of random so that OAEP can work its best.

/**
  This function will attempr to allocate and populate a buffer
  with a DFCI recovery challenge structure. If unsuccessful,
  will return an error and set pointer to NULL.

  @param[out] Challenge     Allocated buffer containing recovery challenge. NULL on error.
  @param[out] ChallengeSize Pointer to UINTN to receive the Size of Challenge object.

  @retval     EFI_SUCCESS           Challenge was successfully created and can be found in buffer.
  @retval     EFI_INVALID_PARAMETER Nuff said.
  @retval     EFI_NOT_FOUND         Could not locate a required protocol or resource.
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate a buffer for the challenge.
  @retval     Others                Error returned from LocateProtocol, GetVariable, GetTime, or GetRNG.

**/
EFI_STATUS
GetRecoveryChallenge (
  OUT DFCI_RECOVERY_CHALLENGE    **Challenge,
  OUT UINTN                       *ChallengeSize
  )
{
  EFI_STATUS                     Status;
  DFCI_RECOVERY_CHALLENGE       *NewChallenge;
  EFI_RNG_PROTOCOL              *RngProtocol;
  CHAR8                         *Element;
  UINTN                          ElementSize;

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__));

  //
  // Check inputs...
  if (Challenge == NULL)
  {
    DEBUG(( DEBUG_ERROR, "%a: NULL pointer provided!\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set default state...
  *Challenge = NULL;
  NewChallenge = NULL;

  //
  // Locate the RNG Protocol. This will be needed for the nonce.
  Status = gBS->LocateProtocol( &gEfiRngProtocolGuid, NULL, (VOID **) &RngProtocol);
  DEBUG(( DEBUG_VERBOSE, "%a: LocateProtocol(RNG) = %r\n", __FUNCTION__, Status));

  //
  // From now on, don't proceed on errors.
  //

  //
  // Allocate the buffer...
  if (!EFI_ERROR( Status ))
  {
    NewChallenge = AllocatePool( sizeof( DFCI_RECOVERY_CHALLENGE ) + DFCI_MULTI_STRING_MAX_SIZE);
    if (NewChallenge == NULL)
    {
      Status = EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Grab the system UUID...
  if (!EFI_ERROR( Status ))
  {
    Status = DfciIdSupportV1GetSerialNumber(&NewChallenge->SerialNumber);
    DEBUG((DEBUG_VERBOSE, "%a: GetSerialNumber = %r\n", __FUNCTION__, Status));
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, "%a: Failed to get the DeviceSerialNumber %r\n", __FUNCTION__, Status));
    }
  }

  //
  // Grab a timestamp...
  if (!EFI_ERROR( Status ))
  {
    Status = gRT->GetTime( &NewChallenge->Timestamp, NULL );
    DEBUG(( DEBUG_VERBOSE, "%a: GetTime() = %r\n", __FUNCTION__, Status ));
  }

  //
  // Generate the random nonce...
  if (!EFI_ERROR( Status ))
  {
    Status = RngProtocol->GetRNG( RngProtocol,
                                  &gEfiRngAlgorithmSp80090Ctr256Guid,
                                  DFCI_RECOVERY_NONCE_SIZE,
                                  &NewChallenge->Nonce.Bytes[0] );
    DEBUG(( DEBUG_VERBOSE, "%a: GetRNG() = %r\n", __FUNCTION__, Status ));
  }

  //
  // If we're still good, set the pointer...
  if (!EFI_ERROR( Status ))
  {
    *Challenge = NewChallenge;
  }

  //
  // Always put away your toys...
  // If there was an error, but the buffer was allocated, free it.
  if (EFI_ERROR(Status ))
  {
    if (NULL != NewChallenge) {
      FreePool (NewChallenge);
    }
    NewChallenge = NULL;
  }

  if (NewChallenge != NULL)
  {
    //
    // There is only room for about 100 characters of identifier.  This should be enough to identify
    // the system that is being recovered.
    //
    NewChallenge->MultiString[0] = '\0';
    Status = DfciIdSupportGetSerialNumber (&Element, &ElementSize);
    if (!EFI_ERROR(Status)) {
      Status = AsciiStrnCatS (&NewChallenge->MultiString[0], DFCI_MULTI_STRING_MAX_SIZE, Element, ElementSize - sizeof(CHAR8));
      FreePool (Element);
    }

    if (!EFI_ERROR(Status)) {
      Status = DfciIdSupportGetProductName (&Element, &ElementSize);
      if (!EFI_ERROR(Status)) {
        Status = AsciiStrnCatS (&NewChallenge->MultiString[0], DFCI_MULTI_STRING_MAX_SIZE, Element, ElementSize - sizeof(CHAR8));
        FreePool (Element);
      }
    }

    if (!EFI_ERROR(Status)) {
      Status = DfciIdSupportGetManufacturer (&Element, &ElementSize);
      if (!EFI_ERROR(Status)) {
        Status = AsciiStrnCatS (&NewChallenge->MultiString[0], DFCI_MULTI_STRING_MAX_SIZE, Element, ElementSize - sizeof(CHAR8));
        FreePool (Element);
      }
    }

    // Print a debug message, but it isn't a big issue if the identification
    // doesn't get added to the recovery packet.
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "Error getting system identifier for recovery packet\n"));
    }
    *ChallengeSize = sizeof(DFCI_RECOVERY_CHALLENGE) + AsciiStrnSizeS (&NewChallenge->MultiString[0],DFCI_MULTI_STRING_MAX_SIZE);
  }

  return Status;
} // GetRecoveryChallenge()


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
  )
{
  EFI_STATUS        Status;
  EFI_RNG_PROTOCOL  *RngProtocol;
  UINT8             ExtraSeed[RANDOM_SEED_BUFFER_SIZE];

  DEBUG(( DEBUG_INFO, "%a()\n", __FUNCTION__));

  //
  // Check inputs...
  if (Challenge == NULL || PublicKey == NULL || EncryptedData == NULL || EncryptedDataSize == NULL)
  {
    DEBUG(( DEBUG_ERROR, "%a: NULL pointer provided!\n", __FUNCTION__));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set default state...
  *EncryptedData = NULL;
  *EncryptedDataSize = 0;

  //
  // We would like to provide a *little* more random for a better seed.
  // NOTE: This *could* be done with a direct call to RandomSeed() rather than
  //       passing it into the Pkcs1v2Encrypt() function. There are merits to
  //       each implementation.
  Status = gBS->LocateProtocol( &gEfiRngProtocolGuid, NULL, (VOID **) &RngProtocol );
  DEBUG(( DEBUG_VERBOSE, "%a: LocateProtocol(RNG) = %r\n", __FUNCTION__, Status));
  // Assuming we found the protocol, let's grab a seed.
  if (!EFI_ERROR( Status ))
  {
    Status = RngProtocol->GetRNG( RngProtocol,
                                  &gEfiRngAlgorithmSp80090Ctr256Guid,
                                  RANDOM_SEED_BUFFER_SIZE,
                                  &ExtraSeed[0] );
    DEBUG(( DEBUG_VERBOSE, "%a: GetRNG() = %r\n", __FUNCTION__, Status));
  }

  //
  // Now, we should be able to encrypt the data and be done with it.
  if (!EFI_ERROR( Status ))
  {
    if (!Pkcs1v2Encrypt( PublicKey,
                         PublicKeySize,
                         (UINT8*)Challenge,
                         ChallengeSize,
                         &ExtraSeed[0],
                         RANDOM_SEED_BUFFER_SIZE,
                         EncryptedData,
                         EncryptedDataSize ))
    {
      DEBUG((DEBUG_ERROR, "%a: Failed to encrypt the challenge!\n", __FUNCTION__));
      Status = EFI_ABORTED;
    }
  }

  return Status;
} // EncryptRecoveryChallenge()
