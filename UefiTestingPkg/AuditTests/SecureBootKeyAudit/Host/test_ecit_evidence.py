import unittest

from Host.EcitEvidence import CLAIMS, TABLE_RULE, parse_evidence, render_evidence

FEATURE = "03092d2c-9a52-4c5c-8bf5-eaf04f45229d"
SIGNATURE = "1.2.840.113549.1.1.11"
DIGEST = "2.16.840.1.101.3.4.2.1"
MATCHING_CLAIMS = {
    "db-update-verification:rsa-sha256",
    "db-update-verification:sha256",
    "db-update-verification-profile:sha256-with-rsa",
}


def representative_records() -> list[str]:
    records = [
        "t=begin features=1 results=47 parse=0x0",
        f"t=feature i=1 id={FEATURE} kind=OIDS",
        f"t=ads f=1 k=OID v={SIGNATURE},{DIGEST}",
        "t=derived f=1 advertised=1 tested=1 not_tested=0",
        (
            "t=feature_summary f=1 advertised=3 verified=3 not_tested=0 "
            "limitations=0 mismatches=0"
        ),
        (
            "t=summary verdict=PASS advertised=3 verified=3 not_tested=0 "
            "unverifiable=0 over_reported=0 unadvertised_success=0 "
            "inconclusive=0 verified_absent=0"
        ),
    ]
    for name in CLAIMS:
        if name in MATCHING_CLAIMS:
            values = "c=ADVERTISED o=SUCCESS v=VERIFIED s=0x0"
        elif CLAIMS[name].feature == FEATURE:
            values = "c=ABSENT o=NOT_TESTED v=NOT_ADVERTISED s=0x8000000000000006"
        else:
            values = "c=UNKNOWN o=NOT_TESTED v=INCONCLUSIVE s=0x8000000000000006"
        records.append(f"t=result n={name} {values}")
    records.append("t=end features=1 results=47 records=54")
    return records


def evidence_text(records: list[str] | None = None) -> str:
    records = representative_records() if records is None else records
    return "\n".join(f"[INFO]        [ECIT_V1] {record}" for record in records)


class EcitEvidenceTests(unittest.TestCase):
    def test_accepts_firmware_order_and_counts_begin_and_end(self) -> None:
        records = representative_records()

        self.assertEqual(len(records), 54)
        self.assertEqual(records[0].split()[0], "t=begin")
        self.assertEqual(records[-1], "t=end features=1 results=47 records=54")
        self.assertLess(
            next(
                i for i, record in enumerate(records) if record.startswith("t=summary")
            ),
            next(
                i for i, record in enumerate(records) if record.startswith("t=result")
            ),
        )
        parse_evidence(evidence_text(records))

    def test_representative_rendering_and_counts(self) -> None:
        evidence = parse_evidence(evidence_text())

        self.assertEqual(len(evidence.results), 47)
        self.assertEqual(evidence.summary["advertised"], 3)
        self.assertEqual(
            render_evidence(evidence),
            [
                "ECIT Secure Boot Audit Validation",
                TABLE_RULE,
                "Feature: Secure Boot Database Update Verification",
                TABLE_RULE,
                (
                    " #  Claim                      Identifier                  "
                    "Observation  Coverage"
                ),
                (
                    " 1  sha256WithRSAEncryption    1.2.840.113549.1.1.11       "
                    "SUCCESS      VERIFIED                      "
                ),
                (
                    " 2  sha256                     2.16.840.1.101.3.4.2.1      "
                    "SUCCESS      VERIFIED                      "
                ),
                (
                    " 3  digest/signature profile   sha256 + sha256WithRSAEncryption "
                    "SUCCESS      VERIFIED                      "
                ),
                "    Derived profile summary: advertised=1 tested=1 not_tested=0",
                (
                    "    Feature summary: advertised=3 verified=3 not_tested=0 "
                    "limitations=0 mismatches=0"
                ),
                TABLE_RULE,
                (
                    "verdict=PASS advertised_claims=3 verified=3 not_tested=0 "
                    "unverifiable=0 over_reported=0 unadvertised_success=0 "
                    "inconclusive=0 verified_absent=0"
                ),
            ],
        )

    def test_serialized_evidence_fits_log_budget(self) -> None:
        serialized = evidence_text().encode("utf-8")

        self.assertLess(len(serialized), 14 * 1024)
        self.assertTrue(
            all(
                line.startswith(b"[INFO]        [ECIT_V1] ")
                for line in serialized.splitlines()
            )
        )

    def test_rejects_missing_end(self) -> None:
        records = representative_records()[:-1]

        with self.assertRaisesRegex(ValueError, "missing its final"):
            parse_evidence(evidence_text(records))

    def test_rejects_duplicate_key(self) -> None:
        records = representative_records()
        records[0] += " features=1"

        with self.assertRaisesRegex(ValueError, "duplicate key"):
            parse_evidence(evidence_text(records))

    def test_rejects_duplicate_feature(self) -> None:
        records = representative_records()
        records.insert(2, records[1])
        records[-1] = "t=end features=1 results=47 records=55"

        with self.assertRaisesRegex(ValueError, "duplicate or invalid feature"):
            parse_evidence(evidence_text(records))

    def test_rejects_duplicate_result(self) -> None:
        records = representative_records()
        result = next(record for record in records if "t=result" in record)
        records.insert(-1, result)
        records[-1] = "t=end features=1 results=47 records=55"

        with self.assertRaisesRegex(ValueError, "duplicate ECIT result"):
            parse_evidence(evidence_text(records))

    def test_rejects_unknown_record(self) -> None:
        records = representative_records()
        records.insert(-1, "t=future value=1")
        records[-1] = "t=end features=1 results=47 records=55"

        with self.assertRaisesRegex(ValueError, "unsupported ECIT_V1 record"):
            parse_evidence(evidence_text(records))

    def test_rejects_unknown_claim(self) -> None:
        records = representative_records()
        index = next(i for i, record in enumerate(records) if "t=result" in record)
        records[index] = records[index].replace(next(iter(CLAIMS)), "future-claim")

        with self.assertRaisesRegex(ValueError, "unknown ECIT claim"):
            parse_evidence(evidence_text(records))

    def test_rejects_feature_reference_before_definition(self) -> None:
        records = representative_records()
        records[1], records[2] = records[2], records[1]

        with self.assertRaisesRegex(ValueError, "references unknown feature"):
            parse_evidence(evidence_text(records))

    def test_rejects_enum_and_numeric_fields(self) -> None:
        replacements = (
            ("kind=OIDS", "kind=oids", "unsupported kind"),
            ("parse=0x0", "parse=zero", "hexadecimal status"),
            ("features=1", "features=-1", "unsigned decimal"),
            ("o=SUCCESS", "o=WORKED", "observation state"),
        )
        for old, new, message in replacements:
            with self.subTest(new=new):
                records = representative_records()
                index = next(i for i, record in enumerate(records) if old in record)
                records[index] = records[index].replace(old, new, 1)
                with self.assertRaisesRegex(ValueError, message):
                    parse_evidence(evidence_text(records))

    def test_rejects_record_count_mismatch(self) -> None:
        records = representative_records()
        records[-1] = "t=end features=1 results=47 records=53"

        with self.assertRaisesRegex(ValueError, "record count mismatch"):
            parse_evidence(evidence_text(records))

    def test_rejects_derived_control_mismatch(self) -> None:
        records = representative_records()
        records[3] = "t=derived f=1 advertised=2 tested=1 not_tested=1"

        with self.assertRaisesRegex(ValueError, "derived control mismatch"):
            parse_evidence(evidence_text(records))

    def test_rejects_feature_summary_mismatch(self) -> None:
        records = representative_records()
        records[4] = records[4].replace("verified=3", "verified=2")

        with self.assertRaisesRegex(ValueError, "feature 1 summary mismatch"):
            parse_evidence(evidence_text(records))

    def test_rejects_global_summary_mismatch(self) -> None:
        records = representative_records()
        records[5] = records[5].replace("advertised=3", "advertised=4")

        with self.assertRaisesRegex(ValueError, "global summary mismatch"):
            parse_evidence(evidence_text(records))


if __name__ == "__main__":
    unittest.main()
