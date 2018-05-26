/** @file -- DfciRecoveryLib.c
This library contains crypto support functions for the DFCI 
recovery feature. 

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.


Copyright (C) 2016 Microsoft Corporation. All Rights Reserved.

**/

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

  @param[out] Challenge    Allocated buffer containing recovery challenge. NULL on error.

  @retval     EFI_SUCCESS           Challenge was successfully created and can be found in buffer.
  @retval     EFI_INVALID_PARAMETER Nuff said.
  @retval     EFI_NOT_FOUND         Could not locate a required protocol or resource.
  @retval     EFI_OUT_OF_RESOURCES  Could not allocate a buffer for the challenge.
  @retval     Others                Error returned from LocateProtocol, GetVariable, GetTime, or GetRNG.

**/
EFI_STATUS
GetRecoveryChallenge (
  OUT DFCI_RECOVERY_CHALLENGE    **Challenge
  )
{
  EFI_STATUS                    Status;
  DFCI_RECOVERY_CHALLENGE    *NewChallenge;
  EFI_RNG_PROTOCOL              *RngProtocol;

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Check inputs...
  if (Challenge == NULL)
  {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - NULL pointer provided!\n" ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set default state...
  *Challenge = NULL;
  NewChallenge = NULL;

  //
  // Locate the RNG Protocol. This will be needed for the nonce.
  Status = gBS->LocateProtocol( &gEfiRngProtocolGuid, NULL, &RngProtocol );
  DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - LocateProtocol(RNG) = %r\n", Status ));

  //
  // From now on, don't proceed on errors.
  //

  //
  // Allocate the buffer...
  if (!EFI_ERROR( Status ))
  {
    NewChallenge = AllocatePool( sizeof( *NewChallenge ) );
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
    DEBUG((DEBUG_VERBOSE, __FUNCTION__" - GetSerialNumber = %r\n", Status));
    if (EFI_ERROR(Status))
    {
      DEBUG((DEBUG_ERROR, __FUNCTION__" - Failed to get the DeviceSerialNumber %r\n", Status));
    }
  }

  //
  // Grab a timestamp...
  if (!EFI_ERROR( Status ))
  {
    Status = gRT->GetTime( &NewChallenge->Timestamp, NULL );
    DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - GetTime() = %r\n", Status ));
  }

  //
  // Generate the random nonce...
  if (!EFI_ERROR( Status ))
  {
    Status = RngProtocol->GetRNG( RngProtocol,
                                  &gEfiRngAlgorithmSp80090Ctr256Guid,
                                  DFCI_RECOVERY_NONCE_SIZE,
                                  &NewChallenge->Nonce.Bytes[0] );
    DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - GetRNG() = %r\n", Status ));
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
  if (EFI_ERROR( Status ) && NewChallenge != NULL)
  {
    FreePool( NewChallenge );
    NewChallenge = NULL;
  }

  return Status;
} // GetRecoveryChallenge()


/**
  Take in a DER-encoded x509 cert buffer and a challenge
  and will attempt to encrypt it for transmission. Encrypted data
  buffer will be allocated and populated on success.

  @param[in]  Challenge     Pointer to an DFCI_RECOVERY_CHALLENGE to be encrypted.
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
  IN  CONST UINT8                *PublicKey,
  IN  UINTN                       PublicKeySize,
  OUT  UINT8                    **EncryptedData,
  OUT  UINTN                     *EncryptedDataSize
  )
{
  EFI_STATUS        Status;
  EFI_RNG_PROTOCOL  *RngProtocol;
  UINT8             ExtraSeed[RANDOM_SEED_BUFFER_SIZE];

  DEBUG(( DEBUG_INFO, __FUNCTION__"()\n" ));

  //
  // Check inputs...
  if (Challenge == NULL || PublicKey == NULL || EncryptedData == NULL || EncryptedDataSize == NULL)
  {
    DEBUG(( DEBUG_ERROR, __FUNCTION__" - NULL pointer provided!\n" ));
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
  Status = gBS->LocateProtocol( &gEfiRngProtocolGuid, NULL, &RngProtocol );
  DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - LocateProtocol(RNG) = %r\n", Status ));
  // Assuming we found the protocol, let's grab a seed.
  if (!EFI_ERROR( Status ))
  {
    Status = RngProtocol->GetRNG( RngProtocol,
                                  &gEfiRngAlgorithmSp80090Ctr256Guid,
                                  RANDOM_SEED_BUFFER_SIZE,
                                  &ExtraSeed[0] );
    DEBUG(( DEBUG_VERBOSE, __FUNCTION__" - GetRNG() = %r\n", Status ));
  }

  //
  // Now, we should be able to encrypt the data and be done with it.
  if (!EFI_ERROR( Status ))
  {
    if (!Pkcs1v2Encrypt( PublicKey,
                         PublicKeySize,
                         (UINT8*)Challenge,
                         sizeof( DFCI_RECOVERY_CHALLENGE ),
                         &ExtraSeed[0],
                         RANDOM_SEED_BUFFER_SIZE,
                         EncryptedData,
                         EncryptedDataSize ))
    {
      DEBUG(( DEBUG_ERROR, __FUNCTION__" - Failed to encrypt the challenge!\n" ));
      Status = EFI_ABORTED;
    }
  }

  return Status;
} // EncryptRecoveryChallenge()
