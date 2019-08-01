/** @file
DfciRequest.h

Defines the Request function to get the configuration from the server

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DFCI_REQUEST_H__
#define __DFCI_REQUEST_H__

#define MAX_DELAY_BEFORE_RETRY 24
#define MIN_DELAY_BEFORE_RETRY  1

/**
 * Process Http Recovery
 *
 * @param NetworkRequest     - Private data
 * @param Message            - Where to store the status message
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
ProcessDfciNetworkRequest (
    IN  DFCI_NETWORK_REQUEST    *NetworkRequest,
    OUT CHAR16                 **Message
  );


/**
 * Process Simple Http Recovery
 *
 * @param NetworkRequest     - Private data
 * @param Message            - Where to store the status message
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
ProcessSimpleNetworkRequest (
    IN  DFCI_NETWORK_REQUEST    *NetworkRequest,
    OUT CHAR16                 **Message
  );

#endif // __DFCI_REQUEST_H__

