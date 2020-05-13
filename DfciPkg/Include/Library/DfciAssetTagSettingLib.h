/** @file
DfciAssetTagSettingLib.h

Interface to SettingProvider for pre SettingsManager access.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
 * Settings Provider AssetTagGet routine for pre SettingsManager access.
 *
 * @param ValueSize
 * @param Value
 *
 * @return EFI_STATUS
 *         EFI_SUCCESS           - Returns value
 *         EFI_INVALID_PARAMETER - Bad parameters
 *         EFI_BUFFER_TOO_SMALL  - Size of new Value is larger than ValueSize
 */
EFI_STATUS
EFIAPI
DfciGetAssetTag (
    IN  OUT   UINTN    *ValueSize,
    OUT       VOID     *Value);
