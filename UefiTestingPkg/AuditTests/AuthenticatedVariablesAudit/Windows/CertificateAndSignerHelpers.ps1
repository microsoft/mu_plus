
# Clear LASTEXITCODE 
$LASTEXITCODE = 0

function GetRootCertificateParams {
    param (
        $KeyLength,
        $CommonName,
        $Unit
    )

    # Organization must stay the same - so we can delete it from the certificate store
    $Organization = $Globals.Certificate.Organization

    # Text Extensions:
    # Code Signing: 2.5.29.37={text}1.3.6.1.5.5.7.3.3
    # Constraint: 2.5.29.19={text}CA=0 (NOT CA) 2.5.29.19={text}CA=1 (CA)
    # Pathlength: pathlength=16 
    #       Arbitrarily long path length, essentially this the length of valid intermediate CA's before an End Entity
    #       so CA -> 1 ... 2  -> EE - Valid
    #          CA -> 1 ... 16 -> EE - Valid
    #          CA -> 1 ... 17 -> EE - Invalid
    #       A pathlength set too short will not be valid when checked by a validity engine

    $CertificateParams = @{
        DnsName = "www.$Organization.com"
        CertStoreLocation = $Globals.Certificate.Store
        KeyAlgorithm = "RSA"
        KeyLength = $KeyLength
        Subject = "CN=$CommonName O=$Organization OU=$Unit"
        NotAfter = (Get-Date).AddYears($Globals.Certificate.LifeYears)
        KeyUsage = @("CertSign", "CRLSign", "DigitalSignature")
        # Basic Constraint : 
        #   CA: A CA certificate, by definition, must have a basic constraints extension with this `CA` boolean value set to "true" in order to be a CA.
        #   pathlength: Limits the number of intermediate certificates allowed by the next certificates
        TextExtension = @("2.5.29.19={text}CA=1&pathlength=16")
    }

    return $CertificateParams
}

function GetIntermediateCertificateParams {
    param (
        $KeyLength,
        $CommonName,
        $Unit,
        $Signer
    )

    # Organization must stay the same - so we can delete it from the certificate store
    $Organization = $Globals.Certificate.Organization

    $CertificateParams = @{
        DnsName = "www.$Organization.com"
        CertStoreLocation = $Globals.Certificate.Store
        KeyAlgorithm = "RSA"
        KeyLength = $KeyLength
        Subject = "CN=$CommonName O=$Organization OU=$Unit"
        NotAfter = (Get-Date).AddYears($Globals.Certificate.LifeYears)
        KeyUsage = @("CertSign", "CRLSign")
        TextExtension = @("2.5.29.19={text}CA=1")
        Signer = $Signer
    }

    return $CertificateParams
}

function GetEndEntityCertificateParams {
    param (
        $KeyLength,
        $CommonName,
        $Unit,
        $Signer
    )
    # Organization must stay the same - so we can delete it from the certificate store
    $Organization = $Globals.Certificate.Organization

    $CertificateParams = @{
        DnsName = "www.$Organization.com"
        CertStoreLocation = $Globals.Certificate.Store
        KeyAlgorithm = "RSA"
        KeyLength = $KeyLength
        Subject = "CN=$CommonName O=$Organization OU=$Unit"
        NotAfter = (Get-Date).AddYears($Globals.Certificate.LifeYears)
        KeyUsage = @("CertSign", "CRLSign")
        TextExtension = @("2.5.29.19={text}")
        Signer = $Signer
    }

    return $CertificateParams
}

function GenerateCertificate {
    <#
    This function generates a certificate used for mock testing
    
    :param KeyLength: The size in bits of the length of the key (ex 2048)
    :param CommonName: Common name field of the certificate
    :param Variable Name: Name of the variable (Not important for the certificate but used to track which pfx is tied to which signed data)
    :param VariablePrefix: Prefix to append to the beginning of the certificate for tracking (Not Important)
    :param Signer: Signing certificate object from the Certificate Store
    
    :return: HashTable Object
        {
            .Cert           # Path to Certificate in the Certificate Store 
            .CertPath       # Path to the der encoded certificate
            .PfxCertPath    # Path to the pfx file generated
        }
    #>

    # Get-ChildItem -Path "Cert:\LocalMachine\My" | Where-Object Thumbprint -EQ 4EFF6B1A0F61B4BF692C77F09889AD151EE8BB58 | Select-Object *

    param (
        $CertificateParams,
        $VariableName,
        $VariablePrefix
    )
    
    # Return object on success
    $PfxCertFilePath = Join-Path -Path $Globals.Layout.CertificateFolder -ChildPath "${VariablePrefix}${VariableName}.pfx"
    $CertFilePath = Join-Path -Path $Globals.Layout.CertificateFolder -ChildPath "${VariablePrefix}${VariableName}.cer"
    $P7bCertFilePath = Join-Path -Path $Globals.Layout.CertificateFolder -ChildPath "${VariablePrefix}${VariableName}.p7b"

    Write-Host "$> New-SelfSignedCertificate " @CertificateParams

    # Generate the new certifcate with the chosen params
    $Output = New-SelfSignedCertificate @CertificateParams
    if ($LASTEXITCODE -ne 0) {
        write-host $Output
        Write-Host "New-SelfSignedCertificate Failed"
        return $null
    }

    # The path of the certificate in the store
    $MockCert = $Globals.Certificate.Store + $Output.Thumbprint 
    
    # Print all the details from the certificate
    # Get-ChildItem -Path $Globals.Certificate.Store | Where-Object Thumbprint -EQ $Output.Thumbprint  | Select-Object * | Write-Host

    # export the certificate as a PFX
    Export-PfxCertificate -Cert $MockCert -FilePath $PfxCertFilePath -Password $Globals.Certificate.SecurePassword | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Export-PfxCertificate Failed"
        return $null
    }

    Export-Certificate -Cert $MockCert -FilePath $CertFilePath | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Export-Certificate Failed"
        return $null
    }

    $ReturnObject = @{
        Cert = $MockCert
        CertPath = $CertFilePath
        PfxCertPath = $PfxCertFilePath
        CertInfo = "${PfxCertFilePath};password" # path to the pfx certificate and the password
    }

    return $ReturnObject
}

function GenerateTestData {
    <#
    This function generates test data for the mock variables and then signs it with the provided certificate

    :param VariableName: UEFI Variable Name (IMPORTANT This needs to match the variable used on the device that is used for signing)
    :param VariablePrefix: Variable prefix used for tracking
    :param VariableData: Variable data - ascii string
    :param PfxCertFilePath: The path to the Pfx Certificate

    :return:
        boolean true if Success, false otherwise
    #>

    param (
        [String]$VariableName,
        [String]$VariablePrefix,
        [String]$VariableData,
        [String]$Signers,
        [String]$TrustAnchor,
        [Array]$AdditionalCertificates,
        [String]$HashAlgorithm
    )

    $TestDataPath = Join-Path -Path $Globals.Layout.TestDataFolder -ChildPath "${VariableName}.bin"
    $ExpectedDataPath = Join-Path -Path $Globals.Layout.TestDataFolder -ChildPath "${VariableName}.bin.c"
    $EmptyTestDataPath = Join-Path -Path  $Globals.Layout.TestDataFolder -ChildPath "${VariableName}Empty.bin"

    # Create the empty file - Piping errors to null
    New-Item -Name ${EmptyTestDataPath} -ItemType File 2>$null
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $TestData = "$VariableData"

    # Create the File with test data that is unique to the variable signing it
    $TestData | Out-File -Encoding Ascii -FilePath ${TestDataPath} -NoNewline
    if ($LASTEXITCODE -ne 0) {
        return $false
    }
    
    write-host $Signers

    # Arguments
    $FormatAuthVarArgs = @($VariableName, $Globals.Variable.Guid, $Globals.Variable.Attributes, $TestDataPath, `
        $Signers, "--export-c-array", "--c-name", "${VariablePrefix}${VariableName}", `
        "--output-dir", $Globals.Layout.TestDataFolder
    )

    # Add the trust anchor if provided
    if (![string]::IsNullOrWhiteSpace($TrustAnchor)) {
        $FormatAuthVarArgs += "--top-level-certificate"
        $FormatAuthVarArgs += $TrustAnchor
    }

    # Add the trust anchor if provided
    if ($AdditionalCertificates.Count -ne 0) {
        $FormatAuthVarArgs += "--additional-certificates"
        $FormatAuthVarArgs += $AdditionalCertificates
    }

    # Add the hash algorithm if provided
    if (![string]::IsNullOrWhiteSpace($HashAlgorithm)) {
        $FormatAuthVarArgs += "--hash-algorithm"
        $FormatAuthVarArgs += $HashAlgorithm
    }
    
    Write-Host "$> python $FormatAuthVar sign " @FormatAuthVarArgs

    # Generate the empty authenticated vatriable
    python $FormatAuthVar sign $FormatAuthVarArgs
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $OutputDir = Join-Path -Path $Globals.Layout.TestDataFolder -ChildPath "${VariablePrefix}${VariableName}"
    $SignedDataFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}.bin.signed"
    $DescribeFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}.bin.describe"
    $ExpectedSourceFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}.bin.c"
    $ExpectedHeaderFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}.bin.h"

    $TestDataLength = $TestData.Length + 1 # +1 for \0
    # Save off the test data as C variables so we can compare them and confirm that Get and Set worked Correctly
    "UINT8 ${VariablePrefix}${VariableName}Expected[$TestDataLength] = `"$TestData`";`n" | Out-File -Encoding Ascii -FilePath ${ExpectedSourceFile}
    "extern UINT8 ${VariablePrefix}${VariableName}Expected[$TestDataLength];`n" | Out-File -Encoding Ascii -FilePath ${ExpectedHeaderFile}

    # Generate the data authenticated variable
    python $FormatAuthVar describe $SignedDataFile --output $DescribeFile
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    # Arguments
    $FormatAuthVarArgs = @($VariableName, $Globals.Variable.Guid, $Globals.Variable.Attributes, $EmptyTestDataPath, `
        $Signers, "--export-c-array", "--c-name", "${VariablePrefix}${VariableName}Empty", `
        "--output-dir", $Globals.Layout.TestDataFolder
    )

    # Add the trust anchor if provided
    if (![string]::IsNullOrWhiteSpace($TrustAnchor)) {
        $FormatAuthVarArgs += "--top-level-certificate"
        $FormatAuthVarArgs += $TrustAnchor
    }

    # Add the trust anchor if provided
    if ($AdditionalCertificates.Count -ne 0) {
        $FormatAuthVarArgs += "--additional-certificates"
        $FormatAuthVarArgs += $AdditionalCertificates
    }

    # Sleep for 1 second to ensure the timestamp is different
    Start-Sleep -Seconds 1

    # Generate the empty authenticated vatriable
    python $FormatAuthVar sign $FormatAuthVarArgs
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    $OutputDir = Join-Path -Path $Globals.Layout.TestDataFolder -ChildPath "${VariablePrefix}${VariableName}Empty"
    $SignedDataFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}Empty.bin.signed"
    $DescribeFile = Join-Path -Path $OutputDir -ChildPath "${VariableName}Empty.bin.describe"

    # Generate the data authenticated variable
    python $FormatAuthVar describe $SignedDataFile --output $DescribeFile
    if ($LASTEXITCODE -ne 0) {
        return $false
    }

    return $true
}