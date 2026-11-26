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
from unittest import mock

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ed25519, rsa
from cryptography.x509.oid import NameOID
from Host import GenerateTestData, VerifyTestData
from Host.VerifyTestData import (
    VerificationError,
    parse_c_arrays,
    verify_corruption_negative,
    verify_fixture,
    verify_self_signed_certificate,
)


class GenerateTestDataDefaultsTests(unittest.TestCase):
    """Resolve generator defaults without writing or generating fixtures."""

    def test_manifest_digest_matches_emitted_crlf_bytes(self) -> None:
        output = mock.Mock(spec=Path)
        manifest = {"fixture_id": "test", "description": "caf\u00e9"}
        digest = GenerateTestData.write_manifest(output, manifest)
        output.write_bytes.assert_called_once()
        emitted = output.write_bytes.call_args.args[0]
        self.assertEqual(json.loads(emitted), manifest)
        self.assertIn(b"\r\n", emitted)
        self.assertNotIn(b"\n", emitted.replace(b"\r\n", b""))
        self.assertEqual(digest, hashlib.sha256(emitted).digest())

    def test_unsigned_image_defaults_to_package_build(self) -> None:
        repo_root = Path(__file__).resolve().parents[4]
        with mock.patch("sys.argv", ["GenerateTestData.py"]), mock.patch.object(
            GenerateTestData, "generate"
        ) as generate:
            args = GenerateTestData.parse_args()
        self.assertEqual(
            args.unsigned_image,
            repo_root
            / "Build"
            / "UefiTestingPkg"
            / "DEBUG_GCC"
            / "X64"
            / "HelloUefiApp.efi",
        )
        generate.assert_not_called()

    def test_aarch64_defaults_select_separate_fixture(self) -> None:
        host_dir = Path(__file__).resolve().parent
        repo_root = Path(__file__).resolve().parents[4]
        with mock.patch("sys.argv", ["GenerateTestData.py", "--arch", "AARCH64"]):
            args = GenerateTestData.parse_args()
        self.assertEqual(args.output_c, host_dir.parent / "UEFI" / "TestDataAArch64.c")
        self.assertEqual(args.output_manifest, host_dir / "TestDataAArch64.manifest.json")
        self.assertEqual(
            args.unsigned_image,
            repo_root / "Build" / "UefiTestingPkg" / "DEBUG_GCC" / "AARCH64" / "HelloUefiApp.efi",
        )

    def test_generated_authenticode_envelopes_verify_independently(self) -> None:
        unsigned = bytearray(512)
        unsigned[:2] = b"MZ"
        struct.pack_into("<I", unsigned, 0x3C, 64)
        unsigned[64:68] = b"PE\0\0"
        struct.pack_into("<H", unsigned, 68, 0x8664)
        struct.pack_into("<H", unsigned, 84, 240)
        struct.pack_into("<H", unsigned, 88, 0x20B)
        for make_certificate, digest, digest_oid, signature_oid in (
            (
                GenerateTestData.generate_rsa_certificate, hashlib.sha256,
                VerifyTestData.SHA256_OID, VerifyTestData.SHA256_WITH_RSA_OID,
            ),
            (
                GenerateTestData.generate_ecdsa_certificate, hashlib.sha256,
                VerifyTestData.SHA256_OID, VerifyTestData.ECDSA_WITH_SHA256_OID,
            ),
            (
                GenerateTestData.generate_mldsa_certificate, hashlib.sha512,
                VerifyTestData.SHA512_OID, VerifyTestData.ML_DSA_87_OID,
            ),
        ):
            with self.subTest(algorithm=signature_oid):
                key, certificate = make_certificate("Envelope test")
                signature, _attributes = GenerateTestData.authenticode_signature(
                    key, certificate,
                    GenerateTestData.authenticode_digest(unsigned, digest),
                )
                image = GenerateTestData.apply_authenticode_signature(unsigned, signature)
                entry = VerifyTestData.win_certificate_contents(image)[0]
                content_info, _ = VerifyTestData.der_decode(
                    entry, asn1Spec=VerifyTestData.rfc2315.ContentInfo()
                )
                signed_data, _ = VerifyTestData.der_decode(
                    content_info["content"], asn1Spec=VerifyTestData.rfc2315.SignedData()
                )
                parameters = signed_data["signerInfos"][0]["digestEncryptionAlgorithm"]["parameters"]
                if signature_oid == VerifyTestData.SHA256_WITH_RSA_OID:
                    self.assertEqual(bytes(parameters), b"\x05\x00")
                else:
                    self.assertFalse(parameters.hasValue())
                verified = VerifyTestData.verify_authenticode_entry(
                    image, entry, digest_oid, signature_oid
                )
                self.assertEqual(
                    verified.public_bytes(VerifyTestData.serialization.Encoding.DER),
                    certificate.public_bytes(VerifyTestData.serialization.Encoding.DER),
                )
                damaged = bytearray(image)
                damaged[16] ^= 1
                with self.assertRaises(VerificationError):
                    VerifyTestData.verify_authenticode_entry(
                        damaged, entry, digest_oid, signature_oid
                    )


class VerifyFixtureArchitectureTests(unittest.TestCase):
    """Verify every checked-in per-architecture fixture set."""

    @staticmethod
    def load(arch):
        host_dir = Path(__file__).resolve().parent
        fixture = VerifyTestData.FIXTURE_ARCHITECTURES[arch]
        manifest_data = (host_dir / fixture["manifest"]).read_bytes()
        arrays = parse_c_arrays(host_dir.parent / "UEFI" / fixture["test_data"])
        return arrays, json.loads(manifest_data), manifest_data

    def test_each_fixture_verifies_for_its_architecture(self) -> None:
        for arch in VerifyTestData.FIXTURE_ARCHITECTURES:
            with self.subTest(arch=arch):
                arrays, manifest, manifest_data = self.load(arch)
                verify_fixture(arrays, manifest, manifest_data)
                self.assertGreater(VerifyTestData.verify_image_machines(arrays, arch), 0)

    def test_signed_images_reject_the_other_architecture(self) -> None:
        for arch, other in (("X64", "AARCH64"), ("AARCH64", "X64")):
            with self.subTest(arch=arch):
                arrays, _manifest, _data = self.load(arch)
                with self.assertRaisesRegex(VerificationError, "is machine"):
                    VerifyTestData.verify_image_machines(arrays, other)


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

    def test_pairwise_images_verify_each_entry_against_its_image(self) -> None:
        with mock.patch.object(
            VerifyTestData,
            "verify_authenticode_entry",
            wraps=VerifyTestData.verify_authenticode_entry,
        ) as verify_entry:
            VerifyTestData.verify_ca_migration_images(self.arrays)
        for name, algorithms in (
            (
                "gSecureBootKeyAuditHelloRsaEcdsa",
                (
                    (VerifyTestData.SHA256_OID, VerifyTestData.SHA256_WITH_RSA_OID),
                    (VerifyTestData.SHA256_OID, VerifyTestData.ECDSA_WITH_SHA256_OID),
                ),
            ),
            (
                "gSecureBootKeyAuditHelloEcdsaMldsa",
                (
                    (VerifyTestData.SHA256_OID, VerifyTestData.ECDSA_WITH_SHA256_OID),
                    (VerifyTestData.SHA512_OID, VerifyTestData.ML_DSA_87_OID),
                ),
            ),
        ):
            image = self.arrays[name]
            entries = VerifyTestData.win_certificate_contents(image)
            for entry, (digest_oid, signature_oid) in zip(entries, algorithms):
                with self.subTest(image=name, signature=signature_oid):
                    verify_entry.assert_any_call(
                        image, entry, digest_oid, signature_oid
                    )

                    def reject_entry(candidate_image, candidate_entry, *_args):
                        if candidate_image == image and candidate_entry == entry:
                            raise VerificationError("pairwise entry rejected")
                        return mock.sentinel.certificate

                    with mock.patch.object(
                        VerifyTestData,
                        "verify_authenticode_entry",
                        side_effect=reject_entry,
                    ), self.assertRaisesRegex(
                        VerificationError, "pairwise entry rejected"
                    ):
                        VerifyTestData.verify_ca_migration_images(self.arrays)

    def test_ecdsa_signer_identity_must_match_certificate(self) -> None:
        _time, signed_data, _payload = VerifyTestData.parse_authentication_2(
            self.arrays["gSecureBootKeyAuditPkEcdsaToMldsa"]
        )
        certificate = VerifyTestData.signed_data_certificate(signed_data)
        for field, serial, issuer, message in (
            (
                "serial",
                certificate.serial_number + 1,
                certificate.issuer.public_bytes(),
                "ECDSA SignedData signer serial number is wrong",
            ),
            (
                "issuer",
                certificate.serial_number,
                b"wrong issuer",
                "ECDSA SignedData signer issuer is wrong",
            ),
        ):
            signer = {
                "digestAlgorithm": {"algorithm": VerifyTestData.SHA256_OID},
                "digestEncryptionAlgorithm": {
                    "algorithm": VerifyTestData.ECDSA_WITH_SHA256_OID,
                },
                "authenticatedAttributes": mock.Mock(hasValue=lambda: False),
                "issuerAndSerialNumber": {
                    "serialNumber": serial,
                    "issuer": mock.sentinel.issuer,
                },
            }
            decoded = {
                "signerInfos": [signer],
                "digestAlgorithms": [{"algorithm": VerifyTestData.SHA256_OID}],
                "contentInfo": {
                    "contentType": VerifyTestData.CMS_DATA_OID,
                    "content": mock.Mock(hasValue=lambda: False),
                },
            }
            with self.subTest(field=field), mock.patch.object(
                VerifyTestData, "decode_exact", return_value=decoded
            ), mock.patch.object(
                VerifyTestData, "signed_data_certificate", return_value=certificate
            ), mock.patch.object(
                VerifyTestData, "der_encode", return_value=issuer
            ), self.assertRaisesRegex(VerificationError, message):
                VerifyTestData.verify_ecdsa_signed_data(
                    signed_data, b"unused", certificate
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
