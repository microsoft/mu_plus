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

/**
 *  Dfci Request from Network
 *
 *  @param[in]  Url             A pointer to the EFI System Table.
 *  @param[in]  UrlSize
 *  @param[in]  DfciIdString      The Dfci Identity Json to send to server
 *  @param[in]  DfciIdStringSize  Size of the Dfci Identity Json
 *  @param[out] JsonStringSize    Where to store a pointer to JsonString
 *  @param[out] JsonStringSize    Size of the JsonString
 *
 *  @retval EFI_SUCCESS        The entry point is executed successfully.
 *  @retval other              Some error occurred.
 *
 *  Caller must free JsonString
 *
 **/
EFI_STATUS
EFIAPI
DfciRequestJsonFromNETWORK (
    IN  CHAR8    *Url,
    IN  UINTN     UrlSize,
    IN  CHAR8    *DfciIdString,
    IN  UINTN     DfciIdStringSize,
    OUT CHAR8   **JsonString,
    OUT UINTN    *JsonStringSize
    );

#endif // __DFCI_REQUEST_H__

