#
# This module contains classes and helpers for working with TCG logs
# as defined in the Trusted Computing Group industry specfications.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

from __future__ import annotations

import struct
import hashlib
import unittest

from edk2toollib.tpm.tpm2_defs import (
    TPM_ALG_SHA1,
    TPM_ALG_SHA256,
    TPM_ALG_SHA384,
    TPM_ALG_SHA512,
)
from typing import Dict, Iterable

SHA1_DIGEST_SIZE = 0x14
SHA256_DIGEST_SIZE = 0x20
SHA384_DIGEST_SIZE = 0x30
SHA512_DIGEST_SIZE = 0x40

# These constants are used in PCR manipulation routines so that
# the values are less "magic".
PCR_0 = 0
PCR_1 = 1
PCR_2 = 2
PCR_3 = 3
PCR_4 = 4
PCR_5 = 5
PCR_6 = 6
PCR_7 = 7
TPM_LOCALITY_3 = 3

EV_PREBOOT_CERT = 0x00000000
EV_POST_CODE = 0x00000001
EV_UNUSED = 0x00000002
EV_NO_ACTION = 0x00000003
EV_SEPARATOR = 0x00000004
EV_ACTION = 0x00000005
EV_EVENT_TAG = 0x00000006
EV_S_CRTM_CONTENTS = 0x00000007
EV_S_CRTM_VERSION = 0x00000008
EV_CPU_MICROCODE = 0x00000009
EV_PLATFORM_CONFIG_FLAGS = 0x0000000A
EV_TABLE_OF_DEVICES = 0x0000000B
EV_COMPACT_HASH = 0x0000000C
EV_IPL = 0x0000000D
EV_IPL_PARTITION_DATA = 0x0000000E
EV_NONHOST_CODE = 0x0000000F
EV_NONHOST_CONFIG = 0x00000010
EV_NONHOST_INFO = 0x00000011
EV_OMIT_BOOT_DEVICE_EVENTS = 0x00000012

EV_EFI_EVENT_BASE = 0x80000000
EV_EFI_VARIABLE_DRIVER_CONFIG = EV_EFI_EVENT_BASE + 1
EV_EFI_VARIABLE_BOOT = EV_EFI_EVENT_BASE + 2
EV_EFI_BOOT_SERVICES_APPLICATION = EV_EFI_EVENT_BASE + 3
EV_EFI_BOOT_SERVICES_DRIVER = EV_EFI_EVENT_BASE + 4
EV_EFI_RUNTIME_SERVICES_DRIVER = EV_EFI_EVENT_BASE + 5
EV_EFI_GPT_EVENT = EV_EFI_EVENT_BASE + 6
EV_EFI_ACTION = EV_EFI_EVENT_BASE + 7
EV_EFI_PLATFORM_FIRMWARE_BLOB = EV_EFI_EVENT_BASE + 8
EV_EFI_HANDOFF_TABLES = EV_EFI_EVENT_BASE + 9
EV_EFI_PLATFORM_FIRMWARE_BLOB2 = EV_EFI_EVENT_BASE + 0xA
EV_EFI_HANDOFF_TABLES2 = EV_EFI_EVENT_BASE + 0xB
EV_EFI_VARIABLE_BOOT2 = EV_EFI_EVENT_BASE + 0xC
EV_EFI_HCRTM_EVENT = EV_EFI_EVENT_BASE + 0x10
EV_EFI_VARIABLE_AUTHORITY = EV_EFI_EVENT_BASE + 0xE0
EV_EFI_SPDM_FIRMWARE_BLOB = EV_EFI_EVENT_BASE + 0xE1
EV_EFI_SPDM_FIRMWARE_CONFIG = EV_EFI_EVENT_BASE + 0xE2

VALUE_FROM_EVENT = {
    "EV_PREBOOT_CERT": EV_PREBOOT_CERT,
    "EV_POST_CODE": EV_POST_CODE,
    "EV_UNUSED": EV_UNUSED,
    "EV_NO_ACTION": EV_NO_ACTION,
    "EV_SEPARATOR": EV_SEPARATOR,
    "EV_ACTION": EV_ACTION,
    "EV_EVENT_TAG": EV_EVENT_TAG,
    "EV_S_CRTM_CONTENTS": EV_S_CRTM_CONTENTS,
    "EV_S_CRTM_VERSION": EV_S_CRTM_VERSION,
    "EV_CPU_MICROCODE": EV_CPU_MICROCODE,
    "EV_PLATFORM_CONFIG_FLAGS": EV_PLATFORM_CONFIG_FLAGS,
    "EV_TABLE_OF_DEVICES": EV_TABLE_OF_DEVICES,
    "EV_COMPACT_HASH": EV_COMPACT_HASH,
    "EV_IPL": EV_IPL,
    "EV_IPL_PARTITION_DATA": EV_IPL_PARTITION_DATA,
    "EV_NONHOST_CODE": EV_NONHOST_CODE,
    "EV_NONHOST_CONFIG": EV_NONHOST_CONFIG,
    "EV_NONHOST_INFO": EV_NONHOST_INFO,
    "EV_OMIT_BOOT_DEVICE_EVENTS": EV_OMIT_BOOT_DEVICE_EVENTS,
    "EV_EFI_EVENT_BASE": EV_EFI_EVENT_BASE,
    "EV_EFI_VARIABLE_DRIVER_CONFIG": EV_EFI_VARIABLE_DRIVER_CONFIG,
    "EV_EFI_VARIABLE_BOOT": EV_EFI_VARIABLE_BOOT,
    "EV_EFI_BOOT_SERVICES_APPLICATION": EV_EFI_BOOT_SERVICES_APPLICATION,
    "EV_EFI_BOOT_SERVICES_DRIVER": EV_EFI_BOOT_SERVICES_DRIVER,
    "EV_EFI_RUNTIME_SERVICES_DRIVER": EV_EFI_RUNTIME_SERVICES_DRIVER,
    "EV_EFI_GPT_EVENT": EV_EFI_GPT_EVENT,
    "EV_EFI_ACTION": EV_EFI_ACTION,
    "EV_EFI_PLATFORM_FIRMWARE_BLOB": EV_EFI_PLATFORM_FIRMWARE_BLOB,
    "EV_EFI_HANDOFF_TABLES": EV_EFI_HANDOFF_TABLES,
    "EV_EFI_PLATFORM_FIRMWARE_BLOB2": EV_EFI_PLATFORM_FIRMWARE_BLOB2,
    "EV_EFI_HANDOFF_TABLES2": EV_EFI_HANDOFF_TABLES2,
    "EV_EFI_VARIABLE_BOOT2": EV_EFI_VARIABLE_BOOT2,
    "EV_EFI_HCRTM_EVENT": EV_EFI_HCRTM_EVENT,
    "EV_EFI_VARIABLE_AUTHORITY": EV_EFI_VARIABLE_AUTHORITY,
    "EV_EFI_SPDM_FIRMWARE_BLOB": EV_EFI_SPDM_FIRMWARE_BLOB,
    "EV_EFI_SPDM_FIRMWARE_CONFIG": EV_EFI_SPDM_FIRMWARE_CONFIG,
}

EVENT_FROM_VALUE = {v: k for k, v in VALUE_FROM_EVENT.items()}

VALUE_FROM_ALG = {
    "sha1": TPM_ALG_SHA1,
    "sha256": TPM_ALG_SHA256,
    "sha384": TPM_ALG_SHA384,
    "sha512": TPM_ALG_SHA512,
}

ALG_FROM_VALUE = {v: k for k, v in VALUE_FROM_ALG.items()}

SIZE_FROM_ALG_NAME = {
    "sha1": SHA1_DIGEST_SIZE,
    "sha256": SHA256_DIGEST_SIZE,
    "sha384": SHA384_DIGEST_SIZE,
    "sha512": SHA512_DIGEST_SIZE,
}

ALG_NAME_FROM_SIZE = {v: k for k, v in SIZE_FROM_ALG_NAME.items()}

SIZE_FROM_ALG = {VALUE_FROM_ALG[k]: v for k, v in SIZE_FROM_ALG_NAME.items()}
ALG_FROM_SIZE = {v: k for k, v in SIZE_FROM_ALG.items()}


def _get_hasher_for_alg(alg: int) -> object:
    """Returns the hasher for the given hash algorithm ID.

    Args:
        alg (int): The algorithm ID.

    Raises:
        ValueError: An invalid algorithm ID was given.

    Returns:
        Hash object: A hashlib Hash object.
    """
    valid_algs = {
        TPM_ALG_SHA256: hashlib.sha256(),
        TPM_ALG_SHA384: hashlib.sha384(),
        TPM_ALG_SHA512: hashlib.sha512(),
    }
    if alg not in valid_algs:
        raise ValueError(f"Invalid algorithm provided! 0x{alg:x}")
    return valid_algs[alg]


class TpmtHa(object):
    _hdr_struct_format = "<H"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(self, alg: int = TPM_ALG_SHA256, digest: bytes = None):
        """Class constructor method.

        Args:
            alg (int, optional): Hash algorithm ID. Defaults to TPM_ALG_SHA256.
            digest (bytes, optional): Digest bytes object. Defaults to None.
        """
        if digest is None:
            self.hash_alg = alg
            self.hash_digest = b"\x00" * SIZE_FROM_ALG[alg]
        else:
            self.hash_alg = ALG_FROM_SIZE[len(digest)]
            self.hash_digest = digest

    def __str__(self):
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\t\tTPMT_HA\n"
        debug_str += "\t\t\tHashAlg  = 0x%04X\n" % self.hash_alg
        debug_str += "\t\t\tDigest   = %s\n" % self.hash_digest.hex().upper()
        return debug_str

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TpmtHa:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TpmtHa: A TpmtHa instance.
        """
        alg, *_ = struct.unpack_from(TpmtHa._hdr_struct_format, binary_data)
        offset = TpmtHa._hdr_struct_size
        digest = binary_data[offset : offset + SIZE_FROM_ALG[alg]]
        return cls(digest=digest)

    def get_size(self):
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        return self._hdr_struct_size + len(self.hash_digest)

    def encode(self):
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        return struct.pack(self._hdr_struct_format, self.hash_alg) + self.hash_digest

    def reset_with_locality(self, locality: int) -> TpmtHa:
        """Resets with a given locality.

        Args:
            locality (int): The locality.

        Raises:
            ValueError: An invalid locality was given.

        Returns:
            TpmtHa: A TpmtHa instance with the locality reset to the requested
            value.
        """
        if locality < 0 or locality > 4:
            raise ValueError(f"Invalid locality provided! 0x{locality:x}")
        self.hash_digest = b"\x00" * (SIZE_FROM_ALG[self.hash_alg] - 1)
        self.hash_digest += struct.pack("B", locality)
        return self

    def hash_data(self, data: bytes) -> bytes:
        """Hashes a given bytes buffer.

        Args:
            data (bytes): The data.

        Returns:
            bytes: The digest.
        """
        hasher = _get_hasher_for_alg(self.hash_alg)
        hasher.update(data)
        return hasher.digest()

    def set_hash_digest(self, digest: bytes) -> None:
        """Sets the digest to the given value.

        Args:
            digest (bytes): The digest value.

        Raises:
            ValueError: The digest value is an invalid size for the assigned
            hash algorithm.
        """
        if len(digest) != SIZE_FROM_ALG[self.hash_alg]:
            raise ValueError(
                "Incorrect digest length! 0x%X != 0x%X"
                % (len(digest), SIZE_FROM_ALG[self.hash_alg])
            )
        self.hash_digest = digest

    def set_hash_data(self, data: bytes) -> None:
        """Set the digest to the hash of the given data.

        Args:
            data (bytes): The data.
        """
        self.hash_digest = self.hash_data(data)

    def extend_digest(self, digest: bytes) -> None:
        """Extend the given digest.

        Args:
            digest (bytes): The digest.

        Raises:
            ValueError: The digest value is an invalid size for the assigned
            hash algorithm.
        """
        if len(digest) != SIZE_FROM_ALG[self.hash_alg]:
            raise ValueError(
                "Incorrect digest length! 0x%X != 0x%X"
                % (len(digest), SIZE_FROM_ALG[self.hash_alg])
            )
        hasher = _get_hasher_for_alg(self.hash_alg)
        hasher.update(self.hash_digest + digest)
        self.hash_digest = hasher.digest()

    def extend_data(self, data: bytes) -> None:
        """Extend the hash of the given data.

        Args:
            data (bytes): The data.
        """
        self.extend_digest(self.hash_data(data))


class TpmtHa_Test(unittest.TestCase):
    def test_initialize_by_digest(self):
        test = TpmtHa(b"\x00" * SHA384_DIGEST_SIZE)
        self.assertEqual(test.HashAlg, TPM_ALG_SHA384)
        test = TpmtHa(b"\x00" * SHA512_DIGEST_SIZE)
        self.assertEqual(test.HashAlg, TPM_ALG_SHA512)

        with self.assertRaises(ValueError):
            test = TpmtHa(b"\x00" * 13)

    def test_initialize_by_alg(self):
        test = TpmtHa(alg=TPM_ALG_SHA384)
        self.assertEqual(len(test.HashDigest), SHA384_DIGEST_SIZE)
        test = TpmtHa(digest=None, alg=TPM_ALG_SHA256)
        self.assertEqual(len(test.HashDigest), SHA256_DIGEST_SIZE)

        with self.assertRaises(ValueError):
            test = TpmtHa(alg=0x00)
        with self.assertRaises(ValueError):
            test = TpmtHa(alg=0xFF)

    def test_reset_with_locality(self):
        test = TpmtHa()
        self.assertEqual(test.HashAlg, TPM_ALG_SHA256)
        self.assertEqual(test.HashDigest, b"\x00" * SHA256_DIGEST_SIZE)
        test.reset_with_locality(3)
        self.assertEqual(
            test.HashDigest, (b"\x00" * (SHA256_DIGEST_SIZE - 1)) + b"\x03"
        )
        test.reset_with_locality(2)
        self.assertEqual(
            test.HashDigest, (b"\x00" * (SHA256_DIGEST_SIZE - 1)) + b"\x02"
        )

        test = TpmtHa(alg=TPM_ALG_SHA384).reset_with_locality(4)
        self.assertEqual(
            test.HashDigest, (b"\x00" * (SHA384_DIGEST_SIZE - 1)) + b"\x04"
        )

        with self.assertRaises(ValueError):
            test = TpmtHa()
            test.reset_with_locality(5)
        with self.assertRaises(ValueError):
            test = TpmtHa()
            test.reset_with_locality(-1)


class TpmlDigestValues(object):
    # typedef struct {
    #     UINT32  count;
    #     TPMT_HA digests[HASH_COUNT];
    # } TPML_DIGEST_VALUES;
    _hdr_struct_format = "<I"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(self, algs: Iterable = None, digests: Dict[int, TpmtHa] = {}):
        """Class constructor method.

        Args:
            algs (Iterable, optional): Hash algorithms supported. Defaults to None.
            digests (Dict[int,TpmtHa], optional): A dictionary of algorithm IDs
              associated with a TpmtHa object. Defaults to {}.
        """
        self.digests = digests
        if algs is not None:
            for alg in algs:
                self.add_algorithm(alg)

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\t\tTPML_DIGEST_VALUES\n"
        debug_str += "\t\t\tDigest Count = 0x%04X\n" % len(self.digests)
        for digest in self.digests.values():
            debug_str += str(digest)
        return debug_str

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TpmlDigestValues:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            CalculatedPcrState: A CalculatedPcrState instance.
        """
        count, *_ = struct.unpack_from(TcgPcrEvent2._hdr_struct_format, binary_data)

        digests = {}
        offset = TpmlDigestValues._hdr_struct_size
        for _ in range(0, count):
            tpmt_ha = TpmtHa.from_binary(binary_data[offset:])
            digests[tpmt_ha.hash_alg] = tpmt_ha
            offset += tpmt_ha.get_size()

        return cls(digests=digests)

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        result = self._hdr_struct_size
        for digest in self.digests.values():
            result += digest.get_size()
        return result

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        result = struct.pack(self._hdr_struct_format, len(self.digests))
        for digest in self.digests.values():
            result += digest.encode()
        return result

    def add_algorithm(self, alg: int) -> None:
        """Adds the given algorithm.

        Args:
            alg (int): Algorithm ID.

        Raises:
            ValueError: The algorithm is already present.
        """
        if alg in self.digests:
            raise ValueError("Algorithm already present! 0x%X" % alg)
        self.digests[alg] = TpmtHa(alg)

    def reset(self) -> TpmlDigestValues:
        """Resets with Locality 0.

        Returns:
            TpmlDigestValues: The digest values object after reset.
        """
        return self.reset_with_locality(0)

    def reset_with_locality(self, locality: int) -> TpmlDigestValues:
        """Resets with the given locality.

        Args:
            locality (int): The locality.

        Returns:
            TpmlDigestValues: The TpmlDigestValues instance after locality reset.
        """
        for digest in self.digests.values():
            digest.reset_with_locality(locality)
        return self

    def set_hash_data(self, data):
        for digest in self.digests.values():
            digest.set_hash_data(data)

    def extend_data(self, data):
        for digest in self.digests.values():
            digest.extend_data(data)


class TcgEfiSpecIdAlgorithmSize(object):
    """TCG EFI Specification ID Algorithm Size structure."""

    # typedef struct {
    #   UINT16    algorithmId;  // TCG defined hashing algorithm ID.
    #   UINT16    digestSize;
    # } TCG_EfiSpecIdEventAlgorithmSize;
    _hdr_struct_format = "<HH"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(self, algorithm_id: int, digest_size: int):
        """Class constructor method.

        Args:
            algorithm_id (int): Algorithm ID.
            digest_size (int): Algorith digest size in bytes.
        """
        self.algorithm_id = algorithm_id
        self.digest_size = digest_size

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\n\tTCG_EFI_SPEC_ID_ALGORITHM_SIZE\n"
        debug_str += "\t\tAlgorithm ID    = 0x%02x\n" % self.algorithm_id
        debug_str += "\t\tDigest Size     = 0x%02X\n" % self.digest_size
        return debug_str

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        return self._hdr_struct_size

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        return struct.pack(self._hdr_struct_format, self.algorithm_id, self.digest_size)

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TcgEfiSpecIdAlgorithmSize:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TcgEfiSpecIdAlgorithmSize: A TcgEfiSpecIdAlgorithmSize instance.
        """
        algorithm_id, digest_size = struct.unpack_from(
            TcgEfiSpecIdAlgorithmSize._hdr_struct_format, binary_data
        )
        return cls(algorithm_id, digest_size)


class TcgEfiSpecIdEvent(object):
    """TCG EFI Specification ID structure."""

    _hdr_struct_format_1 = "<16sIBBBBI"
    _hdr_struct_format_2 = "<B"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format_1) + struct.calcsize(
        _hdr_struct_format_2
    )

    def __init__(
        self,
        signature: bytes,
        platform_class: int,
        spec_version_minor: int,
        spec_version_major: int,
        spec_errata: int,
        uintn_size: int,
        number_of_algorithms: int,
        digest_sizes: Iterable[TcgEfiSpecIdAlgorithmSize],
        vendor_info_size: int,
        vendor_info: bytes,
    ):
        """Class constructor method.

        Args:
            signature (bytes): b"Spec ID Event03"
            platform_class (int): Platform class
            spec_version_minor (int): PC Client PFP Spec minor version
            spec_version_major (int): PC Client PFP Spec major version
            spec_errata (int): PC Client PFP Spc Revision number
            uintn_size (int): UINTN field size
            number_of_algorithms (int): Number of hash algorithms present.
            digest_sizes (Iterable[TcgEfiSpecIdAlgorithmSize]): Algorithm ID
                and corresponding digest size in bytes.
            vendor_info_size (int): Size in bytes of the vendor info present.
            vendor_info (bytes): Custom data provided by the platform firmware.
        """
        self.signature = signature
        self.platform_class = platform_class
        self.spec_version_minor = spec_version_minor
        self.spec_version_major = spec_version_major
        self.spec_errata = spec_errata
        self.uintn_size = uintn_size
        self.number_of_algorithms = number_of_algorithms
        self.digest_sizes = digest_sizes
        self.vendor_info_size = vendor_info_size
        self.vendor_info = vendor_info

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\n\tTCG_EFI_SPEC_ID_EVENT\n"
        debug_str += "\t\tSignature    = %s\n" % self.signature
        debug_str += "\t\tPlatform Class   = 0x%04X\n" % self.platform_class
        debug_str += (
            "\t\tSpec Version   = %02d.%02d\n" % self.spec_version_major,
            self.spec_version_minor,
        )
        debug_str += "\t\tSpec Errata   = %02d\n" % self.spec_errata
        debug_str += "\t\tUINTN Size   = 0x%04x\n" % self.uintn_size
        debug_str += "\t\tAlgorithm Count   = 0x%04x\n" % self.number_of_algorithms
        for digest_size in self.digest_sizes:
            debug_str += str(digest_size)
        debug_str += "\tTCG_EFI_SPEC_ID_EVENT (cont.)\n"
        debug_str += "\t\tVendor Info Size   = 0x%02x\n" % len(self.vendor_info_size)
        debug_str += "\t\tVendor Info:\n"
        debug_str += "\t\t0x%x\n" % self.vendor_info
        return debug_str

    def _get_size_total_digest_sizes(self) -> int:
        """Returns the total size of all digest size structures.

        Returns:
            int: The size in bytes.
        """
        return sum([s.get_size() for s in self.digest_sizes])

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        return (
            self._hdr_struct_size
            + self._get_size_total_digest_sizes()
            + self.vendor_info_size
        )

    def _get_encode_total_digest_sizes(self) -> bytes:
        """Returns the total encoding of all digest size structures.

        Returns:
            bytes: A byte representation of all digest size structures.
        """
        final_digest_size = bytearray()
        for size in self.digest_sizes:
            final_digest_size += size.encode()
        return final_digest_size

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        initial_header = struct.pack(
            self._hdr_struct_format_1,
            self.signature,
            self.platform_class,
            self.spec_version_major,
            self.spec_version_minor,
            self.spec_errata,
            self.uintn_size,
            self.number_of_algorithms,
        )
        final_header = struct.pack(self._hdr_struct_format_2, self.vendor_info_size)
        return (
            initial_header
            + self._get_encode_total_digest_sizes()
            + final_header
            + self.vendor_info
        )

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TcgEfiSpecIdEvent:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TcgEfiSpecIdEvent: A TcgEfiSpecIdEvent instance.
        """
        (
            signature,
            platform_class,
            spec_version_minor,
            spec_version_major,
            spec_errata,
            uintn_size,
            number_of_algorithms,
        ) = struct.unpack_from(TcgEfiSpecIdEvent._hdr_struct_format_1, binary_data)
        offset = struct.calcsize(TcgEfiSpecIdEvent._hdr_struct_format_1)

        digest_sizes = []
        for alg in range(number_of_algorithms):
            digest_sizes.append(
                TcgEfiSpecIdAlgorithmSize.from_binary(binary_data[offset:])
            )
            offset += digest_sizes[alg].get_size()

        vendor_info_size, *_ = struct.unpack_from(
            TcgEfiSpecIdEvent._hdr_struct_format_2, binary_data, offset
        )
        offset += struct.calcsize(TcgEfiSpecIdEvent._hdr_struct_format_2)
        vendor_info = binary_data[offset : offset + vendor_info_size]
        return cls(
            signature,
            platform_class,
            spec_version_minor,
            spec_version_major,
            spec_errata,
            uintn_size,
            number_of_algorithms,
            digest_sizes,
            vendor_info_size,
            vendor_info,
        )


class TcgPcrEvent(object):
    """TCG PCR Event structure."""

    # typedef struct tdTCG_PCR_EVENT {
    #   TCG_PCRINDEX     PCRIndex;
    #   TCG_EVENTTYPE    EventType;
    #   TCG_DIGEST       Digest;        // SHA1 Length = 160-bit (20 bytes)
    #   UINT32           EventSize;
    #   UINT8            Event[1];
    # } TCG_PCR_EVENT;
    _hdr_struct_format = "<II20sI"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(
        self,
        pcr_index: int = 0,
        event_type: int = 0,
        digest: bytes = None,
        event: TcgEfiSpecIdEvent = None,
    ):
        """Class constructor method.

        Args:
            pcr_index (int, optional): PCR index. Defaults to 0.
            event_type (int, optional): Event type. Defaults to 0.
            digest (bytes, optional): Event digest. Defaults to None.
            event (bytes, optional): Event data. Defaults to None.
        """
        self.pcr_index = pcr_index
        self.event_type = event_type
        self.digest = digest
        self.event = event

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\n\tTCG_PCR_EVENT\n"
        debug_str += "\t\tPCR Index    = 0x%04X\n" % self.pcr_index
        debug_str += "\t\tEvent Type   = 0x%04X\n" % self.event_type
        debug_str += "\t\nDigest: 0x%x\n" % self.digest.decode("utf-8")
        debug_str += "\t\tEvent Size   = 0x%08X\n" % len(self.event)
        debug_str += "\t\tEvent        =\n"
        debug_str += "\t\t%s\n" % str(self.event)
        return debug_str

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        return self._hdr_struct_size + self.event.get_size()

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        return (
            struct.pack(
                self._hdr_struct_format,
                self.pcr_index,
                self.event_type,
                self.digest,
                self.event.get_size(),
            )
            + self.event.encode()
        )

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TcgPcrEvent:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TcgPcrEvent: A TcgPcrEvent instance.
        """
        pcr_index, event_type, digest, _ = struct.unpack_from(
            TcgPcrEvent._hdr_struct_format, binary_data
        )
        event = TcgEfiSpecIdEvent.from_binary(
            binary_data[TcgPcrEvent._hdr_struct_size :]
        )
        return cls(pcr_index, event_type, digest, event)


class TcgPcrEvent2(object):
    # typedef struct tdTCG_PCR_EVENT2_HDR{
    #     TCG_PCRINDEX        PCRIndex
    #     TCG_EVENTTYPE       EventType
    #     TPML_DIGEST_VALUES  Digests
    #     UINT32              EventSize
    #     // Event Data immediately follows
    # } TCG_PCR_EVENT2_HDR
    _hdr_struct_format = "<II"
    _hdr_struct_size = struct.calcsize(_hdr_struct_format)

    def __init__(
        self,
        pcr_index: int = 0,
        event_type: int = 0,
        digest_values: TpmlDigestValues = None,
        algs: Iterable = None,
        event_data: bytes = b"",
    ):
        """Class constructor method.

        Args:
            pcr_index (int, optional): PCR index. Defaults to 0.
            event_type (int, optional): Event ID. Defaults to 0.
            digest_values (TpmlDigestValues, optional): Digest values. Defaults to None.
            algs (Iterable, optional): Algorithms supported. Defaults to None.
            event_data (bytes, optional): Event data. Defaults to b''.
        """
        self.pcr_index = pcr_index
        self.event_type = event_type
        self.digest_values = (
            TpmlDigestValues(algs) if digest_values is None else digest_values
        )
        self.event = event_data

    def __str__(self) -> str:
        """Returns a string of this object.

        Returns:
            str: The string representation of this object.
        """
        debug_str = "\n\tTCG_LOG_ENTRY\n"
        debug_str += "\t\tPCR Index    = 0x%04X\n" % self.pcr_index
        debug_str += "\t\tEvent Type   = 0x%04X\n" % self.event_type
        debug_str += str(self.digest_values)
        debug_str += "\tTCG_LOG_ENTRY (cont.)\n"
        debug_str += "\t\tEvent Size   = 0x%08X\n" % len(self.event)
        debug_str += "\t\tEvent        =\n"
        debug_str += "\t\t%s\n" % self.event
        return debug_str

    def get_size(self) -> int:
        """Returns the object size.

        Returns:
            int: The size in bytes.
        """
        result = self._hdr_struct_size
        result += self.digest_values.get_size()
        result += struct.calcsize("<I") + len(self.event)
        return result

    def encode(self) -> bytes:
        """Encodes the object.

        Returns:
            bytes: A byte representation of the object.
        """
        result = struct.pack(self._hdr_struct_format, self.pcr_index, self.event_type)
        result += self.digest_values.encode()
        result += struct.pack("<I", len(self.event)) + self.event
        return result

    @classmethod
    def from_binary(cls, binary_data: bytes) -> TcgPcrEvent2:
        """Returns a new instance from a object byte representation.

        Args:
            binary_data (bytes): Byte representation of the object.

        Returns:
            TcgPcrEvent2: A TcgPcrEvent2 instance.
        """
        pcr_index, event_type = struct.unpack_from(
            TcgPcrEvent2._hdr_struct_format, binary_data
        )

        offset = TcgPcrEvent2._hdr_struct_size
        digest_values = TpmlDigestValues.from_binary(binary_data[offset:])
        offset += digest_values.get_size()
        event_size, *_ = struct.unpack_from("<I", binary_data, offset)
        offset += struct.calcsize("<I")
        event_data = binary_data[offset : offset + event_size]
        return cls(pcr_index, event_type, digest_values, None, event_data)
