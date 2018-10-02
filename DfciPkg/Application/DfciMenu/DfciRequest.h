/** @file
DfciRequest.h

Defines the Request function to get the configuration from the server

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

#ifndef __DFCI_REQUEST_H__
#define __DFCI_REQUEST_H__

#define USER_STATUS_SUCCESS       0x0000000000000000L
#define USER_STATUS_NO_NIC        0x0000000000000001L
#define USER_STATUS_NO_MEDIA      0x0000000000000002L
#define USER_STATUS_NO_SETTINGS   0x0000000000000003L

/**
  Process the Dfci request

  Url                       Uniform Resource Locator
  UserStatus                Used when Status == NOT_FOUND to indicate
                            a specific message should be displayed

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
DfciRequestProcess(
    IN  CHAR8    *Url,
    IN  UINTN     UrlSize,

    // Ip Config Info: TBD
    // IPv4 or IPv6                  // IPv4 only right now
    // Local IP is DHCP, or fixed etc

    OUT UINT64   *UserStatus
    );

#endif // __DFCI_REQUEST_H__

