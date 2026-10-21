# Secure Boot Key ECIT Audit

`SecureBootKeyAuditApp` performs an initial end-to-end check of the UEFI 2.12
Crypto Indicator Table claims for Secure Boot authenticated updates, image
authorization, image revocation, and image verification.

This is a focused ECIT and Secure Boot cryptographic capability audit. A
passing result does not establish whole-platform UEFI 2.12 compatibility;
requirements outside the ECIT and Secure Boot operations listed below remain
unevaluated by this application.

> [!CAUTION]
> The application changes PK, `db`, and `dbx`. Use the QEMU runner for routine
> development. Run on physical hardware only on a dedicated lab device after
> proving an independent recovery path and backing up the platform's Secure
> Boot configuration.

## Run on a dedicated physical test machine

The physical target must be an X64 lab machine that publishes ECIT and can boot
an X64 UEFI Shell from USB. Do not use an end-user or production machine.

### 1. Qualify and back up the target

Before building the USB:

1. Record the machine identity and current firmware version.
2. Export or otherwise back up PK, KEK, `db`, and `dbx`.
3. Account for BitLocker or device-encryption recovery, measured boot, and
   attestation effects.
4. Prove a recovery path that does not depend on the Secure Boot variables or
   authenticated-update path being tested. Examples include direct flash
   restoration, an out-of-band recovery controller, or a platform firmware
   recovery flow.
5. Connect external power.

The recovery mechanism must be exercised before the audit. The application
recognizes and cleans up its own state, but a firmware defect can accept an
update and later reject the corresponding cleanup update.

### 2. Build the application

From the `mu_tiano_platforms` root:

```bash
stuart_ci_build \
  -c .pytool/CISettings.py \
  -p UefiTestingPkg \
  -a X64 \
  -t DEBUG \
  TOOL_CHAIN_TAG=GCC5 \
  BLD_*_SECURE_BOOT_KEY_AUDIT_ONLY=TRUE
```

This builds only the audit and its image-verification payload. Copy this X64
application to the USB:

```text
Build/UefiTestingPkg/DEBUG_GCC5/X64/SecureBootKeyAuditApp.efi
```

All certificates, authenticated updates, signed images, and fixture metadata
are compiled into the application. No private keys or separate fixture files
are required on the target.

### 3. Prepare a clean FAT32 USB

Use this layout:

```text
USB:\
  EFI\
    BOOT\
      BOOTX64.EFI
  SecureBootKeyAuditApp.efi
  startup.nsh
```

`BOOTX64.EFI` must be a compatible X64 UEFI Shell. Copy the checked-in
`startup.nsh` from this directory to the USB root.

Start each target run with these files absent:

```text
SecureBootKeyAuditApp_Cache.dat
SecureBootKeyAuditApp_JUNIT.XML
SecureBootKeyAuditJournal.bin
```

The cache resumes the framework after the cold reboot. Reusing a completed
cache or JUnit file can make old evidence look like a new run.

Example PowerShell preparation, replacing `E:` with the USB drive:

```powershell
$UsbDrive = "E:"
$AuditApp = "\\wsl.localhost\Ubuntu-24.04\home\doug\git\pqc" +
            "\mu_tiano_platforms\Build\UefiTestingPkg\DEBUG_GCC5" +
            "\X64\SecureBootKeyAuditApp.efi"
$StartupScript = "\\wsl.localhost\Ubuntu-24.04\home\doug\git\pqc" +
                 "\mu_tiano_platforms\Common\MU\UefiTestingPkg\AuditTests" +
                 "\SecureBootKeyAudit\startup.nsh"

Remove-Item "$UsbDrive\SecureBootKeyAuditApp_Cache.dat" -ErrorAction Ignore
Remove-Item "$UsbDrive\SecureBootKeyAuditApp_JUNIT.XML" -ErrorAction Ignore
Remove-Item "$UsbDrive\SecureBootKeyAuditJournal.bin" -ErrorAction Ignore
Copy-Item $AuditApp "$UsbDrive\SecureBootKeyAuditApp.efi"
Copy-Item $StartupScript "$UsbDrive\startup.nsh"
```

### 4. Establish the required firmware state

Through firmware setup or the independently tested recovery mechanism:

1. back up the existing Secure Boot configuration;
2. clear PK, `db`, and `dbx`;
3. confirm `SetupMode=1` and `SecureBoot=0`; and
4. put the USB UEFI Shell first in the boot order.

Do not proceed if the machine cannot be placed in this exact baseline or if
the original configuration cannot be restored independently.

### 5. Execute both boots

1. Boot the USB. The shell runs `startup.nsh`.
2. The audit writes its journal and framework cache, then requests a cold
   reset.
3. Ensure the same USB boots again.
4. The shell reruns `startup.nsh`; the framework resumes from
   `SecureBootKeyAuditApp_Cache.dat`.
5. Wait for the application to return to the shell after the second boot.
6. Do not relaunch it again on the same evidence files.

Capture a serial console for the complete run when possible. If the audit
stops after changing Secure Boot state, preserve the USB and serial output and
use the independent recovery mechanism. Do not repeatedly relaunch the
application or improvise cleanup.

### 6. Collect the evidence from the USB

After a complete second boot, the USB root contains:

| File | Purpose |
| --- | --- |
| `SecureBootKeyAuditApp_JUNIT.XML` | Primary result: scenario pass/fail data, operational logs, and versioned structured ECIT evidence |
| `SecureBootKeyAuditJournal.bin` | Independently validated proof that the same fixture and ECIT state resumed across the cold reboot |
| `SecureBootKeyAuditApp_Cache.dat` | Unit-test framework resume state; archive for diagnosis, but do not use it as the result |

Archive those files with the serial log, machine identity, firmware version,
test date, operator, and the pre-test Secure Boot backup. Copy them off the USB
before restoring or reusing it.

Example PowerShell collection:

```powershell
$UsbDrive = "E:"
$Evidence = "\\wsl.localhost\Ubuntu-24.04\home\doug\git\pqc" +
            "\audit-evidence\machine-name-2026-10-19"

New-Item -ItemType Directory -Force $Evidence | Out-Null
Copy-Item "$UsbDrive\SecureBootKeyAuditApp_JUNIT.XML" $Evidence
Copy-Item "$UsbDrive\SecureBootKeyAuditJournal.bin" $Evidence
Copy-Item "$UsbDrive\SecureBootKeyAuditApp_Cache.dat" $Evidence
```

### 7. Validate and read the evidence

From the `SecureBootKeyAudit` directory, replace the example evidence path:

```bash
evidence=/home/doug/git/pqc/audit-evidence/machine-name-2026-10-19

python3 Host/PrintEcitReport.py \
  "$evidence/SecureBootKeyAuditApp_JUNIT.XML"

python3 Host/InspectJournal.py \
  "$evidence/SecureBootKeyAuditJournal.bin"
```

The JUnit must report all scenarios passed with no framework failures or
errors. The journal must decode with:

```text
"phase": "PERSISTENTLY_VERIFIED"
"boot_count": 2
"setup_mode": 1
"secure_boot": 0
"state_name": "ABSENT" for pk, db, and dbx
```

`PrintEcitReport.py` validates the structured evidence, reconstructs the
advertised atomic and derived-profile rows, checks the firmware control totals,
and prints the ECIT table and `ECIT_SUMMARY`. A malformed, truncated, or
internally inconsistent contract is a report error rather than a partial
result. `PASS_WITH_LIMITATIONS` means the exercised advertisements worked but
some advertised profiles remain untested; it is not equivalent to complete
advertised coverage.

### 8. Restore the machine

After preserving the evidence:

1. restore the known-good PK, KEK, `db`, and `dbx` through the independently
   tested recovery path;
2. verify Secure Boot mode and normal boot;
3. verify disk unlock, measured boot, and attestation behavior; and
4. retain the evidence with the restored-configuration verification.

## Prototype the physical workflow on QemuQ35

Use QemuQ35 to exercise the same standalone application, FAT filesystem,
startup script, two-boot sequence, and evidence collection expected on a
physical machine. The commands remain separate so each operator-visible step
can be inspected.

The examples require `mtools` and `dosfstools`. From the
`mu_tiano_platforms` root, define the paths used below:

```bash
root=$PWD
audit_dir="$root/Common/MU/UefiTestingPkg/AuditTests/SecureBootKeyAudit"
build="$root/Build/QemuQ35PkgX64/DEBUG_GCC5"
audit_app="$root/Build/UefiTestingPkg/DEBUG_GCC5/X64/SecureBootKeyAuditApp.efi"
startup="$audit_dir/startup.nsh"
virtual_drive="$build/VirtualDrive.img"
variable_store="$build/FV/QEMUQ35_VARS.fd"
variable_backup="$build/FV/QEMUQ35_VARS.fd.secure-boot-audit-backup"
evidence="$root/Build/SecureBootKeyAuditEvidence"

command -v mcopy
command -v mkfs.vfat
```

Build the standalone application that would be copied to the physical USB:

```bash
stuart_ci_build \
  -c .pytool/CISettings.py \
  -p UefiTestingPkg \
  -a X64 \
  -t DEBUG \
  TOOL_CHAIN_TAG=GCC5 \
  BLD_*_SECURE_BOOT_KEY_AUDIT_ONLY=TRUE

test -f "$audit_app"
sha256sum "$audit_app"
```

Build the QemuQ35 firmware used only as the prototype target:

```bash
stuart_build \
  -c Platforms/QemuQ35Pkg/PlatformBuild.py \
  TOOL_CHAIN_TAG=GCC5
```

Back up and hash the complete QEMU variable store. Refuse to overwrite an
existing backup:

```bash
test -f "$variable_store"
test ! -e "$variable_backup"

cp --preserve=all "$variable_store" "$variable_backup"
baseline_sha=$(sha256sum "$variable_backup" | cut -d' ' -f1)
echo "Baseline variable-store SHA-256: $baseline_sha"
```

Create a clean FAT32 virtual USB and copy only the standalone application:

```bash
rm -f "$virtual_drive"
truncate -s 60M "$virtual_drive"
mkfs.vfat -F 32 "$virtual_drive"

mcopy -D overwrite -i "$virtual_drive" "$audit_app" ::
mdir -i "$virtual_drive" ::
```

Run QEMU. `STARTUP_NSH` copies the checked-in physical startup commands to the
virtual USB. `SHUTDOWN_AFTER_RUN=TRUE` adds a QEMU-only shutdown after the
application returns on boot 2. The application's cold reset on boot 1 occurs
before that shutdown command:

```bash
rm -rf "$evidence"
mkdir -p "$evidence"

set -o pipefail
stuart_build \
  -c Platforms/QemuQ35Pkg/PlatformBuild.py \
  --FlashRom \
  TOOL_CHAIN_TAG=GCC5 \
  RUN_TESTS=FALSE \
  EMPTY_DRIVE=FALSE \
  STARTUP_NSH="$startup" \
  SHUTDOWN_AFTER_RUN=TRUE \
  2>&1 | tee "$evidence/qemu-console.log"
```

The single QEMU process boots twice. On boot 1 the application creates its
cache and journal and requests a cold reset. On boot 2 the same startup script
runs again, the framework resumes, writes JUnit, and returns to the shell.

List and collect the same physical evidence filenames from the virtual USB:

```bash
mdir -i "$virtual_drive" ::

mcopy -n -i "$virtual_drive" \
  ::/SecureBootKeyAuditApp_JUNIT.XML "$evidence/"
mcopy -n -i "$virtual_drive" \
  ::/SecureBootKeyAuditJournal.bin "$evidence/"
mcopy -n -i "$virtual_drive" \
  ::/SecureBootKeyAuditApp_Cache.dat "$evidence/"

ls -l "$evidence"
```

Restore the variable store before interpreting the result, even if QEMU or
evidence collection failed:

```bash
cp --preserve=all "$variable_backup" "$variable_store"
restored_sha=$(sha256sum "$variable_store" | cut -d' ' -f1)

echo "Restored variable-store SHA-256: $restored_sha"
test "$restored_sha" = "$baseline_sha"
rm "$variable_backup"
```

Validate and render the collected evidence:

```bash
python3 "$audit_dir/Host/PrintEcitReport.py" \
  "$evidence/SecureBootKeyAuditApp_JUNIT.XML"

python3 "$audit_dir/Host/InspectJournal.py" \
  "$evidence/SecureBootKeyAuditJournal.bin"
```

## Interpret the result

First check the unit-test result. Every scenario must pass with no framework
errors. The firmware writes compact `[ECIT_V1]` records into the JUnit test
log. `PrintEcitReport.py` validates those records and builds the human-readable
table; the firmware does not format the table. Then read `ECIT_SUMMARY`; a
passing unit-test count does not mean every advertised ECIT combination was
exercised.

| Validation | Meaning | Disposition |
| --- | --- | --- |
| `VERIFIED` | ECIT advertised the claim and the corresponding operation succeeded. | Pass |
| `OVER_REPORTED` | ECIT advertised the claim but the operation failed. | Conformance failure |
| `NOT_TESTED` | ECIT advertised the claim but no exact operational observation was made. | Coverage limitation |
| `NOT_INDEPENDENTLY_VERIFIABLE` | The operation succeeded, but the available evidence cannot independently establish the exact claim. | Coverage limitation |
| `UNADVERTISED_SUCCESS` | An operation succeeded for a claim ECIT did not advertise. | Informational; investigate policy/reporting |
| `VERIFIED_ABSENT` | A negative probe confirmed an unadvertised capability is unavailable. | Informational |
| `NOT_ADVERTISED` | A modeled probe was skipped because the target did not advertise it. | Outside the advertised denominator |
| `INCONCLUSIVE` | The audit could not establish a trustworthy operational result. | Audit failure |

The final verdict means:

- `PASS`: every advertised claim in the denominator was verified;
- `PASS_WITH_LIMITATIONS`: no mismatch was found, but advertised claims remain
  untested or not independently verifiable; or
- `FAIL`: at least one advertised claim was over-reported or the evidence was
  inconclusive.

For signature and digest OID features, ECIT advertises two sets and the audit
treats their Cartesian product as the profile denominator. Testing one exact
pair verifies only that pair. The `Derived profile summary` reports how many
advertised pairs were tested and how many remain.

For each run, archive:

- the complete JUnit XML;
- the serial or console log;
- the printed ECIT table and `ECIT_SUMMARY`;
- the baseline and restored variable-store hashes for QEMU; and
- `SecureBootKeyAuditJournal.bin` for a physical run.

## What the audit exercises

The scenarios require a disposable or independently recoverable Setup Mode
variable store.

| Area | Operational coverage |
| --- | --- |
| ECIT parsing | Header, version, checksum, bounds, entry alignment, duplicate features, OID payloads, and signature-list GUID payloads |
| Authenticated updates | X.509 authorization and RSA/SHA-256, ECDSA-P256/SHA-256, and ML-DSA-87/SHA-512 verification |
| PK lifecycle | Setup Mode enrollment, same-algorithm rotation, RSA/ECDSA/ML-DSA capability-selected transitions, authenticated deletion, and return to Setup Mode |
| Image enforcement | Pre-enrollment denial, valid authorization, tamper denial, fresh `LoadImage()` decisions, and `StartImage()` execution |
| Replay rejection | Identical authenticated-update replay must return `EFI_SECURITY_VIOLATION` without changing the variable |
| Multi-signature fallback | RSA/ML-DSA order reversal and cumulative RSA/ECDSA/ML-DSA CA retirement through `dbx` |
| Signature-list formats | Every advertised V1/V2 full-X.509, image-hash, and TBSCertificate-hash format |
| Recovery safety | Read-only preflight, allowlisted interrupted-state recovery, strict baseline checks, and exact QEMU store restoration |
| Reboot persistence | Fixture-bound, CRC-protected journal, cold reboot, resumed unit test, and ECIT/state reread |

The audit uses separate PK/update-authority and image-signer keys. Image tests
cover RSA-2048/SHA-256, ECDSA-P256/SHA-256, and pure
ML-DSA-87/SHA-512 Authenticode. Every expected authorization executes the
image; every authorization or denial decision starts with a fresh
`LoadImage()`.

The format sweep models 15 authorization and 15 revocation formats. QemuQ35
advertises 14 in each feature and omits V1 SHA-1 image hashes, so the current
reference run verifies all 28 advertised formats and reports the two SHA-1
probes as `NOT_ADVERTISED`.

The application refuses to mutate an existing PK, `db`, or `dbx` unless the
complete state is absent or exactly matches generated test data. This guard
does not replace an independent physical recovery path: a firmware defect can
accept a mutation and later reject its matching cleanup update.

## Persistent reboot journal

`SecureBootKeyAuditJournal.bin` is written to the current test volume before
the reboot boundary. The journal contains no private key material. It records
the fixture ID, SHA-256 of `Host/TestData.manifest.json`, boot count, prior
result, phase, and the recognized Secure Boot baseline. Journal format version
3 records PK, `db`, and `dbx` baseline state without requiring a target crypto
protocol. The firmware flushes each fixed-size journal update before
continuing and rejects an invalid signature, version, size, CRC, fixture
identity, manifest hash, or phase.

The reboot test advances through:

```text
PREPARED -> REBOOT_PENDING -> PERSISTENTLY_VERIFIED
```

A volatile marker distinguishes a real reset from relaunching the application
in the same boot. Project Mu unit-test persistence resumes the active test and
preserves its pre-reset log. After reboot, the audit validates and reparses ECIT
and rereads the Secure Boot baseline before recording
`PERSISTENTLY_VERIFIED`. Cryptographically binding the pre- and post-reset ECIT
snapshots with a self-contained hash implementation is planned work.

The completed journal intentionally remains on the test volume as evidence.
Archive it with the JUnit and serial results, then remove it before starting a
new run on the same volume. QEMU runs with `EMPTY_DRIVE=TRUE` begin with a
fresh test volume. On physical hardware, relaunch the same application from
the same removable test volume if platform boot policy does not automatically
return to it after reset.

Validate and decode the archived journal against its fixture manifest with:

```bash
python3 Host/InspectJournal.py SecureBootKeyAuditJournal.bin
```

## Generate the public fixture

Build `HelloUefiApp.efi`, then run the generator from the
`SecureBootKeyAudit` directory:

```bash
python3 Host/GenerateTestData.py
```

The generator writes:

- `UEFI/TestData.c`, containing public certificates, signature lists,
  authenticated updates, signed test images, fixture ID, and manifest hash;
  and
- `Host/TestData.manifest.json`, containing artifact hashes, explicit
  certificate roles, unsigned-image identity, and ECIT prerequisites. The
  current fixture schema is `secure-boot-key-audit-v6`.

RSA, ECDSA, and ML-DSA private keys exist only in memory while the generator
runs and are never written to disk or emitted into the application. The v6
fixture contains 150 artifacts and 109 authenticated-variable updates.

## Independently verify the public fixture

After generation, verify the exact arrays that will be compiled into the
firmware:

```bash
python3 Host/VerifyTestData.py
python3 -m unittest -v \
  Host.test_inspect_journal \
  Host.test_print_ecit_report \
  Host.test_verify_test_data
```

The verifier does not import the generator or use its construction helpers. It
parses `UEFI/TestData.c` and independently checks:

- every artifact size and SHA-256 binding in the versioned manifest;
- X.509 ESL layout and generated self-signed certificate signatures;
- every generated V1/V2 full-X.509, image-hash, and TBSCertificate-hash
  signature-list layout used by the format sweep;
- every authenticated-variable envelope, payload, signer role, detached CMS
  signature, algorithm, issuer, and serial number;
- every independent PE `WIN_CERTIFICATE` boundary and alignment, the
  Authenticode SHA-256 and SHA-512 digests, `SpcIndirectDataContent`, embedded
  certificates, RSA-2048, ECDSA-P256, and ML-DSA-87 signatures, reversed
  pairwise signature order, single-signer controls, and the three-signature
  migration image over the same unsigned image;
- the three-CA authorization database and cumulative RSA, RSA-plus-ECDSA, and
  RSA-plus-ECDSA-plus-ML-DSA revocation databases;
- single- and multi-entry V1 and V2 SHA-512 TBSCertificate-hash ESLs against
  the embedded image-signer certificates; and
- distinct certificate and public-key fingerprints for the PK/update
  authority and RSA, ECDSA, and ML-DSA image-signer roles.

The command also corrupts the final Authenticode object after rebinding its
manifest artifact hash and requires independent signature verification to
reject it. The unit tests additionally reject role-key reuse and malformed ESL
structure.

## Add a claim test

Modeled claim identity and ECIT matching metadata are centralized in
`UEFI/EcitClaimRegistry.c`. To add operational coverage for a claim:

1. add its stable ID to `ECIT_CLAIM_ID` in `UEFI/AuditTypes.h`;
2. add the matching feature and OID or signature-list GUID descriptor to
   `UEFI/EcitClaimRegistry.c`;
3. add or reuse independently verified fixture data; and
4. list the claim ID in the scenario's `CoveredClaims` array.

`ExecuteAuthVariableScenario()` publishes the observation automatically.
Bespoke scenarios use `RecordEcitClaimObservation()` or
`RecordEcitClaimObservations()` after validating their complete operational
oracle. Keep the adjacent stable-name mapping in `Host/EcitEvidence.py`
synchronized with the firmware registry. The host rejects unknown or missing
claim names and recomputes all firmware control totals before rendering a
report.
