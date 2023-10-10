#Requires -RunAsAdministrator
# This script *unfortunately* requires ADMIN privileges due to New-SelfSignedCertificate

# Load the helper functions
. ./CertificateAndSignerHelpers.ps1

# This script relies on this script to format and sign Authenticated
$FormatAuthVar = "./FormatAuthenticatedVariable.py"

# Have to leave this outside the globals - since it's inaccessible during initialization of the hashtable
$Password = "password"
$DataFolder = "./AdvanceTest"
$TestDataName = "TestData"
$CertName = "Certs"

# Global Variables used throughout the script
$Globals = @{
    Certificate = @{
        Store = "Cert:\LocalMachine\My\"
        Organization = "contoso"
        Password = $Password
        SecurePassword = ConvertTo-SecureString $Password -Force -AsPlainText
        LifeYears = 10 # How long in the future should the Certificate be valid
    }
    Variable = @{
        Attributes = "NV,BS,RT,AT"
        Guid = "b3f4fb27-f382-4484-9b77-226b2b4348bb"
    }
    Layout = @{
        DataFolder = $DataFolder
        CertName = $CertName 
        CertificateFolder = "$DataFolder/$CertName"
        TestDataName = $TestDataName
        TestDataFolder = "$DataFolder/$TestDataName"
    }
}

# Clean up from a pervious run
Remove-Item $Globals.Layout.DataFolder -Recurse -Force -Confirm:$false
New-Item -Path $Globals.Layout.DataFolder -ItemType Directory
New-Item -Path $Globals.Layout.CertificateFolder -ItemType Directory
New-Item -Path $Globals.Layout.TestDataFolder -ItemType Directory


# =======================================================================================
# Test Name: DigestAlgorithms
# Test Description:
#   This test checks to see if authenticated variables supports Digest Algorithms of SHA384
#   and SHA512
#
#   Signers: 1 (End Entity)
#   Additional Certificates: 0 (Only End Entity Certificate included)
#
#   Expectation: Fail
# =============================================================================
# DigestAlgorithms
# =============================================================================
$VariableName = "MockVar"
$CommonName = "Digest Algorithms Support"
$TestGroup = "DigestAlgorithmsSupport"

# =============================================================================
# MockVar Trust Anchor
# =============================================================================
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}TrustAnchor"

$TrustAnchorParams = GetRootCertificateParams $KeyLength $CommonName "Trust Anchor"
$TrustAnchor = GenerateCertificate $TrustAnchorParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}IntermediateSHA384"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate SHA384" $TrustAnchor.Cert
$IntermediateParams["HashAlgorithm"] = "SHA384"
$IntermediateSHA384 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}IntermediateSHA512"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate SHA512" $TrustAnchor.Cert
$IntermediateParams["HashAlgorithm"] = "SHA512"
$IntermediateSHA512 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# =============================================================================
# Generates a certificate with a Hash Algorithm of SHA384
# =============================================================================
# Variable data *should* be different
$VariableData =  "Signed By SHA384 4K Certificate"
# Variable Prefix must be different
$VariablePrefix = "mSHA384${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "SignerSHA384" $IntermediateSHA384.Cert
$EndEntityParams["HashAlgorithm"] = "SHA384"
$Signer = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($Signer.CertInfo) $null @() "SHA384"
if (!$ret) {
    Exit
}

# =============================================================================
# Generates a certificate with a Hash Algorithm of SHA512
# =============================================================================
# Variable data *should* be different
$VariableData =  "Signed By SHA512 4K Certificate"
# Variable Prefix must be different
$VariablePrefix = "mSHA512${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "SignerSHA512" $IntermediateSHA512.Cert
$EndEntityParams["HashAlgorithm"] = "SHA512"
$Signer = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($Signer.CertInfo) $null @() "SHA512"
if (!$ret) {
    Exit
}

# =======================================================================================
# Test Name: TrustAnchor
# Test Description:
#   This test checks to see if authenticated variables supports chaining to the top level
#   issuer
#
#   Signers: 1 (End Entity)
#   Additional Certificates: 0 (Only End Entity Certificate included)
#
#   These variables both chain up to the same intermediate CA, and thus should have the same
#   trust anchor.
#
#   Expectation: Fail
# =============================================================================
# MockVar Trust Anchor
# =============================================================================
$VariableName = "MockVar"
$CommonName = "Trust Anchor Support"
$TestGroup = "TrustAnchorSupport"
# =============================================================================
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}RootCert"

$RootCertParams = GetRootCertificateParams $KeyLength $CommonName "Root Certificate"
$RootCert = GenerateCertificate $RootCertParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}Intermediate0"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate0" $RootCert.Cert
$Intermediate0 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# This is going to be our trust anchor
$VariablePrefix = "m${TestGroup}Intermediate1"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate1" $Intermediate0.Cert
$Intermediate1 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

$VariablePrefix = "m${TestGroup}Intermediate2"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate2" $Intermediate1.Cert
$Intermediate2 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# =============================================================================
#  Generates signature signed by one 2k end entity certificate
# 1 Additional Certficiate(s)
# =============================================================================
$KeyLength = 2048
# Variable data *should* be different
$VariableData =  "Signed by Signer 1 certificate and includes 1 additional certificate(s)"
# Variable Prefix must be different
$VariablePrefix = "mSigner1${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate2.Cert
$2KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($2KMockVar.CertInfo) $Intermediate1.CertPath @()
if (!$ret) {
    Exit
}

# =============================================================================
#  Generates signature signed by one 4k end entity certificate
# 1 Additional Certficiate(s)
# =============================================================================
$KeyLength = 4096
# Variable data *should* be different
$VariableData =  "Signed By ${KeyLength} certificate and includes 1 additional certificate(s)"
# Variable Prefix must be different
$VariablePrefix = "mSigner2${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate1.Cert
$4KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($4KMockVar.CertInfo) $Intermediate1.CertPath @()
if (!$ret) {
    Exit
}

# =======================================================================================
# Test Name: MultipleSigners
# Test Description:
#   This test checks to see if authenticated variables supports chaining to the top level
#   issuer
#
#   Signers: 1 (End Entity)
#   Additional Certificates: 0 (Only End Entity Certificate included)
#
#   Expectation: Fail
# TODO
# =============================================================================
# Multiple Signers Support
# =============================================================================
$VariableName = "MockVar"
$CommonName = "Multiple Signers Support"
$TestGroup = "MultipleSignersSupport"
# =============================================================================
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}RootCert"

$RootCertParams = GetRootCertificateParams $KeyLength $CommonName "Root Certificate"
$RootCert = GenerateCertificate $RootCertParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}Intermediate0"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate0" $RootCert.Cert
$Intermediate0 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

$VariablePrefix = "m${TestGroup}Intermediate1"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate1" $Intermediate0.Cert
$Intermediate1 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# =============================================================================
#  Generates signature signed by one 4k end entity certificate
# 1 Additional Certficiate(s)
# =============================================================================
$KeyLength = 4096
# Variable data *should* be different
$VariableData =  "Signed By Signer 1 certificate and intermediate certificate"
# Variable Prefix must be different
$VariablePrefix = "mSigner1${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer1" $Intermediate0.Cert
$IntialCertificate = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix

$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($IntialCertificate.CertInfo, $Intermediate0.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =============================================================================
#  Generates signature signed by one 4k end entity certificate
# 1 Additional Certficiate(s)
# =============================================================================
$KeyLength = 4096
# Variable data *should* be different
$VariableData =  "Signed By Signer 2 certificate and intermediate certificate"
# Variable Prefix must be different
$VariablePrefix = "mSigner2${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer2" $Intermediate0.Cert
$ReplacementCertificate = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($ReplacementCertificate.CertInfo, $Intermediate1.CertInfo) $null @()
if (!$ret) {
    Exit
}

# The above signing should work because they are signed by the same Intermediate Certificate
# which is weird

 
# =============================================================================
# Copy All the C arrays and variables to a single header and source file
# =============================================================================

$OutFile = Join-Path -Path $Globals.Layout.DataFolder -ChildPath "Exported.c"

# Include Headers
$SourceContents = @()
$SourceContents += "#include `"AuthData.h`"`n#include <Uefi.h>`n`n"
 
Get-ChildItem $Globals.Layout.DataFolder -Filter '*.c' -Recurse `
 | sort creationtime `
 | Where {$_.Name.substring($_.Name.length -3, 3)  -Match 'c'} `
 | Foreach-Object {
    $SourceContents += cat $_.FullName
}

$SourceContents | Out-File -Encoding Ascii -FilePath $OutFile

$OutFile = Join-Path -Path $Globals.Layout.DataFolder -ChildPath "Exported.h"
$HeaderContents = @()

# include guard
$HeaderContents += "#ifndef AUTH_DATA_H_`n#define AUTH_DATA_H_`n`n"
$HeaderContents += "#include <Uefi/UefiBaseType.h>`n`n"

Get-ChildItem $Globals.Layout.DataFolder -Filter '*.h' -Recurse `
 | sort creationtime `
 | Where {$_.Name.substring($_.Name.length -3, 3)  -Match 'h'} `
 | Foreach-Object {
    $HeaderContents += cat $_.FullName
}

#include end guard
$HeaderContents += "#endif AUTH_DATA_H_`n"

$HeaderContents | Out-File -Encoding Ascii -FilePath $OutFile
