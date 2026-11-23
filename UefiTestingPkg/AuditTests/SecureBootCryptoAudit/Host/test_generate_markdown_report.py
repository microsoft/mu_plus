import copy
import io
import json
import tempfile
import unittest
import xml.etree.ElementTree as ET
from contextlib import redirect_stderr
from pathlib import Path
from unittest.mock import patch

from Host.GenerateMarkdownReport import generate_report, main


class GenerateMarkdownReportTests(unittest.TestCase):
    def test_rejects_current_run_without_mode(self) -> None:
        junit = self._junit()
        junit.write_text(junit.read_text().replace("version=0.10", "version=0.12"))
        with self.assertRaisesRegex(ValueError, "mode is required"):
            generate_report(junit)

    def test_probe_all_report_is_not_a_conformance_verdict(self):
        junit = self._junit()
        original = junit.read_text().replace("version=0.10", "version=0.12")
        junit.write_text(original.replace(
            "manifest_sha256=b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db",
            "manifest_sha256=b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db mode=CAPABILITY_GATED",
        ))
        self.assertNotIn("DESTRUCTIVE ROBUSTNESS", generate_report(junit))
        junit.write_text(original.replace(
            "manifest_sha256=b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db",
            "manifest_sha256=b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db mode=ROBUSTNESS_PROBE_ALL",
        ))
        self.assertIn("NOT A CONFORMANCE VERDICT", generate_report(junit))
        junit.write_text(junit.read_text().replace("ROBUSTNESS_PROBE_ALL", "INVALID"))
        with self.assertRaisesRegex(ValueError, "invalid AUDIT_RUN mode"):
            generate_report(junit)

    def _write(self, name: str, contents: str) -> Path:
        temporary_directory = tempfile.TemporaryDirectory(dir=Path(__file__).parent)
        self.addCleanup(temporary_directory.cleanup)
        path = Path(temporary_directory.name) / name
        path.write_text(contents, encoding="utf-8")
        return path

    def _junit(self) -> Path:
        return self._write(
            "result.xml",
            """<?xml version="1.0"?>
<testsuites>
  <testsuite tests="3" failures="0" errors="0" skipped="1">
    <testcase classname="AuditRunIdentity" name="Report audit run identity">
      <system-out>[INFO] [AUDIT_RUN] app=SecureBootCryptoAuditApp version=0.10 arch=X64 firmware_vendor=EDK_II firmware_vendor_truncated=FALSE firmware_revision=0x00010000 uefi_revision=0x00020046 fixture=secure-boot-key-audit-v6 manifest_sha256=b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db</system-out>
    </testcase>
    <testcase classname="EcitClaim" name="image-authorization:hash-v1-sha1">
      <skipped />
      <system-out>[INFO] [ECIT_CLAIM] name=image-authorization:hash-v1-sha1 claim=ABSENT observation=NOT_TESTED validation=NOT_ADVERTISED status=0x8000000000000006</system-out>
    </testcase>
    <testcase classname="EcitCorrelationReport" name="Report correlation">
      <system-out>[INFO] [ECIT_SCENARIO_SUMMARY] rows=1 verified=0
[INFO] [ECIT_TABLE] ECIT Secure Boot Audit Validation
[INFO] [ECIT_SUMMARY] verdict=PASS_WITH_LIMITATIONS advertised_claims=1
verified=0 not_tested=1 unverifiable=0 over_reported=0
unadvertised_success=0 inconclusive=0 verified_absent=0</system-out>
    </testcase>
  </testsuite>
</testsuites>
""",
        )

    def _journal(self) -> dict[str, object]:
        return {
            "journal_version": 3,
            "phase": "PERSISTENTLY_VERIFIED",
            "boot_count": 2,
            "prior_result": "0x0000000000000000",
            "fixture_id": "secure-boot-key-audit-v6",
            "manifest_sha256": (
                "b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db"
            ),
            "setup_mode": 1,
            "secure_boot": 0,
            **{
                name: {
                    "state": 1,
                    "state_name": "ABSENT",
                    "attributes": "0x00000000",
                    "size": 0,
                }
                for name in ("pk", "db", "dbx")
            },
        }

    def test_generates_testcase_and_claim_tables(self) -> None:
        report = generate_report(self._junit(), target_name="QemuQ35")

        self.assertIn("# Secure Boot ECIT Audit Report", report)
        self.assertIn("3 tests; 0 failures; 0 errors; 1 skipped", report)
        self.assertIn("`PASS_WITH_LIMITATIONS`", report)
        self.assertIn("## Plain-language conclusion", report)
        self.assertIn("## What this audit checks", report)
        self.assertIn("## Result terminology", report)
        self.assertIn("## Secure Boot terms used below", report)
        self.assertIn("| Test target | QemuQ35 |", report)
        self.assertIn("### Operational and reporting tests", report)
        self.assertIn("### Modeled ECIT claim tests", report)
        self.assertIn("`image-authorization:hash-v1-sha1`", report)
        self.assertIn("`NOT_ADVERTISED`", report)
        self.assertNotIn("The cold-reboot journal reached", report)

    def test_valid_journal_reports_verified_reboot_checkpoint(self) -> None:
        journal = self._write("journal.json", json.dumps(self._journal()))

        report = generate_report(self._junit(), journal)

        self.assertIn("`PERSISTENTLY_VERIFIED` after 2 boots", report)
        self.assertIn("SetupMode=1; SecureBoot=0; PK=ABSENT; db=ABSENT; dbx=ABSENT", report)
        self.assertIn("The cold-reboot journal reached `PERSISTENTLY_VERIFIED`", report)
        self.assertIn("Reboot-checkpoint Secure Boot state", report)
        self.assertIn("does not prove final cleanup", report)
        self.assertNotIn("Final Secure Boot state", report)
        self.assertNotIn("application returned PK", report)

    def test_journal_does_not_claim_cleanup_after_later_failure(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        suite = root.find("testsuite")
        suite.set("tests", "4")
        suite.set("failures", "1")
        testcase = ET.SubElement(suite, "testcase", classname="ImageFormatSweep")
        ET.SubElement(testcase, "failure", message="Cleanup failed")
        ET.ElementTree(root).write(junit)
        journal = self._write("journal.json", json.dumps(self._journal()))

        report = generate_report(junit, journal)

        self.assertIn("one or more failed", report)
        self.assertIn("does not prove final cleanup", report)
        self.assertNotIn("Final Secure Boot state", report)
        self.assertNotIn("application returned PK", report)

    def test_format_sweep_counts_actual_advertised_modeled_claims(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        suite = root.find("testsuite")
        ET.SubElement(suite, "testcase", classname="ImageFormatSweep", name="Sweep")
        for name, observation, coverage in (
            ("image-authorization:hash-v1-sha256", "SUCCESS", "VERIFIED"),
            ("image-revocation:hash-v1-sha256", "SUCCESS", "VERIFIED"),
            ("image-revocation:hash-v1-sha384", "NOT_TESTED", "NOT_TESTED"),
        ):
            testcase = ET.SubElement(suite, "testcase", classname="EcitClaim", name=name)
            if coverage == "NOT_TESTED":
                ET.SubElement(testcase, "skipped")
            ET.SubElement(testcase, "system-out").text = (
                f"[ECIT_CLAIM] name={name} claim=ADVERTISED "
                f"observation={observation} validation={coverage} status=0x0"
            )
        suite.set("tests", "7")
        suite.set("skipped", "2")
        ET.ElementTree(root).write(junit)

        report = generate_report(junit)

        self.assertIn("1 of 1 advertised modeled authorization claims were verified", report)
        self.assertIn("1 of 2 advertised modeled revocation claims were verified", report)
        self.assertNotIn("All 14", report)

    def test_rejects_invalid_journal_invariants(self) -> None:
        junit = self._junit()
        invalid_values = {
            "journal_version": [2, "3", True],
            "phase": ["PREPARED", "REBOOT_PENDING", "FAILED", 3],
            "boot_count": [0, 1, 3, "2", True],
            "prior_result": ["0x8000000000000006", "0x0000000000000001", 0],
            "fixture_id": ["secure-boot-key-audit-v5"],
            "manifest_sha256": ["0" * 64],
            "setup_mode": [0, "1", True],
            "secure_boot": [1, "0", False],
        }
        for field, values in invalid_values.items():
            for value in [None, *values]:
                with self.subTest(field=field, value=value):
                    evidence = self._journal()
                    if value is None:
                        del evidence[field]
                    else:
                        evidence[field] = value
                    journal = self._write("journal.json", json.dumps(evidence))
                    with self.assertRaisesRegex(ValueError, field):
                        generate_report(junit, journal)

    def test_rejects_nonbaseline_or_inconsistent_variable_records(self) -> None:
        junit = self._junit()
        for name in ("pk", "db", "dbx"):
            for field, value in (
                ("state", 18),
                ("state", True),
                ("state_name", "UNKNOWN"),
                ("attributes", "0x00000027"),
                ("size", 1),
                ("size", False),
            ):
                with self.subTest(variable=name, field=field, value=value):
                    evidence = self._journal()
                    evidence[name][field] = value
                    journal = self._write("journal.json", json.dumps(evidence))
                    with self.assertRaisesRegex(ValueError, f"{name}.{field}"):
                        generate_report(junit, journal)
            for field in self._journal()[name]:
                with self.subTest(variable=name, missing=field):
                    evidence = self._journal()
                    del evidence[name][field]
                    journal = self._write("journal.json", json.dumps(evidence))
                    with self.assertRaisesRegex(ValueError, f"{name}.{field}"):
                        generate_report(junit, journal)
            with self.subTest(variable=name, missing=True):
                evidence = self._journal()
                del evidence[name]
                journal = self._write("journal.json", json.dumps(evidence))
                with self.assertRaisesRegex(ValueError, name):
                    generate_report(junit, journal)

    def test_rejects_malformed_journal_json(self) -> None:
        junit = self._junit()
        for contents in ("{", "[]", "null", '"not a journal"'):
            with self.subTest(contents=contents):
                journal = self._write("journal.json", contents)
                with self.assertRaises(ValueError):
                    generate_report(junit, journal)

    def test_journal_requires_audit_run_identity(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        root.find(".//testcase/system-out").text = ""
        ET.ElementTree(root).write(junit)
        journal = self._write("journal.json", json.dumps(self._journal()))

        with self.assertRaisesRegex(ValueError, "AUDIT_RUN"):
            generate_report(junit, journal)

    def test_rejects_incorrect_or_missing_junit_totals(self) -> None:
        junit = self._junit()
        original = ET.parse(junit).getroot()
        for field in ("tests", "failures", "errors", "skipped"):
            for value in (None, "99", "-1", "invalid"):
                with self.subTest(field=field, value=value):
                    root = copy.deepcopy(original)
                    suite = root.find("testsuite")
                    if value is None:
                        del suite.attrib[field]
                    else:
                        suite.set(field, value)
                    ET.ElementTree(root).write(junit)
                    with self.assertRaisesRegex(ValueError, field):
                        generate_report(junit)

    def test_failed_testcases_cannot_hide_behind_zero_totals(self) -> None:
        junit = self._junit()
        original = ET.parse(junit).getroot()
        for tag, field in (("failure", "failures"), ("error", "errors")):
            with self.subTest(tag=tag):
                root = copy.deepcopy(original)
                ET.SubElement(root.find(".//testcase"), tag)
                ET.ElementTree(root).write(junit)
                with self.assertRaisesRegex(ValueError, field):
                    generate_report(junit)

                root.find("testsuite").set(field, "1")
                ET.ElementTree(root).write(junit)
                report = generate_report(junit)
                self.assertIn("The audit found one or more failed", report)
                self.assertNotIn("All operational scenarios completed", report)
                self.assertNotIn("It means that the operations which ran passed", report)

    def test_rejects_conflicting_testcase_outcomes(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        ET.SubElement(root.find(".//testcase"), "failure")
        ET.SubElement(root.find(".//testcase"), "skipped")
        ET.ElementTree(root).write(junit)

        with self.assertRaisesRegex(ValueError, "multiple outcome"):
            generate_report(junit)

    def test_multiple_suites_are_validated_and_aggregated(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        suite = ET.SubElement(
            root, "testsuite", tests="1", failures="0", errors="0", skipped="0"
        )
        ET.SubElement(suite, "testcase", classname="Additional", name="Additional")
        ET.ElementTree(root).write(junit)
        self.assertIn("4 tests; 0 failures; 0 errors; 1 skipped", generate_report(junit))

        suite.set("tests", "0")
        ET.ElementTree(root).write(junit)
        with self.assertRaisesRegex(ValueError, "tests total"):
            generate_report(junit)

    def test_rejects_inconsistent_root_totals(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        root.set("tests", "0")
        ET.ElementTree(root).write(junit)

        with self.assertRaisesRegex(ValueError, "testsuites tests total"):
            generate_report(junit)

    def test_cli_does_not_write_report_on_invalid_evidence(self) -> None:
        junit = self._junit()
        evidence = self._journal()
        evidence["phase"] = "FAILED"
        journal = self._write("journal.json", json.dumps(evidence))
        output = junit.parent / "report.md"
        args = [
            "GenerateMarkdownReport.py", str(junit),
            "--journal-json", str(journal), "--output", str(output),
        ]

        with patch("sys.argv", args), redirect_stderr(io.StringIO()) as stderr:
            with self.assertRaises(SystemExit) as raised:
                main()

        self.assertEqual(raised.exception.code, 2)
        self.assertIn("journal phase", stderr.getvalue())
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
