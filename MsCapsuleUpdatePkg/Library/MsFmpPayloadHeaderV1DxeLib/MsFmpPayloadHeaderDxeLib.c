/**

Copyright (c) 2016, Microsoft Corporation

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

#include <Protocol/FirmwareManagement.h>
#include <Library/MsFmpPayloadHeaderLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpHelperLib.h>
#include <Library/MemoryAllocationLib.h>


//Define structure here so structure is not public
//
// identifier is used to make sure the data in the header is
// of this structure and version.  If the structure changes update
// the last digit.   
//
#define MS_PAYLOAD_HEADER_IDENTIFIER SIGNATURE_32 ('M', 'S', 'S', '1')


#pragma pack(1)

typedef struct {
  UINT32 Identifier;
  UINT32 HeaderSize;
  UINT32 FwVersion;
  UINT32 LowestSupportedVersion;
} MS_FMP_PAYLOAD_HEADER;

#pragma pack()


/**
function that returns the MS FMP Header Size

@param  Header            Header to evaluate
@param  FmpPayloadSize    Size of FMP payload
@param  Size              The size of the complete MS FMP header (if SUCCESS)

@return  EFI_SUCCESS on success.  
@return  EFI_INVALID_PARAMETER if Header is not a valid MS Fmp Header

**/
EFI_STATUS
EFIAPI
GetMsFmpHeaderSize(
IN       CONST VOID*  Header,
IN       CONST UINTN  FmpPayloadSize,
IN OUT   UINT32*      Size
)
{
  MS_FMP_PAYLOAD_HEADER* h = NULL;
  if (Header == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  h = (MS_FMP_PAYLOAD_HEADER*)Header;
  if ((UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) < (UINTN)h || \
      (UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) >= (UINTN)h + FmpPayloadSize || \
      h->HeaderSize < sizeof(MS_FMP_PAYLOAD_HEADER)) 
  {
    return EFI_INVALID_PARAMETER;
  }

  if (h->Identifier != MS_PAYLOAD_HEADER_IDENTIFIER) {
    return EFI_INVALID_PARAMETER;
  }

  *Size = h->HeaderSize;
  return EFI_SUCCESS;
}

/**
function that returns the Version

@param  Header            Header to evaluate
@param  FmpPayloadSize    Size of FMP payload
@param  Version           The firmware version described in the Ms Fmp Header

@return  EFI_SUCCESS on success.
@return  EFI_INVALID_PARAMETER if Header is not a valid MS Fmp Header

**/
EFI_STATUS
EFIAPI
GetMsFmpVersion(
IN       CONST VOID*     Header,
IN       CONST UINTN     FmpPayloadSize,
IN OUT   UINT32*         Version
)
{
  MS_FMP_PAYLOAD_HEADER* h = NULL;
  if (Header == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  h = (MS_FMP_PAYLOAD_HEADER*)Header;
  if ((UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) < (UINTN)h || \
      (UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) >= (UINTN)h + FmpPayloadSize || \
      h->HeaderSize < sizeof(MS_FMP_PAYLOAD_HEADER)) 
  {
    return EFI_INVALID_PARAMETER;
  }

  if (h->Identifier != MS_PAYLOAD_HEADER_IDENTIFIER) {
    return EFI_INVALID_PARAMETER;
  }

  *Version = h->FwVersion;
  return EFI_SUCCESS;
}


/**
function that returns the lowest supported version described in the 
MS Fmp Header.

@param  Header            Header to evaluate
@param  FmpPayloadSize    Size of FMP payload
@param  Version           The lowest supported version described in this MsFmpHeader

@return  EFI_SUCCESS on success.
@return  EFI_INVALID_PARAMETER if Header is not a valid MS Fmp Header

**/
EFI_STATUS
EFIAPI
GetMsFmpLowestSupportedVersion(
IN       CONST VOID*     Header,
IN       CONST UINTN     FmpPayloadSize,
IN OUT   UINT32*         Version
)
{
  MS_FMP_PAYLOAD_HEADER* h = NULL;
  if (Header == NULL)
  {
    return EFI_INVALID_PARAMETER;
  }

  h = (MS_FMP_PAYLOAD_HEADER*)Header;
  if ((UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) < (UINTN)h || \
      (UINTN)h + sizeof(MS_FMP_PAYLOAD_HEADER) >= (UINTN)h + FmpPayloadSize || \
      h->HeaderSize < sizeof(MS_FMP_PAYLOAD_HEADER)) 
  {
    return EFI_INVALID_PARAMETER;
  }

  if (h->Identifier != MS_PAYLOAD_HEADER_IDENTIFIER) {
    return EFI_INVALID_PARAMETER;
  }

  *Version = h->LowestSupportedVersion;
  return EFI_SUCCESS;
}

