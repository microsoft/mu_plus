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

/**
 * DFCI Private Data
 **/
typedef struct {
    //
    // Parameters
    //
    CHAR8                          *Url;
    UINTN                           UrlSize;
    CHAR8                          *DfciIdString;
    UINTN                           DfciIdStringSize;
    //
    // Common section  -- From here to the end cleared before each NIC attempt
    //
    EFI_HANDLE                      NicHandle;
    EFI_SERVICE_BINDING_PROTOCOL   *HttpSbProtocol;
    EFI_HTTP_CONFIG_DATA            ConfigData;
    EFI_HTTP_PROTOCOL              *HttpProtocol;
    EFI_HANDLE                      HttpChildHandle;
    BOOLEAN                         DhcpRequested;
    EFI_IP4_CONFIG2_PROTOCOL       *Ip4Config2;

    //
    // Fields valid during DHCP delay
    //
    EFI_EVENT                       WaitEvent;

    //
    // IPv4 Sspecific section
    //
    EFI_HTTPv4_ACCESS_POINT         IPv4Node;

    //
    // IPv6 Specific section
    //
    EFI_HTTPv6_ACCESS_POINT         IPv6Node;

} DFCI_PRIVATE_DATA;

extern DFCI_PRIVATE_DATA   mDfciPrivateData;


#endif // __DFCI_PRIVATE_H__