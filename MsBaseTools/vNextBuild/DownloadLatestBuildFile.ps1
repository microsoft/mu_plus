#-------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
#-------------------------------------------------------------------------------
<#
.SYNOPSIS
    Downloads a binary from a UNC path and copies to a new file

.PARAMETER OutFilepath
    Name of output file

.Parameter BuildDefPath
    Path to the root directory for the build.  the largest build number will be used
#>
param(
[Parameter(Mandatory=$true)]
[string] $OutFilepath, 

[Parameter(Mandatory=$true)]
[string] $BuildDefPath
)

#Requires -Version 5.0
#Requires -Modules @{ModuleName="TfsBuildFramework"; MaximumVersion="1.99.99"}

$BuildNumber = $(get-childitem -path $BuildDefPath -Directory | Sort-Object Name -Descending | Select-Object -First 1 ).Name

Write-Host $BuildNumber
write-host "Current Working Directory: $($pwd.Path)"
$filename = $BuildNumber.Replace(".", "_")
$path = join-path -Path (Join-Path -Path (join-path -Path $BuildDefPath -ChildPath $BuildNumber)-ChildPath "Binary") -ChildPath "$filename.bin"
Write-Host $path

if(Test-path $OutFilepath) {
    Remove-Item $OutFilepath
}

$OutputFolder = Split-Path -Path $OutFilepath 
write-host "Output folder is: $OutputFolder"
if(Test-Path $OutputFolder -PathType "Container") {
    Write-Host "Output Folder exists"
} else {
  New-Item -ItemType directory -path $OutputFolder -Force
}
Copy-Item -Path $path -Destination $OutFilepath

Update-TfsBuildNumber -BuildNumber $BuildNumber
Write-Host "TfsBuildNumber:"$BuildNumber

Set-TfsBuildVariable -Name Build.UncPath -Value (join-path -Path $BuildDefPath -ChildPath $BuildNumber)