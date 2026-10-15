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
from cryptography.hazmat.primitives.asymmetric import mldsa, rsa
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
EFI_CERT_X509_SHA512_GUID = uuid.UUID("446dbf63-2502-4cda-bcfa-2465d2b0fe9d")
EFI_CERT_V2_X509_SHA512_GUID = uuid.UUID("4d46147a-fa79-43a9-8836-00341c708a70")
SIGNATURE_OWNER = uuid.UUID("d4f18610-2ce9-4d6c-a3c2-0bd4727db67b")
VARIABLE_ATTRIBUTES = "NV,BS,RT,AT"
FIXTURE_ID = "secure-boot-key-audit-v2"

SHA256_WITH_RSA_OID = "1.2.840.113549.1.1.11"
SHA256_OID = "2.16.840.1.101.3.4.2.1"
SHA512_OID = "2.16.840.1.101.3.4.2.3"
ML_DSA_87_OID = "2.16.840.1.101.3.4.3.19"
CMS_DATA_OID = "1.2.840.113549.1.7.1"
CMS_SIGNED_DATA_OID = "1.2.840.113549.1.7.2"
CMS_CONTENT_TYPE_ATTRIBUTE_OID = "1.2.840.113549.1.9.3"
CMS_MESSAGE_DIGEST_ATTRIBUTE_OID = "1.2.840.113549.1.9.4"
SPC_INDIRECT_DATA_OID = "1.3.6.1.4.1.311.2.1.4"

BASE_TIME = datetime.datetime(2026, 1, 1, tzinfo=datetime.timezone.utc)

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
    "gSecureBootKeyAuditDeleteDbByMldsa": (
        "Authenticated deletion of the test db.",
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
    signature_list.AddSignatureHeader(
        None, SigSize=signature_data.get_total_size()
    )
    signature_list.AddSignatureData(signature_data)
    return signature_list.encode()


def certificate_tbs_hash_to_esl(
    certificate: x509.Certificate,
    version: int,
) -> bytes:
    """Encode a SHA-512 TBSCertificate hash as a V1 or V2 signature list."""
    digest = hashlib.sha512(certificate.tbs_certificate_bytes).digest()
    if version == 1:
        signature_type = EFI_CERT_X509_SHA512_GUID
        signature_data = SIGNATURE_OWNER.bytes_le + digest + bytes(16)
    elif version == 2:
        signature_type = EFI_CERT_V2_X509_SHA512_GUID
        signature_data = digest
    else:
        raise ValueError(f"Unsupported TBS hash signature-list version: {version}")

    signature_size = len(signature_data)
    signature_list_size = 16 + 12 + signature_size
    return (
        signature_type.bytes_le
        + struct.pack("<III", signature_list_size, 0, signature_size)
        + signature_data
    )


def empty_authenticated_update(payload: bytes, timestamp: datetime.datetime) -> bytes:
    """Build the unsigned authentication descriptor allowed in Setup Mode."""
    efi_time = encode_efi_time(timestamp)
    win_certificate = (
        struct.pack("<IHH", 24, 0x0200, 0x0EF1)
        + EFI_CERT_PKCS7_GUID.bytes_le
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
            security_directory_offset
            + 8 : certificate_offset if certificate_offset != 0 else len(image)
        ]
    )
    return digest.digest()


def build_spc_indirect_data(image_digest: bytes) -> bytes:
    """Build a SHA-512 SpcIndirectDataContent value."""
    sha512_algorithm = bytes.fromhex("300d06096086480165030402030500")
    digest_info = der_sequence(
        sha512_algorithm + b"\x04" + der_length(len(image_digest)) + image_digest
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
    spc_indirect_data = build_spc_indirect_data(image_digest)
    signed_attributes = rfc2315.Attributes()
    signed_attributes[0] = pkcs7_attribute(
        CMS_CONTENT_TYPE_ATTRIBUTE_OID,
        der_encode(univ.ObjectIdentifier(SPC_INDIRECT_DATA_OID)),
    )
    signed_attributes[1] = pkcs7_attribute(
        CMS_MESSAGE_DIGEST_ATTRIBUTE_OID,
        der_encode(
            univ.OctetString(
                hashlib.sha512(
                    der_sequence_contents(spc_indirect_data)
                ).digest()
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
    signed_data["digestAlgorithms"][0]["algorithm"] = univ.ObjectIdentifier(
        SHA512_OID
    )
    signed_data["digestAlgorithms"][0]["parameters"] = univ.Null("")
    signed_data["contentInfo"]["contentType"] = univ.ObjectIdentifier(
        SPC_INDIRECT_DATA_OID
    )
    signed_data["contentInfo"]["content"] = univ.Any(spc_indirect_data)
    signed_data["certificates"][0]["certificate"] = certificate_asn1

    signer_info = rfc2315.SignerInfo()
    signer_info["version"] = 1
    signer_info["issuerAndSerialNumber"]["issuer"] = certificate_asn1[
        "tbsCertificate"
    ]["issuer"]
    signer_info["issuerAndSerialNumber"]["serialNumber"] = certificate_asn1[
        "tbsCertificate"
    ]["serialNumber"]
    signer_info["digestAlgorithm"]["algorithm"] = univ.ObjectIdentifier(
        SHA512_OID
    )
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


def apply_authenticode_signature(image: bytes, signature: bytes) -> bytes:
    """Append a PKCS #7 WIN_CERTIFICATE and update the PE data directory."""
    if len(image) % 8 != 0:
        raise ValueError("PE image must be aligned before Authenticode signing")

    certificate_length = 8 + len(signature)
    padded_certificate_length = (certificate_length + 7) & ~7
    win_certificate = (
        struct.pack("<IHH", certificate_length, 0x0200, 0x0002)
        + signature
        + bytes(padded_certificate_length - certificate_length)
    )
    signed_image = bytearray(image)
    struct.pack_into(
        "<II",
        signed_image,
        pe_security_directory_offset(signed_image),
        len(signed_image),
        len(win_certificate),
    )
    signed_image.extend(win_certificate)
    return bytes(signed_image)


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
            chr(value)
            if 0x21 <= value <= 0x7E and value != ord("\\")
            else "."
            for value in chunk
        )
        lines.append(
            f"  {hex_values:<71} // {offset:08x}: {ascii_values}"
        )
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
    key_image_mldsa, certificate_image_mldsa = generate_mldsa_certificate(
        "UEFI 2.12 Audit ML-DSA-87 Image Signer"
    )

    pk_a = certificate_to_esl(certificate_a)
    pk_b = certificate_to_esl(certificate_b)
    pk_mldsa = certificate_to_esl(certificate_mldsa)
    db_tbs_hash_v1 = certificate_tbs_hash_to_esl(certificate_image_mldsa, 1)
    db_tbs_hash_v2 = certificate_tbs_hash_to_esl(certificate_image_mldsa, 2)

    unsigned_image = prepare_unsigned_pe_image(unsigned_image_path.read_bytes())
    image_digest = authenticode_digest(unsigned_image, hashlib.sha512)
    authenticode_signature, _ = mldsa_authenticode_signature(
        key_image_mldsa,
        certificate_image_mldsa,
        image_digest,
    )
    signed_image = apply_authenticode_signature(
        unsigned_image,
        authenticode_signature,
    )
    if authenticode_digest(signed_image, hashlib.sha512) != image_digest:
        raise ValueError("Signed image Authenticode digest does not match")

    artifacts = {
        "gSecureBootKeyAuditPkA": pk_a,
        "gSecureBootKeyAuditPkB": pk_b,
        "gSecureBootKeyAuditPkMldsa": pk_mldsa,
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
        "gSecureBootKeyAuditEnrollDbTbsHashV2": mldsa_authenticated_update(
            db_tbs_hash_v2,
            BASE_TIME + datetime.timedelta(seconds=31),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditDeleteDbByMldsa": mldsa_authenticated_update(
            b"",
            BASE_TIME + datetime.timedelta(seconds=32),
            certificate_mldsa,
            key_mldsa,
            "db",
            EFI_IMAGE_SECURITY_DATABASE_GUID,
        ),
        "gSecureBootKeyAuditHelloMldsa": signed_image,
    }

    expected_payloads = {
        "gSecureBootKeyAuditSetupPkA": pk_a,
        "gSecureBootKeyAuditPkAToB": pk_b,
        "gSecureBootKeyAuditPkBToA": pk_a,
        "gSecureBootKeyAuditDeleteByA": b"",
        "gSecureBootKeyAuditSetupPkMldsa": pk_mldsa,
        "gSecureBootKeyAuditDeleteByMldsa": b"",
        "gSecureBootKeyAuditSetupPkAForMldsa": pk_a,
        "gSecureBootKeyAuditPkAToMldsa": pk_mldsa,
        "gSecureBootKeyAuditPkMldsaToA": pk_a,
        "gSecureBootKeyAuditDeleteByAAfterMldsa": b"",
        "gSecureBootKeyAuditRecoveryDeleteByA": b"",
        "gSecureBootKeyAuditRecoveryDeleteByB": b"",
        "gSecureBootKeyAuditRecoveryDeleteByMldsa": b"",
        "gSecureBootKeyAuditEnrollDbTbsHashV1": db_tbs_hash_v1,
        "gSecureBootKeyAuditEnrollDbTbsHashV2": db_tbs_hash_v2,
        "gSecureBootKeyAuditDeleteDbByMldsa": b"",
    }
    for name, expected_payload in expected_payloads.items():
        if extract_payload(artifacts[name]) != expected_payload:
            raise ValueError(f"{name} does not contain its expected payload")

    manifest = {
        "version": 2,
        "fixture_id": FIXTURE_ID,
        "scenarios": [
            "rsa2048-pk-a-to-b-to-a",
            "setup-mode-to-mldsa87",
            "rsa2048-to-mldsa87-to-rsa2048",
            "mldsa87-authenticode-tbs-hash",
        ],
        "private_keys_emitted": False,
        "signature_owner": str(SIGNATURE_OWNER),
        "unsigned_image": {
            "name": unsigned_image_path.name,
            "size": len(unsigned_image),
            "sha256": hashlib.sha256(unsigned_image).hexdigest(),
        },
        "image_authenticode_sha512": image_digest.hex(),
        "roles": {
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
                "certificate_location": (
                    "gSecureBootKeyAuditHelloMldsa:authenticode"
                ),
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
                ML_DSA_87_OID,
                SHA512_OID,
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
        c_source += emit_array(name, data, ARTIFACT_DESCRIPTIONS[name])
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
