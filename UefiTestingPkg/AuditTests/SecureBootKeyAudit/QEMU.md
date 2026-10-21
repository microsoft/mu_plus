# Secure Boot Key Audit QEMU Prototype

This guide prototypes the physical Secure Boot key audit workflow on the QEMU
platforms in `mu_tiano_platforms`. The physical-machine procedure in
[README.md](README.md) remains the authoritative workflow and deployment
target.

QEMU exercises the same:

- standalone audit application;
- FAT filesystem and `startup.nsh`;
- cold-reset and two-boot framework resume;
- JUnit, journal, and cache artifacts;
- host-side evidence rendering; and
- complete emulated variable-store backup and restoration.

## Architecture support

The QEMU mechanics are shared between the supported platform architectures:

| Architecture | Platform | Writable variable-state image | Audit status |
| --- | --- | --- | --- |
| X64 | QemuQ35Pkg | `QEMUQ35_VARS.fd` | Fully validated |
| AARCH64 | QemuArmVirtPkg | `SECURE_FLASH0.fd` | Harness supported; full audit not yet validated |

The current generated fixture embeds signed X64 PE/COFF images derived from
`QemuQ35PkgX64/DEBUG_GCC5/X64/HelloUefiApp.efi`. An AARCH64 build of
`SecureBootKeyAuditApp.efi` does not make those embedded images AARCH64.
Consequently, AARCH64 can validate the QEMU transport and two-boot mechanics,
but image authorization, revocation, and verification results are not
conformance evidence until the fixture generator produces and independently
verifies architecture-matched AARCH64 images.

Do not compare X64 and AARCH64 ECIT verdicts until both use equivalent native
fixtures.

## Prerequisites

The examples use Linux-hosted QEMU, `mtools`, and `dosfstools`. Run all
commands from the `mu_tiano_platforms` root:

```bash
command -v mcopy
command -v mkfs.vfat
command -v sha256sum
```

Set the target architecture to `X64` or `AARCH64`, then derive all remaining
paths:

```bash
arch=X64

root=$PWD
audit_dir="$root/Common/MU/UefiTestingPkg/AuditTests/SecureBootKeyAudit"
startup="$audit_dir/startup.nsh"

case "$arch" in
  X64)
    platform=QemuQ35Pkg
    platform_build="$root/Platforms/QemuQ35Pkg/PlatformBuild.py"
    build="$root/Build/QemuQ35PkgX64/DEBUG_GCC5"
    variable_store="$build/FV/QEMUQ35_VARS.fd"
    ;;
  AARCH64)
    platform=QemuArmVirtPkg
    platform_build="$root/Platforms/QemuArmVirtPkg/PlatformBuild.py"
    build="$root/Build/QemuArmVirtPkg/DEBUG_GCC5"
    variable_store="$build/FV/SECURE_FLASH0.fd"
    ;;
  *)
    echo "Unsupported architecture: $arch" >&2
    return 2 2>/dev/null || exit 2
    ;;
esac

audit_app="$root/Build/UefiTestingPkg/DEBUG_GCC5/$arch/SecureBootKeyAuditApp.efi"
virtual_drive="$build/VirtualDrive.img"
variable_backup="$variable_store.secure-boot-audit-backup"
evidence="$root/Build/SecureBootKeyAuditEvidence-$arch"

printf 'Architecture:  %s\n' "$arch"
printf 'Platform:      %s\n' "$platform"
printf 'Audit app:     %s\n' "$audit_app"
printf 'Virtual drive: %s\n' "$virtual_drive"
printf 'Variable data: %s\n' "$variable_store"
printf 'Evidence:      %s\n' "$evidence"
```

## 1. Build the standalone application

Build the same standalone binary that would be copied to a physical USB:

```bash
stuart_ci_build \
  -c .pytool/CISettings.py \
  -p UefiTestingPkg \
  -a "$arch" \
  -t DEBUG \
  TOOL_CHAIN_TAG=GCC5 \
  BLD_*_SECURE_BOOT_KEY_AUDIT_ONLY=TRUE

test -f "$audit_app"
sha256sum "$audit_app"
```

For AARCH64, a successful application build proves only that the audit
application is buildable for AARCH64. It does not replace the embedded X64
image fixtures.

## 2. Build the QEMU platform

```bash
stuart_build \
  -c "$platform_build" \
  TOOL_CHAIN_TAG=GCC5
```

Confirm that the architecture-specific writable variable-state image exists:

```bash
test -f "$variable_store"
sha256sum "$variable_store"
```

## 3. Back up the complete variable state

Refuse to overwrite an existing backup:

```bash
test ! -e "$variable_backup"
cp --preserve=all "$variable_store" "$variable_backup"

baseline_sha=$(sha256sum "$variable_backup" | cut -d' ' -f1)
echo "Baseline variable-state SHA-256: $baseline_sha"
```

For QemuQ35, the writable state is the dedicated `QEMUQ35_VARS.fd`. For
QemuArmVirt, the writable secure flash image is `SECURE_FLASH0.fd`; back up and
restore the entire file rather than attempting to extract only its variable
region.

## 4. Create the virtual USB

Create a clean 60 MiB FAT32 image and copy only the architecture-matched audit
application:

```bash
rm -f -- "$virtual_drive"
truncate -s 60M "$virtual_drive"
mkfs.vfat -F 32 "$virtual_drive"

mcopy -D overwrite -i "$virtual_drive" "$audit_app" ::
mdir -i "$virtual_drive" ::
```

The checked-in `startup.nsh` is installed by the platform runner in the next
step.

## 5. Run the two-boot sequence

Prepare a clean evidence directory without deleting unrelated files:

```bash
mkdir -p "$evidence"
rm -f -- \
  "$evidence/qemu-console.log" \
  "$evidence/SecureBootKeyAuditApp_JUNIT.XML" \
  "$evidence/SecureBootKeyAuditJournal.bin" \
  "$evidence/SecureBootKeyAuditApp_Cache.dat" \
  "$evidence/ecit-report.txt" \
  "$evidence/journal.json"
```

Run QEMU:

```bash
set -o pipefail
stuart_build \
  -c "$platform_build" \
  --FlashRom \
  TOOL_CHAIN_TAG=GCC5 \
  RUN_TESTS=FALSE \
  EMPTY_DRIVE=FALSE \
  STARTUP_NSH="$startup" \
  SHUTDOWN_AFTER_RUN=TRUE \
  2>&1 | tee "$evidence/qemu-console.log"
```

`RUN_TESTS=FALSE` preserves the physical artifact names instead of invoking the
platform test harvester. `EMPTY_DRIVE=FALSE` preserves the manually prepared
virtual USB. `SHUTDOWN_AFTER_RUN=TRUE` appends a QEMU-only shutdown after the
application returns on boot 2.

The application requests a cold reset during boot 1. The same QEMU process
then boots again, reruns `startup.nsh`, resumes the framework from its cache,
writes JUnit, and returns to the shell.

## 6. Collect the physical-style artifacts

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

Use `::/filename` for direct `mtools` image access. The `a:/filename` syntax is
not valid with this `-i` workflow.

## 7. Restore before interpreting results

Restore the variable state even if QEMU or evidence collection failed:

```bash
cp --preserve=all "$variable_backup" "$variable_store"
restored_sha=$(sha256sum "$variable_store" | cut -d' ' -f1)

echo "Restored variable-state SHA-256: $restored_sha"
test "$restored_sha" = "$baseline_sha"
rm -- "$variable_backup"
```

Do not interpret the audit as safely completed until the restored hash matches
the baseline hash.

## 8. Render and inspect the evidence

```bash
python3 "$audit_dir/Host/PrintEcitReport.py" \
  "$evidence/SecureBootKeyAuditApp_JUNIT.XML" |
  tee "$evidence/ecit-report.txt"

python3 "$audit_dir/Host/InspectJournal.py" \
  "$evidence/SecureBootKeyAuditJournal.bin" |
  tee "$evidence/journal.json"
```

For X64, apply the result interpretation in [README.md](README.md). For
AARCH64, treat failures involving the embedded signed images as an expected
fixture-architecture limitation, not as platform conformance results. The
AARCH64 audit becomes comparable only after regenerating `TestData.c` from an
AARCH64 `HelloUefiApp.efi`, independently verifying the generated artifacts,
and completing the full two-boot run.
