/** @file
DfciV1SupportLib.h

Contains translation from V1 setting ID's to V2 Setting Strings.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
 * Return V2 string from V1 Id String
 *
 * @param V1Id
 *
 * @return DFCI_SETTING_ID_STRING EFIAPI.  Return NULL if invalid V1 ID
 */
DFCI_SETTING_ID_STRING
EFIAPI
DfciV1TranslateString (DFCI_SETTING_ID_STRING V1Id);

/**
 * Return V2 string from V1 Id ENUM
 *
 * @param V1Id
 *
 * @return DFCI_SETTING_ID_STRING EFIAPI.  Return NULL if invalid V1 ID
 */
DFCI_SETTING_ID_STRING
EFIAPI
DfciV1TranslateEnum (DFCI_SETTING_ID_V1_ENUM V1Id);

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
DfciV1NumberFromId (DFCI_SETTING_ID_STRING Id);


