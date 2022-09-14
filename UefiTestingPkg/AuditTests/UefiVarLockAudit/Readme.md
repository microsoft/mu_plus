# Variable Lock Audit Test

The Variable Lock Audit tests is a tool to ensure that variables have the correct policy.

The test is only a helper.
That is, running the Audit test generates a report.
Developers need to understand the report, and ensure that all the variable have the correct
policy.
The reviewed report is saved as a master known good report.

Later, after ingesting new code and update the firmware, you can run the audit test and compare the new report with the
previously stored master known good report.

## Two versions of the Variable Audit Test

There are two versions of the Variable Audit Test.
One is a shell application to check all the variables visible during Boot Services.
The second version of the Variable Audit Test checks all the variables visible in the OS using
the list of variables generated bu the UEFI version of the test.

## Building the UEFI version of the tool

Just add the following to your platform package .dsc file:

```ini
   UefiTestingPkg/AuditTests/UefiVarLockAudit/UEFI/UefiVarLockAuditTestApp.inf
```

### Running the VarLockAudit Test

Copy UefiVarLockAuditTestApp.efi, UefiVarAudit.py, and UefiVariablesSupportLib.py to a USB
device.
First, you need to run the UEFI Version of the test by booting to the UEFI Shell.
Log into the USB device (ie FS0:).
Run UefiVarLockAuditTestApp.efi to generate the UefiVarLockAudit_manifest.xml file:

```ini
FS0:> UefiVarLockAuditTestApp
```

Boot the system into Windows.
Open an Administrator Cmd window.
Change the drive to the USB device, and to the directory to where UefiVarLockAudit_manifest.xml,
UefiVarAudit.py, and UefiVariablesSupportLib.py are stored.
Run:

```ini
D:\python UefiVarAudit.py -InputXml UefiVarLockAudit_manifest.xml -OutputXml VarLockAuditResults.xml
```

### Analyzing the VarLockAuditResults.xml

Here is an example of a Boot Services variable:

```xml
  <Variable Guid="4C19049F-4137-4DD3-9C10-8B97A83FFDFA" Name="MemoryTypeInformation">
    <Attributes>0x3 NV BS</Attributes>
    <Size>48</Size>
    <Data>09000000580000000A000000200000000000000029000000060000004601000005000000B70000000F00000000000000</Data>
    <ReadyToBoot>
      <ReadStatus>0x0 Success</ReadStatus>
      <WriteStatus>0x0 Success</WriteStatus>
    </ReadyToBoot>
    <FromOs>
      <ReadStatus>0xCB [WinError 203] The system could not find the environment option that was entered.</ReadStatus>
      <WriteStatus>0x13 [WinError 19] The media is write protected.</WriteStatus>
    </FromOs>
  </Variable>
```

It shows the results of reading and writing to the variable at two places.  One is after
ReadyToBoot, and is provided by the UEFI version of the VarLockAudit test.  The other is in the OS.  In this example,
the variable does not have the RT (Runtime) attributes, and Windows cannot read this variable.
It also cannot write to the variable since is is present.

Here is an example of a variable with the Runtime attribute:

```xml
  <Variable Guid="8BE4DF61-93CA-11D2-AA0D-00E098032B8C" Name="BootOrder">
    <Attributes>0x7 NV BS RT</Attributes>
    <Size>10</Size>
    <Data>05000000010002000300</Data>
    <ReadyToBoot>
      <ReadStatus>0x0 Success</ReadStatus>
      <WriteStatus>0x0 Success</WriteStatus>
    </ReadyToBoot>
    <FromOs>
      <ReadStatus>0x0</ReadStatus>
      <WriteStatus>0x0</WriteStatus>
    </FromOs>
  </Variable>
````

The BootOrder variable is readable and writable in the OS.

Here is an example of a write protected RT variable:

```xml
<Variable Guid="8BE4DF61-93CA-11D2-AA0D-00E098032B8C" Name="ConOutDev">
    <Attributes>0x6 BS RT</Attributes>
    <Size>30</Size>
    <Data>02010C00D041030A0000000001010600000202030800001401807FFF0400</Data>
    <ReadyToBoot>
      <ReadStatus>0x0 Success</ReadStatus>
      <WriteStatus>0x0 Success</WriteStatus>
    </ReadyToBoot>
    <FromOs>
      <ReadStatus>0x0</ReadStatus>
      <WriteStatus>0x13 [WinError 19] The media is write protected.</WriteStatus>
    </FromOs>
  </Variable>
```

### Subsequent runs of the Variable Audit test

Once you have validated the results file, you can save it as a Good Master.
You could simply run the tests on every firmware and compare the newly generated VarLockAuditResults.xml
with the Good Master you saved.
You only need to analyze the differences.
