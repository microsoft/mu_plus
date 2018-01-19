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


#ifndef _MS_FMP_PAYLOAD_HEADER_LIB_H__
#define _MS_FMP_PAYLOAD_HEADER_LIB_H__

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
IN       CONST VOID*     Header,
IN       CONST UINTN     FmpPayloadSize,
IN OUT   UINT32*         Size
);



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
);



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
);




#endif
