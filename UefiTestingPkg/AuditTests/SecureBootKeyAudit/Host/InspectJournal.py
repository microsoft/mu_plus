# @file
# Inspect the persistent Secure Boot audit journal.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##
"""Validate and print a SecureBootKeyAudit persistent journal."""

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

JOURNAL_FORMAT = "<IHHIIIQ32s32s32sBBHIIQIIQ"
JOURNAL_SIGNATURE = int.from_bytes(b"SBJR", "little")
JOURNAL_VERSION = 1
PHASE_NAMES = {
    1: "PREPARED",
    2: "REBOOT_PENDING",
    3: "PERSISTENTLY_VERIFIED",
    4: "FAILED",
}


def inspect_journal(journal_path: Path, manifest_path: Path) -> dict[str, object]:
    """Validate a journal and return its decoded evidence."""
    journal_data = journal_path.read_bytes()
    expected_size = struct.calcsize(JOURNAL_FORMAT)
    if len(journal_data) != expected_size:
        raise ValueError(
            f"journal size is {len(journal_data)}, expected {expected_size}"
        )

    fields = struct.unpack(JOURNAL_FORMAT, journal_data)
    (
        signature,
        version,
        record_size,
        stored_crc32,
        phase,
        boot_count,
        prior_result,
        fixture_id_raw,
        manifest_sha256,
        ecit_sha256,
        setup_mode,
        secure_boot,
        reserved,
        pk_state,
        pk_attributes,
        pk_size,
        db_state,
        db_attributes,
        db_size,
    ) = fields

    if signature != JOURNAL_SIGNATURE:
        raise ValueError(f"invalid journal signature: 0x{signature:08x}")
    if version != JOURNAL_VERSION:
        raise ValueError(f"unsupported journal version: {version}")
    if record_size != expected_size:
        raise ValueError(f"record size is {record_size}, expected {expected_size}")
    if phase not in PHASE_NAMES:
        raise ValueError(f"invalid journal phase: {phase}")
    if reserved != 0:
        raise ValueError("journal reserved field is nonzero")

    crc_input = bytearray(journal_data)
    crc_input[8:12] = b"\0" * 4
    calculated_crc32 = zlib.crc32(crc_input) & 0xFFFFFFFF
    if stored_crc32 != calculated_crc32:
        raise ValueError(
            f"journal CRC32 is 0x{stored_crc32:08x}, "
            f"expected 0x{calculated_crc32:08x}"
        )

    manifest_data = manifest_path.read_bytes()
    manifest = json.loads(manifest_data)
    fixture_id = fixture_id_raw.split(b"\0", 1)[0].decode("ascii")
    if fixture_id != manifest["fixture_id"]:
        raise ValueError(
            f"journal fixture ID {fixture_id!r} does not match manifest "
            f"{manifest['fixture_id']!r}"
        )

    calculated_manifest_sha256 = hashlib.sha256(manifest_data).digest()
    if manifest_sha256 != calculated_manifest_sha256:
        raise ValueError(
            "journal manifest SHA-256 does not match the supplied manifest"
        )

    return {
        "journal_version": version,
        "phase": PHASE_NAMES[phase],
        "boot_count": boot_count,
        "prior_result": f"0x{prior_result:016x}",
        "fixture_id": fixture_id,
        "manifest_sha256": manifest_sha256.hex(),
        "ecit_sha256": ecit_sha256.hex(),
        "setup_mode": setup_mode,
        "secure_boot": secure_boot,
        "pk": {
            "state": pk_state,
            "attributes": f"0x{pk_attributes:08x}",
            "size": pk_size,
        },
        "db": {
            "state": db_state,
            "attributes": f"0x{db_attributes:08x}",
            "size": db_size,
        },
        "crc32": f"0x{stored_crc32:08x}",
    }


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    script_dir = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("journal", type=Path, help="journal file to inspect")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=script_dir / "TestData.manifest.json",
        help="fixture manifest bound to the journal",
    )
    return parser.parse_args()


def main() -> None:
    """Validate and print the journal."""
    args = parse_args()
    evidence = inspect_journal(args.journal.resolve(), args.manifest.resolve())
    print(json.dumps(evidence, indent=2))


if __name__ == "__main__":
    main()
