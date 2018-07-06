/** @file
 *Dfci V1 Id support

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

#include <Uefi.h>

#include <DfciSystemSettingTypes.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciV1SupportLib.h>

/**
 * Return V2 string from V1 Id
 *
 * @param V1Id
 *
 * @return DFCI_SETTING_ID_STRING EFIAPI.  Return NULL if invalid V1 ID
 */
DFCI_SETTING_ID_STRING
EFIAPI
DfciV1TranslateString (DFCI_SETTING_ID_STRING V1Id) {

    DEBUG((DEBUG_INFO,__FUNCTION__ " Called - NULL library returns NULL\n"));
    return NULL;
}

/**
 * Return V2 string from V1 Id
 *
 * @param V1Id
 *
 * @return DFCI_SETTING_ID_STRING EFIAPI.  Return NULL if invalid V1 ID
 */
DFCI_SETTING_ID_STRING
EFIAPI
DfciV1TranslateEnum (DFCI_SETTING_ID_V1_ENUM V1Id) {

    DEBUG((DEBUG_INFO,__FUNCTION__ " Called - NULL library returns NULL\n"));
    return NULL;
}

/**
 * Return V1 number string from V2 string Id
 *
 * @param V2 String Id
 *
 * @return DFCI_SETTING_ID_STRING EFIAPI.  Return string version of number
 *
 * Value returned is only valid until the next call to this function
 *
 */
DFCI_SETTING_ID_STRING
EFIAPI
DfciV1NumberFromId (DFCI_SETTING_ID_STRING Id) {

    DEBUG((DEBUG_INFO,__FUNCTION__ " Called - NULL library returns NULL\n"));
    return NULL;
}
