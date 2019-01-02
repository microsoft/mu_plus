/** @file
DfciUpdate.h

DfciUpdate parses the Json String and applies each element

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

#ifndef __DFCI_UPDATE_H__
#define __DFCI_UPDATE_H__

/**
 * BuildUsbRequest
 *
 * @param[in]   FileNameExtension - Extension for file name
 * @param[out]  filename   - Name of the file on USB to retrieve
 *
 **/
EFI_STATUS
EFIAPI
BuildUsbRequest (
    IN  CHAR16       *FileExtension,
    OUT CHAR16      **FileName
  );

/**
 *  Build Json Request.  For a network request, the current system information
 *                       needs to be provided.
 *
 * @param[in]    Dfci 			- Dfci Privates data for
 *  @param[out]  JsonString
 *  @param[out]  JsonStringSize
 *
 **/
EFI_STATUS
EFIAPI
BuildJsonRequest (
    OUT CHAR8             **JsonString,
    OUT UINTN              *JsonStringSize
  );

/**
 * UpdateDfciFromJson
 *
 * Parse the Json String and update DFCI with each element parsed.
 *
 * @param [in]  JsonString      - Dfci Update Json string
 * @param [in]  JsonStringSize  - Size of Json String
 *
 **/
EFI_STATUS
EFIAPI
UpdateDfciFromJson (
    IN  CHAR8   *JsonString,
    IN  UINTN    JsonStringSize
    );

#endif  // __DFCI_UPDATE_H__