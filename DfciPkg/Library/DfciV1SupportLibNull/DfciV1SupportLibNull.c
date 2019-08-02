/** @file
DfciV1SupportLibNull.c

NULL Library for V1Upport

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

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

    DEBUG((DEBUG_INFO, "%a: Called - NULL library returns NULL\n", __FUNCTION__));
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

    DEBUG((DEBUG_INFO, "%a: Called - NULL library returns NULL\n", __FUNCTION__));
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

    DEBUG((DEBUG_INFO, "%a: Called - Returning standard Id\n", __FUNCTION__));
    return Id;
}
