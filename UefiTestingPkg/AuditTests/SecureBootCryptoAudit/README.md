# Secure Boot Crypto Audit

`SecureBootCryptoAuditApp` performs an initial end-to-end check of the UEFI 2.12
Crypto Indicator Table claims for Secure Boot authenticated updates, image
authorization, image revocation, and image verification.

This is a focused ECIT and Secure Boot cryptographic capability audit. A
passing result does not establish whole-platform UEFI 2.12 compatibility;
requirements outside the ECIT and Secure Boot operations listed below remain
unevaluated by this application.

> [!CAUTION]
> The application changes PK, `db`, and `dbx`. Use only a virtual or physical
> test machine that can be completely reflashed or restored through a path
> independent of the Secure Boot implementation under test. Back up the
> platform's Secure Boot configuration before running the audit.

## Run on a recoverable test machine

### Explicit all-profile robustness opt-in

The default audit remains capability-gated. A platform may explicitly set
`gUefiTestingPkgTokenSpaceGuid.PcdSecureBootCryptoAuditProbeAll` to `TRUE` to select all
fixture-backed V2 formats and RSA, ECDSA, and ML-DSA operational scenarios,
including profiles that ECIT does not advertise. In mu_plus, build with:

```bash
stuart_ci_build -c .pytool/CISettings.py -p UefiTestingPkg -a X64 -t DEBUG \
  TOOL_CHAIN_TAG=GCC 'BLD_*_SECURE_BOOT_CRYPTO_AUDIT_ONLY=TRUE' \
  'BLD_*_SECURE_BOOT_CRYPTO_AUDIT_PROBE_ALL=TRUE'
```

This is a destructive robustness experiment, **not a conformance verdict**.
An unsupported format can be accepted for enrollment yet ignored during image
verification, or an installed authority can make signed cleanup impossible.
Start with disposable QEMU images and restore the entire variable store and
virtual drive afterward. Do not use this mode on deployed machines.

Opt-in bypasses advertisement-based selection only. Malformed ECIT, unknown
variable states, failed preconditions, and failed cleanup still stop the
affected scenario. Missing ECIT does not prevent operational probes, but
ECIT-dependent reporting and reboot-checkpoint tests remain unavailable.
An earlier unsafe failure can prevent later steps from running; inspect JUnit
diagnostics rather than treating "probe all" as proof of complete coverage.

The JUnit run identity records `mode=ROBUSTNESS_PROBE_ALL`; Markdown reports
display a warning. ECIT advertisements and their correlation rules are not
changed, and the existing unadvertised-success policy is a separate setting.
Run records from version 0.12 onward must include a valid `mode`; host tools
reject a missing mode rather than silently omitting the robustness warning.
Older run records without a mode remain readable. The `PrerequisiteGate`
test checks valid, missing, and malformed ECIT states without changing
firmware variables; run both build modes to cover both sets of expectations.
Record actual update/load statuses, exact variable-state checks, cleanup and
boot behavior, and full-image restoration. Results describe only the tested
firmware build, not the safety of all in-market devices.

The target may be virtual or physical, but it must be a non-production test
machine that publishes ECIT and can be completely reflashed or restored. It
must boot a UEFI Shell and audit application matching the fixture architecture.
Separate signed-image fixtures are checked in for X64 (`UEFI/TestData.c`) and
AARCH64 (`UEFI/TestDataAArch64.c`); the build selects the one matching the
application architecture. Do not use an end-user or
production machine.

The application and evidence filenames were renamed from `SecureBootKeyAudit`.
Do not resume an interrupted run under the new name. Preserve its evidence,
restore the target independently, and start with fresh audit media.
Checked-in fixture identifiers and generated symbol names retain their
historical names to preserve the signed artifacts and manifest bindings.

The steps below describe removable-media execution on a physical target. For a
recoverable virtual target, use the equivalent procedure in [QEMU.md](QEMU.md).

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

From the `mu_plus` root:

```bash
stuart_ci_build \
  -c .pytool/CISettings.py \
  -p UefiTestingPkg \
  -a X64 \
  -t DEBUG \
  TOOL_CHAIN_TAG=GCC \
  BLD_*_SECURE_BOOT_CRYPTO_AUDIT_ONLY=TRUE
```

This builds only the audit and its image-verification payload. Copy this X64
application to the USB:

```text
Build/UefiTestingPkg/DEBUG_GCC/X64/SecureBootCryptoAuditApp.efi
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
  SecureBootCryptoAuditApp.efi
  startup.nsh
```

`BOOTX64.EFI` must be a compatible X64 UEFI Shell. Copy the checked-in
`startup.nsh` from this directory to the USB root.

Start each target run with these files absent:

```text
SecureBootCryptoAuditApp_Cache.dat
SecureBootCryptoAuditApp_JUNIT.XML
SecureBootCryptoAuditJournal.bin
```

The cache resumes the framework after the cold reboot. Reusing a completed
cache or JUnit file can make old evidence look like a new run.

From the `mu_plus` root, run this PowerShell example after
replacing `E:` with the USB drive:

```powershell
$RepoRoot = (Get-Location).Path
$UsbDrive = "E:"
$AuditApp = Join-Path $RepoRoot `
  "Build\UefiTestingPkg\DEBUG_GCC\X64\SecureBootCryptoAuditApp.efi"
$StartupScript = Join-Path $RepoRoot `
  "UefiTestingPkg\AuditTests\SecureBootCryptoAudit\startup.nsh"

foreach ($Name in @(
    "SecureBootCryptoAuditApp_Cache.dat",
    "SecureBootCryptoAuditApp_JUNIT.XML",
    "SecureBootCryptoAuditJournal.bin"
)) {
    $Path = Join-Path $UsbDrive $Name
    if (Test-Path $Path -ErrorAction Stop) {
        Remove-Item $Path -ErrorAction Stop
    }
}
Copy-Item $AuditApp "$UsbDrive\SecureBootCryptoAuditApp.efi"
Copy-Item $StartupScript "$UsbDrive\startup.nsh"
```

### 4. Establish the required firmware state

Through firmware setup or the independently tested recovery mechanism:

1. back up the existing Secure Boot configuration;
2. clear PK, `db`, and `dbx`;
3. confirm `SetupMode=1` and `SecureBoot=0`; and
4. make the USB UEFI Shell the persistent first/default boot option for both
   audit boots.

Do not proceed if the machine cannot be placed in this exact baseline or if
the original configuration cannot be restored independently.

### 5. Execute both boots

Do not rely on a one-time boot-menu selection. The audit requests a cold reset,
so the persistent boot order must return to the same USB and `startup.nsh`
without operator intervention.

1. Boot the USB. The shell runs `startup.nsh`.
2. The audit writes its journal and framework cache, then requests a cold
   reset.
3. Confirm that the persistent boot order starts the same USB again.
4. The shell reruns `startup.nsh`; the framework resumes from
   `SecureBootCryptoAuditApp_Cache.dat`.
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
| `SecureBootCryptoAuditApp_JUNIT.XML` | Primary result: run identity, scenario pass/fail data, operational logs, and versioned structured ECIT evidence |
| `SecureBootCryptoAuditJournal.bin` | Independently validated proof that the same fixture and ECIT state resumed across the cold reboot |
| `SecureBootCryptoAuditApp_Cache.dat` | Unit-test framework resume state; archive for diagnosis, but do not use it as the result |

Archive those files with the serial log, machine identity, firmware version,
test date, operator, and the pre-test Secure Boot backup. Copy them off the USB
before restoring or reusing it.

From the `mu_plus` root, replace the example machine and date in
this PowerShell collection:

```powershell
$RepoRoot = (Get-Location).Path
$UsbDrive = "E:"
$Evidence = Join-Path $RepoRoot `
  "Build\SecureBootCryptoAuditEvidence\machine-name-YYYY-MM-DD"

New-Item -ItemType Directory -Force $Evidence | Out-Null
Copy-Item "$UsbDrive\SecureBootCryptoAuditApp_JUNIT.XML" $Evidence
Copy-Item "$UsbDrive\SecureBootCryptoAuditJournal.bin" $Evidence
Copy-Item "$UsbDrive\SecureBootCryptoAuditApp_Cache.dat" $Evidence
```

### 7. Validate and read the evidence

From the `SecureBootCryptoAudit` directory, replace the example evidence path:

```bash
evidence="${HOME}/audit-evidence/machine-name-YYYY-MM-DD"

python3 Host/PrintEcitReport.py \
  "$evidence/SecureBootCryptoAuditApp_JUNIT.XML"

python3 Host/InspectJournal.py \
  "$evidence/SecureBootCryptoAuditJournal.bin" |
  tee "$evidence/journal.json"

python3 Host/GenerateMarkdownReport.py \
  "$evidence/SecureBootCryptoAuditApp_JUNIT.XML" \
  --journal-json "$evidence/journal.json" \
  --target-name "machine-name" \
  --output "$evidence/SecureBootCryptoAuditReport.md"
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

`GenerateMarkdownReport.py` produces a reviewable report with run identity,
the current findings, per-feature ECIT totals, and a result row for every
operational, profile-coverage, and modeled-claim testcase. Pass
`--journal-json` only with the output of `InspectJournal.py`, which validates
the binary journal's CRC and binding to the supplied manifest. The Markdown
generator additionally requires its fixture ID and manifest SHA-256 to match
JUnit's `[AUDIT_RUN]`, journal version 3, `PERSISTENTLY_VERIFIED`, exactly two
boots, a successful prior result, and the clean Setup Mode baseline
(`SetupMode=1`, `SecureBoot=0`, absent PK/`db`/`dbx` with zero size and
attributes). It recomputes JUnit totals from testcase outcomes and rejects
missing or inconsistent suite totals and conflicting testcase outcomes.
Invalid evidence is an error; no report is written. Without a journal, the
report makes no journal-based recovery claim.

The journal proves the clean baseline at the reboot checkpoint, before later
destructive scenarios. It does not prove final cleanup or restoration of the
original platform configuration. Preserve failed testcase diagnostics and
independently verify post-audit restoration. Both report tools validate and
aggregate JUnit totals; the format-sweep finding counts actual modeled native
claim results rather than assuming a fixed number of advertised formats.
Host evidence parsing preserves repeated contextual feature entries, unions
their advertisements for claim correlation, and retains each entry's coverage
rows. Like the firmware validator, it rejects repetitions of the four
singleton Secure Boot update/image authorization and verification features.

The current identity binds the fixture, not a unique execution: artifacts
from separate runs using the same fixture and manifest cannot be distinguished
by this check. Keep each run's evidence together as described above.

The checked-in `Host/TestData.manifest.json` (X64) and
`Host/TestDataAArch64.manifest.json` (AARCH64) are byte-bound evidence with
CRLF line endings. Do not normalize their line endings: each raw SHA-256 is
embedded in the matching application and checked against the reboot journal.
The generator hashes exactly the CRLF bytes it writes.
Keep textual diffs enabled so changes to these manifests remain reviewable.

For QEMU, pass
`--restored-variable-store-sha256` with the independently verified restored
store hash to include it in the report. `--target-name` accepts a
non-sensitive platform or lab-target label; do not put serial numbers in the
report.

### 8. Restore the machine

After preserving the evidence:

1. restore the known-good PK, KEK, `db`, and `dbx` through the independently
   tested recovery path;
2. restore the original boot order;
3. verify Secure Boot mode and normal boot;
4. verify disk unlock, measured boot, and attestation behavior; and
5. retain the evidence with the restored-configuration verification.

## QEMU developer prototype

Physical platforms are the primary target and the workflow above is
authoritative. QEMU is a secondary development environment for exercising the
same standalone application, FAT filesystem, two-boot sequence, evidence
collection, and recovery checks.

See [QEMU.md](QEMU.md) for the architecture-parameterized QemuQ35/X64 and
QemuArmVirt/AARCH64 procedure. Each architecture embeds signed images derived
from its own `HelloUefiApp.efi`, so X64 and AARCH64 results are each
architecture-native.

## Targets without ECIT

An absent or invalid ECIT is an explicit audit failure, but it does not prevent
the application from collecting an RSA Secure Boot baseline. RSA is the
interoperability baseline expected from the current ecosystem; the audit does
not infer support for ECDSA, ML-DSA, V2 signature lists, or other extended
capabilities when ECIT is unavailable. In this mode, the audit runs only:

- audit run identity;
- ECIT correlation and recovery-state classifier self-tests;
- interrupted-state recovery using RSA PK A;
- RSA-2048 PK A-to-B rotation and cleanup; and
- RSA-authenticated PK update replay rejection.

The reboot-persistence, ML-DSA, capability-path, format-sweep, correlation
report, modeled-claim, and profile-coverage tests skip because their ECIT
prerequisite was not met. The application does not request a reboot in this
mode. A complete RSA-only negative run has 66 native tests: 6 pass, the ECIT
parse test fails, and 59 skip.

This mode is intended to characterize platforms such as an upstream OVMF build
that does not publish ECIT. It is not an ECIT conformance result, and the
Secure Boot variable store must still be independently recoverable because the
RSA baseline tests modify PK and `db`.

## Interpret the result

First check the unit-test result. Every operational scenario must pass with no
framework errors. Each modeled ECIT claim is also a native JUnit test: verified
claims pass, coverage limitations and unadvertised claims skip, and mismatches
or inconclusive results fail. Two profile-coverage tests skip while advertised
digest/signature combinations remain unmodeled.

The correlation testcase writes compact `[ECIT_V2]` advertisement and derived
profile metadata into its JUnit log. `PrintEcitReport.py` validates that
metadata, merges it with the native `EcitClaim` testcase outcomes, and builds
the human-readable table; the firmware does not format or duplicate the result
table. Read the generated `ECIT_SUMMARY` after the native results because
passing modeled claims do not mean every advertised ECIT combination was
exercised.

| Validation | Meaning | Native JUnit result |
| --- | --- | --- |
| `VERIFIED` | ECIT advertised the claim and the corresponding operation succeeded. | Pass |
| `OVER_REPORTED` | ECIT advertised the claim but the operation failed. | Fail |
| `NOT_TESTED` | ECIT advertised the claim but no exact operational observation was made. | Skip |
| `NOT_INDEPENDENTLY_VERIFIABLE` | The operation succeeded, but the available evidence cannot independently establish the exact claim. | Skip |
| `UNADVERTISED_SUCCESS` | An operation succeeded for a claim ECIT did not advertise. | Fail by default; pass with a warning only after policy opt-in |
| `VERIFIED_ABSENT` | A precise negative probe confirmed an unadvertised capability is unavailable. | Pass |
| `NOT_ADVERTISED` | A modeled probe was skipped because the target did not advertise it. | Skip |
| `INCONCLUSIVE` | The audit could not establish a trustworthy operational result. | Fail |

Platforms that intentionally accept successful unadvertised operations can set
`gUefiTestingPkgTokenSpaceGuid.PcdSecureBootCryptoAuditAllowUnadvertisedSuccess`
to `TRUE`. The default is `FALSE`. This policy changes the disposition of an
observed unadvertised success; it does not enable additional destructive
probes.

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

### Example validated QemuQ35 result

The table below is a representative fixture-v6 QemuQ35/X64 result, not a
required platform result. A target's ECIT advertisements determine its own
denominator, and the generated report is authoritative for that run.

| Feature | Advertised rows | Verified | Not tested |
| --- | ---: | ---: | ---: |
| Unknown opaque feature | 1 | 0 | 1 |
| Secure Boot Image Verification | 74 | 8 | 66 |
| Secure Boot Image Authorization | 14 | 14 | 0 |
| Secure Boot Image Revocation | 14 | 14 | 0 |
| Secure Boot Database Update Verification | 119 | 8 | 111 |
| Secure Boot Database Update Authorization | 1 | 1 | 0 |
| **Total** | **223** | **45** | **178** |

The same run produced 66 native JUnit testcases: 62 passed, two modeled V1
SHA-1 signature-list claims skipped because they were not advertised, and two
profile-coverage cases skipped because advertised digest/signature pairings
remain untested. It had zero failures and zero errors. Run
`Host/PrintEcitReport.py` to obtain the complete per-claim table instead of
treating this example as a fixed allowlist.

The `AuditRunIdentity` testcase records one `[AUDIT_RUN]` line containing the
application version, target architecture, sanitized firmware vendor, firmware
revision, UEFI revision, fixture ID, and fixture-manifest SHA-256. The host
report validates and prints this record before the ECIT table. This
non-sensitive firmware identity supplements, but does not replace, the
operator-recorded machine identity and restoration evidence.

For each run, archive:

- the complete JUnit XML;
- the serial or console log;
- the printed ECIT table and `ECIT_SUMMARY`;
- the baseline and restored variable-store hashes for QEMU; and
- `SecureBootCryptoAuditJournal.bin` for a physical run.

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

`SecureBootCryptoAuditJournal.bin` is written to the current test volume before
the reboot boundary. The journal contains no private key material. It records
the fixture ID, SHA-256 of the architecture's fixture manifest, boot count, prior
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
python3 Host/InspectJournal.py SecureBootCryptoAuditJournal.bin
```

For an AARCH64 run, add `--arch AARCH64` so the AARCH64 manifest is used.

## Generate the public fixture

Signed test images are architecture-specific, so each architecture has its own
fixture. Build `HelloUefiApp.efi` for the target architecture, then run the
generator from the `SecureBootCryptoAudit` directory:

```bash
python3 Host/GenerateTestData.py              # X64 (default)
python3 Host/GenerateTestData.py --arch AARCH64
```

The X64 input defaults to the QemuQ35Pkg build output; the AARCH64 input
defaults to `Build/UefiTestingPkg/DEBUG_GCC/AARCH64/HelloUefiApp.efi` from the
QemuPkg CI build. The generator refuses an input whose PE machine type does
not match `--arch`.

For each architecture the generator writes:

| Architecture | Firmware source | Manifest |
| --- | --- | --- |
| X64 | `UEFI/TestData.c` | `Host/TestData.manifest.json` |
| AARCH64 | `UEFI/TestDataAArch64.c` | `Host/TestDataAArch64.manifest.json` |

- The firmware source contains public certificates, signature lists,
  authenticated updates, signed test images, fixture ID, and manifest hash.
  `SecureBootCryptoAuditApp.inf` compiles the one matching the build
  architecture.
- The manifest contains artifact hashes, explicit certificate roles,
  unsigned-image identity, and ECIT prerequisites. The current fixture schema
  is `secure-boot-key-audit-v6`.

Each run generates fresh keys, so the X64 and AARCH64 fixtures use different
keys and manifests; regenerate only the architecture whose image changed.

RSA, ECDSA, and ML-DSA private keys exist only in memory while the generator
runs and are never written to disk or emitted into the application. The v6
fixture contains 150 artifacts and 109 authenticated-variable updates.

## Independently verify the public fixture

### Test ECIT structural validation

From the `mu_plus` root, build and run the native C validator tests
with the existing host unit-test framework:

```bash
stuart_ci_build \
  -c .pytool/CISettings.py \
  -p UefiTestingPkg -a X64 -t NOOPT \
  TOOL_CHAIN_TAG=GCC \
  'BLD_*_SECURE_BOOT_CRYPTO_AUDIT_ONLY=TRUE'
```

The host executable and firmware application compile the same
`UEFI/EcitTable.c`. Its bounded `ValidateEcitTable()` helper does not discover
configuration tables or change claim state. Tests cover valid and empty
tables, actual truncated allocations, checksum/header/reserved-field errors,
entry lengths and eight-byte alignment, duplicate singleton features,
unconsumed trailing bytes, entry-count mismatch, and lengths immediately
below, at, and above the one-MiB limit. AddressSanitizer is enabled by the
standard host-test configuration. Repeated opaque features (and the existing
revocation-feature policy) retain their previous behavior; this is not a
blanket prohibition on repeated feature GUIDs or a payload-semantic validator.

The firmware wrapper logs validation failures and only populates claims after
successful validation. A configuration-table pointer supplies its declared
length, not an independently measured allocation size; bounded host tests do
not establish that arbitrary firmware pointers are mapped/readable.

### Verify the generated fixture

After generation, verify the exact arrays that will be compiled into the
firmware:

```bash
python3 Host/VerifyTestData.py                # X64 (default)
python3 Host/VerifyTestData.py --arch AARCH64
python3 -m unittest -v \
  Host.test_inspect_journal \
  Host.test_print_ecit_report \
  Host.test_generate_markdown_report \
  Host.test_verify_test_data
```

The verifier does not import the generator or use its construction helpers. It
parses the selected architecture's fixture source and independently checks:

- that every signed image targets the selected architecture's PE machine type;
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
