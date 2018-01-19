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


#ifndef __CAPSULE_KEY_LIB__
#define __CAPSULE_KEY_LIB__

//
// Certificates for Capsule verification 
// This should be the CA not the actual leaf signer
//

#pragma pack(push)
#pragma pack(1)

typedef struct {
   CONST UINT8  *Key;                // Pointer to Certificate key
   CONST UINT32 KeySize;             // Size of key
} CAPSULE_VERIFICATION_CERTIFICATE;

typedef struct {
   CONST UINT8 NumberOfCertificates;                                   // Number of Certificates
   CONST CAPSULE_VERIFICATION_CERTIFICATE *CapsuleVerifyCertificates;  // Pointer to list of Certificates
} CAPSULE_VERIFICATION_CERTIFICATE_LIST;

#pragma pack(pop)

extern CONST CAPSULE_VERIFICATION_CERTIFICATE_LIST CapsuleVerifyCertificateList;

#endif //__CAPSULE_KEY_LIB__