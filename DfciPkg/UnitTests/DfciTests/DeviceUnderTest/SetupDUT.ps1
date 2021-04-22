## @file
# Setup Device Under Test
#
#
# NOTE: This file should only be run on systems suitable for testing, and connected
#       to an external network.  In addition to installing python 3 and some support
#       pip modules, this script configures the firewall to allow the System Under
#       Test to respond to pings (how the test environment notes the system is present),
#       and allows the Robot Framework Port for use by test environment.
#
#
#
#
#
# Copyright (C) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##


## Device Under Test setup script for DFCI test automation
## To be run after OS install on Device Under Test

function MyStartProcess {
    param([string]$Program, [string]$Arguments)

    $pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = $Program
    $pinfo.RedirectStandardError = $true
    $pinfo.RedirectStandardOutput = $true
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = $Arguments
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    $p.WaitForExit()
    $stdout = $p.StandardOutput.ReadToEnd()
    $stderr = $p.StandardError.ReadToEnd()
    Write-Host "stdout: $stdout"
    Write-Host "stderr: $stderr"
    Write-Host "exit code: " + $p.ExitCode
    return $p.ExitCode
}

## install python & modules

$storageDir = $Env:temp
$webclient = New-Object System.Net.WebClient
$url = "https://www.python.org/ftp/python/3.9.4/python-3.9.4-amd64.exe"
$file = "$storageDir\python-3.9.4-amd64.exe"

Write-Host "Downloading " + $url
$webclient.DownloadFile($url,$file)

Write-Host "Installing Python39"
$rc = MyStartProcess $File "/quiet PrependPath=1 InstallAllUsers=1 TargetDir=`"C:\Python39`" "

if ($rc -eq 0)
{
    #install modules
    MyStartProcess "C:\Python39\python.exe" "-m pip install --upgrade pip"
    MyStartProcess "C:\Python39\python.exe" "-m pip install robotframework"
    MyStartProcess "C:\Python39\python.exe" "-m pip install robotremoteserver"
    MyStartProcess "C:\Python39\python.exe" "-m pip install pypiwin32"
}

## Disable windows recovery during boot - With frequent rebooting, the OS will
## think the system is broken, and try to enter recovery. Turn this off.
bcdedit /set “{current}” bootstatuspolicy ignoreallfailures

## Disable Windows Update
New-Item HKLM:\SOFTWARE\Policies\Microsoft\Windows -Name WindowsUpdate
New-Item HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate -Name AU
New-ItemProperty HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU -Name NoAutoUpdate -Value 1

## Configure Firewall

Import-Module NetSecurity
# enable ping response
New-NetFirewallRule -Name Allow_Ping -DisplayName “Allow Ping”  -Description “Packet Internet Groper ICMPv4” -Protocol ICMPv4 -IcmpType 8 -Enabled True -Profile Any -Action Allow

# allow robotserver port 8270
New-NetFirewallRule -Name Allow_robotserver -DisplayName “Allow Python Robot server 8270” -Protocol TCP -LocalPort 8270 -Description “PyRobot server” -Enabled True -Profile Any -Action Allow

##set up task scheduler to run robot server
Register-ScheduledTask -Xml (get-content 'PyRobotServer.xml' | out-string) -TaskName "PyRobot Server" –Force

if ( -not (Test-Path -path C:\Test -PathType Container )) {
    try {
        New-Item -Path "C:\" -Name "Test" -ItemType Directory -ErrorAction Stop | Out-Null #-Force
    }
    catch {
        Write-Error -Message "Unable to create directory 'C:\Test'. Error was: $_" -ErrorAction Stop
    }
}

if ( -not (Test-Path -path C:\Test\Lib -PathType Container )) {
    try {
        New-Item -Path "C:\Test" -Name "Lib" -ItemType Directory -ErrorAction Stop | Out-Null #-Force
    }
    catch {
        Write-Error -Message "Unable to create directory 'C:\Test\Lib'. Error was: $_" -ErrorAction Stop
    }
}

Copy-Item $PSScriptRoot\PyRobotRemote.py -Destination "C:\Test"
Copy-Item $PSScriptRoot\UefiVariablesSupportLib.py -Destination "C:\Test\Lib"

Write-Host "Please restart your system and verify that the PyRobotRemote server starts automatically."