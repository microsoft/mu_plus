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


#ifndef __FMP_HELPER_LIB__
#define __FMP_HELPER_LIB__

/**
Locate all FMP instances installed in the system and return them in 
a null terminated list.  Caller must free the list when finished.

@param[out] EFI_FIRMWARE_MANAGEMENT_PROTOCOL*** List of protocol instances

@retval Status
**/
EFI_STATUS
EFIAPI
GetAllFmp(
OUT EFI_FIRMWARE_MANAGEMENT_PROTOCOL*** FmpList
);


/**
Locate a FMP protocol instance with a descriptor matching
the input parameters.  On success this function allocates
memory for the ImageDescriptor (the caller must free) and
will set the pointer to the FMP.

@param[in] EFI_GUID		 The guid of the FMP instance
@param[in] UINT8         The ImageIndex that matches the descriptor
@param[in optional] EFI_FIRMWARE_MANAGEMENT_PROTOCOL** NULL terminated list of Fmp Instances to ignore
@param[out] EFI_FIRMWARE_IMAGE_DESCRIPTOR**  Descriptor that matches
@param[out optional] EFI_FIRMWARE_MANAGEMENT_PROTOCOL** Protocol instance.  If NULL it will not be returned.


@retval Status			 Success when descriptor and fmp found.
@retval Error		     Not found or other error condition
**/
EFI_STATUS
EFIAPI
GetFmpAndDescriptor(
IN EFI_GUID*								        FmpGuid,
IN UINT8									        ImageIndex,
IN OPTIONAL EFI_FIRMWARE_MANAGEMENT_PROTOCOL**		ExcludeFmp,
OUT EFI_FIRMWARE_IMAGE_DESCRIPTOR**			        Descriptor,
OUT OPTIONAL EFI_FIRMWARE_MANAGEMENT_PROTOCOL**		        FmpInstance
);

#endif