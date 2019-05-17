/** @file
DfciPrivate.h

Header file for Dfci Private Data

Copyright (c) 2018, Microsoft Corporation

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

#ifndef __DFCI_PRIVATE_H__
#define __DFCI_PRIVATE_H__

#include <Protocol/ServiceBinding.h>
#include <Protocol/Http.h>
#include <Protocol/Ip4Config2.h>

#include "DfciUtility.h"

typedef struct _DFCI_NETWORK_REQUEST DFCI_NETWORK_REQUEST;

/**
 *  Function to process the main logic of the Dfci network provider
 *
 * @param[in]  Network Request
 *
 * @retval EFI_SUCCESS -       Packet processed normally
 * @retval Error -             Error processing packet
 */
typedef
EFI_STATUS
(EFIAPI *DFCI_MAIN_LOGIC) (
    IN  DFCI_NETWORK_REQUEST   *NetworkRequest,
    OUT BOOLEAN                *DoneProcessing
);

/**
 * DFCI Private Data
 **/
struct _DFCI_NETWORK_REQUEST {
    //
    // Shared input from DfciMenu Initialization
    //
    CHAR8                              *ZeroTouchThumbprint;
    UINTN                               ZeroTouchThumbprintSize;
    CHAR8                              *OwnerThumbprint;
    UINTN                               OwnerThumbprintSize;
    CHAR8                              *HttpsThumbprint;
    UINTN                               HttpsThumbprintSize;
    CONST UINT8                        *HttpsCert;
    UINTN                               HttpsCertSize;
    CHAR8                              *RegistrationId;
    UINTN                               RegistrationIdSize;
    CHAR8                              *TenantId;
    UINTN                               TenantIdSize;
    DFCI_SYSTEM_INFORMATION             DfciInfo;
    DFCI_MAIN_LOGIC                     MainLogic;

    struct {
        //
        // Managed by ProcessDfciNetworkRequest
        //
        // This section is managed by MainLogic
        VOID                           *Registration;
        EFI_EVENT                       RegistrationEvent;
    } Main;

    struct {
        //
        // Input parameters for network operations
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_REQUEST)
        CHAR8                          *Url;
        UINTN                           UrlSize;
        CHAR8                          *BootstrapUrl;
        UINTN                           BootstrapUrlSize;
        CHAR8                          *Body;
        UINTN                           BodySize;
    } HttpRequest;

    struct {
        //
        // Output Parameters  Caller to free Body and Headers
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_RESPONSE)
        CHAR8                          *Body;
        UINTN                           BodySize;
        EFI_HTTP_HEADER                *Headers;
        UINTN                           HeaderCount;
    } HttpResponse;

    struct {
        //
        // Http Request Status
        //
        // This section is cleared by a CleanupNetworkRequest (CLEANUP_STATUS)
        CHAR8                          *HttpReturnCode;
        CHAR8                          *HttpMessage;
        EFI_HTTP_STATUS_CODE            HttpStatus;
    } HttpStatus;

    struct {
        //
        // NIC Section
        //
        // This section is managed by the function TryEachNICThenProcessRequest
        EFI_HANDLE                      NicHandle;
        EFI_SERVICE_BINDING_PROTOCOL   *HttpSbProtocol;
        EFI_HTTP_CONFIG_DATA            ConfigData;
        EFI_HTTP_PROTOCOL              *HttpProtocol;
        EFI_HANDLE                      HttpChildHandle;
        BOOLEAN                         DhcpRequested;

        //
        // IPv4 Specific section
        //
        EFI_HTTPv4_ACCESS_POINT         IPv4Node;

        //
        // IPv6 Specific section
        //
        EFI_HTTPv6_ACCESS_POINT         IPv6Node;

        //
        // Fields valid during DHCP delay
        //
        EFI_EVENT                       WaitEvent;
    } HttpNic;

};

extern  DFCI_NETWORK_REQUEST    mDfciNetworkRequest;

#endif // __DFCI_PRIVATE_H__