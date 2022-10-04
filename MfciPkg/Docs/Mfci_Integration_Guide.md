# MFCI UEFI Integration Guide

## MfciPkg Provides

* a structure for encoding a manufacturer policy that is bound to a specific device and usage instance (they are 1-time use)
* a definition of the structure's digital signature format, authentication & authorization
* a test certificate and private key for development & testing
* a production Microsoft certificate and Enhanced Key Usage for authorizing Microsoft's cloud MFCI service
* a manufacturing-line-facing UEFI interface, based upon UEFI variables, that supports creation, installation, and
    removal of policies
* a UEFI-implementor-facing UEFI interface that reports the in-effect policy and notifications of policy changes
* a reference UEFI implementation that handles the authentication, target validation, and structure parsing,
translating signed policy blobs into actionable 64-bit policies
* examples of policy change handlers for [Secure Boot Clear](../MfciDxe/SecureBootClear.c) and [TPM Clear](../MfciDxe/TpmClear.c)
* tests including data and scripts that simulate the manufacturer unlock process

## UEFI Integration Overview

1. Include MfciPkg and its dependencies in your platform
2. Author code that sets the [MFCI Per-Device Targeting Variable Names](../Include/MfciVariables.h) with the
manufacturer, product, serial number, and optional OEM targeting values.  These variables MUST be set prior the EndOfDxe
event.
3. Where applicable to your platform, leverage MFCI's [PEI](../Include/Ppi/MfciPolicyPpi.h) and/or [DXE](../Include/Protocol/MfciProtocol.h)
interfaces to synchronously query the in-effect MFCI policy, or register callbacks for notification of MFCI policy changes

## Including MfciPkg and dependencies

### FDF

Add the following to your EDK2 Flash Descriptor File (FDF)

```INI
INF  MfciPkg/MfciDxe/MfciDxe.inf
```

```INI
INF  MfciPkg/MfciPei/MfciPei.inf
```

### DSC

#### Including Modules
MfciPkg provides a ```.dsc.inc``` that can be ```!include``` in your platform DSC.
An example follows:

```INI
!include MfciPkg/MfciPkg.dsc.inc
```

Mfci is still considered pre-production, so OPT_INTO_MFCI_PRE_PRODUCTION will need to be set at the top of the Dsc file
```INI
  DEFINE OPT_INTO_MFCI_PRE_PRODUCTION   = TRUE
```

Additionally, an instance of MfciRetrievePolicyLib will need to be specified. The default instance of MfciRetrievePolicyLibNull
must be overridden. Two other instances are available in the MfciPkg, or a custom version can be authored.
```INI
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaHob/MfciRetrievePolicyLibViaHob.inf
  MfciRetrievePolicyLib|MfciPkg/Library/MfciRetrievePolicyLibViaVariable/MfciRetrievePolicyLibViaVariable.inf
```

#### Including Pkcs Certificates
[Mfci Policy Blobs](Mfci_Structures.md) need to be digitally signed for the system to consume their data.  
The public portion of the Public/Private key needs to be included into for the MfciPkg to be able to verify
policy blobs. This is done through the PCDs PcdMfciPkcs7CertBufferXdr.

To help convert the Mfci Pkcs certificate into a PCD, the BinToPcd.py in MU_BASECORE can be used. 

Additionally, PcdMfciPkcs7RequiredLeafEKU needs to be filled out with the Extended Key Usage information. 
The below examples are from the Unit Test portion of this package. The Unit test can be followed to 
see how to convert a Cert into a binary pcd, and how to add the Extended Key Usage string into the PCD.

```INI
  DEFINE  MFCI_POLICY_EKU_TEST   = "1.3.6.1.4.1.311.45.255.255"

  # the unit test uses the test certificate that will also be used for testing end-to-end scenarios
  !include MfciPkg/Private/Certs/CA-test.dsc.inc
  gMfciPkgTokenSpaceGuid.PcdMfciPkcs7RequiredLeafEKU  |$(MFCI_POLICY_EKU_TEST)   # use the test version

```

### MfciPkg Dependencies

* Variable Policy
  * Variable policy is used to protect MFCI's security data (UEFI variables) from malicious tampering
* EDK2's SecureBootVariableLib, specifically:
  * ```DeleteSecureBootVariables()```
* EDK2's BaseCryptLib, specifically:
  * ```Pkcs7GetAttachedContent()```
  * ```Pkcs7Verify()```
  * ```VerifyEKUsInPkcs7Signature()```

## Populating Device Targeting Variables

An integrator must author code that sets the [MFCI Per-Device Targeting Variable Names](../Include/MfciVariables.h) with
the manufacturer, product, serial number, and optional OEM targeting values.  These variables MUST be set prior the
EndOfDxe event.

See the header file for type information on these variables, they must exactly match the contents of policies for
targeting to successfully match them.

## Take action based upon MFCI Policy & changes

The final piece of code integration is to enumerate the list of behaviors and actions to be supported, and activate
their code by calling either MFCI's [PEI](../Include/Ppi/MfciPolicyPpi.h) and/or [DXE](../Include/Protocol/MfciProtocol.h)
interfaces to synchronously query the in-effect MFCI policy, or register callbacks for notification of MFCI policy changes.

## State Machine

MfciPkg's public interface defines UEFI variable mailboxes for installing and removing policy blobs, see
[MFCI Per-Device Targeting Variable Names](../Include/MfciVariables.h)

A machine with no policy blob installed is deemed to have have a configuration policy value of 0x0 and is
said to be running in the default customer mode, typically the most secure state with all security enabled.

There are 2 groups of mailboxes prefixed with ```Current...``` and ```Next...```.  The ```Current...```
mailboxes represent the policy in effect for the current boot.  The ```Next...``` mailboxes are used for queuing
installation of a new policy.

## Boot Timeline Example

1. PEI
    1. PEI is considered too early in the boot root of trust to perform policy blob validation. A pre-validated, cached
        copy of configuration policy is available to PEI modules via the [PEI](../Include/Ppi/MfciPolicyPpi.h)
        interface. Policy will not change in the PEI phase, so a policy change notification is not part of the PEI
        interface. If a policy change occurs later in boot, the platform is guaranteed to reboot prior to the BDS phase.
2. DXE prior to StartOfBds
    1. Drivers that need security policy augmentation should both get the current pre-verified
    policy and register for notification of policy changes
    2. OEM-supplied code populates UEFI variables that communicate the Make, Model, & SN targeting
    information to MFCI policy driver
3. StartOfBds event
    1. The MFCI Policy Driver will...
        1. Check for the presence of a policy.  If present...
            1. Verify its signature, delete the policy on failure
            2. Verify its structure, delete the policy on failure
            3. If the resulting policy is different from the current policy, notify registered handlers,
            make the new policy active, reboot
        2. If a next policy is not present, and a pre-verified policy was used for this boot, roll
        nonces, delete the pre-verified policy, & notify registered handlers
