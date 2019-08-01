/** @file -- TpmPpProtocol.h
Defines a protocol which provides a function to confirm a
TPM Physical Presence Request.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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
