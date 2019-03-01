/** @file
DfciMenu.c

Device Firmware Configuration Interface - Menu to request update of firmware
configuration from configured portal.

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

#include <Uefi/UefiInternalFormRepresentation.h>

#include <Guid/DfciMenuGuid.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/DfciSettingsGuid.h>
#include <Guid/DfciEventGroup.h>

#include <DfciSystemSettingTypes.h>

#include <Protocol/DevicePath.h>
#include <Protocol/DfciAuthentication.h>
#include <Protocol/DfciSettingPermissions.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiConfigAccess.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DfciSettingsLib.h>
#include <Library/DfciUiSupportLib.h>
#include <Library/HiiLib.h>
#include <Library/JsonLiteParser.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/ZeroTouchSettingsLib.h>

#include <Settings/DfciSettings.h>

#include "DfciMenu.h"
#include "DfciRequest.h"
#include "DfciUsb.h"
#include "DfciUpdate.h"

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
    VENDOR_DEVICE_PATH             VendorDevicePath;
    EFI_DEVICE_PATH_PROTOCOL       End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()
HII_VENDOR_DEVICE_PATH                  mHiiVendorDevicePath = {
    {
        {
            HARDWARE_DEVICE_PATH,
            HW_VENDOR_DP,
            {
                (UINT8)(sizeof(VENDOR_DEVICE_PATH)),
                (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8)
            }
        },
        EFI_CALLER_ID_GUID
    },
    {
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        {
            (UINT8)(END_DEVICE_PATH_LENGTH),
            (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
        }
    }
};

// DFCI HII Package GUID: {  93a27eb8-233a-43d8-b81b-925a38a80988
#define DFCI_HII_PACKAGE_LIST_GUID                                                 \
{                                                                                  \
    0x93a27eb8, 0x233a, 0x43d8, {0xb8, 0x1b, 0x92, 0x5a, 0x38, 0xa8, 0x09, 0x88}   \
}

#define DFCI_MENU_SIGNATURE SIGNATURE_32 ('i', 'c', 'f', 'D')
#define DEFAULT_USB_FILE_NAME L"DfciUpdate.Dfi"
#define MAX_MSG_SIZE 350

//*---------------------------------------------------------------------------------------*
//* Global Variables                                                                      *
//*---------------------------------------------------------------------------------------*
STATIC DFCI_AUTHENTICATION_PROTOCOL           *mAuthenticationProtocol = NULL;
STATIC DFCI_MENU_CONFIGURATION                 mDfciMenuConfiguration;
STATIC DFCI_SETTING_PERMISSIONS_PROTOCOL      *mDfciSettingsPermissionProtocol = NULL;

//* Dfci Settings
STATIC DFCI_CERT_STRINGS                       mZeroTouchCert;    // ZeroTouch information
STATIC DFCI_CERT_STRINGS                       mOwnerCert;        // Owner information
STATIC DFCI_CERT_STRINGS                       mUserCert;         // User information
STATIC DFCI_IDENTITY_MASK                      mIdMask;           // Identities installed
STATIC CHAR8                                  *mDfciUrl = NULL;
STATIC UINTN                                   mDfciUrlSize;

//*---------------------------------------------------------------------------------------*
//* Hii Config Access functions                                                                  *
//*---------------------------------------------------------------------------------------*
EFI_STATUS
EFIAPI
ExtractConfig (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  CONST EFI_STRING                        Request,
    OUT EFI_STRING                             *Progress,
    OUT EFI_STRING                             *Results
    );

EFI_STATUS
EFIAPI
RouteConfig (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  CONST EFI_STRING                        Configuration,
    OUT EFI_STRING                             *Progress
    );

EFI_STATUS
EFIAPI
DriverCallback (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  EFI_BROWSER_ACTION                      Action,
    IN  EFI_QUESTION_ID                         QuestionId,
    IN  UINT8                                   Type,
    IN  EFI_IFR_TYPE_VALUE                     *Value,
    OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
    );

//
// Private internal data
//
typedef struct {
    UINTN                           Signature;
    EFI_HANDLE                      DriverHandle;
    EFI_HII_HANDLE                  HiiHandle;
    EFI_HII_CONFIG_ACCESS_PROTOCOL  ConfigAccess;
} DFCI_MENU_PRIVATE;

DFCI_MENU_PRIVATE  mDfciMenuPrivate = {
    DFCI_MENU_SIGNATURE,
    NULL,
    NULL,
    {
        ExtractConfig,
        RouteConfig,
        DriverCallback
    }
};

/**
 * ConvertToCHAR16 - Converts a CHAR8 string to a CHAR16 string.
 *
 * @param[in]  Test
 *
 * @return     String as CHAR16.  Caller is responsible for freeing returned
 *             string
 */
STATIC
CHAR16 *
ConvertToCHAR16 (
    IN CHAR8 *Text
  ) {

    EFI_STATUS  Status;
    CHAR16     *WideString;
    UINTN       WideStringLen;
    UINTN       WideStringSize;


    WideStringLen = AsciiStrnLenS (Text, MAX_MSG_SIZE) + 1;
    WideStringSize = (WideStringLen + 1) * sizeof(CHAR16);

    WideString = AllocatePool (WideStringSize);
    if (NULL == WideString) {
        return NULL;
    }

    Status = AsciiStrToUnicodeStrS (Text, WideString, WideStringLen);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to convert Ascii to Unicode. Code=%r\n"));
        FreePool (WideString);
        return NULL;
    }

    return WideString;
}

/**
 * SetStringEntry16 - sets the HiiString, and verify that it was accepted.
 *
 * @param[in]  IdName
 * @param[in]  StringValue
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
SetString16Entry (
    IN  EFI_STRING_ID  IdName,
    IN  CHAR16        *StringValue
  ) {

    EFI_STATUS  Status = EFI_SUCCESS;


    if (IdName != HiiSetString(mDfciMenuPrivate.HiiHandle,IdName, StringValue, NULL)) {
       DEBUG((DEBUG_ERROR, "%a - Failed to set string for %d: %s. \n", __FUNCTION__,  IdName, StringValue));
       Status = EFI_NO_MAPPING;
    }

    return Status;
}

/**
 * SetStringEntry - Converts the string to CHAR16, and calls SetString16Entry
 *
 * @param[in]  IdName
 * @param[in]  StringValue
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
SetStringEntry (
    IN  EFI_STRING_ID  IdName,
    IN  CHAR8         *StringValue
  ) {

    EFI_STATUS  Status = EFI_SUCCESS;
    CHAR16     *WideString;


    WideString = ConvertToCHAR16 (StringValue);
    Status = SetString16Entry (IdName, WideString);
    if (NULL != WideString) {
        FreePool (WideString);
    }

    return Status;
}

/**
 * Check if Dfci is enabled
 *
 * @return BOOLEAN      FALSE == No Dfci present
 *                      TRUE  == Dfci Present
 *
 *  Dfci requires more than just the OwnerKey installed.
 *
 */
STATIC
BOOLEAN
CheckIfDfciEnrolled (
    VOID
  ) {

    EFI_STATUS         Status;
    BOOLEAN            IsDfciMenuEnabled = FALSE;
    UINT8             *Cert;
    UINTN              CertSize;

    mDfciMenuConfiguration.DfciZeroTouchOptGrayOut = MENU_FALSE;
    mDfciMenuConfiguration.DfciZeroTouchCertAvailable = MENU_FALSE;
    mDfciMenuConfiguration.DfciZeroTouchEnabled = MENU_FALSE;
    mDfciMenuConfiguration.DfciOwnerEnabled = MENU_FALSE;
    mDfciMenuConfiguration.DfciUserEnabled = MENU_FALSE;

    Status =  mAuthenticationProtocol->GetEnrolledIdentities ( mAuthenticationProtocol, &mIdMask);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to get owner ids. %r\n", __FUNCTION__, Status));
        return FALSE;
    }

    Status = GetZeroTouchCertificate (&Cert, &CertSize);
    if (!EFI_ERROR(Status)) {
        mDfciMenuConfiguration.DfciZeroTouchCertAvailable = MENU_TRUE;
        FreePool (Cert);
        DEBUG((DEBUG_INFO, "%a: Zero Touch certificate is available\n", __FUNCTION__));
    }

    if (IS_ZTD_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_ZTD  , NULL, 0, &mZeroTouchCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get ZTD cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciZeroTouchEnabled = MENU_TRUE;
            IsDfciMenuEnabled = TRUE;
        }
    }
    if (IS_OWNER_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_OWNER , NULL, 0, &mOwnerCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get owner cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciOwnerEnabled = MENU_TRUE;
        }
    }
    if (IS_USER_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_USER , NULL, 0, &mUserCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get user cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciUserEnabled = MENU_TRUE;
            IsDfciMenuEnabled = TRUE;
        }
    }

    return IsDfciMenuEnabled;
}

/**
 * Get A Setting
 *
 * @param[in]   IdName
 * @param[in]   ValuePtr
 * @param[out]  ValueSize
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
GetASetting (
    IN  DFCI_SETTING_ID_STRING  IdName,
    IN  VOID                  **ValuePtr,
    OUT UINTN                  *ValueSize
  ) {

    EFI_STATUS      Status;


    *ValuePtr = NULL;
    *ValueSize = 0;
    Status = GetDfciSetting (IdName,
                             ValueSize,
                             NULL);
    if (EFI_ERROR(Status) && (EFI_BUFFER_TOO_SMALL != Status)) {
        DEBUG((DEBUG_ERROR, "%a - Unable to check %a. %r\n", __FUNCTION__, IdName, Status));
        *ValueSize = 0;
    }

    if (0 == *ValueSize) {
        DEBUG((DEBUG_ERROR, "%a - Invalid size for %a.\n", __FUNCTION__, IdName));
        goto CLEANUP_SETTING_EXIT;
    }

    *ValuePtr = (UINT8 *) AllocatePool (*ValueSize);
    if (NULL == *ValuePtr) {
        DEBUG((DEBUG_ERROR, "%a - Unable to allocate memory for %a. %r\n", __FUNCTION__, IdName, Status));
        goto CLEANUP_SETTING_EXIT;
    }

    Status = GetDfciSetting (IdName,
                             ValueSize,
                             *ValuePtr);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Unable to get %a. %r\n", __FUNCTION__, IdName, Status));
        goto CLEANUP_SETTING_EXIT;
    }

    if (*ValueSize == AsciiStrnLenS (*ValuePtr, *ValueSize)) { // No terminating NULL
        DEBUG((DEBUG_ERROR, "%a - No terminating NULL in URL string\n",  __FUNCTION__));
        goto CLEANUP_SETTING_EXIT;
    }

    return Status;

CLEANUP_SETTING_EXIT:
    if (*ValuePtr != NULL) {
        FreePool (*ValuePtr);
    }
    return EFI_NOT_FOUND;
}

/**
 * Get Dfci Parameters.
 *
 * @param    NONE
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
GetDfciParameters (
    VOID
  ) {

    STATIC BOOLEAN     AlreadyRun = FALSE;
    DFCI_CERT_STRINGS  Cert;
    DFCI_IDENTITY_MASK RecoveryMask;
    EFI_HII_HANDLE    *RecoveryHandle;
    UINTN              i;
    CHAR8             *Name;
    UINTN              NameSize;
    EFI_STATUS         Status;


    if (!AlreadyRun) {
        AlreadyRun = TRUE;

        //
        // If the Setup UI supports a reduce function capability, it needs to set the
        // the dynamic PCD PcdSetupUiReducedFunction.  This prevent changing the OPT IN
        // state unless the local user has permission.
        //
        if (PcdGetBool(PcdSetupUiReducedFunction)) {
            mDfciMenuConfiguration.DfciZeroTouchOptGrayOut = MENU_TRUE;
            DEBUG((DEBUG_INFO, "%a: Reduced function DFci Menu\n", __FUNCTION__));
        }

        mDfciMenuConfiguration.DfciHttpRecoveryEnabled = MENU_FALSE;
        mDfciMenuConfiguration.DfciRecoveryEnabled = MENU_FALSE;
        //
        // Populate Cert information
        //
        if (mZeroTouchCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_ZTD_SUBJECT_FIELD), mZeroTouchCert.SubjectString);
        }

        if (mZeroTouchCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_ZTD_THUMBPRINT_FIELD), mZeroTouchCert.ThumbprintString);
        }

        if (mOwnerCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_OWNER_SUBJECT_FIELD), mOwnerCert.SubjectString);
        }

        if (mOwnerCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_OWNER_THUMBPRINT_FIELD), mOwnerCert.ThumbprintString);
        }

        if (mUserCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_USER_SUBJECT_FIELD), mUserCert.SubjectString);
        }

        if (mUserCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_USER_THUMBPRINT_FIELD), mUserCert.ThumbprintString);
        }

        //
        // Check if hard unenroll is enabled
        //
        RecoveryMask = 0;
        Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__DFCI_RECOVERY, &RecoveryMask);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get permission for recovery %r\n", __FUNCTION__, Status));
        }

        DEBUG((DEBUG_INFO, "%a - mIdMask=%x, RecoveryMask=%x\n", __FUNCTION__, mIdMask, RecoveryMask));
        RecoveryMask &= mIdMask;

        if (RecoveryMask == 0) {
            DEBUG((DEBUG_INFO, "%a - No Identities have DFCI Recovery Permissions\n",  __FUNCTION__));
        }

        // Insure there is at least one certificate for any of the Id's that have recovery permission
        for (i = DFCI_IDENTITY_SIGNER_OWNER; i >= DFCI_IDENTITY_SIGNER_ZTD; i>>=1) {
            if (i & RecoveryMask) {
                Status =  mAuthenticationProtocol->GetCertInfo(mAuthenticationProtocol, i, NULL, 0, &Cert);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "%a - Unable to get the cert info for %x. %r\n", __FUNCTION__, i, Status));
                } else {
                    RecoveryHandle = HiiGetHiiHandles(&gDfciRecoveryFormsetGuid);
                    if (RecoveryHandle != NULL) {
                        mDfciMenuConfiguration.DfciRecoveryEnabled = MENU_TRUE;
                        DEBUG((DEBUG_INFO, "Dfci Recovery is enabled\n"));
                        FreePool (RecoveryHandle);
                    }
                    if (Cert.SubjectString != NULL) {
                        FreePool(Cert.SubjectString);
                    }
                    if (Cert.IssuerString != NULL) {
                        FreePool(Cert.IssuerString);
                    }
                    if (Cert.ThumbprintString != NULL) {
                        FreePool(Cert.ThumbprintString);
                    }
                    break;
                }
            }
        }

        RecoveryMask = 0;
        Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__ZTD_RECOVERY, &RecoveryMask);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get permission for recovery %r\n", __FUNCTION__, Status));
        }

        if (RecoveryMask != 0) {
            mDfciMenuConfiguration.DfciRecoveryEnabled = MENU_TRUE;
            DEBUG((DEBUG_INFO, "%a - Ztd Recovery enabled\n",  __FUNCTION__));
        }

        Status = GetASetting (DFCI_SETTING_ID__DFCI_URL, (VOID **) &mDfciUrl, &mDfciUrlSize);
        if (!EFI_ERROR(Status) && (mDfciUrlSize >= 1)) {
            mDfciMenuConfiguration.DfciHttpRecoveryEnabled = MENU_TRUE;
            SetStringEntry (STRING_TOKEN(STR_DFCI_URL_FIELD), mDfciUrl);
            DEBUG((DEBUG_INFO, "Dfci Http Recovery is enabled\n"));
        }

        Status = GetASetting (DFCI_SETTING_ID__MDM_FRIENDLY_NAME, (VOID **) &Name, &NameSize);
        if (!EFI_ERROR(Status) && (NameSize >= 1)) {
            mDfciMenuConfiguration.DfciFriendlyName = MENU_TRUE;
            SetStringEntry (STRING_TOKEN(STR_DFCI_MDM_FRIENDLY_NAME), Name);
            DEBUG((DEBUG_INFO, "Dfci MDM.FriendlyName is enabled\n"));
        }

        Status = GetASetting (DFCI_SETTING_ID__MDM_TENANT_NAME, (VOID **) &Name, &NameSize);
        if (!EFI_ERROR(Status) && (NameSize >= 1)) {
            mDfciMenuConfiguration.DfciTennantName = MENU_TRUE;
            SetStringEntry (STRING_TOKEN(STR_DFCI_MDM_TENANT_NAME), Name);
            DEBUG((DEBUG_INFO, "Dfci MDM.Tenant is enabled\n"));
        }
    }

    return EFI_SUCCESS;
}

/**
*  This function is the main entry of the Dfci Menu application.
*
* @param[in]   ImageHandle
* @param[in]   SystemTable
*
**/
EFI_STATUS
EFIAPI
DfciMenuEntry(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
  ) {

    EFI_STATUS      Status;


    Status = gBS->LocateProtocol(&gDfciAuthenticationProtocolGuid,
                                 NULL,
                                 (VOID **)&mAuthenticationProtocol);
    if (EFI_ERROR(Status) || (mAuthenticationProtocol == NULL)) {
        DEBUG((DEBUG_ERROR, "%a -  DfciAuthentication protocol not available. %r\n", __FUNCTION__, Status));
        ASSERT(FALSE);  // Fatal error - There is a Depex for this protocol
        return FALSE;
    }

    // Get all Id's that have Dfci Recovery permission.
    Status = gBS->LocateProtocol(&gDfciSettingPermissionsProtocolGuid,
                                 NULL,
                                 (VOID **) &mDfciSettingsPermissionProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - DfciSettingPermissionsProtocolGuid not available. %r\n", __FUNCTION__, Status));
        ASSERT(FALSE); // Fatal error - again, there is a Depex for this protocol
        return FALSE;
    }

    if (!CheckIfDfciEnrolled ()) {        // Check if system is managed by DFCI
        DEBUG((DEBUG_INFO, "%a - Error getting Cert Information.\n", __FUNCTION__));
    }

    // Install Device Path Protocol and Config Access protocol to driver handle
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
        &mDfciMenuPrivate.DriverHandle,
        &gEfiDevicePathProtocolGuid,
        &mHiiVendorDevicePath,
        &gEfiHiiConfigAccessProtocolGuid,
        &mDfciMenuPrivate.ConfigAccess,
        NULL
        );

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Error on InstallMultipleProtocol. Code=%r\n", __FUNCTION__, Status));
    } else {
        //
        // Publish our HII data
        //
        mDfciMenuPrivate.HiiHandle = HiiAddPackages (
            &gDfciMenuFormsetGuid,
            mDfciMenuPrivate.DriverHandle,
            DfciMenuVfrBin,
            DfciMenuStrings,
            NULL
            );

        if (mDfciMenuPrivate.HiiHandle == NULL) {
            DEBUG((DEBUG_ERROR, "%a - Error on HiiAddPackages. Code=%r\n", __FUNCTION__, Status));

            Status = EFI_OUT_OF_RESOURCES;
        }
        if (!EFI_ERROR(Status)) {
            // Signal that DfciMenu is loaded and available.
            Status = gBS->InstallProtocolInterface (&mDfciMenuPrivate.DriverHandle,
                                                    &gDfciMenuFormsetGuid,
                                                     EFI_NATIVE_INTERFACE,
                                                     NULL);
        }
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a: Error during init - Uninstalling DfciMenu. Code=%r\n", __FUNCTION__, Status));
            gBS->UninstallMultipleProtocolInterfaces (
                 mDfciMenuPrivate.DriverHandle,
                &gEfiDevicePathProtocolGuid,
                &mHiiVendorDevicePath,
                &gEfiHiiConfigAccessProtocolGuid,
                &mDfciMenuPrivate.ConfigAccess,
                NULL
                );
            if (NULL != mDfciMenuPrivate.HiiHandle) {
                HiiRemovePackages(mDfciMenuPrivate.HiiHandle);
            }
        }
    }

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a: Dfci Menu Loaded.  There was an error along the way. Code=%r\n", __FUNCTION__, Status));
    } else {
        DEBUG((DEBUG_LOAD, "%a: Dfci Menu Loaded.\n", __FUNCTION__));
    }

    // Always load the menu.
    return EFI_SUCCESS;
}

/**
 * Display Message Box - Displays a message box with the status of the Dfci Request. If the
 *                       Dfci request appears normal, allow a restart to apply the new settings
 *
 * @param StatusIn
 * @param MessageText    Used when a specific message is required
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
DisplayMessageBox (
    EFI_STRING_ID MsgToken,
    IN EFI_STATUS StatusIn,
    IN CHAR16    *MessageText  OPTIONAL
  ) {

    UINT32                    MessageBoxType;
    EFI_STRING                pTitle;
    EFI_STRING                pCaption;
    EFI_STRING                pBody;
    EFI_STRING                pTmp;
    EFI_STATUS                Status;
    DFCI_MB_RESULT            SwmResult;


    MessageBoxType = DFCI_MB_OK;
    SwmResult = DFCI_MB_IDOK;
    pTitle   = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_TITLE), NULL);
    pCaption = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_CAPTION_FAIL), NULL);

    switch (StatusIn) {
    case EFI_SUCCESS:
        if (NULL != pCaption) {  // Free the "Fail" Caption
            FreePool (pCaption);
        }

        pCaption = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_CAPTION), NULL);
        pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, MsgToken, NULL);
        if ((NULL != pBody) && (NULL != MessageText)) {
            pTmp = AllocatePool(MAX_MSG_SIZE);
            if (NULL != pTmp) {
                UnicodeSPrint(pTmp, MAX_MSG_SIZE, pBody, MessageText);
                FreePool (pBody);
                pBody = pTmp;
            }
        }

        MessageBoxType = DFCI_MB_RESTART;
        break;

    case EFI_NOT_FOUND:
        pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NOT_FOUND), NULL);
        if ((NULL != pBody) && (NULL != MessageText)) {
            pTmp = AllocatePool(MAX_MSG_SIZE);
            if (NULL != pTmp) {
                UnicodeSPrint(pTmp, MAX_MSG_SIZE, pBody, MessageText);
                FreePool (pBody);
                pBody = pTmp;
            }
        }

        break;

    case EFI_NO_MEDIA:
        pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NO_MEDIA), NULL);
        break;

    default:
        pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_INTERNAL_ERROR), NULL);
        if (NULL != pBody) {
            pTmp = AllocatePool(MAX_MSG_SIZE);
            if (NULL != pTmp) {
                UnicodeSPrint(pTmp, MAX_MSG_SIZE, pBody, StatusIn);
                FreePool (pBody);
                pBody = pTmp;
            }
        }
        break;
    }

    if ((NULL == pTitle) || (NULL == pCaption) || (NULL == pBody)) {
        DEBUG((DEBUG_ERROR, "Invalid message parameters. pTitle=%p, pCaption=%p, pBody=%p\n", pTitle, pCaption, pBody));
    }

    Status = DfciUiDisplayMessageBox (pTitle,
                                      pBody,             // Dialog body text.
                                      pCaption,          // Dialog caption text.
                                      MessageBoxType,    // Show Restart button.
                                      0,                 // No timeout
                                      &SwmResult);       // Return result.
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "MessageBox failed. Code=%r\n", Status));
    }

    if (DFCI_MB_IDRESTART == SwmResult) {
        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }

    if (NULL != pTitle) {
        FreePool (pTitle);
    }

    if (NULL != pCaption) {
        FreePool (pCaption);
    }

    if (NULL != pBody) {
        FreePool (pBody);
    }

    return Status;
}

/**
 * Issue DfciRequest to the network
 *
 * @param   NONE
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
IssueDfciNetworkRequest (
    VOID
  ) {

    CHAR8       *DfciIdString;
    UINTN        DfciIdStringSize;
    CHAR8       *JsonString;
    UINTN        JsonStringSize;
    EFI_STATUS   Status;
    CHAR16      *Url;

    //
    // Start UI Spinner if one is present
    //
    EfiEventGroupSignal (&gDfciConfigStartEventGroupGuid);

    // Platform Late Locking event.  For now, just signal
    // ReadyToBoot().
    EfiEventGroupSignal (&gEfiEventPreReadyToBootGuid);

    JsonString = NULL;
    DfciIdString = NULL;
    //
    // Process request updates from the network
    //
    Status = BuildJsonRequest (&DfciIdString, &DfciIdStringSize);

    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error building Json Request. Code=%r\n", Status));
    } else {
        Status = DfciRequestJsonFromNETWORK (mDfciUrl,     mDfciUrlSize,
                                             DfciIdString, DfciIdStringSize,
                                            &JsonString,  &JsonStringSize);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error processing Dfci Network Request. Code=%r\n", Status));
        } else {
            DEBUG((DEBUG_INFO, "DfciNetwork Request processed normally\n"));
            UpdateDfciFromJson (JsonString, JsonStringSize);
        }
    }

    if (NULL != JsonString) {
        FreePool (JsonString);
    }

    if (NULL != DfciIdString) {
        FreePool (DfciIdString);
    }

    //
    // Stop UI Spinner
    //
    EfiEventGroupSignal (&gDfciConfigCompleteEventGroupGuid);

    //
    // Inform user that operation is complete
    //
    Url = ConvertToCHAR16 (mDfciUrl);

    DisplayMessageBox (STRING_TOKEN(STR_DFCI_MB_NEW_SETTINGS), Status, Url);

    if (NULL != Url) {
        FreePool (Url);
    }

    return Status;
}

/**
 * Issue DfciUsbRequest - load settings from a USB drive
 *
 * @param  NONE
 *
 * @return EFI_STATUS
 */
STATIC
EFI_STATUS
IssueDfciUsbRequest (
    VOID
  ) {

    EFI_STATUS    Status;
    CHAR16       *FileName;
    CHAR16       *FileName2;
    CHAR8        *JsonString;
    UINTN         JsonStringSize;
    //
    // Start UI Spinner if one is present
    //
    EfiEventGroupSignal (&gDfciConfigStartEventGroupGuid);

    // Platform Late Locking event.  For now, just signal
    // ReadyToBoot().
    EfiEventGroupSignal (&gEfiEventPreReadyToBootGuid);

    FileName = NULL;
    JsonString = NULL;

    //
    // Process request
    //
    Status = BuildUsbRequest (L".Dfi", &FileName);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error building Usb Request. Code=%r\n", Status));
    } else {
        Status = DfciRequestJsonFromUSB (FileName,
                                         &JsonString,
                                         &JsonStringSize);
        if (EFI_ERROR(Status)) {
            FileName2 = AllocateCopyPool (sizeof (DEFAULT_USB_FILE_NAME), DEFAULT_USB_FILE_NAME);
            if (NULL != FileName2) {
                Status = DfciRequestJsonFromUSB (FileName2,
                                                 &JsonString,
                                                 &JsonStringSize);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "Error loading backup file\n"));
                    FreePool (FileName2);
                } else {
                    FreePool (FileName);
                    FileName = FileName2;
                }
            }
        }

        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error processing Dfci Usb Request. Code=%r\n", Status));
        } else {
            DEBUG((DEBUG_INFO, "DfciUsb Request processed normally\n"));
            Status = UpdateDfciFromJson (JsonString, JsonStringSize);
        }
    }

    //
    // Stop UI Spinner
    //
    EfiEventGroupSignal (&gDfciConfigCompleteEventGroupGuid);

    //
    // Inform user that operation is complete
    //
    DisplayMessageBox (STRING_TOKEN(STR_DFCI_MB_NEW_SETTINGS), Status, FileName);

    if (NULL != JsonString) {
        FreePool (JsonString);
    }

    if (NULL != FileName) {
        FreePool (FileName);
    }

    return Status;
}

/**
 *  This function processes the results of changes in configuration.
 *
 *  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param in] Action              Specifies the type of action taken by the browser.
 *  @param[in]  QuestionId         A unique value which is sent to the original
 *                                 exporting driver so that it can identify the type
 *                                 of data to expect.
 *  @param[in]  Type               The type of value for the question.
 *  @param[in]  Value              A pointer to the data being sent to the original
 *                                 exporting driver.
 *  @param[out] ActionRequest      On return, points to the action requested by the
 *                                 callback function.
 *
 *  @retval EFI_SUCCESS            The callback successfully handled the action.
 *  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
 *                                 variable and its data.
 *  @retval EFI_DEVICE_ERROR       The variable could not be saved.
 *  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
 *                                 callback.
**/
EFI_STATUS
EFIAPI
DriverCallback (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  EFI_BROWSER_ACTION                     Action,
    IN  EFI_QUESTION_ID                        QuestionId,
    IN  UINT8                                  Type,
    IN  EFI_IFR_TYPE_VALUE                     *Value,
    OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
    ) {
    EFI_STATUS               Status;

    DEBUG ((DEBUG_INFO, "*Hii-Dfci* - Question ID=0x%08x Type=0x%04x Action=0x%04x Value=0x%lx\n", QuestionId, Type, Action, Value->u64));

    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    Status = EFI_UNSUPPORTED;

    switch (Action) {

        case EFI_BROWSER_ACTION_FORM_OPEN:
            switch (QuestionId) {
            case DFCI_MENU_INIT_QUESTION_ID:
                DEBUG((DEBUG_INFO," HttpRecovery is %d\n", mDfciMenuConfiguration.DfciHttpRecoveryEnabled));
                DEBUG((DEBUG_INFO," DfciRecovery is %d\n", mDfciMenuConfiguration.DfciRecoveryEnabled));
                break;

            case DFCI_MENU_INIT2_QUESTION_ID:
            case DFCI_MENU_INIT3_QUESTION_ID:
            default:
                break;
            }

            break;

        case EFI_BROWSER_ACTION_CHANGED:
            switch (QuestionId) {
            case DFCI_MENU_HTTP_UPDATE_NOW_QUESTION_ID:
                DEBUG((DEBUG_INFO," Http Recovery was selected\n"));
                IssueDfciNetworkRequest ();
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_USB_UPDATE_NOW_QUESTION_ID:
            case DFCI_MENU_USB_INSTALL_NOW_QUESTION_ID:
                DEBUG((DEBUG_INFO," Usb Recovery was selected\n"));
                IssueDfciUsbRequest ();
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_RECOVERY_INFO_QUESTION_ID:
            case DFCI_MENU_RECOVERY_NOW_QUESTION_ID:
                DEBUG((DEBUG_INFO," Full Recovery was selected\n"));
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_CONFIGURE_QUESTION_ID:
                DEBUG((DEBUG_INFO," Move to Configure Menu\n"));
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_ZUM_OPT_IN_QUESTION_ID:
                DEBUG((DEBUG_INFO," Opt In selected\n"));

                SetZeroTouchState (ZERO_TOUCH_OPT_IN);
                mDfciMenuConfiguration.DfciOptInChanged = MENU_TRUE;

                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
                Status = EFI_SUCCESS;
                DisplayMessageBox (STRING_TOKEN(STR_DFCI_MB_OPT_CHANGE), Status, NULL);
                break;

            case DFCI_MENU_ZUM_OPT_OUT_QUESTION_ID:
                DEBUG((DEBUG_INFO," Opt Out selected\n"));

                SetZeroTouchState (ZERO_TOUCH_OPT_OUT);
                mDfciMenuConfiguration.DfciOptInChanged = MENU_TRUE;

                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
                Status = EFI_SUCCESS;
                DisplayMessageBox (STRING_TOKEN(STR_DFCI_MB_OPT_CHANGE), Status, NULL);
                break;

            default:
                break;
            }

        default:
            break;
    }

    return Status;
}

/**
 *  This function processes the results of changes in configuration.
 *
 *  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
 *                                 format.
 *  @param[out]  Progress          A pointer to a string filled in with the offset of
 *                                 the most recent '&' before the first failing
 *                                 name/value pair (or the beginning of the string if
 *                                 the failure is in the first name/value pair) or
 *                                 the terminating NULL if all was successful.
 *
 *  @retval EFI_SUCCESS            The Results is processed successfully.
 *  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
 *  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
 *                                 driver.
 *
 **/
EFI_STATUS
EFIAPI
RouteConfig (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  CONST EFI_STRING                       Configuration,
    OUT EFI_STRING                             *Progress
    ) {
    EFI_STATUS    Status;

    if (Configuration == NULL || Progress == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (Configuration == NULL) {
        return EFI_UNSUPPORTED;
    }
    if (StrStr(Configuration, L"OFFSET") == NULL) {
        return EFI_UNSUPPORTED;
    }
    Status = EFI_SUCCESS;

    DEBUG((DEBUG_INFO, "%a: complete. Code = %r\n", __FUNCTION__, Status));
    return Status;
}

/**
 *  This function allows a caller to extract the current configuration for one
 *  or more named elements from the target driver.
 *
 *  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
 *  @param[in]  Request            A null-terminated Unicode string in
 *                                 <ConfigRequest> format.
 *  @param[out]  Progress          On return, points to a character in the Request
 *                                 string. Points to the string's null terminator if
 *                                 request was successful. Points to the most recent
 *                                 '&' before the first failing name/value pair (or
 *                                 the beginning of the string if the failure is in
 *                                 the first name/value pair) if the request was not
 *                                 successful.
 *  @param[out]  Results           A null-terminated Unicode string in
 *                                 <ConfigAltResp> format which has all values filled
 *                                 in for the names in the Request string. String to
 *                                 be allocated by the called function.
 *
 *  @retval EFI_SUCCESS            The Results is filled with the requested values.
 *  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
 *  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
 *  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
 *                                 driver.
 *
 **/
EFI_STATUS
EFIAPI
ExtractConfig (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
    IN  CONST EFI_STRING                        Request,
    OUT EFI_STRING                             *Progress,
    OUT EFI_STRING                             *Results
    ) {
    EFI_STATUS              Status;

    if (Progress == NULL || Results == NULL) {
        return EFI_INVALID_PARAMETER;
    }
    if (Request == NULL) {
        return EFI_UNSUPPORTED;
    }
    if (StrStr(Request, L"OFFSET") == NULL) {
        return EFI_UNSUPPORTED;
    }

    // The Request string may be truncated as it is long.  Insure \n gets out
    DEBUG((DEBUG_INFO, "%a: Request=%s\n", __FUNCTION__));
    DEBUG((DEBUG_INFO, "%s", Request));
    DEBUG((DEBUG_INFO, "\n"));

    Status = GetDfciParameters ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to get Dfci Parameters. Code=%r\n", Status));
    }

    if (HiiIsConfigHdrMatch(Request, &gDfciMenuFormsetGuid, L"DfciMenuConfig")) {
        Status = gHiiConfigRouting->BlockToConfig(
            gHiiConfigRouting,
            Request,
            (UINT8 *)&mDfciMenuConfiguration,
            sizeof(mDfciMenuConfiguration),
            Results,
            Progress
            );
        DEBUG((DEBUG_INFO, "%a: Size is %d, Code=%r\n", __FUNCTION__, sizeof(mDfciMenuConfiguration), Status));
    }

    Status = EFI_SUCCESS;
    DEBUG((DEBUG_INFO, "%a: complete. Code = %r\n", __FUNCTION__, Status));
    return Status;
}