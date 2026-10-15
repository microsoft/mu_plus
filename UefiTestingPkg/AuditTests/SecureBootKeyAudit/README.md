# Secure Boot Key ECIT Audit

`SecureBootKeyAuditApp` performs an initial end-to-end check of the UEFI 2.12
Crypto Indicator Table claims for Secure Boot database updates.

The scenarios require a disposable Setup Mode variable store. They:

1. validate the ECIT header, checksum, entries, and bounds;
2. require X.509 database-update authorization plus RSA/SHA-256 and
   ML-DSA-87/SHA-512 update verification;
3. rotate RSA-2048 PK A to RSA-2048 PK B and back;
4. enroll an ML-DSA-87 PK directly from Setup Mode and use it to authorize its
   own deletion; and
5. enroll RSA-2048 PK A, rotate it to ML-DSA-87, use ML-DSA-87 to rotate back
   to RSA-2048, and restore Setup Mode;
6. enroll an ML-DSA-87 PK and a V1 SHA-512 TBSCertificate hash in `db`, then
   load and start an ML-DSA-87-signed UEFI application; and
7. repeat the image authorization with the UEFI 2.12 V2 digest-only
   TBSCertificate-hash format.

The ML-DSA authenticated updates use pure ML-DSA-87 CMS as specified by RFC
9882. Because the detached CMS signatures omit signed attributes, their CMS
digest algorithm is SHA-512.

The image tests use SHA-512 Authenticode and pure ML-DSA-87. The generator
embeds the signer certificate in the signed image and stores only its
TBSCertificate hash in `db`.

The application refuses to mutate an existing PK or `db` unless it exactly
matches generated test data. A failure can still leave test state installed if
firmware accepts a mutation but becomes unable to authenticate the matching
cleanup update. Run only with a disposable QEMU variable store.

## Generate the public fixture

Build `HelloUefiApp.efi`, then run the generator from the
`SecureBootKeyAudit` directory:

```bash
python3 Host/GenerateTestData.py
```

The generator writes:

- `UEFI/TestData.c`, containing public certificates, signature lists,
  authenticated updates, and the signed test image; and
- `Host/TestData.manifest.json`, containing hashes and ECIT prerequisites.

RSA and ML-DSA private keys exist only in memory while the generator runs and
are never written to disk or emitted into the application.
