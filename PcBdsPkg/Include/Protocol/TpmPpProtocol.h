/** @file -- TpmPpProtocol.h
Defines a protocol which provides a function to confirm a
TPM Physical Presence Request.

Copyright (c) 2015 - 2018, Microsoft Corporation

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

#ifndef _TPM_PP_PROTOCOL_H_
#define _TPM_PP_PROTOCOL_H_

extern EFI_GUID gTpmPpProtocolGuid;

//=================================================================================================
//
//  Protocol Definition
//
//=================================================================================================

typedef struct  _TPM_PP_PROTOCOL  TPM_PP_PROTOCOL;

/**
  Handles all the logic to prompt the user for confirmation of a
  TPM Physical Presence request.

  NOTE: Currently, we should NOT return from this function.
        The system will reset after performing the requested action.

  @param[in]  This

  @retval Any   Since we should never return from this function, any
                return value is an anomaly.

**/
typedef
EFI_STATUS
(EFIAPI *TPM_PP_USER_CONFIRM)(
    IN TPM_PP_PROTOCOL    *This
);


// TPM PP protocol structure
//
struct _TPM_PP_PROTOCOL
{
    TPM_PP_USER_CONFIRM                PromptForConfirmation;
};

#endif // _TPM_PP_PROTOCOL_H_
