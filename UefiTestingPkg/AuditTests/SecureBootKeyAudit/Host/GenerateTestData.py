# @file
# Generate public test artifacts for SecureBootKeyAuditApp.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
"""Generate RSA and ML-DSA PK rotation chains for the Secure Boot ECIT audit.

The generated application data proves that firmware can:

1. enroll test PK A while in Setup Mode;
2. replace PK A with PK B using an update signed by PK A;
3. replace PK B with PK A using an update signed by PK B;
4. directly enroll and use an ML-DSA-87 PK; and
5. roll RSA PK A to ML-DSA-87 and use ML-DSA-87 to roll back to RSA.

Private keys exist only in memory while this script runs. The generated C file
contains certificates, signature lists, and authenticated updates, but no
private key material.
"""

import argparse
import datetime
import hashlib
import io
import json
import struct
import uuid
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec, mldsa, padding, rsa
from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat
from cryptography.x509.oid import NameOID
from edk2toollib.uefi.authenticated_variables_structure_support import (
    EfiSignatureDataFactory,
    EfiSignatureList,
    EfiVariableAuthentication2Builder,
)
from pyasn1.codec.der.decoder import decode as der_decode
from pyasn1.codec.der.encoder import encode as der_encode
from pyasn1.type import univ
from pyasn1_modules import rfc2315, rfc2459, rfc5280, rfc5652

EFI_GLOBAL_VARIABLE_GUID = "8be4df61-93ca-11d2-aa0d-00e098032b8c"
EFI_IMAGE_SECURITY_DATABASE_GUID = "d719b2cb-3d3a-4596-a3bc-dad00e67656f"
EFI_CERT_PKCS7_GUID = uuid.UUID("4aafd29d-68df-49ee-8aa9-347d375665a7")
EFI_CERT_SHA1_GUID = uuid.UUID("826ca512-cf10-4ac9-b187-be01496631bd")
EFI_CERT_SHA256_GUID = uuid.UUID("c1c41626-504c-4092-aca9-41f936934328")
EFI_CERT_SHA384_GUID = uuid.UUID("ff3e5307-9fd0-48c9-85f1-8ad56c701e01")
EFI_CERT_SHA512_GUID = uuid.UUID("093e0fae-a6c4-4f50-9f1b-d41e2b89c19a")
EFI_CERT_X509_GUID = uuid.UUID("a5c059a1-94e4-4aa7-87b5-ab155c2bf072")
EFI_CERT_X509_SHA256_GUID = uuid.UUID("3bd2a492-96c0-4079-b420-fcf98ef103ed")
EFI_CERT_X509_SHA384_GUID = uuid.UUID("7076876e-80c2-4ee6-aad2-28b349a6865b")
EFI_CERT_X509_SHA512_GUID = uuid.UUID("446dbf63-2502-4cda-bcfa-2465d2b0fe9d")
EFI_CERT_V2_SHA256_GUID = uuid.UUID("65325437-ca92-441e-943d-4e003faeda97")
EFI_CERT_V2_SHA384_GUID = uuid.UUID("45db3553-f9a6-4026-8e48-eb1e4b07185c")
EFI_CERT_V2_SHA512_GUID = uuid.UUID("d2291d97-f066-4abf-9c57-6363e5fd6903")
EFI_CERT_V2_X509_GUID = uuid.UUID("79518039-4715-4393-bdf8-d8e3c67fbac7")
EFI_CERT_V2_X509_SHA256_GUID = uuid.UUID("af4d21cd-7627-4b81-9e96-d4f3bbc3fae9")
EFI_CERT_V2_X509_SHA384_GUID = uuid.UUID("78860e15-f2eb-4d16-8156-231518fb7688")
EFI_CERT_V2_X509_SHA512_GUID = uuid.UUID("4d46147a-fa79-43a9-8836-00341c708a70")
SIGNATURE_OWNER = uuid.UUID("d4f18610-2ce9-4d6c-a3c2-0bd4727db67b")
VARIABLE_ATTRIBUTES = "NV,BS,RT,AT"
FIXTURE_ID = "secure-boot-key-audit-v6"

SHA256_WITH_RSA_OID = "1.2.840.113549.1.1.11"
ECDSA_WITH_SHA256_OID = "1.2.840.10045.4.3.2"
SHA256_OID = "2.16.840.1.101.3.4.2.1"
SHA512_OID = "2.16.840.1.101.3.4.2.3"
ML_DSA_87_OID = "2.16.840.1.101.3.4.3.19"
CMS_DATA_OID = "1.2.840.113549.1.7.1"
CMS_SIGNED_DATA_OID = "1.2.840.113549.1.7.2"
CMS_CONTENT_TYPE_ATTRIBUTE_OID = "1.2.840.113549.1.9.3"
CMS_MESSAGE_DIGEST_ATTRIBUTE_OID = "1.2.840.113549.1.9.4"
SPC_INDIRECT_DATA_OID = "1.3.6.1.4.1.311.2.1.4"

BASE_TIME = datetime.datetime(2026, 1, 1, tzinfo=datetime.timezone.utc)

FORMAT_SWEEP_DEFINITIONS = (
    ("X509V1", "X.509 certificate", EFI_CERT_X509_GUID, "certificate", None, False),
    (
        "X509V2",
        "X.509 certificate (V2)",
        EFI_CERT_V2_X509_GUID,
        "certificate",
        None,
        True,
    ),
    ("Sha1V1", "SHA-1 image hash", EFI_CERT_SHA1_GUID, "image", "sha1", False),
    (
        "Sha256V1",
        "SHA-256 image hash",
        EFI_CERT_SHA256_GUID,
        "image",
        "sha256",
        False,
    ),
    (
        "Sha256V2",
        "SHA-256 image hash (V2)",
        EFI_CERT_V2_SHA256_GUID,
        "image",
        "sha256",
        True,
    ),
    (
        "Sha384V1",
        "SHA-384 image hash",
        EFI_CERT_SHA384_GUID,
        "image",
        "sha384",
        False,
    ),
    (
        "Sha384V2",
        "SHA-384 image hash (V2)",
        EFI_CERT_V2_SHA384_GUID,
        "image",
        "sha384",
        True,
    ),
    (
        "Sha512V1",
        "SHA-512 image hash",
        EFI_CERT_SHA512_GUID,
        "image",
        "sha512",
        False,
    ),
    (
        "Sha512V2",
        "SHA-512 image hash (V2)",
        EFI_CERT_V2_SHA512_GUID,
        "image",
        "sha512",
        True,
    ),
    (
        "TbsSha256V1",
        "X.509 TBS hash (SHA-256)",
        EFI_CERT_X509_SHA256_GUID,
        "tbs",
        "sha256",
        False,
    ),
    (
        "TbsSha256V2",
        "X.509 TBS hash (SHA-256, V2)",
        EFI_CERT_V2_X509_SHA256_GUID,
        "tbs",
        "sha256",
        True,
    ),
    (
        "TbsSha384V1",
        "X.509 TBS hash (SHA-384)",
        EFI_CERT_X509_SHA384_GUID,
        "tbs",
        "sha384",
        False,
    ),
    (
        "TbsSha384V2",
        "X.509 TBS hash (SHA-384, V2)",
        EFI_CERT_V2_X509_SHA384_GUID,
        "tbs",
        "sha384",
        True,
    ),
    (
        "TbsSha512V1",
        "X.509 TBS hash (SHA-512)",
        EFI_CERT_X509_SHA512_GUID,
        "tbs",
        "sha512",
        False,
    ),
    (
        "TbsSha512V2",
        "X.509 TBS hash (SHA-512, V2)",
        EFI_CERT_V2_X509_SHA512_GUID,
        "tbs",
        "sha512",
        True,
    ),
)

SPC_PE_IMAGE_DATA = bytes.fromhex(
    "3033060a2b06010401823702010f"
    "3025030100a020a21e801c"
    "003c003c003c004f00620073006f006c006500740065003e003e003e"
)

C_HEADER = """\
/** @file
  Generated public test data for SecureBootKeyAuditApp.

  Regenerate with Host/GenerateTestData.py. Private keys are not emitted.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "TestData.h"
"""

ARTIFACT_DESCRIPTIONS = {
    "gSecureBootKeyAuditPkA": (
        "PK A variable payload.",
        "Structure: EFI_SIGNATURE_LIST containing one X.509 certificate.",
        "Key: RSA-2048 test authority A.",
        "Certificate signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditPkB": (
        "PK B variable payload.",
        "Structure: EFI_SIGNATURE_LIST containing one X.509 certificate.",
        "Key: RSA-2048 test authority B.",
        "Certificate signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditPkMldsa": (
        "ML-DSA PK variable payload.",
        "Structure: EFI_SIGNATURE_LIST containing one X.509 certificate.",
        "Key: ML-DSA-87 test authority.",
        "Certificate signature: pure ML-DSA-87.",
    ),
    "gSecureBootKeyAuditSetupPkA": (
        "Setup Mode enrollment of PK A.",
        "Structure: unsigned EFI_VARIABLE_AUTHENTICATION_2 followed by PK A.",
        "Authorization: permitted only while firmware is in Setup Mode.",
        "Payload key: RSA-2048 X.509 test authority A.",
        "Certificate signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditSetupPkMldsa": (
        "Setup Mode enrollment of the ML-DSA PK.",
        "Structure: unsigned EFI_VARIABLE_AUTHENTICATION_2 followed by the ML-DSA PK.",
        "Authorization: permitted only while firmware is in Setup Mode.",
        "Payload key: ML-DSA-87 X.509 test authority.",
        "Certificate signature: pure ML-DSA-87.",
    ),
    "gSecureBootKeyAuditSetupPkAForMldsa": (
        "Setup Mode enrollment of PK A for the RSA-to-ML-DSA roll.",
        "Structure: unsigned EFI_VARIABLE_AUTHENTICATION_2 followed by PK A.",
        "Authorization: permitted only while firmware is in Setup Mode.",
        "Payload key: RSA-2048 X.509 test authority A.",
        "Certificate signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditPkAToB": (
        "Authenticated rotation from PK A to PK B.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by PK B.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority A.",
        "Update signature: SHA-256 with RSA.",
        "Payload key: RSA-2048 X.509 test authority B.",
    ),
    "gSecureBootKeyAuditPkBToA": (
        "Authenticated rotation from PK B to PK A.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by PK A.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority B.",
        "Update signature: SHA-256 with RSA.",
        "Payload key: RSA-2048 X.509 test authority A.",
    ),
    "gSecureBootKeyAuditPkAToMldsa": (
        "Authenticated rotation from RSA PK A to the ML-DSA PK.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by the ML-DSA PK.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority A.",
        "Update signature: SHA-256 with RSA.",
        "Payload key: ML-DSA-87 X.509 test authority.",
    ),
    "gSecureBootKeyAuditPkMldsaToA": (
        "Authenticated rotation from the ML-DSA PK back to RSA PK A.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by PK A.",
        "Authorization: RFC 9882 CMS signed by the ML-DSA-87 test authority.",
        "CMS digest algorithm: SHA-512, as required without signed attributes.",
        "Update signature: pure ML-DSA-87.",
        "Payload key: RSA-2048 X.509 test authority A.",
    ),
    "gSecureBootKeyAuditDeleteByA": (
        "Authenticated deletion of PK A.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority A.",
        "Update signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditDeleteByMldsa": (
        "Authenticated deletion of the directly enrolled ML-DSA PK.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: RFC 9882 CMS signed by the ML-DSA-87 test authority.",
        "CMS digest algorithm: SHA-512, as required without signed attributes.",
        "Update signature: pure ML-DSA-87.",
    ),
    "gSecureBootKeyAuditDeleteByAAfterMldsa": (
        "Authenticated deletion of RSA PK A after the ML-DSA roll-back.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority A.",
        "Update signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditRecoveryDeleteByA": (
        "Recovery-only authenticated deletion of RSA PK A.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority A.",
        "Update signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditRecoveryDeleteByB": (
        "Recovery-only authenticated deletion of RSA PK B.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: PKCS#7 signed by RSA-2048 test authority B.",
        "Update signature: SHA-256 with RSA.",
    ),
    "gSecureBootKeyAuditRecoveryDeleteByMldsa": (
        "Recovery-only authenticated deletion of the ML-DSA PK.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: RFC 9882 CMS signed by the ML-DSA-87 test authority.",
        "CMS digest algorithm: SHA-512, as required without signed attributes.",
        "Update signature: pure ML-DSA-87.",
    ),
    "gSecureBootKeyAuditDbTbsHashV1": (
        "V1 EFI signature list containing the image-signer TBS hash.",
        "Signature type: EFI_CERT_X509_SHA512_GUID.",
        "Signature data: owner GUID, SHA-512 TBSCertificate digest, EFI_TIME.",
        "The zero EFI_TIME denotes a non-revocation image authorization entry.",
    ),
    "gSecureBootKeyAuditDbTbsHashV2": (
        "UEFI 2.12 V2 EFI signature list containing the image-signer TBS hash.",
        "Signature type: EFI_CERT_V2_X509_SHA512_GUID.",
        "Signature data: SHA-512 TBSCertificate digest only.",
        "V2 deliberately omits SignatureOwner and EFI_TIME.",
    ),
    "gSecureBootKeyAuditEnrollDbTbsHashV1": (
        "Authenticated enrollment of the V1 ML-DSA TBS hash in db.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by the V1 ESL.",
        "Authorization: RFC 9882 CMS signed by the active ML-DSA-87 PK.",
        "Update signature: pure ML-DSA-87 with SHA-512.",
    ),
    "gSecureBootKeyAuditEnrollDbTbsHashV2": (
        "Authenticated enrollment of the UEFI 2.12 V2 TBS hash in db.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 followed by the V2 ESL.",
        "Authorization: RFC 9882 CMS signed by the active ML-DSA-87 PK.",
        "Update signature: pure ML-DSA-87 with SHA-512.",
    ),
    "gSecureBootKeyAuditDeleteDbV1ByMldsa": (
        "Authenticated deletion of the V1 test db.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: RFC 9882 CMS signed by the active ML-DSA-87 PK.",
        "Update signature: pure ML-DSA-87 with SHA-512.",
    ),
    "gSecureBootKeyAuditDeleteDbV2ByMldsa": (
        "Authenticated deletion of the V2 test db.",
        "Structure: EFI_VARIABLE_AUTHENTICATION_2 with an empty payload.",
        "Authorization: RFC 9882 CMS signed by the active ML-DSA-87 PK.",
        "Update signature: pure ML-DSA-87 with SHA-512.",
    ),
    "gSecureBootKeyAuditHelloMldsa": (
        "HelloUefiApp PE/COFF image signed with ML-DSA-87 Authenticode.",
        "Image digest: SHA-512.",
        "CMS signature: ML-DSA-87 over contentType and messageDigest attrs.",
        "Signer authorization: SHA-512 TBSCertificate hash stored in db.",
        "The image contains the public signer certificate but no private key.",
    ),
    "gSecureBootKeyAuditDbMultiSigner": (
        "V2 EFI signature list containing both image-signer TBS hashes.",
        "Signers: RSA-2048/SHA-256 and ML-DSA-87/SHA-512.",
    ),
    "gSecureBootKeyAuditDbxRsaSigner": (
        "V2 EFI signature list revoking the RSA image signer TBS hash.",
    ),
    "gSecureBootKeyAuditDbxMldsaSigner": (
        "V2 EFI signature list revoking the ML-DSA image signer TBS hash.",
    ),
    "gSecureBootKeyAuditDbxBothSigners": (
        "V2 EFI signature list revoking both image-signer TBS hashes.",
    ),
    "gSecureBootKeyAuditEnrollDbMultiSigner": (
        "Authenticated enrollment of both image-signer TBS hashes in db.",
    ),
    "gSecureBootKeyAuditEnrollDbxRsaSigner": (
        "Authenticated replacement of dbx with the RSA signer TBS hash.",
    ),
    "gSecureBootKeyAuditEnrollDbxMldsaSigner": (
        "Authenticated replacement of dbx with the ML-DSA signer TBS hash.",
    ),
    "gSecureBootKeyAuditEnrollDbxBothSigners": (
        "Authenticated replacement of dbx with both signer TBS hashes.",
    ),
    "gSecureBootKeyAuditDeleteDbxByMldsa": (
        "Authenticated deletion of the multi-signature test dbx.",
    ),
    "gSecureBootKeyAuditDeleteDbMultiSignerByMldsa": (
        "Authenticated deletion of the multi-signature test db.",
    ),
    "gSecureBootKeyAuditHelloRsaMldsa": (
        "HelloUefiApp with independent RSA then ML-DSA WIN_CERTIFICATE entries.",
    ),
    "gSecureBootKeyAuditHelloMldsaRsa": (
        "HelloUefiApp with independent ML-DSA then RSA WIN_CERTIFICATE entries.",
    ),
    "gSecureBootKeyAuditDbCaMigration": (
        "V2 EFI signature list authorizing RSA, ECDSA, and ML-DSA CA TBS hashes.",
    ),
    "gSecureBootKeyAuditDbxMigrationRsa": (
        "V2 EFI signature list cumulatively revoking the RSA CA.",
    ),
    "gSecureBootKeyAuditDbxMigrationRsaEcdsa": (
        "V2 EFI signature list cumulatively revoking RSA and ECDSA CAs.",
    ),
    "gSecureBootKeyAuditDbxMigrationAll": (
        "V2 EFI signature list cumulatively revoking all three CAs.",
    ),
    "gSecureBootKeyAuditEnrollDbCaMigration": (
        "RSA-authenticated enrollment of all three CA TBS hashes in db.",
    ),
    "gSecureBootKeyAuditEnrollDbxMigrationRsa": (
        "RSA-authenticated replacement of dbx with the retired RSA CA.",
    ),
    "gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsa": (
        "RSA-authenticated replacement of dbx with retired RSA and ECDSA CAs.",
    ),
    "gSecureBootKeyAuditEnrollDbxMigrationAll": (
        "RSA-authenticated replacement of dbx with all three retired CAs.",
    ),
    "gSecureBootKeyAuditDeleteDbxCaMigrationByA": (
        "RSA-authenticated deletion of the cumulative CA migration dbx.",
    ),
    "gSecureBootKeyAuditDeleteDbCaMigrationByA": (
        "RSA-authenticated deletion of the cumulative CA migration db.",
    ),
    "gSecureBootKeyAuditHelloRsa": (
        "HelloUefiApp with one RSA/SHA-256 WIN_CERTIFICATE entry.",
    ),
    "gSecureBootKeyAuditHelloEcdsa": (
        "HelloUefiApp with one ECDSA/SHA-256 WIN_CERTIFICATE entry.",
    ),
    "gSecureBootKeyAuditHelloRsaEcdsaMldsa": (
        "HelloUefiApp with RSA, ECDSA, then ML-DSA WIN_CERTIFICATE entries.",
    ),
}


def build_certificate(common_name: str, key, signature_hash):
    """Build a self-signed test authority certificate."""
    name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, common_name)])
    return (
        x509.CertificateBuilder()
        .subject_name(name)
        .issuer_name(name)
        .public_key(key.public_key())
        .serial_number(x509.random_serial_number())
        .not_valid_before(BASE_TIME)
        .not_valid_after(BASE_TIME + datetime.timedelta(days=3650))
        .add_extension(x509.BasicConstraints(ca=True, path_length=None), critical=True)
        .add_extension(
            x509.KeyUsage(
                digital_signature=True,
                content_commitment=False,
                key_encipherment=False,
                data_encipherment=False,
                key_agreement=False,
                key_cert_sign=True,
                crl_sign=True,
                encipher_only=None,
                decipher_only=None,
            ),
            critical=True,
        )
        .sign(key, signature_hash)
    )


def generate_rsa_certificate(common_name: str):
    """Generate a self-signed RSA-2048 test authority."""
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    return key, build_certificate(common_name, key, hashes.SHA256())


def generate_mldsa_certificate(common_name: str):
    """Generate a self-signed ML-DSA-87 test authority."""
    key = mldsa.MLDSA87PrivateKey.generate()
    return key, build_certificate(common_name, key, None)


def generate_ecdsa_certificate(common_name: str):
    """Generate a self-signed ECDSA P-256 test authority."""
    key = ec.generate_private_key(ec.SECP256R1())
    return key, build_certificate(common_name, key, hashes.SHA256())


def encode_efi_time(timestamp: datetime.datetime) -> bytes:
    """Encode a timestamp as EFI_TIME."""
    return struct.pack(
        "<HBBBBBBIhBB",
        timestamp.year,
        timestamp.month,
        timestamp.day,
        timestamp.hour,
        timestamp.minute,
        timestamp.second,
        0,
        0,
        0,
        0,
        0,
    )


def certificate_to_esl(certificate: x509.Certificate) -> bytes:
    """Encode one X.509 certificate as an EFI_SIGNATURE_LIST."""
    certificate_der = certificate.public_bytes(Encoding.DER)
    signature_list = EfiSignatureList(
        typeguid=EfiSignatureDataFactory.EFI_CERT_X509_GUID
    )
    with io.BytesIO(certificate_der) as certificate_stream:
        signature_data = EfiSignatureDataFactory.create(
            EfiSignatureDataFactory.EFI_CERT_X509_GUID,
            certificate_stream,
            SIGNATURE_OWNER,
        )
    signature_list.AddSignatureHeader(None, SigSize=signature_data.get_total_size())
    signature_list.AddSignatureData(signature_data)
    return signature_list.encode()


def raw_signature_list(
    signature_type: uuid.UUID,
    payload: bytes,
    ownerless: bool,
) -> bytes:
    """Encode one V1 or ownerless V2 EFI signature-list entry."""
    signature_data = payload if ownerless else SIGNATURE_OWNER.bytes_le + payload
    signature_list_size = 16 + 12 + len(signature_data)
    return (
        signature_type.bytes_le
        + struct.pack("<III", signature_list_size, 0, len(signature_data))
        + signature_data
    )


def format_sweep_esl(
    definition: tuple[str, str, uuid.UUID, str, str | None, bool],
    certificate: x509.Certificate,
    image_digests: dict[str, bytes],
) -> bytes:
    """Build one advertised image-authorization signature-list format."""
    _, _, signature_type, payload_kind, hash_name, ownerless = definition
    if payload_kind == "certificate":
        payload = certificate.public_bytes(Encoding.DER)
    elif payload_kind == "image":
        if hash_name is None:
            raise ValueError("Image-hash format is missing its hash algorithm")
        payload = image_digests[hash_name]
    elif payload_kind == "tbs":
        if hash_name is None:
            raise ValueError("TBS-hash format is missing its hash algorithm")
        payload = hashlib.new(hash_name, certificate.tbs_certificate_bytes).digest()
        if not ownerless:
            payload += bytes(16)
    else:
        raise ValueError(f"Unsupported format-sweep payload kind: {payload_kind}")

    return raw_signature_list(signature_type, payload, ownerless)


def certificate_tbs_hashes_to_esl(
    certificates: list[x509.Certificate],
    version: int,
) -> bytes:
    """Encode SHA-512 TBSCertificate hashes in one V1 or V2 signature list."""
    signature_entries = []
    for certificate in certificates:
        digest = hashlib.sha512(certificate.tbs_certificate_bytes).digest()
        if version == 1:
            signature_type = EFI_CERT_X509_SHA512_GUID
            signature_entries.append(SIGNATURE_OWNER.bytes_le + digest + bytes(16))
        elif version == 2:
            signature_type = EFI_CERT_V2_X509_SHA512_GUID
            signature_entries.append(digest)
        else:
            raise ValueError(f"Unsupported TBS hash signature-list version: {version}")

    if not signature_entries:
        raise ValueError("At least one certificate is required")

    signature_size = len(signature_entries[0])
    signature_list_size = 16 + 12 + signature_size * len(signature_entries)
    return (
        signature_type.bytes_le
        + struct.pack("<III", signature_list_size, 0, signature_size)
        + b"".join(signature_entries)
    )


def certificate_tbs_hash_to_esl(
    certificate: x509.Certificate,
    version: int,
) -> bytes:
    """Encode one SHA-512 TBSCertificate hash as a signature list."""
    return certificate_tbs_hashes_to_esl([certificate], version)


def empty_authenticated_update(payload: bytes, timestamp: datetime.datetime) -> bytes:
    """Build the unsigned authentication descriptor allowed in Setup Mode."""
    efi_time = encode_efi_time(timestamp)
    win_certificate = (
        struct.pack("<IHH", 24, 0x0200, 0x0EF1) + EFI_CERT_PKCS7_GUID.bytes_le
    )
    return efi_time + win_certificate + payload


def signed_authenticated_update(
    payload: bytes,
    timestamp: datetime.datetime,
    signing_certificate: x509.Certificate,
    signing_key,
    variable_name: str = "PK",
    variable_guid: str = EFI_GLOBAL_VARIABLE_GUID,
) -> bytes:
    """Build an EFI_VARIABLE_AUTHENTICATION_2 update signed with RSA/SHA-256."""
    builder = EfiVariableAuthentication2Builder(
        name=variable_name,
        guid=variable_guid,
        attributes=VARIABLE_ATTRIBUTES,
        payload=payload,
        efi_time=timestamp,
    )
    builder.sign(
        signing_certificate,
        signing_key,
        hash_algorithm=hashes.SHA256(),
    )
    return builder.finalize().encode()


def mldsa_cms_signed_data(
    content: bytes,
    signing_certificate: x509.Certificate,
    signing_key: mldsa.MLDSA87PrivateKey,
) -> bytes:
    """Build detached RFC 9882 SignedData without signed attributes."""
    certificate_der = signing_certificate.public_bytes(Encoding.DER)
    certificate_asn1, remainder = der_decode(
        certificate_der, asn1Spec=rfc5280.Certificate()
    )
    if remainder:
        raise ValueError("ML-DSA certificate has trailing DER data")

    signature = signing_key.sign(content)
    signing_key.public_key().verify(signature, content)

    signed_data = rfc5652.SignedData()
    signed_data["version"] = "v1"

    digest_algorithm = rfc5652.DigestAlgorithmIdentifier()
    digest_algorithm["algorithm"] = univ.ObjectIdentifier(SHA512_OID)
    signed_data["digestAlgorithms"][0] = digest_algorithm
    signed_data["encapContentInfo"]["eContentType"] = univ.ObjectIdentifier(
        CMS_DATA_OID
    )

    certificate_choice = rfc5652.CertificateChoices()
    certificate_choice["certificate"] = certificate_asn1
    signed_data["certificates"][0] = certificate_choice

    signer_info = rfc5652.SignerInfo()
    signer_info["version"] = "v1"
    signer_info["sid"]["issuerAndSerialNumber"]["issuer"] = certificate_asn1[
        "tbsCertificate"
    ]["issuer"]
    signer_info["sid"]["issuerAndSerialNumber"]["serialNumber"] = certificate_asn1[
        "tbsCertificate"
    ]["serialNumber"]
    signer_info["digestAlgorithm"]["algorithm"] = univ.ObjectIdentifier(SHA512_OID)
    signer_info["signatureAlgorithm"]["algorithm"] = univ.ObjectIdentifier(
        ML_DSA_87_OID
    )
    signer_info["signature"] = signature
    signed_data["signerInfos"][0] = signer_info
    return der_encode(signed_data)


def mldsa_authenticated_update(
    payload: bytes,
    timestamp: datetime.datetime,
    signing_certificate: x509.Certificate,
    signing_key: mldsa.MLDSA87PrivateKey,
    variable_name: str = "PK",
    variable_guid: str = EFI_GLOBAL_VARIABLE_GUID,
) -> bytes:
    """Build an EFI_VARIABLE_AUTHENTICATION_2 update signed with ML-DSA-87."""
    builder = EfiVariableAuthentication2Builder(
        name=variable_name,
        guid=variable_guid,
        attributes=VARIABLE_ATTRIBUTES,
        payload=payload,
        efi_time=timestamp,
    )
    signed_data = mldsa_cms_signed_data(
        builder.get_digest(),
        signing_certificate,
        signing_key,
    )
    win_certificate = (
        struct.pack("<IHH", 24 + len(signed_data), 0x0200, 0x0EF1)
        + EFI_CERT_PKCS7_GUID.bytes_le
    )
    return encode_efi_time(timestamp) + win_certificate + signed_data + payload


def der_length(length: int) -> bytes:
    """Encode one DER length."""
    if length < 0x80:
        return bytes([length])

    encoded = length.to_bytes((length.bit_length() + 7) // 8, "big")
    return bytes([0x80 | len(encoded)]) + encoded


def der_sequence(contents: bytes) -> bytes:
    """Wrap bytes in a DER SEQUENCE."""
    return b"\x30" + der_length(len(contents)) + contents


def der_sequence_contents(sequence: bytes) -> bytes:
    """Return the content octets from one definite-length DER SEQUENCE."""
    if len(sequence) < 2 or sequence[0] != 0x30:
        raise ValueError("Value is not a DER SEQUENCE")

    length_byte = sequence[1]
    if length_byte < 0x80:
        header_size = 2
        content_size = length_byte
    else:
        length_size = length_byte & 0x7F
        if length_size == 0 or 2 + length_size > len(sequence):
            raise ValueError("DER SEQUENCE has an invalid length")

        header_size = 2 + length_size
        content_size = int.from_bytes(sequence[2:header_size], "big")

    if header_size + content_size != len(sequence):
        raise ValueError("DER SEQUENCE length does not match its buffer")

    return sequence[header_size:]


def pe_security_directory_offset(image: bytes) -> int:
    """Return the PE Certificate Table data-directory file offset."""
    if image[:2] != b"MZ":
        raise ValueError("Input image does not have a DOS header")

    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    if image[pe_offset : pe_offset + 4] != b"PE\0\0":
        raise ValueError("Input image does not have a PE header")

    optional_header_offset = pe_offset + 24
    optional_header_magic = struct.unpack_from("<H", image, optional_header_offset)[0]
    if optional_header_magic == 0x10B:
        data_directory_offset = optional_header_offset + 96
    elif optional_header_magic == 0x20B:
        data_directory_offset = optional_header_offset + 112
    else:
        raise ValueError(
            f"Unsupported PE optional-header magic: 0x{optional_header_magic:04x}"
        )

    return data_directory_offset + (4 * 8)


def prepare_unsigned_pe_image(image: bytes) -> bytes:
    """Reject pre-signed input and pad it for a WIN_CERTIFICATE."""
    security_directory_offset = pe_security_directory_offset(image)
    certificate_offset, certificate_size = struct.unpack_from(
        "<II", image, security_directory_offset
    )
    if certificate_offset != 0 or certificate_size != 0:
        raise ValueError("HelloUefiApp input image is already signed")

    return image + bytes((-len(image)) % 8)


def authenticode_digest(image: bytes, hash_constructor) -> bytes:
    """Hash a PE image according to the Authenticode PE image rules."""
    pe_offset = struct.unpack_from("<I", image, 0x3C)[0]
    optional_header_offset = pe_offset + 24
    checksum_offset = optional_header_offset + 64
    security_directory_offset = pe_security_directory_offset(image)
    certificate_offset, certificate_size = struct.unpack_from(
        "<II", image, security_directory_offset
    )
    if certificate_offset != 0 and certificate_offset + certificate_size != len(image):
        raise ValueError("PE attribute certificate table must end at end of file")

    digest = hash_constructor()
    digest.update(image[:checksum_offset])
    digest.update(image[checksum_offset + 4 : security_directory_offset])
    digest.update(
        image[
            security_directory_offset + 8 : certificate_offset
            if certificate_offset != 0
            else len(image)
        ]
    )
    return digest.digest()


def build_spc_indirect_data(image_digest: bytes, digest_oid: str) -> bytes:
    """Build a SpcIndirectDataContent value for a supported image digest."""
    digest_algorithms = {
        SHA256_OID: bytes.fromhex("300d06096086480165030402010500"),
        SHA512_OID: bytes.fromhex("300d06096086480165030402030500"),
    }
    if digest_oid not in digest_algorithms:
        raise ValueError(f"Unsupported Authenticode digest OID: {digest_oid}")

    digest_info = der_sequence(
        digest_algorithms[digest_oid]
        + b"\x04"
        + der_length(len(image_digest))
        + image_digest
    )
    return der_sequence(SPC_PE_IMAGE_DATA + digest_info)


def pkcs7_attribute(oid: str, value: bytes) -> rfc2315.Attribute:
    """Build one PKCS #7 authenticated attribute."""
    attribute = rfc2315.Attribute()
    attribute["type"] = univ.ObjectIdentifier(oid)
    attribute["values"][0] = univ.Any(value)
    return attribute


def mldsa_authenticode_signature(
    private_key: mldsa.MLDSA87PrivateKey,
    certificate: x509.Certificate,
    image_digest: bytes,
) -> tuple[bytes, bytes]:
    """Build an ML-DSA-87 Authenticode PKCS #7 signature."""
    spc_indirect_data = build_spc_indirect_data(image_digest, SHA512_OID)
    signed_attributes = rfc2315.Attributes()
    signed_attributes[0] = pkcs7_attribute(
        CMS_CONTENT_TYPE_ATTRIBUTE_OID,
        der_encode(univ.ObjectIdentifier(SPC_INDIRECT_DATA_OID)),
    )
    signed_attributes[1] = pkcs7_attribute(
        CMS_MESSAGE_DIGEST_ATTRIBUTE_OID,
        der_encode(
            univ.OctetString(
                hashlib.sha512(der_sequence_contents(spc_indirect_data)).digest()
            )
        ),
    )
    encoded_signed_attributes = der_encode(signed_attributes)
    signature = private_key.sign(encoded_signed_attributes)
    private_key.public_key().verify(signature, encoded_signed_attributes)

    certificate_der = certificate.public_bytes(Encoding.DER)
    certificate_asn1, trailing = der_decode(
        certificate_der,
        asn1Spec=rfc2459.Certificate(),
    )
    if trailing:
        raise ValueError("Unexpected trailing data in ML-DSA certificate")

    signed_data = rfc2315.SignedData()
    signed_data["version"] = 1
    signed_data["digestAlgorithms"][0]["algorithm"] = univ.ObjectIdentifier(SHA512_OID)
    signed_data["digestAlgorithms"][0]["parameters"] = univ.Null("")
    signed_data["contentInfo"]["contentType"] = univ.ObjectIdentifier(
        SPC_INDIRECT_DATA_OID
    )
    signed_data["contentInfo"]["content"] = univ.Any(spc_indirect_data)
    signed_data["certificates"][0]["certificate"] = certificate_asn1

    signer_info = rfc2315.SignerInfo()
    signer_info["version"] = 1
    signer_info["issuerAndSerialNumber"]["issuer"] = certificate_asn1["tbsCertificate"][
        "issuer"
    ]
    signer_info["issuerAndSerialNumber"]["serialNumber"] = certificate_asn1[
        "tbsCertificate"
    ]["serialNumber"]
    signer_info["digestAlgorithm"]["algorithm"] = univ.ObjectIdentifier(SHA512_OID)
    signer_info["digestAlgorithm"]["parameters"] = univ.Null("")
    for index, attribute in enumerate(signed_attributes):
        signer_info["authenticatedAttributes"][index] = attribute
    signer_info["digestEncryptionAlgorithm"]["algorithm"] = univ.ObjectIdentifier(
        ML_DSA_87_OID
    )
    signer_info["encryptedDigest"] = signature
    signed_data["signerInfos"][0] = signer_info

    content_info = rfc2315.ContentInfo()
    content_info["contentType"] = univ.ObjectIdentifier(CMS_SIGNED_DATA_OID)
    content_info["content"] = univ.Any(der_encode(signed_data))
    return der_encode(content_info), encoded_signed_attributes


def rsa_authenticode_signature(
    private_key: rsa.RSAPrivateKey,
    certificate: x509.Certificate,
    image_digest: bytes,
) -> tuple[bytes, bytes]:
    """Build an RSA-2048/SHA-256 Authenticode PKCS #7 signature."""
    spc_indirect_data = build_spc_indirect_data(image_digest, SHA256_OID)
    signed_attributes = rfc2315.Attributes()
    signed_attributes[0] = pkcs7_attribute(
        CMS_CONTENT_TYPE_ATTRIBUTE_OID,
        der_encode(univ.ObjectIdentifier(SPC_INDIRECT_DATA_OID)),
    )
    signed_attributes[1] = pkcs7_attribute(
        CMS_MESSAGE_DIGEST_ATTRIBUTE_OID,
        der_encode(
            univ.OctetString(
                hashlib.sha256(der_sequence_contents(spc_indirect_data)).digest()
            )
        ),
    )
    encoded_signed_attributes = der_encode(signed_attributes)
    signature = private_key.sign(
        encoded_signed_attributes,
        padding.PKCS1v15(),
        hashes.SHA256(),
    )
    private_key.public_key().verify(
        signature,
        encoded_signed_attributes,
        padding.PKCS1v15(),
        hashes.SHA256(),
    )

    certificate_der = certificate.public_bytes(Encoding.DER)
    certificate_asn1, trailing = der_decode(
        certificate_der,
        asn1Spec=rfc2459.Certificate(),
    )
    if trailing:
        raise ValueError("Unexpected trailing data in RSA certificate")

    signed_data = rfc2315.SignedData()
    signed_data["version"] = 1
    signed_data["digestAlgorithms"][0]["algorithm"] = univ.ObjectIdentifier(SHA256_OID)
    signed_data["digestAlgorithms"][0]["parameters"] = univ.Null("")
    signed_data["contentInfo"]["contentType"] = univ.ObjectIdentifier(
        SPC_INDIRECT_DATA_OID
    )
    signed_data["contentInfo"]["content"] = univ.Any(spc_indirect_data)
    signed_data["certificates"][0]["certificate"] = certificate_asn1

    signer_info = rfc2315.SignerInfo()
    signer_info["version"] = 1
    signer_info["issuerAndSerialNumber"]["issuer"] = certificate_asn1["tbsCertificate"][
        "issuer"
    ]
    signer_info["issuerAndSerialNumber"]["serialNumber"] = certificate_asn1[
        "tbsCertificate"
    ]["serialNumber"]
    signer_info["digestAlgorithm"]["algorithm"] = univ.ObjectIdentifier(SHA256_OID)
    signer_info["digestAlgorithm"]["parameters"] = univ.Null("")
    for index, attribute in enumerate(signed_attributes):
        signer_info["authenticatedAttributes"][index] = attribute
    signer_info["digestEncryptionAlgorithm"]["algorithm"] = univ.ObjectIdentifier(
        SHA256_WITH_RSA_OID
    )
    signer_info["digestEncryptionAlgorithm"]["parameters"] = univ.Null("")
    signer_info["encryptedDigest"] = signature
    signed_data["signerInfos"][0] = signer_info

    content_info = rfc2315.ContentInfo()
    content_info["contentType"] = univ.ObjectIdentifier(CMS_SIGNED_DATA_OID)
    content_info["content"] = univ.Any(der_encode(signed_data))
    return der_encode(content_info), encoded_signed_attributes


def ecdsa_authenticode_signature(
    private_key: ec.EllipticCurvePrivateKey,
    certificate: x509.Certificate,
    image_digest: bytes,
) -> tuple[bytes, bytes]:
    """Build an ECDSA P-256/SHA-256 Authenticode PKCS #7 signature."""
    spc_indirect_data = build_spc_indirect_data(image_digest, SHA256_OID)
    signed_attributes = rfc2315.Attributes()
    signed_attributes[0] = pkcs7_attribute(
        CMS_CONTENT_TYPE_ATTRIBUTE_OID,
        der_encode(univ.ObjectIdentifier(SPC_INDIRECT_DATA_OID)),
    )
    signed_attributes[1] = pkcs7_attribute(
        CMS_MESSAGE_DIGEST_ATTRIBUTE_OID,
        der_encode(
            univ.OctetString(
                hashlib.sha256(der_sequence_contents(spc_indirect_data)).digest()
            )
        ),
    )
    encoded_signed_attributes = der_encode(signed_attributes)
    signature = private_key.sign(
        encoded_signed_attributes,
        ec.ECDSA(hashes.SHA256()),
    )
    private_key.public_key().verify(
        signature,
        encoded_signed_attributes,
        ec.ECDSA(hashes.SHA256()),
    )

    certificate_der = certificate.public_bytes(Encoding.DER)
    certificate_asn1, trailing = der_decode(
        certificate_der,
        asn1Spec=rfc2459.Certificate(),
    )
    if trailing:
        raise ValueError("Unexpected trailing data in ECDSA certificate")

    signed_data = rfc2315.SignedData()
    signed_data["version"] = 1
    signed_data["digestAlgorithms"][0]["algorithm"] = univ.ObjectIdentifier(SHA256_OID)
    signed_data["digestAlgorithms"][0]["parameters"] = univ.Null("")
    signed_data["contentInfo"]["contentType"] = univ.ObjectIdentifier(
        SPC_INDIRECT_DATA_OID
    )
    signed_data["contentInfo"]["content"] = univ.Any(spc_indirect_data)
    signed_data["certificates"][0]["certificate"] = certificate_asn1

    signer_info = rfc2315.SignerInfo()
    signer_info["version"] = 1
    signer_info["issuerAndSerialNumber"]["issuer"] = certificate_asn1["tbsCertificate"][
        "issuer"
    ]
    signer_info["issuerAndSerialNumber"]["serialNumber"] = certificate_asn1[
        "tbsCertificate"
    ]["serialNumber"]
    signer_info["digestAlgorithm"]["algorithm"] = univ.ObjectIdentifier(SHA256_OID)
    signer_info["digestAlgorithm"]["parameters"] = univ.Null("")
    for index, attribute in enumerate(signed_attributes):
        signer_info["authenticatedAttributes"][index] = attribute
    signer_info["digestEncryptionAlgorithm"]["algorithm"] = univ.ObjectIdentifier(
        ECDSA_WITH_SHA256_OID
    )
    signer_info["encryptedDigest"] = signature
    signed_data["signerInfos"][0] = signer_info

    content_info = rfc2315.ContentInfo()
    content_info["contentType"] = univ.ObjectIdentifier(CMS_SIGNED_DATA_OID)
    content_info["content"] = univ.Any(der_encode(signed_data))
    return der_encode(content_info), encoded_signed_attributes


def win_certificate(signature: bytes) -> bytes:
    """Wrap one PKCS #7 signature in an aligned WIN_CERTIFICATE."""
    certificate_length = 8 + len(signature)
    padded_certificate_length = (certificate_length + 7) & ~7
    return (
        struct.pack("<IHH", certificate_length, 0x0200, 0x0002)
        + signature
        + bytes(padded_certificate_length - certificate_length)
    )


def apply_authenticode_signatures(
    image: bytes,
    signatures: list[bytes],
) -> bytes:
    """Append independent WIN_CERTIFICATE entries to one PE image."""
    if len(image) % 8 != 0:
        raise ValueError("PE image must be aligned before Authenticode signing")
    if not signatures:
        raise ValueError("At least one Authenticode signature is required")

    certificate_table = b"".join(win_certificate(signature) for signature in signatures)
    signed_image = bytearray(image)
    struct.pack_into(
        "<II",
        signed_image,
        pe_security_directory_offset(signed_image),
        len(signed_image),
        len(certificate_table),
    )
    signed_image.extend(certificate_table)
    return bytes(signed_image)


def apply_authenticode_signature(image: bytes, signature: bytes) -> bytes:
    """Append one PKCS #7 WIN_CERTIFICATE to a PE image."""
    return apply_authenticode_signatures(image, [signature])


def extract_payload(authenticated_update: bytes) -> bytes:
    """Return the variable payload following EFI_VARIABLE_AUTHENTICATION_2."""
    if len(authenticated_update) < 40:
        raise ValueError("authenticated update is shorter than its fixed header")
    certificate_length = struct.unpack_from("<I", authenticated_update, 16)[0]
    payload_offset = 16 + certificate_length
    if certificate_length < 24 or payload_offset > len(authenticated_update):
        raise ValueError("authenticated update has an invalid certificate length")
    return authenticated_update[payload_offset:]


def emit_array(name: str, data: bytes, description: tuple[str, ...]) -> str:
    """Render one byte array and matching UINTN size."""
    lines = ["", "/**"]
    lines.extend(f"  {line}" for line in description)
    lines.extend(["**/", f"CONST UINT8  {name}[] = {{"])
    for offset in range(0, len(data), 12):
        chunk = data[offset : offset + 12]
        hex_values = ", ".join(f"0x{value:02x}" for value in chunk) + ","
        ascii_values = "".join(
            chr(value) if 0x21 <= value <= 0x7E and value != ord("\\") else "."
            for value in chunk
        )
        lines.append(f"  {hex_values:<71} // {offset:08x}: {ascii_values}")
    if not data:
        lines.append("  0x00,")
    lines.extend(["};", f"CONST UINTN  {name}Size = {len(data)};"])
    return "\n".join(lines) + "\n"


def write_text(path: Path, text: str, crlf: bool = False) -> None:
    """Write text, optionally normalized to EDK2 CRLF line endings."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if crlf:
        text = text.replace("\r\n", "\n").replace("\n", "\r\n")
        path.write_bytes(text.encode("ascii"))
    else:
        path.write_text(text, encoding="utf-8")


def generate(
    output_c: Path,
    output_manifest: Path,
    unsigned_image_path: Path,
) -> None:
    """Generate the complete public rotation fixture."""
    key_a, certificate_a = generate_rsa_certificate("UEFI 2.12 Audit PK A")
    key_b, certificate_b = generate_rsa_certificate("UEFI 2.12 Audit PK B")
    key_mldsa, certificate_mldsa = generate_mldsa_certificate(
        "UEFI 2.12 Audit ML-DSA-87 PK"
    )
    key_ecdsa, certificate_ecdsa = generate_ecdsa_certificate(
        "UEFI 2.12 Audit ECDSA P-256 PK"
    )
    key_image_mldsa, certificate_image_mldsa = generate_mldsa_certificate(
        "UEFI 2.12 Audit ML-DSA-87 Image Signer"
    )
    key_image_rsa, certificate_image_rsa = generate_rsa_certificate(
        "UEFI 2.12 Audit RSA-2048 Image Signer"
    )
    key_image_ecdsa, certificate_image_ecdsa = generate_ecdsa_certificate(
        "UEFI 2.12 Audit ECDSA P-256 Image Signer"
    )

    pk_a = certificate_to_esl(certificate_a)
    pk_b = certificate_to_esl(certificate_b)
    pk_mldsa = certificate_to_esl(certificate_mldsa)
    pk_ecdsa = certificate_to_esl(certificate_ecdsa)
    db_tbs_hash_v1 = certificate_tbs_hash_to_esl(certificate_image_mldsa, 1)
    db_tbs_hash_v2 = certificate_tbs_hash_to_esl(certificate_image_mldsa, 2)
    db_multi_signer = certificate_tbs_hashes_to_esl(
        [certificate_image_rsa, certificate_image_mldsa],
        2,
    )
    dbx_rsa_signer = certificate_tbs_hash_to_esl(certificate_image_rsa, 2)
    dbx_mldsa_signer = certificate_tbs_hash_to_esl(
        certificate_image_mldsa,
        2,
    )
    dbx_both_signers = certificate_tbs_hashes_to_esl(
        [certificate_image_rsa, certificate_image_mldsa],
        2,
    )
    db_ca_migration = certificate_tbs_hashes_to_esl(
        [
            certificate_image_rsa,
            certificate_image_ecdsa,
            certificate_image_mldsa,
        ],
        2,
    )
    db_ca_rsa = certificate_tbs_hash_to_esl(certificate_image_rsa, 2)
    db_ca_ecdsa = certificate_tbs_hash_to_esl(certificate_image_ecdsa, 2)
    db_ca_rsa_ecdsa = certificate_tbs_hashes_to_esl(
        [certificate_image_rsa, certificate_image_ecdsa],
        2,
    )
    db_ca_ecdsa_mldsa = certificate_tbs_hashes_to_esl(
        [certificate_image_ecdsa, certificate_image_mldsa],
        2,
    )
    dbx_migration_rsa = certificate_tbs_hash_to_esl(
        certificate_image_rsa,
        2,
    )
    dbx_migration_rsa_ecdsa = certificate_tbs_hashes_to_esl(
        [certificate_image_rsa, certificate_image_ecdsa],
        2,
    )
    dbx_migration_all = certificate_tbs_hashes_to_esl(
        [
            certificate_image_rsa,
            certificate_image_ecdsa,
            certificate_image_mldsa,
        ],
        2,
    )

    unsigned_image = prepare_unsigned_pe_image(unsigned_image_path.read_bytes())
    image_digest_sha256 = authenticode_digest(unsigned_image, hashlib.sha256)
    image_digest_sha512 = authenticode_digest(unsigned_image, hashlib.sha512)
    image_digests = {
        "sha1": authenticode_digest(
            unsigned_image,
            lambda: hashlib.sha1(usedforsecurity=False),
        ),
        "sha256": image_digest_sha256,
        "sha384": authenticode_digest(unsigned_image, hashlib.sha384),
        "sha512": image_digest_sha512,
    }
    mldsa_signature, _ = mldsa_authenticode_signature(
        key_image_mldsa,
        certificate_image_mldsa,
        image_digest_sha512,
    )
    rsa_signature, _ = rsa_authenticode_signature(
        key_image_rsa,
        certificate_image_rsa,
        image_digest_sha256,
    )
    ecdsa_signature, _ = ecdsa_authenticode_signature(
        key_image_ecdsa,
        certificate_image_ecdsa,
        image_digest_sha256,
    )
    signed_image = apply_authenticode_signature(
        unsigned_image,
        mldsa_signature,
    )
    rsa_mldsa_image = apply_authenticode_signatures(
        unsigned_image,
        [rsa_signature, mldsa_signature],
    )
    mldsa_rsa_image = apply_authenticode_signatures(
        unsigned_image,
        [mldsa_signature, rsa_signature],
    )
    rsa_image = apply_authenticode_signature(unsigned_image, rsa_signature)
    ecdsa_image = apply_authenticode_signature(unsigned_image, ecdsa_signature)
    rsa_ecdsa_mldsa_image = apply_authenticode_signatures(
        unsigned_image,
        [rsa_signature, ecdsa_signature, mldsa_signature],
    )
    rsa_ecdsa_image = apply_authenticode_signatures(
        unsigned_image,
        [rsa_signature, ecdsa_signature],
    )
    ecdsa_mldsa_image = apply_authenticode_signatures(
        unsigned_image,
        [ecdsa_signature, mldsa_signature],
    )
    for image in (
        signed_image,
        rsa_mldsa_image,
        mldsa_rsa_image,
        rsa_image,
        ecdsa_image,
        rsa_ecdsa_image,
        ecdsa_mldsa_image,
        rsa_ecdsa_mldsa_image,
    ):
        if (
            authenticode_digest(image, hashlib.sha256) != image_digest_sha256
            or authenticode_digest(image, hashlib.sha512) != image_digest_sha512
        ):
            raise ValueError("Signed image Authenticode digest does not match")

    artifacts = {
        "gSecureBootKeyAuditPkA": pk_a,
        "gSecureBootKeyAuditPkB": pk_b,
        "gSecureBootKeyAuditPkMldsa": pk_mldsa,
        "gSecureBootKeyAuditPkEcdsa": pk_ecdsa,
        "gSecureBootKeyAuditSetupPkA": empty_authenticated_update(
            pk_a, BASE_TIME + datetime.timedelta(seconds=1)
        ),
        "gSecureBootKeyAuditPkAToB": signed_authenticated_update(
            pk_b,
            BASE_TIME + datetime.timedelta(seconds=2),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditPkBToA": signed_authenticated_update(
            pk_a,
            BASE_TIME + datetime.timedelta(seconds=3),
            certificate_b,
            key_b,
        ),
        "gSecureBootKeyAuditDeleteByA": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=4),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditSetupPkMldsa": empty_authenticated_update(
            pk_mldsa, BASE_TIME + datetime.timedelta(seconds=5)
        ),
        "gSecureBootKeyAuditSetupPkEcdsa": empty_authenticated_update(
            pk_ecdsa, BASE_TIME + datetime.timedelta(seconds=5)
        ),
        "gSecureBootKeyAuditDeleteByEcdsa": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=6),
            certificate_ecdsa,
            key_ecdsa,
        ),
        "gSecureBootKeyAuditDeleteByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=6),
            certificate_mldsa,
            key_mldsa,
        ),
        "gSecureBootKeyAuditSetupPkAForMldsa": empty_authenticated_update(
            pk_a, BASE_TIME + datetime.timedelta(seconds=7)
        ),
        "gSecureBootKeyAuditPkAToMldsa": signed_authenticated_update(
            pk_mldsa,
            BASE_TIME + datetime.timedelta(seconds=8),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditPkMldsaToA": mldsa_authenticated_update(
            pk_a,
            BASE_TIME + datetime.timedelta(seconds=9),
            certificate_mldsa,
            key_mldsa,
        ),
        "gSecureBootKeyAuditDeleteByAAfterMldsa": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=10),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditPkAToEcdsa": signed_authenticated_update(
            pk_ecdsa,
            BASE_TIME + datetime.timedelta(seconds=8),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditPkEcdsaToMldsa": signed_authenticated_update(
            pk_mldsa,
            BASE_TIME + datetime.timedelta(seconds=9),
            certificate_ecdsa,
            key_ecdsa,
        ),
        "gSecureBootKeyAuditDeleteByEcdsaAfterTransition": (
            signed_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=10),
                certificate_ecdsa,
                key_ecdsa,
            )
        ),
        "gSecureBootKeyAuditDeleteByMldsaAfterTransition": (
            mldsa_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=10),
                certificate_mldsa,
                key_mldsa,
            )
        ),
        "gSecureBootKeyAuditRecoveryDeleteByA": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=20),
            certificate_a,
            key_a,
        ),
        "gSecureBootKeyAuditRecoveryDeleteByB": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=21),
            certificate_b,
            key_b,
        ),
        "gSecureBootKeyAuditRecoveryDeleteByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=22),
            certificate_mldsa,
            key_mldsa,
        ),
        "gSecureBootKeyAuditRecoveryDeleteByEcdsa": signed_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=22),
            certificate_ecdsa,
            key_ecdsa,
        ),
        "gSecureBootKeyAuditDbTbsHashV1": db_tbs_hash_v1,
        "gSecureBootKeyAuditDbTbsHashV2": db_tbs_hash_v2,
        "gSecureBootKeyAuditEnrollDbTbsHashV1": mldsa_authenticated_update(
            db_tbs_hash_v1,
            BASE_TIME + datetime.timedelta(seconds=30),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditDeleteDbV1ByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=31),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbTbsHashV2": mldsa_authenticated_update(
            db_tbs_hash_v2,
            BASE_TIME + datetime.timedelta(seconds=32),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditDeleteDbV2ByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=33),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditHelloMldsa": signed_image,
        "gSecureBootKeyAuditDbMultiSigner": db_multi_signer,
        "gSecureBootKeyAuditDbxRsaSigner": dbx_rsa_signer,
        "gSecureBootKeyAuditDbxMldsaSigner": dbx_mldsa_signer,
        "gSecureBootKeyAuditDbxBothSigners": dbx_both_signers,
        "gSecureBootKeyAuditEnrollDbMultiSigner": mldsa_authenticated_update(
            db_multi_signer,
            BASE_TIME + datetime.timedelta(seconds=40),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxRsaSigner": mldsa_authenticated_update(
            dbx_rsa_signer,
            BASE_TIME + datetime.timedelta(seconds=41),
            certificate_mldsa,
            key_mldsa,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxMldsaSigner": mldsa_authenticated_update(
            dbx_mldsa_signer,
            BASE_TIME + datetime.timedelta(seconds=42),
            certificate_mldsa,
            key_mldsa,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxBothSigners": mldsa_authenticated_update(
            dbx_both_signers,
            BASE_TIME + datetime.timedelta(seconds=43),
            certificate_mldsa,
            key_mldsa,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditDeleteDbxByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=44),
            certificate_mldsa,
            key_mldsa,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditDeleteDbMultiSignerByMldsa": (
            mldsa_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=45),
                certificate_mldsa,
                key_mldsa,
                "db",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditHelloRsaMldsa": rsa_mldsa_image,
        "gSecureBootKeyAuditHelloMldsaRsa": mldsa_rsa_image,
        "gSecureBootKeyAuditDbCaRsa": db_ca_rsa,
        "gSecureBootKeyAuditDbCaEcdsa": db_ca_ecdsa,
        "gSecureBootKeyAuditDbCaRsaEcdsa": db_ca_rsa_ecdsa,
        "gSecureBootKeyAuditDbCaEcdsaMldsa": db_ca_ecdsa_mldsa,
        "gSecureBootKeyAuditDbCaMigration": db_ca_migration,
        "gSecureBootKeyAuditDbxMigrationRsa": dbx_migration_rsa,
        "gSecureBootKeyAuditDbxMigrationRsaEcdsa": (dbx_migration_rsa_ecdsa),
        "gSecureBootKeyAuditDbxMigrationAll": dbx_migration_all,
        "gSecureBootKeyAuditEnrollDbCaMigration": signed_authenticated_update(
            db_ca_migration,
            BASE_TIME + datetime.timedelta(seconds=50),
            certificate_a,
            key_a,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxMigrationRsa": (
            signed_authenticated_update(
                dbx_migration_rsa,
                BASE_TIME + datetime.timedelta(seconds=51),
                certificate_a,
                key_a,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsa": (
            signed_authenticated_update(
                dbx_migration_rsa_ecdsa,
                BASE_TIME + datetime.timedelta(seconds=52),
                certificate_a,
                key_a,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditEnrollDbxMigrationAll": (
            signed_authenticated_update(
                dbx_migration_all,
                BASE_TIME + datetime.timedelta(seconds=53),
                certificate_a,
                key_a,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditDeleteDbxCaMigrationByA": (
            signed_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=54),
                certificate_a,
                key_a,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditDeleteDbCaMigrationByA": (
            signed_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=55),
                certificate_a,
                key_a,
                "db",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditEnrollDbCaRsaByA": signed_authenticated_update(
            db_ca_rsa,
            BASE_TIME + datetime.timedelta(seconds=50),
            certificate_a,
            key_a,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbCaRsaEcdsaByA": signed_authenticated_update(
            db_ca_rsa_ecdsa,
            BASE_TIME + datetime.timedelta(seconds=50),
            certificate_a,
            key_a,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbCaRsaMldsaByA": signed_authenticated_update(
            db_multi_signer,
            BASE_TIME + datetime.timedelta(seconds=50),
            certificate_a,
            key_a,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxCaRsaByA": signed_authenticated_update(
            dbx_rsa_signer,
            BASE_TIME + datetime.timedelta(seconds=51),
            certificate_a,
            key_a,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbxCaRsaMldsaByA": signed_authenticated_update(
            dbx_both_signers,
            BASE_TIME + datetime.timedelta(seconds=52),
            certificate_a,
            key_a,
            "dbx",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbCaEcdsaByEcdsa": signed_authenticated_update(
            db_ca_ecdsa,
            BASE_TIME + datetime.timedelta(seconds=50),
            certificate_ecdsa,
            key_ecdsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditEnrollDbCaEcdsaMldsaByEcdsa": (
            signed_authenticated_update(
                db_ca_ecdsa_mldsa,
                BASE_TIME + datetime.timedelta(seconds=50),
                certificate_ecdsa,
                key_ecdsa,
                "db",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsa": (
            signed_authenticated_update(
                db_ca_ecdsa,
                BASE_TIME + datetime.timedelta(seconds=51),
                certificate_ecdsa,
                key_ecdsa,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditEnrollDbxCaEcdsaMldsaByEcdsa": (
            signed_authenticated_update(
                db_ca_ecdsa_mldsa,
                BASE_TIME + datetime.timedelta(seconds=52),
                certificate_ecdsa,
                key_ecdsa,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsa": (
            signed_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=54),
                certificate_ecdsa,
                key_ecdsa,
                "dbx",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditDeleteDbCaMigrationByEcdsa": (
            signed_authenticated_update(
                b"",
                BASE_TIME + datetime.timedelta(seconds=55),
                certificate_ecdsa,
                key_ecdsa,
                "db",
                EFI_IMAGE_SECURITY_DATABASE_GUID,
            )
        ),
        "gSecureBootKeyAuditHelloRsa": rsa_image,
        "gSecureBootKeyAuditHelloEcdsa": ecdsa_image,
        "gSecureBootKeyAuditHelloRsaEcdsa": rsa_ecdsa_image,
        "gSecureBootKeyAuditHelloEcdsaMldsa": ecdsa_mldsa_image,
        "gSecureBootKeyAuditHelloRsaEcdsaMldsa": rsa_ecdsa_mldsa_image,
    }

    format_sweep_cases = []
    format_sweep_artifacts = {}
    for index, definition in enumerate(FORMAT_SWEEP_DEFINITIONS):
        suffix, display_name, _, _, _, _ = definition
        esl_name = f"gSecureBootKeyAuditFormat{suffix}"
        enroll_db_name = f"gSecureBootKeyAuditEnrollDbFormat{suffix}"
        delete_db_name = f"gSecureBootKeyAuditDeleteDbFormat{suffix}"
        enroll_dbx_name = f"gSecureBootKeyAuditEnrollDbxFormat{suffix}"
        delete_dbx_name = f"gSecureBootKeyAuditDeleteDbxFormat{suffix}"
        esl = format_sweep_esl(
            definition,
            certificate_image_rsa,
            image_digests,
        )
        base_second = 100 + index * 4
        format_sweep_artifacts.update(
            {
                esl_name: esl,
                enroll_db_name: signed_authenticated_update(
                    esl,
                    BASE_TIME + datetime.timedelta(seconds=base_second),
                    certificate_a,
                    key_a,
                    "db",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                ),
                delete_db_name: signed_authenticated_update(
                    b"",
                    BASE_TIME + datetime.timedelta(seconds=base_second + 1),
                    certificate_a,
                    key_a,
                    "db",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                ),
                enroll_dbx_name: signed_authenticated_update(
                    esl,
                    BASE_TIME + datetime.timedelta(seconds=base_second),
                    certificate_a,
                    key_a,
                    "dbx",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                ),
                delete_dbx_name: signed_authenticated_update(
                    b"",
                    BASE_TIME + datetime.timedelta(seconds=base_second + 1),
                    certificate_a,
                    key_a,
                    "dbx",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                ),
            }
        )
        format_sweep_cases.append(
            {
                "name": display_name,
                "esl": esl_name,
                "enroll_db": enroll_db_name,
                "delete_db": delete_db_name,
                "enroll_dbx": enroll_dbx_name,
                "delete_dbx": delete_dbx_name,
            }
        )
    revocation_db_esl = format_sweep_artifacts[format_sweep_cases[0]["esl"]]
    format_sweep_artifacts.update(
        {
            "gSecureBootKeyAuditEnrollDbForRevocationSweep": (
                signed_authenticated_update(
                    revocation_db_esl,
                    BASE_TIME + datetime.timedelta(seconds=200),
                    certificate_a,
                    key_a,
                    "db",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                )
            ),
            "gSecureBootKeyAuditDeleteDbForRevocationSweep": (
                signed_authenticated_update(
                    b"",
                    BASE_TIME + datetime.timedelta(seconds=201),
                    certificate_a,
                    key_a,
                    "db",
                    EFI_IMAGE_SECURITY_DATABASE_GUID,
                )
            ),
        }
    )
    artifacts.update(format_sweep_artifacts)

    expected_payloads = {
        "gSecureBootKeyAuditSetupPkA": pk_a,
        "gSecureBootKeyAuditPkAToB": pk_b,
        "gSecureBootKeyAuditPkBToA": pk_a,
        "gSecureBootKeyAuditDeleteByA": b"",
        "gSecureBootKeyAuditSetupPkMldsa": pk_mldsa,
        "gSecureBootKeyAuditSetupPkEcdsa": pk_ecdsa,
        "gSecureBootKeyAuditDeleteByEcdsa": b"",
        "gSecureBootKeyAuditDeleteByMldsa": b"",
        "gSecureBootKeyAuditSetupPkAForMldsa": pk_a,
        "gSecureBootKeyAuditPkAToMldsa": pk_mldsa,
        "gSecureBootKeyAuditPkMldsaToA": pk_a,
        "gSecureBootKeyAuditDeleteByAAfterMldsa": b"",
        "gSecureBootKeyAuditPkAToEcdsa": pk_ecdsa,
        "gSecureBootKeyAuditPkEcdsaToMldsa": pk_mldsa,
        "gSecureBootKeyAuditDeleteByEcdsaAfterTransition": b"",
        "gSecureBootKeyAuditDeleteByMldsaAfterTransition": b"",
        "gSecureBootKeyAuditRecoveryDeleteByA": b"",
        "gSecureBootKeyAuditRecoveryDeleteByB": b"",
        "gSecureBootKeyAuditRecoveryDeleteByMldsa": b"",
        "gSecureBootKeyAuditRecoveryDeleteByEcdsa": b"",
        "gSecureBootKeyAuditEnrollDbTbsHashV1": db_tbs_hash_v1,
        "gSecureBootKeyAuditEnrollDbTbsHashV2": db_tbs_hash_v2,
        "gSecureBootKeyAuditDeleteDbV1ByMldsa": b"",
        "gSecureBootKeyAuditDeleteDbV2ByMldsa": b"",
        "gSecureBootKeyAuditEnrollDbMultiSigner": db_multi_signer,
        "gSecureBootKeyAuditEnrollDbxRsaSigner": dbx_rsa_signer,
        "gSecureBootKeyAuditEnrollDbxMldsaSigner": dbx_mldsa_signer,
        "gSecureBootKeyAuditEnrollDbxBothSigners": dbx_both_signers,
        "gSecureBootKeyAuditDeleteDbxByMldsa": b"",
        "gSecureBootKeyAuditDeleteDbMultiSignerByMldsa": b"",
        "gSecureBootKeyAuditEnrollDbCaMigration": db_ca_migration,
        "gSecureBootKeyAuditEnrollDbxMigrationRsa": dbx_migration_rsa,
        "gSecureBootKeyAuditEnrollDbxMigrationRsaEcdsa": (dbx_migration_rsa_ecdsa),
        "gSecureBootKeyAuditEnrollDbxMigrationAll": dbx_migration_all,
        "gSecureBootKeyAuditDeleteDbxCaMigrationByA": b"",
        "gSecureBootKeyAuditDeleteDbCaMigrationByA": b"",
        "gSecureBootKeyAuditEnrollDbCaRsaByA": db_ca_rsa,
        "gSecureBootKeyAuditEnrollDbCaRsaEcdsaByA": db_ca_rsa_ecdsa,
        "gSecureBootKeyAuditEnrollDbCaRsaMldsaByA": db_multi_signer,
        "gSecureBootKeyAuditEnrollDbxCaRsaByA": dbx_rsa_signer,
        "gSecureBootKeyAuditEnrollDbxCaRsaMldsaByA": dbx_both_signers,
        "gSecureBootKeyAuditEnrollDbCaEcdsaByEcdsa": db_ca_ecdsa,
        "gSecureBootKeyAuditEnrollDbCaEcdsaMldsaByEcdsa": db_ca_ecdsa_mldsa,
        "gSecureBootKeyAuditEnrollDbxCaEcdsaByEcdsa": db_ca_ecdsa,
        "gSecureBootKeyAuditEnrollDbxCaEcdsaMldsaByEcdsa": db_ca_ecdsa_mldsa,
        "gSecureBootKeyAuditDeleteDbxCaMigrationByEcdsa": b"",
        "gSecureBootKeyAuditDeleteDbCaMigrationByEcdsa": b"",
    }
    for case in format_sweep_cases:
        expected_payloads[case["enroll_db"]] = artifacts[case["esl"]]
        expected_payloads[case["delete_db"]] = b""
        expected_payloads[case["enroll_dbx"]] = artifacts[case["esl"]]
        expected_payloads[case["delete_dbx"]] = b""
    expected_payloads["gSecureBootKeyAuditEnrollDbForRevocationSweep"] = (
        revocation_db_esl
    )
    expected_payloads["gSecureBootKeyAuditDeleteDbForRevocationSweep"] = b""
    for name, expected_payload in expected_payloads.items():
        if extract_payload(artifacts[name]) != expected_payload:
            raise ValueError(f"{name} does not contain its expected payload")

    manifest = {
        "version": 6,
        "fixture_id": FIXTURE_ID,
        "scenarios": [
            "rsa2048-pk-a-to-b-to-a",
            "setup-mode-to-mldsa87",
            "rsa2048-to-mldsa87-to-rsa2048",
            "mldsa87-authenticated-update-replay",
            "mldsa87-authenticode-tbs-hash",
            "rsa2048-mldsa87-authenticode-multisign-revocation",
            "rsa2048-ecdsa-mldsa87-cumulative-ca-retirement",
            "rsa2048-image-authorization-revocation-format-sweep",
        ],
        "private_keys_emitted": False,
        "signature_owner": str(SIGNATURE_OWNER),
        "unsigned_image": {
            "name": unsigned_image_path.name,
            "size": len(unsigned_image),
            "sha256": hashlib.sha256(unsigned_image).hexdigest(),
        },
        "image_authenticode_sha256": image_digest_sha256.hex(),
        "image_authenticode_sha512": image_digest_sha512.hex(),
        "format_sweep": format_sweep_cases,
        "roles": {
            "ecdsa_pk_update_authority": {
                "certificate_artifact": "gSecureBootKeyAuditPkEcdsa",
                "subject": certificate_ecdsa.subject.rfc4514_string(),
                "certificate_sha256": hashlib.sha256(
                    certificate_ecdsa.public_bytes(Encoding.DER)
                ).hexdigest(),
                "public_key_sha256": hashlib.sha256(
                    certificate_ecdsa.public_key().public_bytes(
                        Encoding.DER,
                        PublicFormat.SubjectPublicKeyInfo,
                    )
                ).hexdigest(),
            },
            "mldsa_pk_update_authority": {
                "certificate_artifact": "gSecureBootKeyAuditPkMldsa",
                "subject": certificate_mldsa.subject.rfc4514_string(),
                "certificate_sha256": hashlib.sha256(
                    certificate_mldsa.public_bytes(Encoding.DER)
                ).hexdigest(),
                "public_key_sha256": hashlib.sha256(
                    certificate_mldsa.public_key().public_bytes(
                        Encoding.DER,
                        PublicFormat.SubjectPublicKeyInfo,
                    )
                ).hexdigest(),
            },
            "mldsa_image_signer": {
                "certificate_location": ("gSecureBootKeyAuditHelloMldsa:authenticode"),
                "subject": certificate_image_mldsa.subject.rfc4514_string(),
                "certificate_sha256": hashlib.sha256(
                    certificate_image_mldsa.public_bytes(Encoding.DER)
                ).hexdigest(),
                "public_key_sha256": hashlib.sha256(
                    certificate_image_mldsa.public_key().public_bytes(
                        Encoding.DER,
                        PublicFormat.SubjectPublicKeyInfo,
                    )
                ).hexdigest(),
                "tbs_sha512": hashlib.sha512(
                    certificate_image_mldsa.tbs_certificate_bytes
                ).hexdigest(),
            },
            "rsa_image_signer": {
                "certificate_location": (
                    "gSecureBootKeyAuditHelloRsaMldsa:authenticode[0]"
                ),
                "subject": certificate_image_rsa.subject.rfc4514_string(),
                "certificate_sha256": hashlib.sha256(
                    certificate_image_rsa.public_bytes(Encoding.DER)
                ).hexdigest(),
                "public_key_sha256": hashlib.sha256(
                    certificate_image_rsa.public_key().public_bytes(
                        Encoding.DER,
                        PublicFormat.SubjectPublicKeyInfo,
                    )
                ).hexdigest(),
                "tbs_sha512": hashlib.sha512(
                    certificate_image_rsa.tbs_certificate_bytes
                ).hexdigest(),
            },
            "ecdsa_image_signer": {
                "certificate_location": (
                    "gSecureBootKeyAuditHelloRsaEcdsaMldsa:authenticode[1]"
                ),
                "subject": certificate_image_ecdsa.subject.rfc4514_string(),
                "certificate_sha256": hashlib.sha256(
                    certificate_image_ecdsa.public_bytes(Encoding.DER)
                ).hexdigest(),
                "public_key_sha256": hashlib.sha256(
                    certificate_image_ecdsa.public_key().public_bytes(
                        Encoding.DER,
                        PublicFormat.SubjectPublicKeyInfo,
                    )
                ).hexdigest(),
                "tbs_sha512": hashlib.sha512(
                    certificate_image_ecdsa.tbs_certificate_bytes
                ).hexdigest(),
            },
        },
        "required_ecit": {
            "database_update_authorization": ["EFI_CERT_X509_GUID"],
            "database_update_verification": [
                SHA256_WITH_RSA_OID,
                SHA256_OID,
                ML_DSA_87_OID,
                SHA512_OID,
            ],
            "image_authorization": [
                "EFI_CERT_X509_SHA512_GUID",
                "EFI_CERT_V2_X509_SHA512_GUID",
            ],
            "image_verification": [
                SHA256_WITH_RSA_OID,
                ECDSA_WITH_SHA256_OID,
                SHA256_OID,
                ML_DSA_87_OID,
                SHA512_OID,
            ],
            "image_revocation": [
                "EFI_CERT_V2_X509_SHA512_GUID",
            ],
        },
        "artifacts": {
            name: {
                "size": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
            for name, data in artifacts.items()
        },
    }
    manifest_text = json.dumps(manifest, indent=2) + "\n"
    manifest_sha256 = hashlib.sha256(manifest_text.encode("utf-8")).digest()

    c_source = C_HEADER
    for name, data in artifacts.items():
        c_source += emit_array(
            name,
            data,
            ARTIFACT_DESCRIPTIONS.get(
                name,
                (
                    "Capability-selected Secure Boot audit fixture.",
                    "See Host/TestData.manifest.json for its size and SHA-256 binding.",
                ),
            ),
        )
    c_source += emit_array(
        "gSecureBootKeyAuditFixtureId",
        FIXTURE_ID.encode("ascii"),
        (
            "Stable identifier for this fixture schema.",
            "Recorded in the persistent audit journal across reboot boundaries.",
        ),
    )
    c_source += emit_array(
        "gSecureBootKeyAuditManifestSha256",
        manifest_sha256,
        (
            "SHA-256 of Host/TestData.manifest.json.",
            "Binds persistent journal records to the generated fixture manifest.",
        ),
    )
    c_source += "\nCONST SECURE_BOOT_FORMAT_SWEEP_CASE  "
    c_source += "gSecureBootKeyAuditFormatSweepCases[] = {\n"
    for case in format_sweep_cases:
        c_source += (
            "  {\n"
            f'    "{case["name"]}",\n'
            f"    {case['esl']},\n"
            f"    {case['esl']}Size,\n"
            f"    {case['enroll_db']},\n"
            f"    {case['enroll_db']}Size,\n"
            f"    {case['delete_db']},\n"
            f"    {case['delete_db']}Size,\n"
            f"    {case['enroll_dbx']},\n"
            f"    {case['enroll_dbx']}Size,\n"
            f"    {case['delete_dbx']},\n"
            f"    {case['delete_dbx']}Size\n"
            "  },\n"
        )
    c_source += "};\n\n"
    c_source += (
        "CONST UINTN  gSecureBootKeyAuditFormatSweepCaseCount = "
        "ARRAY_SIZE (gSecureBootKeyAuditFormatSweepCases);\n"
    )

    write_text(output_c, c_source, crlf=True)
    write_text(output_manifest, manifest_text)


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    script_dir = Path(__file__).resolve().parent
    uefi_dir = script_dir.parent / "UEFI"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-c",
        type=Path,
        default=uefi_dir / "TestData.c",
        help="generated C output path",
    )
    parser.add_argument(
        "--output-manifest",
        type=Path,
        default=script_dir / "TestData.manifest.json",
        help="public artifact manifest output path",
    )
    parser.add_argument(
        "--unsigned-image",
        type=Path,
        default=(
            script_dir.parents[5]
            / "Build"
            / "QemuQ35PkgX64"
            / "DEBUG_GCC5"
            / "X64"
            / "HelloUefiApp.efi"
        ),
        help="unsigned HelloUefiApp.efi input path",
    )
    return parser.parse_args()


def main() -> None:
    """Generate the fixture and manifest."""
    args = parse_args()
    generate(
        args.output_c.resolve(),
        args.output_manifest.resolve(),
        args.unsigned_image.resolve(),
    )
    print(f"Wrote {args.output_c.resolve()}")
    print(f"Wrote {args.output_manifest.resolve()}")


if __name__ == "__main__":
    main()
