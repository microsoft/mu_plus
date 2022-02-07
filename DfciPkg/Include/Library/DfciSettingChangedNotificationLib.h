/** @file
DfciSettingChangedNotificationLib.h

Allows reporting of a potential reboot when a setting has been changed.

Copyright (c) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
 * Process Setting Changed    - Called for any setting that has changed.  Not called when a setting
 *                              has not changed.  The setting value is also supplied.
 *
 * @param SettingId           - Setting that was changed
 * @param AuthToken           - Auth token to apply the setting
 * @param Type                - Type of setting
 * @param ValueSize           - Size of the new value
 * @param Value               - Pointer to a datatype defined by the Type
 * @param Flags               - Flags from Setting provider
 *
 * @return EFI_SUCCESS        - Notification acknowledged
 *         other                Error occurred processing the notification
 */
EFI_STATUS
EFIAPI
DfciSettingChangedNotification (
  IN DFCI_SETTING_ID_STRING  Id,
  IN CONST DFCI_AUTH_TOKEN   *AuthToken,
  IN DFCI_SETTING_TYPE       Type,
  IN UINTN                   ValueSize,
  IN CONST VOID              *Value,
  IN DFCI_SETTING_FLAGS      Flags
  );

/**
 * Process Reset notification - called when DFCI is requesting a Reset
 *
 * Allows some implementations to delay the requested reset.  The default is to
 * reset the system immediately.
 *
 * @param NONE
 *
 * @return NONE         - Handle Reset Request
 **/
VOID
EFIAPI
DfciSettingChangedResetNotification (
  VOID
  );
