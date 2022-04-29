# Using MuSecureBootKeySelectorLib

## About

This document describes the overview of Secure Boot related changes from SecurityPkg and how `MuSecureBootKeySelectorLib`
and `SecureBootKeyStoreLib` are introduced to leverage the changes to support multiple key chains needed by OEMs.

### Involved Definitions

| Definitions | Definition Paths | Implementation Paths |
| --- | --- | --- |
| **UefiSecureBoot.h** | SecurityPkg/Include/UefiSecureBoot.h | N/A |
| **SecureBootVariableLib.h** | SecurityPkg/Include/Library/SecureBootVariableLib.h | SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf |
| **PlatformPKProtectionLib.h** | SecurityPkg/Include/Library/PlatformPKProtectionLib.h | SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf (*Required If Platforms Uses Variable Policy to Protect PK*) |
| **MuSecureBootKeySelectorLib.h** | MsCorePkg/Include/Library/MuSecureBootKeySelectorLib.h | MsCorePkg/Library/MuSecureBootKeySelectorLib/MuSecureBootKeySelectorLib.inf |
| **SecureBootKeyStoreLib.h** | MsCorePkg/Include/Library/SecureBootKeyStoreLib.h | **Platform Supplied** |

## Update for Secure Boot Variable Operation in SecurityPkg

With the improvement from `SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf`, this library begins to
support the Secure Boot related variables (db, dbt, dbx, KEK, PK, etc.) deletion and enrollment without much external
dependencies: `gEfiVariableWriteArchProtocolGuid` is required; if platform supports variable policy, `gEdkiiVariablePolicyProtocolGuid`
is also required.

The `Delete**` interfaces as well as the `EnrollFromInput` could be leveraged to delete or write individual secure boot
variable with time based authenticated payload. `IsSecureBootEnabled` is also added to query the secure boot state quickly.

### Interface `DeleteSecureBootVariables`

In addition, the newly introduced interface from `SecureBootVariableLib`, `DeleteSecureBootVariables`, streamline all
secure boot variable deletion in one function to facilitate the usage for platforms.

### Interface `SetSecureBootVariablesToDefault`

Another newly introduced interface from `SecureBootVariableLib`, `SetSecureBootVariablesToDefault`, streamline all
secure boot variable enrollment in one function to facilitate the usage for platforms. This function takes the platform
supplied `SECURE_BOOT_PAYLOAD_INFO` structure (defined in `SecurityPkg/Include/UefiSecureBoot.h`) that contains pointers
to target secure boot variables (db, dbt, dbx, KEK, PK, etc.), creates time based authenticated payload, disable PK
variable protection if needed, and write the variables. Specifically, the content from `SECURE_BOOT_PAYLOAD_INFO` must
be formatted to `EFI_SIGNATURE_LIST` structures.

### Interface `SecureBootCreateDataFromInput`

This interface is a helper function to convert DER formatted certificates to `EFI_SIGNATURE_LIST`. Callers can collect all
applicable DER formatted certificates' addresses and sizes in `SECURE_BOOT_CERTIFICATE_INFO` arrays. The collected array
can be passed into this interface to create a single, concatenated `EFI_SIGNATURE_LIST` in one call.

This function is especially useful for platforms that currently carry these certificates in DER format to populate
`SECURE_BOOT_PAYLOAD_INFO` structure.

## Creation of MuSecureBootKeySelectorLib

The introduction of this library instance is to support platforms that needs to install mutiple chains of secure boot keys
under various circumstances. i.e. Platform would like to support only 1st party signed applications vs. 1st party and 3rd
party signed application.

This library provides a set of `SECURE_BOOT_PAYLOAD_INFO`, each entry formatted to `EFI_SIGNATURE_LIST` structure. This
set of keys will be used with `MuSecureBootKeySelectorLib` to select which key set to be applied to the system. Along with
the signature list pointers, each key could provide a unicode string to denote its name, which might be used in other
modules for informational purpose.

The provided key sets should persist in the lifetime of its consuming module. The caller should not free the key store
buffer after usage.

*Note: If the platforms currently deliver any associated certificates in DER format, one should use `SecureBootCreateDataFromInput`
function to convert them into `EFI_SIGNATURE_LIST` format.*

## Usage of MuSecureBootKeySelectorLib

This library provides an interface to query or set the current status of secure boot variables in accordance to the
key sets provided in `MuSecureBootKeySelectorLib`.

## Platform Integration

### Platform Code

Platforms held the responsibility in authoring a library instance of `MuSecureBootKeySelectorLib` to provide certificates
or signature lists for secure boot operation usage.

Optional: if platforms do not use variable policy to protect `PK`, one should also author an instance of `PlatformPKProtectionLib`
to disarm the protection when PK is about to be updated.

### Platform DSC Change

Add the following items in the platform DSC file:

```bash
  SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
  # Optional
  PlatformPKProtectionLib|SecurityPkg/Library/PlatformPKProtectionLibVarPolicy/PlatformPKProtectionLibVarPolicy.inf
  MuSecureBootKeySelectorLib|MsCorePkg/Library/MuSecureBootKeySelectorLib/MuSecureBootKeySelectorLib.inf
  # Platform Code
  SecureBootKeyStoreLib|PlatformPkg/Library/SecureBootKeyStoreLibPlat/SecureBootKeyStoreLibPlat.inf
```
