# Secure Boot Crypto Audit QEMU Prototype

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

For the explicit all-profile robustness opt-in and its build switch, see
[README.md](README.md#explicit-all-profile-robustness-opt-in). Run a default
baseline and an opt-in build against the same firmware, restoring the complete
variable-store and drive images between runs. Keep separate evidence directories
and record both application and firmware SHA-256 hashes. Do not reuse the
framework cache between modes.

QemuQ35Pkg results are results for the local Project Mu firmware, not stock
OVMF. A stock OVMF comparison requires a separately identified CODE/VARS pair
and may lack ECIT; do not generalize local capability support to older devices.

The QEMU mechanics are shared between the supported platform architectures:

| Architecture | Platform | Writable variable-state image | Audit status |
| --- | --- | --- | --- |
| X64 | QemuQ35Pkg | `QEMUQ35_VARS.fd` | Fully validated |
| AARCH64 | QemuArmVirtPkg | `SECURE_FLASH0.fd` | Validated with the AARCH64 fixture |

Each architecture embeds signed images derived from its own
`HelloUefiApp.efi`: X64 uses `UEFI/TestData.c` and AARCH64 uses
`UEFI/TestDataAArch64.c`, selected by `SecureBootCryptoAuditApp.inf`. The two
fixtures are generated with different keys and are bound to different
manifests, so the application, journal, and host tools must all use the same
architecture.

Compare X64 and AARCH64 ECIT verdicts by claim, not by fixture identity.

AARCH64 authenticated-variable and image tests require the QemuArmVirtPkg
Standalone MM RNG provider, a 16 KiB Standalone MM stack, 0x8400-byte
authenticated variables, `DxeImageVerificationLib2`, published ECIT, and a
OneCrypto release whose AARCH64 assembly has BTI landing pads.

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
audit_dir="$root/Common/MU/UefiTestingPkg/AuditTests/SecureBootCryptoAudit"
startup="$audit_dir/startup.nsh"

case "$arch" in
  X64)
    platform=QemuQ35Pkg
    platform_build="$root/Platforms/QemuQ35Pkg/PlatformBuild.py"
    build="$root/Build/QemuQ35PkgX64/DEBUG_GCC"
    variable_store="$build/FV/QEMUQ35_VARS.fd"
    ;;
  AARCH64)
    platform=QemuArmVirtPkg
    platform_build="$root/Platforms/QemuArmVirtPkg/PlatformBuild.py"
    build="$root/Build/QemuArmVirtPkg/DEBUG_GCC"
    variable_store="$build/FV/SECURE_FLASH0.fd"
    ;;
  *)
    echo "Unsupported architecture: $arch" >&2
    return 2 2>/dev/null || exit 2
    ;;
esac

audit_app="$root/Build/QemuPkg/DEBUG_GCC/$arch/SecureBootCryptoAuditApp.efi"
virtual_drive="$build/VirtualDrive.img"
variable_backup="$variable_store.secure-boot-audit-backup"
evidence="$root/Build/SecureBootCryptoAuditEvidence-$arch"

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
  -p QemuPkg \
  -a "$arch" \
  -t DEBUG \
  TOOL_CHAIN_TAG=GCC \
  BLD_*_SECURE_BOOT_CRYPTO_AUDIT_ONLY=TRUE

test -f "$audit_app"
sha256sum "$audit_app"
```

The application build selects the fixture matching `$arch` automatically.

## 2. Build the QEMU platform

```bash
stuart_build \
  -c "$platform_build" \
  TOOL_CHAIN_TAG=GCC
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

The Linux platform runner also requires `mtool.conf` beside a manually
created image. The following uses its normal `a:` mapping and refuses to
continue if `/mnt/a` is already mounted. In that case, select the same unused
letter as `LinuxVirtualDrive` before proceeding.

```bash
if mountpoint -q /mnt/a; then
  echo "STOP: /mnt/a is mounted; resolve the platform drive mapping first." >&2
  return 1 2>/dev/null || exit 1
fi
if [ ! -f "$build/mtool.conf" ]; then
  printf 'drive+ a:\n  file="%s" exclusive\n' "$virtual_drive" > "$build/mtool.conf"
fi
MTOOLSRC="$build/mtool.conf" mdir a:
```

Create the configuration with `printf` inside Linux/WSL so its line endings
are LF, not Windows CRLF. If the existing mapping fails validation, stop and
inspect it rather than overwriting an unrelated mapping or launching QEMU.

The checked-in `startup.nsh` is installed by the platform runner in the next
step.

## 5. Run the two-boot sequence

Prepare a clean evidence directory without deleting unrelated files:

```bash
mkdir -p "$evidence"
rm -f -- \
  "$evidence/qemu-console.log" \
  "$evidence/SecureBootCryptoAuditApp_JUNIT.XML" \
  "$evidence/SecureBootCryptoAuditJournal.bin" \
  "$evidence/SecureBootCryptoAuditApp_Cache.dat" \
  "$evidence/ecit-report.txt" \
  "$evidence/journal.json"
```

Run QEMU:

```bash
set -o pipefail
stuart_build \
  -c "$platform_build" \
  --FlashRom \
  TOOL_CHAIN_TAG=GCC \
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
  ::/SecureBootCryptoAuditApp_JUNIT.XML "$evidence/"
mcopy -n -i "$virtual_drive" \
  ::/SecureBootCryptoAuditJournal.bin "$evidence/"
mcopy -n -i "$virtual_drive" \
  ::/SecureBootCryptoAuditApp_Cache.dat "$evidence/"

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
  "$evidence/SecureBootCryptoAuditApp_JUNIT.XML" |
  tee "$evidence/ecit-report.txt"

python3 "$audit_dir/Host/InspectJournal.py" \
  --arch "$arch" \
  "$evidence/SecureBootCryptoAuditJournal.bin" |
  tee "$evidence/journal.json"

python3 "$audit_dir/Host/GenerateMarkdownReport.py" \
  "$evidence/SecureBootCryptoAuditApp_JUNIT.XML" \
  --journal-json "$evidence/journal.json" \
  --restored-variable-store-sha256 "$restored_sha" \
  --target-name "$platform" \
  --output "$evidence/SecureBootCryptoAuditReport.md"
```

Apply the result interpretation in [README.md](README.md) to both
architectures.
