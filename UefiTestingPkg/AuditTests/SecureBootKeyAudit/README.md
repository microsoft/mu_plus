# Secure Boot Key ECIT Audit

`SecureBootKeyAuditApp` performs an initial end-to-end check of the UEFI 2.12
Crypto Indicator Table claims for Secure Boot database updates.

This is a focused ECIT and Secure Boot cryptographic capability audit. A
passing result does not establish whole-platform UEFI 2.12 compatibility;
requirements outside the ECIT and Secure Boot operations listed below remain
unevaluated by this application.

The scenarios require a disposable or independently recoverable Setup Mode
variable store. They:

1. parse the ECIT header, checksum, entries, bounds, and advertised claims once
   without using those claims to gate safe operational tests;
2. require X.509 database-update authorization plus RSA/SHA-256 and
   ML-DSA-87/SHA-512 update verification;
3. rotate RSA-2048 PK A to RSA-2048 PK B and back;
4. enroll an ML-DSA-87 PK directly from Setup Mode and use it to authorize its
   own deletion; and
5. enroll RSA-2048 PK A, rotate it to ML-DSA-87, use ML-DSA-87 to rotate back
   to RSA-2048, and restore Setup Mode;
6. enroll an ML-DSA-87 PK and a V1 SHA-512 TBSCertificate hash in `db`, then
   prove Secure Boot enforcement is active, reject the image before `db`
   enrollment, reject a body-tampered image after enrollment, then separately
   load and start the authorized ML-DSA-87-signed UEFI application; and
7. repeat the image authorization with the UEFI 2.12 V2 digest-only
   TBSCertificate-hash format; and
8. simulate an interrupted run with a recognized test PK, preserve a read-only
   preflight snapshot, prove inspection does not mutate the PK, and then enter
   an explicitly logged recovery phase; and
9. write a versioned, CRC-protected journal to the test volume, request a cold
   reboot, resume the active unit test, reread ECIT and Secure Boot state, and
   prove both match the pre-reboot evidence; and
10. enroll a `db` entry with an ML-DSA-87 authenticated update, replay the
    identical envelope, require `EFI_SECURITY_VIOLATION`, and prove the `db`
    attributes, size, bytes, and SHA-256 remain unchanged; and
11. authorize RSA-2048/SHA-256 and ML-DSA-87/SHA-512 signers, execute images
    containing both independent `WIN_CERTIFICATE` entries in both orders,
    prove either remaining signer authorizes the image after the other signer
    is revoked, and deny both images after both signers are revoked.

The ML-DSA authenticated updates use pure ML-DSA-87 CMS as specified by RFC
9882. Because the detached CMS signatures omit signed attributes, their CMS
digest algorithm is SHA-512.

The image tests use RSA-2048/SHA-256 and pure ML-DSA-87/SHA-512
Authenticode. The generator uses separate keys for the PK/update authority,
RSA image signer, and ML-DSA image signer. It embeds each image-signer
certificate in its signature, stores only the certificate's TBSCertificate
hash in `db`, and uses the PK authority to authenticate the `db` and `dbx`
updates. The multi-signature fixtures contain the same signatures over the
same unsigned image in opposite certificate-table orders.

The final correlation test reports each ECIT claim independently from the
observed operation as verified, over-reported, under-reported, verified absent,
not tested, inconclusive, or not independently verifiable. Because the current
ECIT verification payload is a flat OID set, the SHA-512 plus ML-DSA-87 image
profile is reported as an operationally successful candidate pair with
`FLAT_PROFILE_AMBIGUITY`, not as an exact profile advertised by ECIT.

The application refuses to mutate an existing PK, `db`, or `dbx` unless it
exactly matches generated test data. Baseline preflight is read-only and logs
the mode, attributes, sizes, and recognized state before any explicit
recovery mutation. A recovery attempt validates the complete
mode/PK/`db`/`dbx` tuple before its first `SetVariable()` call; unknown or
inconsistent state remains untouched for host evidence capture and baseline
restoration.

A failure can still leave test state installed if firmware accepts a mutation
but becomes unable to authenticate the matching cleanup update. Run only with
a disposable QEMU variable store or on a dedicated physical test device with
an independently proven recovery path.

Authenticated-variable conformance steps use a data-driven executor that
validates the required pre-state, exact returned status, expected post-state,
and atomicity for rejected operations. It emits before/after variable
SHA-256 evidence into the unit-test log. Each scenario also declares the ECIT
claims it covers. The executor records successful, failed, or inconclusive
observations for those claims without test-specific reporting code.

The final ECIT correlation test emits both machine-readable `ECIT_RESULT` /
`ECIT_SUMMARY` records and a DumpEcit-style coverage table.
The table groups every ECIT entry, enumerates every advertised OID or signature
list type, and shows its operational observation and validation state. Claims
without a corresponding scenario are explicitly `NOT_TESTED` rather than
omitted from the denominator. The overall verdict distinguishes `PASS` from
`PASS_WITH_LIMITATIONS`, so incomplete coverage or an operationally successful
but independently unverifiable profile is not hidden by the passing unit-test
count.

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
oracle. The parser and coverage reporter both consume the registry, so they do
not require a second claim mapping.

## Platform qualification requirement

Release qualification requires successful execution on both the reference
QEMU platform and a physical device that advertises the capabilities under
test. Physical-device execution is destructive testing of Secure Boot
variables and must only be performed on a dedicated device for which the
operator:

- is authorized to clear and restore the Secure Boot variable store;
- has backed up the platform's known-good Secure Boot configuration;
- has verified a recovery mechanism that does not depend on the keys or
  authenticated-update path under test; and
- can recover the device if a partially accepted transition leaves it unable
  to authenticate the generated cleanup update.

The recovery mechanism must be exercised before the audit, not merely
documented. Examples include a platform firmware recovery flow, an
out-of-band management recovery mechanism, or direct flash restoration. Do
not run the mutating scenarios on an end-user or production device.

## Run from USB on a physical machine

`SecureBootKeyAuditApp` is an X64 UEFI application. The QemuQ35 build can be
copied to a FAT32 USB drive and launched from an X64 UEFI Shell, but the
physical run does not have QEMU's host-authoritative variable-store snapshot
and restoration. The application performs recognized-state cleanup; it cannot
guarantee recovery if firmware accepts a mutation and later rejects its
matching cleanup update.

Do not run this procedure until every physical-device qualification
requirement above is satisfied. In particular:

- use a dedicated lab device, not a production or end-user system;
- export or otherwise back up PK, KEK, `db`, and `dbx`;
- account for BitLocker, device-encryption recovery, measured boot, and
  attestation effects;
- connect external power;
- exercise the independent firmware or flash recovery path before the audit;
  and
- configure the device with `SetupMode=1`, `SecureBoot=0`, PK absent, and
  both `db` and `dbx` absent.

The audit refuses to recover or overwrite an unknown PK/`db`/`dbx` tuple, but
that guard is not a substitute for an independent recovery path.

### Build the application

From the `mu_tiano_platforms` root:

```bash
stuart_setup \
  -c Platforms/QemuQ35Pkg/PlatformBuild.py \
  TOOL_CHAIN_TAG=GCC5

stuart_build \
  -c Platforms/QemuQ35Pkg/PlatformBuild.py \
  TOOL_CHAIN_TAG=GCC5
```

The resulting application is:

```text
Build/QemuQ35PkgX64/DEBUG_GCC5/X64/SecureBootKeyAuditApp.efi
```

The public certificates, authenticated updates, signed test images, fixture
identity, and manifest hash are compiled into the application. No private key
material is present, and no separate fixture files are required on the target.
The QemuQ35 output is X64-only; an AARCH64 target requires a corresponding
AARCH64 platform build.

### Prepare the USB drive

Format the drive as FAT32 and use this layout:

```text
USB:\
  EFI\
    BOOT\
      BOOTX64.EFI
  SecureBootKeyAuditApp.efi
  startup.nsh
```

`BOOTX64.EFI` must be a trusted X64 UEFI Shell compatible with the target.
The application uses `ShellLib` for its journal, so launching it from a shell
is preferred over renaming the audit application itself to `BOOTX64.EFI`.

Copy the application from Windows PowerShell, replacing `E:` with the USB
drive:

```powershell
$UsbDrive = "E:"
$AuditApp = "\\wsl.localhost\Ubuntu-24.04\home\doug\git\pqc" +
            "\mu_tiano_platforms\Build\QemuQ35PkgX64\DEBUG_GCC5" +
            "\X64\SecureBootKeyAuditApp.efi"

Copy-Item $AuditApp "$UsbDrive\SecureBootKeyAuditApp.efi"
```

Create `startup.nsh` in the USB root:

```nsh
@echo -off
echo Starting the ECIT Secure Boot audit...
SecureBootKeyAuditApp.efi
echo Audit application returned.
```

Keeping the script and application together avoids depending on whether the
shell maps the USB drive as `fs0:`, `fs1:`, or another alias.

### Execute and resume

1. Confirm the device identity, external power, backed-up Secure Boot
   configuration, and tested independent recovery path.
2. Enter firmware setup and place the device in Setup Mode with PK, `db`, and
   `dbx` absent.
3. Put the USB UEFI Shell first in the boot order.
4. Boot the USB and allow `startup.nsh` to launch the audit.
5. The persistent-journal scenario requests one cold reboot.
6. Ensure the same USB boots again. The shell must rerun `startup.nsh` from
   the same removable volume so the framework and journal resume on boot 2.
7. After the application returns, archive the console or serial log and
   `SecureBootKeyAuditJournal.bin`.
8. Restore the machine's known-good Secure Boot configuration through the
   independently validated recovery path, then verify its PK, KEK, `db`,
   `dbx`, Secure Boot mode, disk-unlock behavior, and normal boot.

The QEMU host runner automatically harvests JUnit results and restores a
copied variable store. Those protections are not present on a physical
machine. Capture a serial console when possible; otherwise preserve all
operator-visible output before restoring the device.

If the audit stops after changing Secure Boot state, do not repeatedly
relaunch it or improvise cleanup. Preserve the USB journal and logs, record
the observed PK/`db` state, and use the independent recovery mechanism.

## Persistent reboot journal

`SecureBootKeyAuditJournal.bin` is written to the current test volume before
the reboot boundary. The journal contains no private key material. It records
the fixture ID, SHA-256 of `Host/TestData.manifest.json`, SHA-256 of the parsed
ECIT, boot count, prior result, phase, and the recognized Secure Boot baseline.
Journal format version 2 records PK, `db`, and `dbx` baseline state. The
firmware flushes each fixed-size journal update before continuing and rejects
an invalid signature, version, size, CRC, fixture identity, manifest hash, or
phase.

The reboot test advances through:

```text
PREPARED -> REBOOT_PENDING -> PERSISTENTLY_VERIFIED
```

A volatile marker distinguishes a real reset from relaunching the application
in the same boot. Project Mu unit-test persistence resumes the active test and
preserves its pre-reset log. After reboot, the audit reparses ECIT and rereads
the Secure Boot baseline before recording `PERSISTENTLY_VERIFIED`.

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
  current fixture schema is `secure-boot-key-audit-v3`.

RSA and ML-DSA private keys exist only in memory while the generator runs and
are never written to disk or emitted into the application.

## Independently verify the public fixture

After generation, verify the exact arrays that will be compiled into the
firmware:

```bash
python3 Host/VerifyTestData.py
python3 -m unittest -v Host.test_inspect_journal Host.test_verify_test_data
```

The verifier does not import the generator or use its construction helpers. It
parses `UEFI/TestData.c` and independently checks:

- every artifact size and SHA-256 binding in the versioned manifest;
- X.509 ESL layout and generated self-signed certificate signatures;
- every authenticated-variable envelope, payload, signer role, detached CMS
  signature, algorithm, issuer, and serial number;
- every independent PE `WIN_CERTIFICATE` boundary and alignment, the
  Authenticode SHA-256 and SHA-512 digests, `SpcIndirectDataContent`, embedded
  certificates, RSA-2048 and ML-DSA-87 signatures, and reversed signature
  order over the same unsigned image;
- single- and multi-entry V1 and V2 SHA-512 TBSCertificate-hash ESLs against
  the embedded image-signer certificates; and
- distinct certificate and public-key fingerprints for the PK/update
  authority, RSA image-signer, and ML-DSA image-signer roles.

The command also corrupts the final Authenticode object after rebinding its
manifest artifact hash and requires independent signature verification to
reject it. The unit tests additionally reject role-key reuse and malformed ESL
structure.
