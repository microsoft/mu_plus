/**@file
SettingsManagerDxe.c

Entry code for Settings Manager

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SettingsManager.h"

DFCI_SETTING_ACCESS_PROTOCOL            mSystemSettingAccessProtocol = { SystemSettingAccessSet, SystemSettingAccessGet, SystemSettingsAccessReset };
DFCI_SETTING_PROVIDER_SUPPORT_PROTOCOL  mProviderProtocol            = { RegisterProvider };
DFCI_SETTING_PERMISSIONS_PROTOCOL       mPermissionProtocol          = { SystemSettingPermissionGetPermission, SystemSettingPermissionResetPermission, SystemSettingPermissionIdentityChange };
DFCI_AUTHENTICATION_PROTOCOL            *mAuthProtocol               = NULL;

typedef struct {
  CHAR8    *Id;
  CHAR8    *Value;
} INIT_TABLE_ENTRY;

static INIT_TABLE_ENTRY  mInitTable[] = {
  { DEVICE_ID_MANUFACTURER,  NULL },
  { DEVICE_ID_PRODUCT_NAME,  NULL },
  { DEVICE_ID_SERIAL_NUMBER, NULL }
};

// Settings manager does not support "Atomic" operations at this time.  That means
// the delayed response and LKG handler are ignored, and the settings cannot be
// undone.
//
// Apply Settings Protocol
DFCI_APPLY_PACKET_PROTOCOL  mApplySettingsProtocol = {
  DFCI_APPLY_PACKET_SIGNATURE,
  DFCI_APPLY_PACKET_VERSION,
  {
    0,
    0,
    0
  },
  ApplyNewSettingsPacket,
  SetSettingsResponse,             // Not used by settings manager -
  SettingsLKG_Handler              // Not supported by settings manager
};

/**
 * SetSettingsResponse
 *
 * Settings Manager doesn't support delayed response
 *
 * @param This
 * @param Data
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SetSettingsResponse (
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data
  )
{
  return EFI_SUCCESS;
}

/**
 *  Last Known Good handler
 *
 *  Not supported in Settings manager at this time..
 *
 * @param[in] This:            Apply Packet Protocol
 * @param[in] Operation
 *                        DISCARD   discards the in memory changes, and retores from NV STORE
 *                        COMMIT    Saves the current settings to NV Store
 *
 * @return EFI_STATUS EFIAPI
 */
EFI_STATUS
EFIAPI
SettingsLKG_Handler (
  IN  CONST DFCI_APPLY_PACKET_PROTOCOL  *This,
  IN        DFCI_INTERNAL_PACKET        *Data,
  IN        UINT8                       Operation
  )
{
  return EFI_SUCCESS;
}

/**
Notify function for running and acting on the requests (input, debug, etc)

@param[in]  Event   The Event that is being processed.
@param[in]  Context The Event Context.

**/
VOID
EFIAPI
SettingManagerOnStartOfBds (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  gBS->CloseEvent (Event);

  DEBUG_CODE_BEGIN ();
  // print registered  on debug builds
  DebugPrintProviderList ();
  DebugPrintGroups ();
  DEBUG_CODE_END ();

  // install setting access
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Context, // Image handle was stored as the context
                  &gDfciSettingAccessProtocolGuid,
                  &mSystemSettingAccessProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to Install DFCI Settings Access Protocol. %r\n", Status));
  }
}

VOID
PublishDeviceIdentifier (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       i;
  XmlNode     *List                        = NULL;
  XmlNode     *DeviceIdIdentifiersNode     = NULL;
  XmlNode     *DeviceIdIdentifiersListNode = NULL;
  CHAR8       *XmlString                   = NULL;
  UINTN       StringSize                   = 0;

  //
  // Populate Device Id Variable.
  //
  Status = DfciIdSupportGetManufacturer (&mInitTable[0].Value, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to obtain Manufacturer\n", __FUNCTION__));
    goto NO_XML;
  }

  Status = DfciIdSupportGetProductName (&mInitTable[1].Value, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to obtain Product Name\n", __FUNCTION__));
    goto NO_XML;
  }

  Status = DfciIdSupportGetSerialNumber (&mInitTable[2].Value, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to obtain Serial Number\n", __FUNCTION__));
    goto NO_XML;
  }

  Status = EFI_OUT_OF_RESOURCES;
  List   = New_DeviceIdPacketNodeList ();
  if (List == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to create new DeviceId Packet List Node\n", __FUNCTION__));
    goto NO_XML;
  }

  // Get SettingsPacket Node
  DeviceIdIdentifiersNode = GetDeviceIdPacketNode (List);
  if (DeviceIdIdentifiersNode == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to Get GetDeviceIdPacketNode Node\n"));
    goto NO_XML;
  }

  Status = AddDfciVersionNode (DeviceIdIdentifiersNode, DFCI_FEATURE_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to add Dfci Version node. Code = %r", __FUNCTION__, Status));
    goto NO_XML;
  }

  //
  // Get the Settings Node List Node
  //
  DeviceIdIdentifiersListNode = GetDeviceIdListNodeFromPacketNode (DeviceIdIdentifiersNode);
  if (DeviceIdIdentifiersListNode == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to Get DeviceId List Node from Packet Node\n"));
    goto NO_XML;
  }

  for (i = 0; i < (sizeof (mInitTable)/sizeof (INIT_TABLE_ENTRY)); i++) {
    Status = SetDeviceIdIdentifier (
               DeviceIdIdentifiersListNode,
               mInitTable[i].Id,
               mInitTable[i].Value
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to set %a node. Code = %r\n", mInitTable[i].Id, Status));
      goto NO_XML;
    }
  }

  // print the list
  DEBUG ((DEBUG_INFO, "PRINTING DEVICE ID XML - Start\n"));
  DebugPrintXmlTree (List, 0);
  DEBUG ((DEBUG_INFO, "PRINTING DEVICE ID  XML - End\n"));

  // now output as xml string

  Status = XmlTreeToString (List, TRUE, &StringSize, &XmlString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - XmlTreeToString failed.  %r\n", __FUNCTION__, Status));
    goto NO_XML;
  }

  // Save variable
  Status = gRT->SetVariable (DFCI_DEVICE_ID_VAR_NAME, &gDfciDeviceIdVarNamespace, DFCI_DEVICE_ID_VAR_ATTRIBUTES, StringSize, XmlString);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Failed to write Device Id Xml variable %r\n", __FUNCTION__, Status));
    goto NO_XML;
  }

  // Success
  DEBUG ((DEBUG_INFO, "%a - Device Id Settings Xml Var Set with data size: 0x%X\n", __FUNCTION__, StringSize));

NO_XML:
  //
  // free memory allocated
  //
  if (NULL != XmlString) {
    FreePool (XmlString);
  }

  if (NULL != List) {
    FreeXmlTree (&List);
  }

  for (i = 0; i < (sizeof (mInitTable)/sizeof (INIT_TABLE_ENTRY)); i++) {
    if (NULL != mInitTable[i].Value) {
      FreePool (mInitTable[i].Value);
    }
  }
}

/**
 * Install UefiDeviceId at ReadyToBoot before the late locking variables are locked.
 *
 */
VOID
EFIAPI
SettingsManagerOnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  PERF_CALLBACK_BEGIN (&gEfiEventReadyToBootGuid);

  // Check for Settings Provisioning
  Status = PopulateCurrentSettingsIfNeeded ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Populate Current Settings If Needed returned an error. %r\n", __FUNCTION__, Status));
  }

  // If DFCI is not enabled in the build, do not publish the Device Identifier, and ensure any previous
  // identifier has been deleted.
  if (FeaturePcdGet (PcdDfciEnabled)) {
    PublishDeviceIdentifier ();
  } else {
    // Ensure variable is not present
    DEBUG ((DEBUG_INFO, "%a - Dfci is disabled.  Not publishing the Device Identifier\n", __FUNCTION__));
    Status = gRT->SetVariable (DFCI_DEVICE_ID_VAR_NAME, &gDfciDeviceIdVarNamespace, DFCI_DEVICE_ID_VAR_ATTRIBUTES, 0, NULL);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to delete Device Id Xml variable %r\n", __FUNCTION__, Status));
    }
  }

  PERF_CALLBACK_END (&gEfiEventReadyToBootGuid);

  gBS->CloseEvent (Event);

  return;
}

/**
Pass thru function for using the Auth Protocol to get auth and token

@param[in]  SignedData      - Pointer to signed data
@param[in]  SignedDataLen   - Length of signed data
@param[in]  Signature       - Pointer to WIN_CERT_UEFI_GUID that contains the signature
@param[in,out] AuthToken - returned Auth Token.  Caller must allocate.  data only valid if success.

**/
EFI_STATUS
EFIAPI
CheckAuthAndGetToken (
  IN     UINT8            *SignedData,
  IN     UINTN            SignedDataLen,
  IN     WIN_CERTIFICATE  *Signature,
  IN OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  EFI_STATUS  Status;

  // get mAuthProtocol
  if (mAuthProtocol == NULL) {
    Status = gBS->LocateProtocol (
                    &gDfciAuthenticationProtocolGuid,
                    NULL,
                    (VOID **)&mAuthProtocol
                    );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - Failed to locate AuthProtocol.  Can't use check auth.  %r\n", __FUNCTION__, Status));
      mAuthProtocol = NULL;
      return Status;
    }
  }

  return mAuthProtocol->AuthWithSignedData (mAuthProtocol, SignedData, SignedDataLen, Signature, AuthToken);
}

/**
Pass thru function for using the Auth Protocol to dispose of an auth token
so it can no longer be used in the system.

@param[in]  AuthToken - Pointer to auth token to dispose of
**/
EFI_STATUS
EFIAPI
AuthTokenDispose (
  IN DFCI_AUTH_TOKEN  *AuthToken
  )
{
  if ((AuthToken == NULL) || (*AuthToken == DFCI_AUTH_TOKEN_INVALID)) {
    return EFI_SUCCESS;
  }

  // Can't get here if mAuthProtocol is NULL
  if (mAuthProtocol != NULL) {
    return mAuthProtocol->DisposeAuthToken (mAuthProtocol, AuthToken);
  }

  DEBUG ((DEBUG_ERROR, "%a - Can't dispose of auth token because no AuthProtocol. \n", __FUNCTION__));
  return EFI_NOT_READY;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

**/
EFI_STATUS
EFIAPI
Init (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT   InitEvent;
  EFI_STATUS  Status;

  // Install Setting Provider Support Protocol and Permission Protocol
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gDfciSettingsProviderSupportProtocolGuid,
                  &mProviderProtocol,
                  &gDfciSettingPermissionsProtocolGuid,
                  &mPermissionProtocol,
                  &gDfciApplySettingsProtocolGuid,
                  &mApplySettingsProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to Install DFCI Settings Provider Support/Permission Protocol/Settings Apply. %r\n", Status));
    goto EXIT;
  }

  //
  // Register notify function to print all settings and publish SettingsAccess on BdsEntry Event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SettingManagerOnStartOfBds,
                  ImageHandle, // set the context to the image handle
                  &gDfciStartOfBdsNotifyGuid,
                  &InitEvent
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for End of Dxe. %r\n", __FUNCTION__, Status));
  }

  //
  // Register notify function to re-publish Settings at ReadyToBoot so current settings can be placed in FACS.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SettingsManagerOnReadyToBoot,
                  ImageHandle, // set the context to the image handle
                  &gEfiEventReadyToBootGuid,
                  &InitEvent
                  );

  if (InitEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Create Event Ex for Ready to Boot failed\n", __FUNCTION__));
  }

EXIT:
  return Status;
}
