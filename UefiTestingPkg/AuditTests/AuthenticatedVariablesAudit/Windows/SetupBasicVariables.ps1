#Requires -RunAsAdministrator
# This script *unfortunately* requires ADMIN privileges due to New-SelfSignedCertificate

# Load the helper functions
. ./CertificateAndSignerHelpers.ps1

# This script relies on this script to format and sign Authenticated
$FormatAuthVar = "./FormatAuthenticatedVariable.py"

# Have to leave this outside the globals - since it's inaccessible during initialization of the hashtable
$Password = "password"
$DataFolder = "./BasicTest"
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
# Test Name: Variable Key Length Support
# Test Description:
#   This test checks to see if variables signed by certificates with varying key
#   lengths are supported by the underlying crypto package
#
#   Signers: 1 (End Entity)
#   Additional Certificates: 0 (Only End Entity Certificate included)
#
#   Expectation: All the keys should work
#
# ┌──────────────────────────────┐ 4096
# │  Trust Anchor                │
# └─────────┬────────────────────┘
#           │ Issues
#           │
# ┌─────────▼────────────────────┐ 4096
# │  Intermediate Certificate    │
# └─────────┬────────────────────┘
#           │  Issues
# ┌─────────▼───────────┐  ─┐ 2048, 3072, 4096
# │  Leaf Certificate   │   │ Certificate(s) included
# └─────────┬───────────┘  ─┘
#           │
#           │  Sign
#           │ 
#     ┌─────▼──────┐
#     │   Digest   │ PKCS#7 
#     └────────────┘
#
# =============================================================================
# 2k Keys
# =============================================================================
$VariableName = "MockVar"
# Verify 2k, 3k, 4k keylength certificate support
$CommonName = "Variable Key Length Support"
$TestGroup = "VariableKeyLengthSupport"

# =============================================================================
# MockVar Trust Anchor
# =============================================================================
# Trust anchor should be able to be any size
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${KeyLength}TrustAnchor"

$TrustAnchorParams = GetRootCertificateParams $KeyLength $CommonName "Trust Anchor"
$TrustAnchor = GenerateCertificate $TrustAnchorParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${KeyLength}Intermediate"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate" $TrustAnchor.Cert
$Intermediate = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# =============================================================================
# Generates a 2K certificate of chain depth 1 signed by the trust anchor
# =============================================================================
$KeyLength = 2048
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} Certificate"
# Variable Prefix must be different
$VariablePrefix = "m${KeyLength}${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$2KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($2KMockVar.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =============================================================================
# Generates a 3K certificate of chain depth 1 signed by the trust anchor
# =============================================================================
$KeyLength = 3072
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} Certificate"
# Variable Prefix must be different - as this will appended to the front of the C Variable to keep them distinct
$VariablePrefix = "m${KeyLength}${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$3KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($3KMockVar.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =============================================================================
# Generates a 4K certificate of chain depth 1 signed by the trust anchor
# =============================================================================
$KeyLength = 4096
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} Certificate"
# Variable Prefix must be different - as this will appended to the front of the C Variable to keep them distinct
$VariablePrefix = "m${KeyLength}${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$4KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($4KMockVar.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =======================================================================================
# Test Name: Additional Certificates
# Test Description:
#   This test checks to see if variables signed by a single certificate and has
#   additional certificates added to the certificate chain still works. This will
#   increase the signature size as it add's additional certificates looking for a
#   breaking point
#
#   Signers: 1 (End Entity)
#   Added Certificates: 1..3 (Intermediate Certificate)
#
#   Expectation: All the signed variables should work, further the Openssl code
#       will check the certificate chain to ensure the chain is valid
#
# =============================================================================
# 2k Keys
# =============================================================================
$VariableName = "MockVar"
# Verify 2k, 3k, 4k keylength certificate support
$CommonName = "Additional Certificates"
$TestGroup = "AdditionalCertificates"

# =============================================================================
# MockVar Trust Anchor
# =============================================================================
# Trust anchor should be able to be any size
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${KeyLength}TrustAnchor"

$TrustAnchorParams = GetRootCertificateParams $KeyLength $CommonName "Trust Anchor"
$TrustAnchor = GenerateCertificate $TrustAnchorParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${KeyLength}Intermediate0"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate0" $TrustAnchor.Cert
$Intermediate0 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

$VariablePrefix = "m${KeyLength}Intermediate1"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate1" $Intermediate0.Cert
$Intermediate1 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

$VariablePrefix = "m${KeyLength}Intermediate1"
$IntermediateParams = GetIntermediateCertificateParams $KeyLength $CommonName "Intermediate2" $Intermediate1.Cert
$Intermediate2 = GenerateCertificate $IntermediateParams $VariableName $VariablePrefix

# =============================================================================
#  Generates signature signed by one 2k end entity certificate
# 1 Additional Certficiate(s)
# =============================================================================
$KeyLength = 2048
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate and includes 1 additional certificate(s)"
# Variable Prefix must be different
$VariablePrefix = "m1${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$2KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($2KMockVar.CertInfo) $null `
    @($Intermediate2.CertPath)
if (!$ret) {
    Exit
}

# =============================================================================
# Generates signature signed by one 3k end entity certificate
# 2 Additional Certficiate(s)
# =============================================================================
$KeyLength = 3072
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate and includes 2 additional certificate(s)"
# Variable Prefix must be different - as this will appended to the front of the C Variable to keep them distinct
$VariablePrefix = "m2${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$3KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($3KMockVar.CertInfo) $null `
     @($Intermediate2.CertPath, $Intermediate1.CertPath)
if (!$ret) {
    Exit
}

# =============================================================================
#  Generates signature signed by one 4k end entity certificate
# 3 Additional Certficiate(s)
# =============================================================================
$KeyLength = 4096
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate and includes 3 additional certificate(s)"
# Variable Prefix must be different - as this will appended to the front of the C Variable to keep them distinct
$VariablePrefix = "m3${TestGroup}"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $Intermediate.Cert
$4KMockVar = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($4KMockVar.CertInfo) $null `
    @($Intermediate2.CertPath, $Intermediate1.CertPath, $Intermediate0.CertPath)
if (!$ret) {
    Exit
}

# =======================================================================================
# Test Name: PreventUpdate
# Test Description:
#   This test checks to see if the basic premise of authenticated variables works.
#   I.E an invalid certificate may not update a variable
#
#   Signers: 1 (End Entity)
#
#   Expectation: Should fail pkcs7_verify
#
# =============================================================================
# 2k Keys
# =============================================================================
$VariableName = "MockVar"
$CommonName = "Prevent Update"
$TestGroup = "PreventUpdate"

# =============================================================================
# MockVar Trust Anchor
# =============================================================================
# Trust anchor should be able to be any size
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}TrustAnchor"

$TrustAnchorParams = GetRootCertificateParams $KeyLength $CommonName "Trust Anchor"
$TrustAnchor = GenerateCertificate $TrustAnchorParams $VariableName $VariablePrefix

# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}RougeAnchor"
$RougeParams = GetRootCertificateParams $KeyLength $CommonName "Rouge Anchor"
$Rouge = GenerateCertificate $RougeParams $VariableName $VariablePrefix

# =============================================================================
# Generates a 4k signature issued by a trust anchor
# =============================================================================
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} valid certificate"
# Variable Prefix must be different
$KeyLength = 4096
$VariablePrefix = "m${TestGroup}InitVariable"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $TrustAnchor.Cert
$ValidSigner = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($ValidSigner.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =============================================================================
# Generates a 2k signature issued by a rouge certificate (no relation)
# =============================================================================

$KeyLength = 2048
$VariablePrefix = "m${TestGroup}InvalidVariable"

# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} rouge certificate"
$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Rouge Signer" $Rouge.Cert
$RougeSigner = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($RougeSigner.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =======================================================================================
# Test Name: PreventRollback
# Test Description:
#   This test checks to see if a system allows for a variable signed in the past to
#   rollback and update a variable it isn't supposed to
#
#   Signers: 1 (End Entity)
#
#   Expectation: Should fail pkcs7_verify
#
# =============================================================================
# 2k Keys
# =============================================================================
$VariableName = "MockVar"
$CommonName = "Prevent Rollback"
$TestGroup = "PreventRollback"

# =============================================================================
# MockVar Trust Anchor
# =============================================================================
# Trust anchor should be able to be any size
$KeyLength = 4096 
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}TrustAnchor"

$TrustAnchorParams = GetRootCertificateParams $KeyLength $CommonName "Trust Anchor"
$TrustAnchor = GenerateCertificate $TrustAnchorParams $VariableName $VariablePrefix

# =============================================================================
# Generates a 4k signature issued by a trust anchor
# =============================================================================
# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate in the past"
# Variable Prefix must be different
$KeyLength = 4096
$VariablePrefix = "m${TestGroup}PastVariable"

$EndEntityParams = GetEndEntityCertificateParams $KeyLength $CommonName "Signer" $TrustAnchor.Cert
$Signer = GenerateCertificate $EndEntityParams $VariableName $VariablePrefix
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($Signer.CertInfo) $null @()
if (!$ret) {
    Exit
}

# There needs to be a difference in the time from when they were both signed. The signing script by default
# takes the current time as the time the variable was created so lets just wait a second before signing
# the next variable
Start-Sleep -Seconds 1

$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate in the future"
# Variable Prefix must be different
$VariablePrefix = "m${TestGroup}FutureVariable"

# Variable data *should* be different
$VariableData =  "Test: ${TestGroup} - Description: Signed By ${KeyLength} certificate in the past"
$ret = GenerateTestData $VariableName $VariablePrefix $VariableData @($Signer.CertInfo) $null @()
if (!$ret) {
    Exit
}

# =============================================================================
# delete the certs from the keystore
# =============================================================================

# Locate by organization and delete
$Organization = $Globals.Certificate.Organization
$items = Get-ChildItem -Path $Globals.Certificate.Store -Recurse
foreach ($item in $items) {
     if($item.Subject -like "*$Organization*") {
         $item | Remove-Item -Force
     }
}

# Remove-Item $Globals.Layout.CertificateFolder -Recurse -Force -Confirm:$false
# Remove-Item $Globals.Layout.TestDataFolder -Recurse -Force -Confirm:$false

# =============================================================================
# Copy All the C arrays and variables to a single header and source file
# =============================================================================

$OutFile = Join-Path -Path $Globals.Layout.DataFolder -ChildPath "Exported.c"

#Include Headers
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
