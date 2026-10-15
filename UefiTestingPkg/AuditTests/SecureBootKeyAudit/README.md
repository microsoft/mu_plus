# Secure Boot Key ECIT Audit

`SecureBootKeyAuditApp` performs an initial end-to-end check of the UEFI 2.12
Crypto Indicator Table claims for Secure Boot database updates.

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
   prove both match the pre-reboot evidence.

The ML-DSA authenticated updates use pure ML-DSA-87 CMS as specified by RFC
9882. Because the detached CMS signatures omit signed attributes, their CMS
digest algorithm is SHA-512.

The image tests use SHA-512 Authenticode and pure ML-DSA-87. The generator
embeds the signer certificate in the signed image and stores only its
TBSCertificate hash in `db`.

The final correlation test reports each ECIT claim independently from the
observed operation as verified, over-reported, under-reported, verified absent,
not tested, inconclusive, or not independently verifiable. Because the current
ECIT verification payload is a flat OID set, the SHA-512 plus ML-DSA-87 image
profile is reported as an operationally successful candidate pair with
`FLAT_PROFILE_AMBIGUITY`, not as an exact profile advertised by ECIT.

The application refuses to mutate an existing PK or `db` unless it exactly
matches generated test data. Baseline preflight is read-only and logs the mode,
attributes, sizes, and recognized state before any explicit recovery mutation.
A failure can still leave test state installed if firmware accepts a mutation
but becomes unable to authenticate the matching cleanup update. Run only with
a disposable QEMU variable store or on a dedicated physical test device with
an independently proven recovery path.

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

## Persistent reboot journal

`SecureBootKeyAuditJournal.bin` is written to the current test volume before
the reboot boundary. The journal contains no private key material. It records
the fixture ID, SHA-256 of `Host/TestData.manifest.json`, SHA-256 of the parsed
ECIT, boot count, prior result, phase, and the recognized Secure Boot baseline.
The firmware flushes each fixed-size journal update before continuing and
rejects an invalid signature, version, size, CRC, fixture identity, manifest
hash, or phase.

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
  authenticated updates, the signed test image, fixture ID, and manifest hash;
  and
- `Host/TestData.manifest.json`, containing hashes and ECIT prerequisites.

RSA and ML-DSA private keys exist only in memory while the generator runs and
are never written to disk or emitted into the application.
