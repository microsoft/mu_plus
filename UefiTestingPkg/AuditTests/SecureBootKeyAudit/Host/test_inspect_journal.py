# @file
# Tests for the Secure Boot audit journal inspector.
#
# Copyright (c) Microsoft Corporation.
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import hashlib
import json
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from InspectJournal import (
    JOURNAL_FORMAT,
    JOURNAL_SIGNATURE,
    JOURNAL_VERSION,
    inspect_journal,
)


class InspectJournalTests(unittest.TestCase):
    """Verify journal integrity and fixture binding."""

    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root = Path(self.temp_dir.name)
        self.manifest = self.root / "TestData.manifest.json"
        self.journal = self.root / "SecureBootKeyAuditJournal.bin"
        self.manifest_data = (
            json.dumps(
                {
                    "version": 3,
                    "fixture_id": "secure-boot-key-audit-v3",
                },
                indent=2,
            )
            + "\n"
        ).encode()
        self.manifest.write_bytes(self.manifest_data)
        self.journal.write_bytes(self._build_journal())

    def _build_journal(self) -> bytes:
        fixture_id = b"secure-boot-key-audit-v3".ljust(32, b"\0")
        values = [
            JOURNAL_SIGNATURE,
            JOURNAL_VERSION,
            struct.calcsize(JOURNAL_FORMAT),
            0,
            3,
            2,
            0,
            fixture_id,
            hashlib.sha256(self.manifest_data).digest(),
            bytes(range(32)),
            1,
            0,
            0,
            1,
            0,
            0,
            1,
            0,
            0,
            1,
            0,
            0,
        ]
        journal = bytearray(struct.pack(JOURNAL_FORMAT, *values))
        values[3] = zlib.crc32(journal) & 0xFFFFFFFF
        return struct.pack(JOURNAL_FORMAT, *values)

    def test_valid_journal_is_decoded(self) -> None:
        evidence = inspect_journal(self.journal, self.manifest)

        self.assertEqual(evidence["phase"], "PERSISTENTLY_VERIFIED")
        self.assertEqual(evidence["boot_count"], 2)
        self.assertEqual(evidence["fixture_id"], "secure-boot-key-audit-v3")

    def test_corrupt_journal_is_rejected(self) -> None:
        journal = bytearray(self.journal.read_bytes())
        journal[-1] ^= 0xFF
        self.journal.write_bytes(journal)

        with self.assertRaisesRegex(ValueError, "CRC32"):
            inspect_journal(self.journal, self.manifest)

    def test_wrong_manifest_is_rejected(self) -> None:
        self.manifest.write_text(
            '{"version": 3, "fixture_id": "different-fixture"}\n',
            encoding="utf-8",
        )

        with self.assertRaisesRegex(ValueError, "fixture ID"):
            inspect_journal(self.journal, self.manifest)


if __name__ == "__main__":
    unittest.main()
