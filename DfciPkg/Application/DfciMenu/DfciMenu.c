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
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>

#include <Settings/DfciSettings.h>

#include "DfciMenu.h"
#include "DfciRequest.h"

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

#define MAX_MSG_SIZE 200

//*---------------------------------------------------------------------------------------*
//* Global Variables                                                                      *
//*---------------------------------------------------------------------------------------*
static EFI_HII_HANDLE                          mHiiHandle;
static EFI_GUID                                mDfciPackageListGuid = DFCI_HII_PACKAGE_LIST_GUID;
static DFCI_AUTHENTICATION_PROTOCOL           *mAuthenticationProtocol = NULL;
static DFCI_MENU_CONFIGURATION                 mDfciMenuConfiguration;
static DFCI_SETTING_PERMISSIONS_PROTOCOL      *mDfciSettingsPermissionProtocol = NULL;

//* Dfci Settings
static DFCI_CERT_STRINGS                       mZeroTouchCert;    // ZeroTouch information
static DFCI_CERT_STRINGS                       mOwnerCert;        // Owner information
static DFCI_CERT_STRINGS                       mUserCert;         // User information
static DFCI_IDENTITY_MASK                      mIdMask;           // Identities installed
static CHAR8                                  *mDfciUrl = NULL;
static UINTN                                   mDfciUrlSize;
static CHAR16                                 *mHttpThumbprint = NULL;

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
 * SetStringEntry16 - sets the HiiString, and verify that it was accepted.
 *
 * @param IdName
 * @param StringValue
 *
 * @return EFI_STATUS
 */
EFI_STATUS
SetString16Entry (
    EFI_STRING_ID IdName,
    CHAR16 *StringValue
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
 * @param IdName
 * @param StringValue
 *
 * @return EFI_STATUS
 */
EFI_STATUS
SetStringEntry (
    EFI_STRING_ID  IdName,
    CHAR8         *StringValue
  )
{
    EFI_STATUS  Status = EFI_SUCCESS;
    CHAR16     *WideString;
    UINTN       WideStringLen;
    UINTN       WideStringSize;


    WideStringLen = AsciiStrnLenS (StringValue, MAX_MSG_SIZE) + 1;
    WideStringSize = (WideStringLen + 1) * sizeof(CHAR16);

    WideString = AllocatePool (WideStringSize);
    if (NULL == WideString) {
        return EFI_OUT_OF_RESOURCES;
    }

    Status = AsciiStrToUnicodeStrS (StringValue, WideString, WideStringLen);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Unable to conver Ascii to Unicode. Code=%r\n", Status));
    } else {
        Status = SetString16Entry (IdName, WideString);
    }

    FreePool (WideString);

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
BOOLEAN
CheckIfDfciEnrolled (
    VOID
  ) {
    EFI_STATUS         Status;
    BOOLEAN            IsDfciMenuEnabled = FALSE;

    mDfciMenuConfiguration.DfciZeroTouchEnabled = FALSE;
    mDfciMenuConfiguration.DfciOwnerEnabled = FALSE;
    mDfciMenuConfiguration.DfciUserEnabled = FALSE;

    Status =  mAuthenticationProtocol->GetEnrolledIdentities ( mAuthenticationProtocol, &mIdMask);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "%a - Failed to get owner ids. %r\n", __FUNCTION__, Status));
        return FALSE;
    }

    if (IS_ZTD_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_ZTD  , NULL, 0, &mZeroTouchCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get ZTD cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciZeroTouchEnabled = TRUE;
            IsDfciMenuEnabled = TRUE;
        }
    }
    if (IS_OWNER_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_OWNER , NULL, 0, &mOwnerCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get owner cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciOwnerEnabled = TRUE;
        }
    }
    if (IS_USER_IDENTITY_ENROLLED(mIdMask)) {
        Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_USER , NULL, 0, &mUserCert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get user cert. %r\n", __FUNCTION__, Status));
        } else {
            mDfciMenuConfiguration.DfciUserEnabled = TRUE;
            IsDfciMenuEnabled = TRUE;
        }
    }

    return IsDfciMenuEnabled;
}

/**
 * Get A Setting
 *
 * @param IdName
 * @param ValuePtr
 * @param ValueSize
 *
 * @return EFI_STATUS
 */
EFI_STATUS
GetASetting (
    IN  DFCI_SETTING_ID_STRING  IdName,
    IN  VOID                  **ValuePtr,
    OUT UINTN                  *ValueSize
    )
{
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
 *
 * @return EFI_STATUS
 */
EFI_STATUS
GetDfciParameters (
    VOID
  ) 
{
    EFI_STATUS         Status;
    UINTN              i;
    DFCI_IDENTITY_MASK DfciMask;
    DFCI_CERT_STRINGS  Cert;
    VOID              *dummy;
    static BOOLEAN     AlreadyRun = FALSE;


    if (!AlreadyRun) {
        AlreadyRun = TRUE;

        mDfciMenuConfiguration.DfciHttpRecoveryEnabled = FALSE;
        mDfciMenuConfiguration.DfciRecoveryEnabled = FALSE;
        //
        // Populate Cert information
        //
        if (mZeroTouchCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_ZTD_SUBJECT_FIELD), mZeroTouchCert.SubjectString);
        }
//        if (mZeroTouchCert.IssuerString != NULL) {
//            SetString16Entry(STRING_TOKEN(STR_DFCI_ZTD_ISSUER_FIELD), mZeroTouchCert.IssuerString);
//        }
        if (mZeroTouchCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_ZTD_THUMBPRINT_FIELD), mZeroTouchCert.ThumbprintString);
        }
        if (mOwnerCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_OWNER_SUBJECT_FIELD), mOwnerCert.SubjectString);
        }
//        if (mOwnerCert.IssuerString != NULL) {
//            SetString16Entry(STRING_TOKEN(STR_DFCI_OWNER_ISSUER_FIELD), mOwnerCert.IssuerString);
//        }
        if (mOwnerCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_OWNER_THUMBPRINT_FIELD), mOwnerCert.ThumbprintString);
        }
        if (mUserCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_USER_SUBJECT_FIELD), mUserCert.SubjectString);
        }
//        if (mUserCert.IssuerString != NULL) {
//            SetString16Entry(STRING_TOKEN(STR_DFCI_USER_ISSUER_FIELD), mUserCert.IssuerString);
//        }
        if (mUserCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_USER_THUMBPRINT_FIELD), mUserCert.ThumbprintString);
        }

        //
        // Check if hard unenroll is enabled
        //
        Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__DFCI_RECOVERY, &DfciMask);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get permission for recovery %r\n", __FUNCTION__, Status));
            return TRUE;
        }

        DEBUG((DEBUG_INFO, "%a - mIdMask=%x, DfciMask=%x\n", __FUNCTION__, mIdMask, DfciMask));
        mIdMask &= DfciMask;

        if (mIdMask == 0) {
            DEBUG((DEBUG_INFO, "%a - No Identities have DFCI Recovery Permissions\n",  __FUNCTION__));
            return TRUE;
        }

        // Insure there is at least one certificate for any of the Id's that have recovery permission
        for (i = DFCI_IDENTITY_SIGNER_OWNER; i != 0; i>>=1) {
            if (i & mIdMask) {
                Status =  mAuthenticationProtocol->GetCertInfo(mAuthenticationProtocol, i, NULL, 0, &Cert);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, "%a - Unable to get the cert info for %x. %r\n", __FUNCTION__, i, Status));
                } else {
                    Status = gBS->LocateProtocol (&gDfciRecoveryFormsetGuid, NULL, (VOID **) &dummy);
                    if (!EFI_ERROR(Status)) {
                        mDfciMenuConfiguration.DfciRecoveryEnabled = TRUE;
                        DEBUG((DEBUG_INFO, "Dfci Recovery is enabled\n"));
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

        Status = GetASetting (DFCI_SETTING_ID__DFCI_URL,      &mDfciUrl,   &mDfciUrlSize);
        if (EFI_ERROR(Status) || (mDfciUrlSize <= 1)) {
            goto CLEANUP_EXIT;
        }

        SetStringEntry (STRING_TOKEN(STR_DFCI_URL_FIELD), mDfciUrl);
        mDfciMenuConfiguration.DfciHttpRecoveryEnabled = TRUE;
        DEBUG((DEBUG_INFO, "Dfci Http Recovery is enabled\n"));
    }

    return EFI_SUCCESS;

CLEANUP_EXIT:
    if (NULL != mDfciUrl) {
        FreePool (mDfciUrl);
        mDfciUrl = NULL;
    }
    mDfciUrlSize = 0;
    return EFI_NOT_FOUND;
}

/**
  This function is the main entry of the Dfci Menu application.
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
                                 &mDfciSettingsPermissionProtocol);
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
        mHiiHandle = mDfciMenuPrivate.HiiHandle;
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
            if (NULL != mHiiHandle) {
                HiiRemovePackages(mHiiHandle);
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
 * @param UserStatusIn
 *
 * @return EFI_STATUS
 */
EFI_STATUS
DisplayMessageBox (
    IN EFI_STATUS StatusIn,
    IN UINT64     UserStatusIn
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

    if (EFI_ERROR(StatusIn) && (EFI_NOT_FOUND != StatusIn)) {
        pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_INTERNAL_ERROR), NULL);
        if (NULL != pBody) {
            pTmp = AllocatePool(MAX_MSG_SIZE);
            if (NULL != pTmp) {
                UnicodeSPrint(pTmp, MAX_MSG_SIZE, pBody, UserStatusIn, StatusIn);
            }
            FreePool (pBody);
            pBody = pTmp;
        }
    } else {
        switch (UserStatusIn) {
        case USER_STATUS_SUCCESS:
            if (NULL != pCaption) {  // Free the "Fail" Caption
                FreePool (pCaption);
            }
            pCaption = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_CAPTION), NULL);
            pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NEW_SETTINGS), NULL);
            MessageBoxType = DFCI_MB_RESTART;
            break;

        case USER_STATUS_NO_NIC:
            pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NO_NIC), NULL);
            break;

        case USER_STATUS_NO_MEDIA:
            pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NO_MEDIA), NULL);
            break;

        case USER_STATUS_NO_SETTINGS:
            pBody    = HiiGetString(mDfciMenuPrivate.HiiHandle, STRING_TOKEN(STR_DFCI_MB_NO_SETTINGS), NULL);
            break;

        default:
            pBody = NULL;
            break;
        }
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

    return Status;
}

/**
 * Issue DfciRequest
 *
 * @param
 *
 * @return EFI_STATUS
 */
EFI_STATUS
IssueDfciRequest (
    VOID
  ) {
    EFI_STATUS                      Status;
    UINT64                          UserStatus = USER_STATUS_SUCCESS;

    //
    // Start UI Spinner if one is present
    //
    EfiEventGroupSignal (&gDfciConfigStartEventGroupGuid);

    //
    // Process request
    //
    Status = DfciRequestProcess (mDfciUrl,   mDfciUrlSize,
                                 &UserStatus);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, "Error processing Dfci Request. Code=%r\n", Status));
    } else {
        DEBUG((DEBUG_INFO, "Dfci Request processed normally\n"));
    }

    //
    // Stop UI Spinner
    //
    EfiEventGroupSignal (&gDfciConfigCompleteEventGroupGuid);

    //
    // Inform user that operation is complete
    //
    DisplayMessageBox (Status, UserStatus);

    return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.
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
            default:
                break;
            }
            break;


        case EFI_BROWSER_ACTION_CHANGED:
            switch (QuestionId) {
            case DFCI_MENU_HTTP_UPDATE_NOW_QUESTION_ID:
                DEBUG((DEBUG_INFO," Http Recovery was selected\n"));
                IssueDfciRequest ();
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_USB_UPDATE_NOW_QUESTION_ID:
                DEBUG((DEBUG_INFO," Usb Recovery was selected\n"));

                DEBUG((DEBUG_ERROR," NOT IMPLEMENTED YET\n"));

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

                DEBUG((DEBUG_ERROR," NOT IMPLEMENTED YET\n"));

                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
                Status = EFI_SUCCESS;
                break;

            case DFCI_MENU_ZUM_OPT_OUT_QUESTION_ID:
                DEBUG((DEBUG_INFO," Opt Out selected\n"));

                DEBUG((DEBUG_ERROR," NOT IMPLEMENTED YET\n"));
          
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
                Status = EFI_SUCCESS;
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
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

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
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

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

