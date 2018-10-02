/** @file
DfciSettingsLib.h

Library provides a method for DXE drivers to access Dfci Settings.

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

#ifndef __DFCI_SETTINGS_LIB_H__
#define __DFCI_SETTINGS_LIB_H__

/**
Function to Get a Dfci Setting.
If the setting has not be previously set this function will return the default.  However it will
not cause the default to be set.

@param Id:          The DFCI_SETTING_ID_STRING of the Dfci
@param ValueSize:   IN=Size Of Buffer or 0 to get size, OUT=Size of returned Value
@param Value:       Ptr to a buffer for the setting to be returned.


@retval: Success - Setting was returned in Value
@retval: EFI_ERROR.  Settings was not returned in Value.
**/
EFI_STATUS
EFIAPI
GetDfciSetting
(
  IN      DFCI_SETTING_ID_STRING   Id,
  IN  OUT UINTN                   *ValueSize,
  OUT     VOID                    *Value
);

#endif  // __DFCI_SETTINGS_LIB_H__
