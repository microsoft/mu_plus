/** @file
 *Device Firmware Configuration Interface - Menu to request update of firmware configuration from Microsoft Portal.

Copyright (c) 2018, Microsoft Corporation.

**/

#include <Uefi.h>

#include <Uefi/UefiInternalFormRepresentation.h>

#include <Guid/DfciMenuGuid.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/DfciSettingsGuid.h>
#include <Guid/DfciEventGroup.h>
#include <Guid/TlsAuthentication.h>

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
static DFCI_CERT_STRINGS                       mOwnerCert;        // Owner information
static DFCI_IDENTITY_MASK                      mIdMask;           // Identities installed
static CHAR8                                  *mDfciUrl = NULL;
static UINTN                                   mDfciUrlSize;
static VOID                                   *mHttpsCert = NULL;
static UINTN                                   mHttpsCertSize;
static CHAR16                                 *mHttpsThumbprint = NULL;

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
       DEBUG((DEBUG_ERROR, __FUNCTION__ " - Failed to set string for %d: %s. \n", IdName, StringValue));
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
        DEBUG((DEBUG_ERROR,"Unable to conver Ascii to Unicode. Code=%r\n", Status));
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
 */
BOOLEAN
CheckIfDfciEnrolled (
    VOID
  ) {
    EFI_STATUS         Status;


    Status =  mAuthenticationProtocol->GetEnrolledIdentities ( mAuthenticationProtocol, &mIdMask);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Failed to get owner ids. %r\n", Status));
        return FALSE;
    }
    if (!IS_OWNER_IDENTITY_ENROLLED(mIdMask)) {
        DEBUG((DEBUG_INFO, __FUNCTION__ " DFCI is not enabled\n"));
        return FALSE;
    }
    Status =  mAuthenticationProtocol->GetCertInfo ( mAuthenticationProtocol, DFCI_IDENTITY_SIGNER_OWNER , NULL, 0, &mOwnerCert);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Failed to get owner cert. %r\n", Status));
        return FALSE;
    }
    // Dfci is enabled.

    return TRUE;
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
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Unable to check %a. %r\n", IdName, Status));
        *ValueSize = 0;
    }

    if (0 == *ValueSize) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Invalid size for %a.\n", IdName));
        goto CLEANUP_SETTING_EXIT;
    }

    *ValuePtr = (UINT8 *) AllocatePool (*ValueSize);
    if (NULL == *ValuePtr) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Unable to allocate memory for %a. %r\n", IdName, Status));
        goto CLEANUP_SETTING_EXIT;
    }

    Status = GetDfciSetting (IdName,
                             ValueSize,
                             *ValuePtr);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - Unable to get %a. %r\n", IdName, Status));
        goto CLEANUP_SETTING_EXIT;
    }

    if (*ValueSize == AsciiStrnLenS (*ValuePtr, *ValueSize)) { // No terminating NULL
        DEBUG((DEBUG_ERROR, __FUNCTION__ " No terminating NULL in URL string\n"));
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

        mDfciMenuConfiguration.DfciHttpsRecoveryEnabled = FALSE;
        mDfciMenuConfiguration.DfciRecoveryEnabled = FALSE;

        //
        // Get Cert information
        //
        if (mOwnerCert.SubjectString != NULL) {
            SetString16Entry (STRING_TOKEN(STR_DFCI_SUBJECT_FIELD), mOwnerCert.SubjectString);
        }
        if (mOwnerCert.IssuerString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_ISSUER_FIELD), mOwnerCert.IssuerString);
        }
        if (mOwnerCert.ThumbprintString != NULL) {
            SetString16Entry(STRING_TOKEN(STR_DFCI_THUMBPRINT_FIELD), mOwnerCert.ThumbprintString);
        }

        //
        // Check if hard unenroll is enabled
        //
        Status = mDfciSettingsPermissionProtocol->GetPermission(mDfciSettingsPermissionProtocol, DFCI_SETTING_ID__DFCI_RECOVERY, &DfciMask);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "%a - Failed to get permission for recovery %r\n", __FUNCTION__, Status));
            return TRUE;
        }

        DEBUG((DEBUG_INFO, __FUNCTION__ " - mIdMask=%x, DfciMask=%x\n",mIdMask,DfciMask));
        mIdMask &= DfciMask;

        if (mIdMask == 0) {
            DEBUG((DEBUG_INFO, __FUNCTION__ " - No Identities have DFCI Recovery Permissions\n"));
            return TRUE;
        }

        // Insure there is at least one certificate for any of the Id's that have recovery permission
        for (i = DFCI_IDENTITY_SIGNER_OWNER; i != 0; i>>=1) {
            if (i & mIdMask) {
                Status =  mAuthenticationProtocol->GetCertInfo(mAuthenticationProtocol, i, NULL, 0, &Cert);
                if (EFI_ERROR(Status)) {
                    DEBUG((DEBUG_ERROR, __FUNCTION__ " - Unable to get the cert info for %x. %r\n", i, Status));
                } else {
                    Status = gBS->LocateProtocol (&gDfciRecoveryFormsetGuid, NULL, (VOID **) &dummy);
                    if (!EFI_ERROR(Status)) {
                        mDfciMenuConfiguration.DfciRecoveryEnabled = TRUE;
                        DEBUG((DEBUG_ERROR,"Dfci Recovery is enabled\n"));
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
        if (EFI_ERROR(Status) || (mDfciUrlSize == 0)) {
            goto CLEANUP_EXIT;
        }

        Status = GetASetting (DFCI_SETTING_ID__DFCI_URL_CERT, &mHttpsCert, &mHttpsCertSize);
        if (EFI_ERROR(Status) || (mHttpsCertSize == 0)) {
            goto CLEANUP_EXIT;
        }

        SetStringEntry (STRING_TOKEN(STR_DFCI_ISSUER_FIELD), mDfciUrl);
        Status = mAuthenticationProtocol->GetCertInfo (mAuthenticationProtocol, 0, mHttpsCert, mHttpsCertSize, &Cert);
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR, "Error cgetting thumbprint for HTTPS certificate. Status = %r\n", Status));
        } else {
            if (Cert.SubjectString != NULL) {
                FreePool(Cert.SubjectString);
            }
            if (Cert.IssuerString != NULL) {
                FreePool(Cert.IssuerString);
            }
            if (Cert.ThumbprintString != NULL) {
                SetString16Entry(STRING_TOKEN(STR_DFCI_HTTPS_THUMBPRINT_FIELD), Cert.ThumbprintString);
                FreePool(Cert.ThumbprintString);
            }
        }
        mDfciMenuConfiguration.DfciHttpsRecoveryEnabled = TRUE;
        DEBUG((DEBUG_ERROR,"Dfci Https Recovery is enabled\n"));
    }

    return EFI_SUCCESS;

CLEANUP_EXIT:
    if (NULL != mDfciUrl) {
        FreePool (mDfciUrl);
        mDfciUrl = NULL;
    }
    if (NULL != mHttpsCert) {
        FreePool (mHttpsCert);
        mHttpsCert = NULL;
    }
    mHttpsCertSize = 0;
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
        DEBUG((DEBUG_ERROR, __FUNCTION__ " -  DfciAuthentication protocol not available. %r\n", Status));
        ASSERT(FALSE);  // Fatal error - There is a Depex for this protocol
        return FALSE;
    }

    // Get all Id's that have Dfci Recovery permission.
    Status = gBS->LocateProtocol(&gDfciSettingPermissionsProtocolGuid,
                                 NULL,
                                 &mDfciSettingsPermissionProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR, __FUNCTION__ " - DfciSettingPermissionsProtocolGuid not available. %r\n", Status));
        ASSERT(FALSE); // Fatal error - again, there is a Depex for this protocol
        return FALSE;
    }

    if (!CheckIfDfciEnrolled ()) {        // Check if system is managed by DFCI
        DEBUG((DEBUG_INFO,__FUNCTION__ ": Dfci not enrolled.  Exiting.\n"));
        Status = EFI_NOT_FOUND;
        goto EXIT_DFCI;                   // Exit if not managed
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
        DEBUG((DEBUG_ERROR,__FUNCTION__ ": Error on InstallMultipleProtocol. Code=%r\n", Status));
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
            DEBUG((DEBUG_ERROR,__FUNCTION__ ": Error on HiiAddPackages. Code=%r\n", Status));

            Status = EFI_OUT_OF_RESOURCES;
        }
        mHiiHandle = mDfciMenuPrivate.HiiHandle;
        if (!EFI_ERROR(Status)) {
            // Signal SurfaceFrontPage that DfciMenu is loaded and available.
            Status = gBS->InstallProtocolInterface (&mDfciMenuPrivate.DriverHandle,
                                                    &gDfciMenuFormsetGuid,
                                                     EFI_NATIVE_INTERFACE,
                                                     NULL);
        }
        if (EFI_ERROR(Status)) {
            DEBUG((DEBUG_ERROR,__FUNCTION__ ": Error during init - Uninstalling DfciMenu. Code=%r\n", Status));
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

EXIT_DFCI:
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,__FUNCTION__ ": Unloading DfciMenu due to error. Code=%r\n",Status));
    } else {
        DEBUG((DEBUG_LOAD,__FUNCTION__ ": Dfci Menu Loaded.\n"));
    }

    return Status;
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
        DEBUG((DEBUG_ERROR,"Invalid message parameters. pTitle=%p, pCaption=%p, pBody=%p\n",pTitle,pCaption,pBody));
    }

    Status = DfciUiDisplayMessageBox (pTitle,
                                      pBody,             // Dialog body text.
                                      pCaption,          // Dialog caption text.
                                      MessageBoxType,    // Show Restart button.
                                      0,                 // No timeout
                                      &SwmResult);       // Return result.
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"MessageBox failed. Code=%r\n",Status));
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
                                 mHttpsCert, mHttpsCertSize,
                                 &UserStatus);
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Error processing Dfci Request. Code=%r\n", Status));
    } else {
        DEBUG((DEBUG_INFO,"Dfci Request processed normally\n"));
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
                DEBUG((DEBUG_ERROR," HttpsRecovery is %d\n", mDfciMenuConfiguration.DfciHttpsRecoveryEnabled));
                DEBUG((DEBUG_ERROR," SemmRecovery is %d\n",  mDfciMenuConfiguration.DfciRecoveryEnabled));
                break;
            default:
                break;
            }
            break;


        case EFI_BROWSER_ACTION_CHANGED:
            switch (QuestionId) {
            case DFCI_MENU_HTTPS_UPDATE_NOW_QUESTION_ID:
                DEBUG((DEBUG_ERROR," Https Recovery was selected\n"));
                IssueDfciRequest ();
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;
            case DFCI_MENU_USB_UPDATE_NOW_QUESTION_ID:
                DEBUG((DEBUG_ERROR," Usb Recovery was selected\n"));

                DEBUG((DEBUG_ERROR," NOT IMPLEMENTED YET\n"));

                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
                Status = EFI_SUCCESS;
                break;
            case DFCI_MENU_RECOVERY_NOW_QUESTION_ID:
                DEBUG((DEBUG_ERROR," Full Recovery was selected\n"));
                *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
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

    DEBUG((DEBUG_INFO,__FUNCTION__ " complete. Code = %r\n",Status));
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

    DEBUG((DEBUG_INFO,__FUNCTION__ " - Request=%s\n"));
    DEBUG((DEBUG_INFO,"%s",Request));
    DEBUG((DEBUG_INFO,"\n"));

    Status = GetDfciParameters ();
    if (EFI_ERROR(Status)) {
        DEBUG((DEBUG_ERROR,"Unable to get Dfci Parameters. Code=%r\n"));
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
        DEBUG((DEBUG_INFO,__FUNCTION__ " Size is %d, Code=%r\n",sizeof(mDfciMenuConfiguration),Status));
    }

    Status = EFI_SUCCESS;
    DEBUG((DEBUG_INFO,__FUNCTION__ " complete. Code = %r\n",Status));
    return Status;
}

