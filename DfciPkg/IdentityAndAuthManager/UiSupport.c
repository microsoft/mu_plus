/** @file
UiSupport.c

Each platform has its own UI.  All UI requests are handled between this module
and the platform supplied DfciUiSupportLib.

Copyright (C) Microsoft Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "IdentityAndAuthManager.h"

#include <Protocol/DfciAuthentication.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DfciUiSupportLib.h>

#define CERT_DETAILS_MAX_STRING_LEN  (1024)

/**
Takes strings to display as input for a DFCI Auth Dialog Box and returns user's choice as a BOOLEAN ouput
**/
static
BOOLEAN
GetConfirmationWithDfciAuthDialog (
  IN CHAR16               *CaptionText,
  IN CHAR16               *TitleText,
  IN CHAR16               *BodyText,
  IN CHAR16               *CertText,
  IN CHAR16               *ConfirmationText,
  IN CHAR16               *PwdErrText,
  IN CHAR16               *ThumbprintCode,
  IN BOOLEAN              Password,
  IN OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  BOOLEAN         Confirmation    = FALSE;
  CHAR16          *ErrorText      = L"";
  UINT8           MaxAttempt      = 3;
  CHAR16          *PasswordBuffer = NULL;
  EFI_STATUS      Status;
  DFCI_MB_RESULT  UiResult = DFCI_MB_IDNO;

  if ((AuthToken == NULL) ||
      (CaptionText == NULL) ||
      (TitleText == NULL) ||
      (BodyText == NULL) ||
      (CertText == NULL) ||
      (ConfirmationText == NULL) ||
      (PwdErrText == NULL) ||
      (ThumbprintCode == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Input Strings/AuthToken is NULL", __FUNCTION__));
    goto Exit;
  }

  DEBUG ((DEBUG_INFO, "%a: ThumbprintCode is %s\n", __FUNCTION__, ThumbprintCode));

  do {
    Status = DfciUiDisplayAuthDialog (
               TitleText,
               CaptionText,
               BodyText,
               CertText,
               ConfirmationText,
               ErrorText,
               Password,
               ThumbprintCode,
               &UiResult,
               &PasswordBuffer
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - PasswordDialog() failed! %r\n", __FUNCTION__, Status));
      break;
    } else if (UiResult == DFCI_MB_IDOK) {
      Status = mAuthProtocol.AuthWithPW (
                               &mAuthProtocol,
                               PasswordBuffer,
                               Password ? StrLen (PasswordBuffer) : 0,
                               AuthToken
                               );

      if (*AuthToken == DFCI_AUTH_TOKEN_INVALID) {
        DEBUG ((DEBUG_INFO, "%a: Invalid entry \n", __FUNCTION__));
        if (Password) {
          ErrorText = L"The certificate thumbprint is incorrect. Try again.\r -OR- \rThe password is incorrect. Try Again.";
        } else {
          ErrorText = L"The certificate thumbprint is incorrect. Try Again.";
        }

        if (*AuthToken != DFCI_AUTH_TOKEN_INVALID) {
          mAuthProtocol.DisposeAuthToken (&mAuthProtocol, AuthToken);
        }

        Confirmation = FALSE;
      } else {
        DEBUG ((DEBUG_INFO, "%a: ThumbprintCode is %s\n", __FUNCTION__, ThumbprintCode));
        Confirmation = TRUE;
        break;
      }
    } else if (UiResult == DFCI_MB_IDCANCEL) {
      Confirmation = FALSE;
      break;
    }

    MaxAttempt--;
  } while ((Confirmation == FALSE) && (MaxAttempt > 0));

  if ((MaxAttempt == 0) && (Confirmation == FALSE)) {
    DEBUG ((EFI_D_INFO, "Password attempt failed \n"));
    Status = DfciUiDisplayMessageBox (
               TitleText,
               PwdErrText,
               L"Activation attempt limit reached.",
               DFCI_MB_OK | DFCI_MB_STYLE_ALERT1,
               0,
               &UiResult
               );
  }

Exit:

  if (PasswordBuffer != NULL) {
    FreePool (PasswordBuffer);
  }

  return Confirmation;
}

/**
Takes strings to display as input for a password dialog box and returns user's choice as a boolean ouput
**/
static
BOOLEAN
GetConfirmationWithPasswordDialog (
  IN CHAR16               *CaptionText,
  IN CHAR16               *TitleText,
  IN CHAR16               *BodyText,
  IN CHAR16               *PwdErrText,
  IN OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  BOOLEAN         Confirmation    = FALSE;
  CHAR16          *ErrorText      = L"";
  UINT16          MaxAttempt      = 3;
  CHAR16          *PasswordBuffer = NULL;
  EFI_STATUS      Status;
  DFCI_MB_RESULT  UiResult = DFCI_MB_IDNO;

  if ((AuthToken == NULL) ||
      (CaptionText == NULL) ||
      (TitleText == NULL) ||
      (BodyText == NULL) ||
      (PwdErrText == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Input Strings/AuthToken is NULL", __FUNCTION__));
    goto Exit;
  }

  do {
    Status = DfciUiDisplayPasswordDialog (
               TitleText,
               CaptionText,
               BodyText,
               ErrorText,
               &UiResult,
               &PasswordBuffer
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a - PasswordDialog() failed! %r\n", __FUNCTION__, Status));
      break;
    } else if (UiResult == DFCI_MB_IDOK) {
      Status = mAuthProtocol.AuthWithPW (
                               &mAuthProtocol,
                               PasswordBuffer,
                               StrLen (PasswordBuffer),
                               AuthToken
                               );

      if (*AuthToken == DFCI_AUTH_TOKEN_INVALID) {
        ErrorText = L"The Password is incorrect. Try Again.";
      } else {
        Confirmation = TRUE;
        break;
      }
    } else if (UiResult == DFCI_MB_IDCANCEL) {
      Confirmation = FALSE;
      break;
    }

    MaxAttempt--;
  } while ((Confirmation == FALSE) && (MaxAttempt > 0));

  if ((MaxAttempt == 0) && (Confirmation == FALSE)) {
    DEBUG ((EFI_D_INFO, "Password attempt failed \n"));
    Status = DfciUiDisplayMessageBox (
               TitleText,
               PwdErrText,
               L"Password attempt limit reached.",
               DFCI_MB_OK | DFCI_MB_STYLE_ALERT1,
               0,
               &UiResult
               );
  }

  if (PasswordBuffer != NULL) {
    FreePool (PasswordBuffer);
  }

Exit:
  return Confirmation;
}

/**
Get Certificate Details formatted for the DFCI enroll dialog box.

Caller must free returned string unless it is NULL.

**/
static
CHAR16 *
EFIAPI
QueryCertificateDetails (
  IN UINT8  *TrustedCert,
  IN UINTN  TrustedCertSize
  )
{
  EFI_STATUS  Status;
  CHAR16      *StringToPrint = NULL;
  CHAR16      *Field;

  if (TrustedCert == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Data Parameter\n", __FUNCTION__));
    goto CLEANUP;
  }

  StringToPrint = AllocatePool (sizeof (CHAR16) * CERT_DETAILS_MAX_STRING_LEN);
  if (StringToPrint == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for StringToPrint\n", __FUNCTION__));
    goto CLEANUP;
  }

  *StringToPrint = L'\0';

  Status =  GetCertInfo (
              &mAuthProtocol,
              0,
              TrustedCert,
              TrustedCertSize,
              DFCI_CERT_SUBJECT,
              DFCI_CERT_FORMAT_CHAR16,
              (VOID **)&Field,
              NULL
              );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get cert info\n", __FUNCTION__));
    goto CLEANUP;
  }

  // Append Subject Name
  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"Subject:           ");
  if (Field != NULL) {
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, Field);
    FreePool (Field);
  } else {
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"UNKNOWN");
  }

  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"\n");

  // Append Issuer
  Status =  GetCertInfo (
              &mAuthProtocol,
              0,
              TrustedCert,
              TrustedCertSize,
              DFCI_CERT_ISSUER,
              DFCI_CERT_FORMAT_CHAR16,
              (VOID **)&Field,
              NULL
              );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get cert info\n", __FUNCTION__));
    goto CLEANUP;
  }

  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"Issuer:              ");
  if (Field != NULL) {
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, Field);
    FreePool (Field);
  } else {
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"UNKNOWN");
  }

  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"\n");

  // Append Thumbprint
  Status =  GetCertInfo (
              &mAuthProtocol,
              0,
              TrustedCert,
              TrustedCertSize,
              DFCI_CERT_THUMBPRINT,
              DFCI_CERT_FORMAT_CHAR16_UI,
              (VOID **)&Field,
              NULL
              );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get cert info\n", __FUNCTION__));
    goto CLEANUP;
  }

  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"Thumbprint:      ");
  if (Field != NULL) {
    StrnCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, Field, 56); // don't display the last two bytes
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"    ");
    FreePool (Field);
  } else {
    StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"UNKNOWN");
  }

  StrCatS (StringToPrint, CERT_DETAILS_MAX_STRING_LEN, L"\n");

CLEANUP:

  return StringToPrint;
}

/**
Takes the Cert Data as an input and returns what the user decides.

@param[in]    Password  Local user password is set or not.
@param[in]    TrustedCertSize  CertSize
@param[in]    TrustedCert      Cert
@param[out]   AuthToken        Returned Authtoken. Caller needs to DisposeAuthToken.

@return  UserConfirmation  Returns the boolean value of if the user chose to unenroll or not

**/
static
BOOLEAN
UiEnrollRequest (
  IN BOOLEAN           Password,
  IN UINTN             TrustedCertSize,
  IN UINT8             *TrustedCert,
  OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  CHAR16   *BodyText    = L"Device Firmware Configuration Interface(DFCI) will be activated on this device using the following certificate. \r";
  CHAR16   *CaptionText = L"Confirm activation of Device Firmware Configuration Interface";
  CHAR16   *ThumbprintString;
  CHAR16   *CertText;
  CHAR16   *ConfirmationText;
  CHAR16   *PwdErrText;
  CHAR16   *TitleText       = L"Activate Device Firmware Configuration Interface Mode";
  BOOLEAN  UserConfirmation = FALSE;
  CHAR16   ThumbprintCode[3];

  CertText = QueryCertificateDetails (TrustedCert, TrustedCertSize);
  if (CertText == NULL) {
    CertText    = AllocatePool (L'\0');
    CertText[0] = L'\0';
  }

  // Get the certificate Thumbprint to verify the last two bytes of what the user has entered
  ThumbprintString = NULL;
  GetCertInfo (
    &mAuthProtocol,
    0,
    TrustedCert,
    TrustedCertSize,
    DFCI_CERT_THUMBPRINT,
    DFCI_CERT_FORMAT_CHAR16_UI,
    (VOID **)&ThumbprintString,
    NULL
    );

  if ((ThumbprintString != NULL) && ((StrLen (ThumbprintString)) == SHA1_FINGERPRINT_DIGEST_STRING_SIZE_UI)) {
    StrCpyS (ThumbprintCode, sizeof (ThumbprintCode), ThumbprintString + (StrLen (ThumbprintString) - 2));
  }

  if (NULL == ThumbprintString) {
    FreePool (ThumbprintString);
  }

  PwdErrText = L"\rThe Maximum number of activation attempts has been reached. Device Firmware Configuration Interface has not been enabled on this device. \r";
  if (Password) {
    ConfirmationText = L"To confirm activation, enter the last two digits of certificate thumbprint and the UEFI settings password. Then click ok to activate DFCI on this Device.";
  } else {
    ConfirmationText = L"\rTo confirm activation, enter the last two digits of certificate thumbprint. Then click ok to activate DFCI on this Device. \r";
  }

  UserConfirmation = GetConfirmationWithDfciAuthDialog (
                       CaptionText,
                       TitleText,
                       BodyText,
                       CertText,
                       ConfirmationText,
                       PwdErrText,
                       ThumbprintCode,
                       Password,
                       AuthToken
                       );

  DEBUG ((DEBUG_INFO, "%a: Confirmation is %d\n", __FUNCTION__, UserConfirmation));

  // Free Pointers
  if (CertText != NULL) {
    FreePool (CertText);
  }

  return UserConfirmation;
}

/**
Takes the Cert Data as an input and returns what the user decides.

@param  IN  Password   Local user password is set or not.
@param  OUT AuthToken  Also returns a AuthToken if a password is set so the ClearDfci can use it. Caller needs to DisposeAuthToken.

@return  UserConfirmation  Returns the boolean value of if the user chose to unenroll or not

**/
static
BOOLEAN
UiUnenrollRequest (
  IN BOOLEAN           Password,
  OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  CHAR16          *BodyText;
  CHAR16          *CaptionText;
  CHAR16          *PwdErrText;
  EFI_STATUS      Status;
  CHAR16          *TitleText;
  DFCI_MB_RESULT  UiResult;
  BOOLEAN         UserConfirmation;

  UiResult         = DFCI_MB_IDNO;
  UserConfirmation = FALSE;

  CaptionText = L"Confirm deactivation of Device Firmware Configuration Mode";
  TitleText   = L"Deactivate Device Firmware Configuration Mode";

  if (Password != FALSE) {
    BodyText = L"\rA request to deactivate DFCI has been made on this device.\r\rChanges to UEFI settings on this device are protected by a local password. To complete the request to deactivate DFCI, please enter the UEFI settings password and click Ok. ";
    //
    // This is an unenroll request. There is no cert detail to scan.
    //
    PwdErrText       = L"\rThe Maximum number of invalid password attempts has been reached. Device Firmware Configuration (DFCI) has not been deactivated on this device. \r";
    UserConfirmation = GetConfirmationWithPasswordDialog (
                         CaptionText,
                         TitleText,
                         BodyText,
                         PwdErrText,
                         AuthToken
                         );
  } else {
    BodyText = L"\rA request to deactivate DFCI has been made on this device.\r\rTo complete the request to deactivate DFCI, click OK. Cancel terminates the request ";

    //
    // Display MessageBox and return user confirmation for deactivation.
    //
    Status = DfciUiDisplayMessageBox (
               TitleText,
               BodyText,
               CaptionText,
               DFCI_MB_OKCANCEL | DFCI_MB_STYLE_ALERT1 | DFCI_MB_DEFBUTTON2,
               0,
               &UiResult
               );

    if (EFI_ERROR (Status) != FALSE) {
      DEBUG ((DEBUG_ERROR, "%a - MessageBox() failed! %r\n", __FUNCTION__, Status));
    } else if (UiResult == DFCI_MB_IDOK) {
      UserConfirmation = TRUE;
    }
  }

  return UserConfirmation;
}

/**
  This routine is called by DFCI to prompt a local user to confirm certificate
  provisioning operations.

  @param  TrustedCert            Supplies a pointer to a trusted certificate.
  @param  TrustedCertSize        Supplies the size in bytes of the trusted
                                 certificate.
  @param  AuthToken              Supplies a pointer that will receive an
                                 authentication token.

  @return EFI_NOT_READY          Indicates that UI components are not available.
  @return EFI_ACCESS_DENIED      The user rejected the operation.
  @return EFI_SUCCESS            The user approved the operation.

**/
EFI_STATUS
EFIAPI
LocalGetAnswerFromUser (
  UINT8                *TrustedCert,
  UINTN                TrustedCertSize,
  OUT DFCI_AUTH_TOKEN  *AuthToken
  )
{
  BOOLEAN     Confirmation;
  EFI_STATUS  Status;

  Confirmation = FALSE;

  //
  // Try to prepare UI for use.
  //
  if (FALSE == DfciUiIsUiAvailable ()) {
    DEBUG ((DEBUG_ERROR, "%a: UI not ready!\n", __FUNCTION__));
    Status = EFI_NOT_READY;
    goto Cleanup;
  }

  //
  // Determine if a system password has been set. This is accomplished by trying
  // to authenticate with a NULL password. This only succeeds if no password has
  // been set.
  //
  Status = mAuthProtocol.AuthWithPW (&mAuthProtocol, NULL, 0, AuthToken);
  if ((Status != EFI_SUCCESS) && (Status != EFI_SECURITY_VIOLATION)) {
    DEBUG ((DEBUG_ERROR, "%a: error checking if password set! (%r)\n", __FUNCTION__, Status));
    Status = EFI_DEVICE_ERROR;
    goto Cleanup;
  }

  //
  // Prompt the user to confirm the enroll/unenroll operation. Note that the
  // above AuthWithPW returned success if the system password is not set.
  //
  if (Status != EFI_SUCCESS) {
    //
    // Password is set.
    //
    if (TrustedCertSize == 0) {
      Confirmation = UiUnenrollRequest (TRUE, AuthToken);
    } else {
      Confirmation = UiEnrollRequest (
                       TRUE,
                       TrustedCertSize,
                       TrustedCert,
                       AuthToken
                       );
    }
  } else {
    //
    // Password is not set.
    //
    if (TrustedCertSize == 0) {
      Confirmation = UiUnenrollRequest (FALSE, AuthToken);
    } else {
      Confirmation = UiEnrollRequest (
                       FALSE,
                       TrustedCertSize,
                       TrustedCert,
                       AuthToken
                       );
    }
  }

  //
  // Tell the caller if the user approved or rejected the request.
  //
  if (Confirmation) {
    DEBUG ((DEBUG_INFO, "%a: USER APPROVED\n", __FUNCTION__));
    Status = EFI_SUCCESS;
  } else {
    DEBUG ((DEBUG_INFO, "%a: USER REJECTED\n", __FUNCTION__));
    Status = EFI_ACCESS_DENIED;
  }

Cleanup:
  return Status;
}
