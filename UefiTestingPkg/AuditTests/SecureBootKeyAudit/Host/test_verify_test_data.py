# @file
# Unit tests for independent Secure Boot audit fixture verification.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import copy
import datetime
import hashlib
import json
import struct
import unittest
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ed25519, rsa
from cryptography.x509.oid import NameOID
from Host.VerifyTestData import (
    VerificationError,
    parse_c_arrays,
    verify_corruption_negative,
    verify_fixture,
    verify_self_signed_certificate,
)


class VerifyTestDataTests(unittest.TestCase):
    """Test positive and deliberately invalid final fixture objects."""

    @classmethod
    def setUpClass(cls) -> None:
        cls.host_dir = Path(__file__).resolve().parent
        cls.manifest_data = (cls.host_dir / "TestData.manifest.json").read_bytes()
        cls.manifest = json.loads(cls.manifest_data)
        cls.arrays = parse_c_arrays(cls.host_dir.parent / "UEFI" / "TestData.c")

    @staticmethod
    def bind_manifest(arrays, manifest):
        """Bind a modified manifest to its generated metadata array."""
        manifest_data = (json.dumps(manifest, indent=2) + "\n").encode()
        arrays["gSecureBootKeyAuditManifestSha256"] = hashlib.sha256(
            manifest_data
        ).digest()
        return manifest_data

    @staticmethod
    def self_signed_certificate(private_key, algorithm):
        """Create a certificate for testing rejected key profiles."""
        name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "Invalid Fixture")])
        return (
            x509.CertificateBuilder()
            .subject_name(name)
            .issuer_name(name)
            .public_key(private_key.public_key())
            .serial_number(1)
            .not_valid_before(
                datetime.datetime(2026, 1, 1, tzinfo=datetime.timezone.utc)
            )
            .not_valid_after(
                datetime.datetime(2036, 1, 1, tzinfo=datetime.timezone.utc)
            )
            .sign(private_key, algorithm)
        )

    def test_committed_fixture_is_valid(self) -> None:
        evidence = verify_fixture(
            self.arrays,
            self.manifest,
            self.manifest_data,
        )
        self.assertTrue(evidence["role_keys_distinct"])
        self.assertEqual(
            evidence["authenticated_updates_verified"],
            47 + len(self.manifest["format_sweep"]) * 4 + 2,
        )
        self.assertEqual(evidence["multisigned_images_verified"], 5)

    def test_corrupted_authenticode_signature_is_rejected(self) -> None:
        self.assertEqual(
            verify_corruption_negative(
                self.arrays,
                self.manifest,
                self.manifest_data,
            ),
            "PASS",
        )

    def test_role_key_reuse_is_rejected(self) -> None:
        arrays = dict(self.arrays)
        manifest = copy.deepcopy(self.manifest)
        manifest["roles"]["mldsa_image_signer"]["public_key_sha256"] = manifest[
            "roles"
        ]["mldsa_pk_update_authority"]["public_key_sha256"]
        manifest_data = self.bind_manifest(arrays, manifest)

        with self.assertRaisesRegex(
            VerificationError,
            "reuses the ML-DSA key",
        ):
            verify_fixture(arrays, manifest, manifest_data)

    def test_malformed_esl_is_rejected(self) -> None:
        arrays = dict(self.arrays)
        manifest = copy.deepcopy(self.manifest)
        name = "gSecureBootKeyAuditDbTbsHashV2"
        malformed = bytearray(arrays[name])
        struct.pack_into("<I", malformed, 16, len(malformed) + 1)
        arrays[name] = bytes(malformed)
        manifest["artifacts"][name]["sha256"] = hashlib.sha256(malformed).hexdigest()
        manifest_data = self.bind_manifest(arrays, manifest)

        with self.assertRaisesRegex(
            VerificationError,
            "EFI_SIGNATURE_LIST size is wrong",
        ):
            verify_fixture(arrays, manifest, manifest_data)

    def test_non_mldsa_pqc_role_is_rejected(self) -> None:
        certificate = self.self_signed_certificate(
            ed25519.Ed25519PrivateKey.generate(),
            None,
        )

        with self.assertRaisesRegex(
            VerificationError,
            "key profile is unsupported",
        ):
            verify_self_signed_certificate(certificate)

    def test_wrong_rsa_key_size_is_rejected(self) -> None:
        certificate = self.self_signed_certificate(
            rsa.generate_private_key(public_exponent=65537, key_size=1024),
            hashes.SHA256(),
        )

        with self.assertRaisesRegex(
            VerificationError,
            "RSA key is not 2048 bits",
        ):
            verify_self_signed_certificate(certificate)


if __name__ == "__main__":
    unittest.main()
