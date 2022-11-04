# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent

$mypwd = ConvertTo-SecureString -String "12345" -Force -AsPlainText
$currentdir = Get-Location


# Create a Self Signed root public/private key, $RootCert
# certificate is good for 300 months from creation date
# certificate is exportable, uses sha256 hash function and is stored locally 
# certificate is expected to be used to sign code
# 
$RootCert = New-SelfSignedCertificate -Subject "A Test Root CA"  -KeyLength  4096 -NotAfter (Get-Date).AddMonths(300) -HashAlgorithm sha256 -KeyExportPolicy Exportable  -CertStoreLocation "cert:\LocalMachine\My" -KeyUsageProperty Sign

# Export the self signed keys to a Pfx (Personal Information Exchange)
Export-PfxCertificate -Cert $RootCert -FilePath $currentdir\Root.pfx -Password $mypwd

# Export the Public key portion of the certificate
Export-Certificate -Cert $RootCert -FilePath $currentdir\Root.cer -Type cer


#
# Create a CA public/private key that is signed by the $RootCert
# certificate is good for 150 months from creation date
# certificate is exportable, uses sha256 hash function and is stored locally 
# certificate is expected to be used to sign code
# 
$CA = New-SelfSignedCertificate -Subject "A UEFI Test CA"  -KeyLength  4096 -NotAfter (Get-Date).AddMonths(150) -HashAlgorithm sha256 -Signer $RootCert -KeyExportPolicy Exportable -CertStoreLocation "cert:\LocalMachine\My" -KeyUsageProperty Sign -KeyUsage DigitalSignature,KeyEncipherment,CertSign

# Export the CA keys to a Pfx (Personal Information Exchange) (Contains boht private and public)
Export-PfxCertificate -Cert $CA -FilePath $currentdir\CA.pfx -Password $mypwd

# Export the Public key portion of the certificate
Export-Certificate -Cert $CA -FilePath $currentdir\CA.cer -Type cer

#
# Create a leaf test key, which will be used for signing Mfci firmware updates
# certificate is 3072 bytes (maximum available for signing mfci packets)
# certificate is good for 30 months
# certificate is exportable, uses sha256 hash function
# cetificate is signed by CA
# Text Extensions - EKU Any purpose (2.5.29.37)
# include EKU  1.3.6.1.5.5.7.3.3, which signifies that certificate can be used for code signing 
# include EKU 1.3.6.1.4.1.311.45.255.255 which is the EKU that is used by Mfci as an additional check when verifying certificate used to sign packet (this needs to match PcdMfciPkcs7RequiredLeafEKU) 
$LeafTest = New-SelfSignedCertificate -Subject "FwPolicy test leaf" -KeyLength  3072 -NotAfter (Get-Date).AddMonths(30) -HashAlgorithm sha256 -Signer $CA -KeyExportPolicy Exportable -KeySpec Signature -CertStoreLocation "cert:\LocalMachine\My" -KeyUsageProperty Sign -TextExtension @("2.5.29.37={text}1.3.6.1.4.1.311.45.255.255,1.3.6.1.5.5.7.3.3") -KeyUsage DigitalSignature,KeyEncipherment

# Export the leaf-test keys to a Pfx (Personal Information Exchange) (contains both private and public)
Export-PfxCertificate -Cert $LeafTest -ChainOption BuildChain -FilePath $currentdir\Leaf-test.pfx -Password $mypwd

# Export the Public key portion of the certificate
Export-Certificate -Cert $LeafTest -FilePath $currentdir\Leaf-test.cer -Type cer


## Other Test Certificates outside of the main chain.

#
# Create a different CA, still signed by Root CA, but not used to sign the leaf certificate
#
$CaNotTrusted = New-SelfSignedCertificate -Subject "Not Trusted UEFI Test CA"  -KeyLength  4096 -NotAfter (Get-Date).AddMonths(150) -HashAlgorithm sha256 -Signer $RootCert -KeyExportPolicy Exportable -CertStoreLocation "cert:\LocalMachine\My" -KeyUsageProperty Sign

# Export the CA keys to a Pfx (Personal Information Exchange) (contains both private and public)
Export-PfxCertificate -Cert $CaNotTrusted -FilePath $currentdir\CA-NotTrusted.pfx -Password $mypwd

# Export the Public key portion of the certificate
Export-Certificate -Cert $CaNotTrusted -FilePath $currentdir\CA-NotTrusted.cer -Type cer


#
# Create a leaf test key, with an unmatched EKU, signed by the same CA has the leaf test with the valid EKU
# 
$LeafTestBadEku = New-SelfSignedCertificate -Subject "Another test leaf With unmatched EKU" -KeyLength  3072 -NotAfter (Get-Date).AddMonths(30) -HashAlgorithm sha256 -Signer $CA -KeyExportPolicy Exportable -KeySpec KeyExchange -CertStoreLocation "cert:\LocalMachine\My" -KeyUsageProperty Sign -TextExtension @("2.5.29.37={text}1.3.6.1.4.1.311.255.0.0,1.3.6.1.5.5.7.3.3") 

# Export the CA keys to a Pfx (Personal Information Exchange)
Export-PfxCertificate -Cert $LeafTestBadEku -FilePath $currentdir\Leaf-NoEku.pfx -Password $mypwd

# Export the Public key portion of the certificate
Export-Certificate -Cert $LeafTestBadEku -FilePath $currentdir\Leaf-NoEku.cer -Type cer


#Remove-Item $RootCert
#Remove-Item $CA
#Remove-Item $LeafTest
#Remove-Item $CaNotTrusted
#Remove-Item $LeafTestBadEku