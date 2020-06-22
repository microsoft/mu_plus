/** @file
  Declares the interface to register a callback for MFCI
  Policy change notifications

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL_H_
#define _MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL_H_

typedef struct _MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL
{
  MFCI_POLICY_CHANGE_CALLBACK    Callback;
} MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL;

#endif // _MFCI_POLICY_CHANGE_NOTIFY_PROTOCOL_H_
