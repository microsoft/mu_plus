import copy
import hashlib
import io
import json
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zlib
from contextlib import redirect_stderr
from pathlib import Path
from unittest.mock import patch

from Host.AuditReport import extract_audit_run_identity, extract_report, generate_report, main
from Host.InspectJournal import JOURNAL_FORMAT, JOURNAL_SIGNATURE, JOURNAL_VERSION
from Host.test_ecit_evidence import (
    evidence_text,
    representative_results,
    v2_evidence_text,
)


class AuditReportTests(unittest.TestCase):
    def test_audit_run_requires_mode_from_version_012(self) -> None:
        identity = (
            "[AUDIT_RUN] app=SecureBootCryptoAuditApp version={version} arch=X64 "
            "firmware_vendor=EDK_II firmware_vendor_truncated=FALSE "
            "firmware_revision=0x00010000 uefi_revision=0x00020046 "
            "fixture=secure-boot-key-audit-v6 "
            "manifest_sha256="
            "b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db"
        )
        for version in ("0.10", "0.11", "0.12", "0.12.0", "0.13", "0.100", "1.0", "0.invalid"):
            for mode in ("", "CAPABILITY_GATED", "ROBUSTNESS_PROBE_ALL", "INVALID"):
                with self.subTest(version=version, mode=mode):
                    record = identity.format(version=version)
                    if mode:
                        record += f" mode={mode}"
                    root = ET.fromstring(f"<testcase><system-out>{record}</system-out></testcase>")
                    if version == "0.invalid":
                        with self.assertRaisesRegex(ValueError, "invalid AUDIT_RUN version"):
                            extract_audit_run_identity(root)
                    elif mode == "INVALID":
                        with self.assertRaisesRegex(ValueError, "invalid AUDIT_RUN mode"):
                            extract_audit_run_identity(root)
                    elif not mode and version not in {"0.10", "0.11"}:
                        with self.assertRaisesRegex(ValueError, "mode is required"):
                            extract_audit_run_identity(root)
                    else:
                        self.assertEqual(
                            extract_audit_run_identity(root),
                            record.replace("[AUDIT_RUN] ", "AUDIT_RUN ", 1),
                        )

    def _write_junit(self, contents: str) -> Path:
        temporary_directory = tempfile.TemporaryDirectory(dir=Path(__file__).parent)
        self.addCleanup(temporary_directory.cleanup)
        path = Path(temporary_directory.name) / "result.xml"
        path.write_text(contents, encoding="utf-8")
        return path

    def test_legacy_fallback_extracts_records_without_log_prefixes(self) -> None:
        junit = self._write_junit(
            """<?xml version="1.0"?>
<testsuites>
  <testsuite tests="1" failures="0" errors="0" skipped="0">
    <testcase classname="EcitCorrelationReport">
      <system-out>[INFO] [ECIT_SCENARIO_SUMMARY] rows=1 verified=1
[INFO] [ECIT_TABLE] ECIT Secure Boot Audit Validation
[INFO] [ECIT_TABLE]  1  X.509 certificate SUCCESS VERIFIED
[INFO] [ECIT_SUMMARY] verdict=PASS advertised_claims=1 verified=1
[INFO] unrelated output</system-out>
    </testcase>
  </testsuite>
</testsuites>
"""
        )

        self.assertEqual(
            extract_report(junit),
            [
                "UNIT_TEST_SUMMARY tests=1 failures=0 errors=0 skipped=0",
                "rows=1 verified=1",
                "ECIT Secure Boot Audit Validation",
                " 1  X.509 certificate SUCCESS VERIFIED",
                "verdict=PASS advertised_claims=1 verified=1",
            ],
        )

    def test_structured_evidence_takes_precedence_over_legacy_records(self) -> None:
        junit = self._write_junit(
            f"""<?xml version="1.0"?>
<testsuites>
  <testsuite tests="1" failures="0" errors="0" skipped="0">
    <testcase classname="EcitCorrelationReport">
      <system-out>{evidence_text()}
[INFO] [ECIT_TABLE] stale legacy table</system-out>
    </testcase>
  </testsuite>
</testsuites>
"""
        )

        report = extract_report(junit)
        self.assertEqual(
            report[0],
            "UNIT_TEST_SUMMARY tests=1 failures=0 errors=0 skipped=0",
        )
        self.assertEqual(report[1], "ECIT Secure Boot Audit Validation")
        self.assertNotIn("stale legacy table", report)

    def _v2_junit(self) -> Path:
        testcases = []
        for result in representative_results().values():
            if result.coverage in {
                "NOT_TESTED",
                "NOT_ADVERTISED",
                "NOT_INDEPENDENTLY_VERIFIABLE",
            }:
                outcome = "<skipped />"
            elif result.coverage in {"OVER_REPORTED", "INCONCLUSIVE"}:
                outcome = "<failure />"
            else:
                outcome = ""
            testcases.append(
                f'<testcase classname="EcitClaim" name="{result.name}">'
                f"{outcome}<system-out>[INFO] [ECIT_CLAIM] "
                f"name={result.name} claim={result.claim} "
                f"observation={result.observation} "
                f"validation={result.coverage} status=0x{result.status:x}"
                "</system-out></testcase>"
            )
        testcases.append(
            '<testcase classname="AuditRunIdentity">'
            "<system-out>[INFO] [AUDIT_RUN] "
            "app=SecureBootCryptoAuditApp version=0.10 arch=X64 "
            "firmware_vendor=EDK_II firmware_vendor_truncated=FALSE "
            "firmware_revision=0x00010000 uefi_revision=0x00020070 "
            "fixture=secure-boot-key-audit-v6 "
            "manifest_sha256="
            "b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db"
            "</system-out></testcase>"
        )
        testcases.append(
            '<testcase classname="EcitCorrelationReport">'
            f"<system-out>{v2_evidence_text()}</system-out></testcase>"
        )
        junit = self._write_junit(
            '<?xml version="1.0"?><testsuites><testsuite '
            'tests="49" failures="39" errors="0" skipped="5">'
            f"{''.join(testcases)}</testsuite></testsuites>"
        )
        return junit

    def test_v2_combines_native_claims_with_metadata(self) -> None:
        junit = self._v2_junit()
        report = extract_report(junit)

        self.assertEqual(
            report[0],
            "UNIT_TEST_SUMMARY tests=49 failures=39 errors=0 skipped=5",
        )
        self.assertEqual(
            report[1],
            "AUDIT_RUN app=SecureBootCryptoAuditApp version=0.10 arch=X64 "
            "firmware_vendor=EDK_II firmware_vendor_truncated=FALSE "
            "firmware_revision=0x00010000 uefi_revision=0x00020070 "
            "fixture=secure-boot-key-audit-v6 "
            "manifest_sha256="
            "b07ac802e30515d42b956e5fbfdd48f2974d63e6e36f21e7d06ac56313ff44db",
        )
        self.assertIn(
            "verdict=PASS advertised_claims=3 verified=3 not_tested=0 "
            "unverifiable=0 over_reported=0 unadvertised_success=0 "
            "inconclusive=0 verified_absent=0",
            report,
        )

    def test_markdown_uses_structured_evidence_and_reads_xml_once(self) -> None:
        junit = self._v2_junit()
        with patch(
            "Host.AuditReport.ET.parse", wraps=ET.parse
        ) as parse, patch(
            "Host.AuditReport.render_evidence", return_value=["changed text layout"]
        ) as render:
            report = generate_report(junit)
        parse.assert_called_once_with(junit)
        render.assert_not_called()
        self.assertIn("49 tests; 39 failures; 0 errors; 5 skipped", report)
        self.assertIn("3 verified of 3 advertised rows; 0 not tested", report)
        self.assertIn("| **Total** | **3** | **3** | **0** | **0** | **0** |", report)
        self.assertNotIn("223 advertised rows", report)
        self.assertNotIn("66 native tests", report)

    def test_rejects_malformed_audit_run_identity(self) -> None:
        junit = self._write_junit(
            """<?xml version="1.0"?>
<testsuites><testsuite tests="2" failures="0" errors="0" skipped="0">
  <testcase classname="AuditRunIdentity">
    <system-out>[INFO] [AUDIT_RUN] app=SecureBootCryptoAuditApp</system-out>
  </testcase>
  <testcase classname="EcitCorrelationReport">
    <system-out>[INFO] [ECIT_TABLE] legacy</system-out>
  </testcase>
</testsuite></testsuites>
"""
        )

        with self.assertRaisesRegex(ValueError, "invalid AUDIT_RUN fields"):
            extract_report(junit)

    def test_rejects_junit_without_correlation_test(self) -> None:
        junit = self._write_junit(
            """<?xml version="1.0"?>
<testsuites><testsuite tests="1" failures="0" errors="0" skipped="0"><testcase classname="Other" /></testsuite></testsuites>
"""
        )

        with self.assertRaisesRegex(ValueError, "does not contain"):
            extract_report(junit)

    def test_rejects_unchecked_outcomes_and_aggregates_suites(self) -> None:
        junit = self._write_junit(
            '<testsuites><testsuite tests="1" failures="0" errors="0" skipped="0">'
            '<testcase classname="EcitCorrelationReport"><system-out>'
            '[ECIT_TABLE] legacy</system-out></testcase></testsuite></testsuites>'
        )
        original = junit.read_text()
        for tag, field in (("failure", "failures"), ("error", "errors")):
            with self.subTest(tag=tag):
                root = ET.fromstring(original)
                suite = ET.SubElement(
                    root, "testsuite", tests="1", failures="0", errors="0", skipped="0"
                )
                testcase = ET.SubElement(suite, "testcase", classname="LaterOperation")
                ET.SubElement(testcase, tag)
                ET.ElementTree(root).write(junit)
                with self.assertRaisesRegex(ValueError, field):
                    extract_report(junit)

                suite.set(field, "1")
                ET.ElementTree(root).write(junit)
                totals = "failures=1 errors=0" if tag == "failure" else "failures=0 errors=1"
                self.assertEqual(
                    extract_report(junit)[0],
                    f"UNIT_TEST_SUMMARY tests=2 {totals} skipped=0",
                )


class MarkdownReportTests(unittest.TestCase):
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

    def test_profile_coverage_details_remain_in_report(self) -> None:
        junit = self._junit()
        root = ET.parse(junit).getroot()
        suite = root.find("testsuite")
        suite.set("tests", "4")
        suite.set("skipped", "2")
        testcase = ET.SubElement(
            suite, "testcase", classname="EcitProfileCoverage", name="Image verification"
        )
        ET.SubElement(testcase, "skipped")
        ET.SubElement(testcase, "system-out").text = (
            "advertised=42 tested=3 not_tested=39 mismatches=0"
        )
        ET.ElementTree(root).write(junit)
        report = generate_report(junit)
        self.assertIn(
            "3 of 42 advertised exact pairings were tested; 39 remain untested; "
            "0 mismatches.", report
        )

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
        output = junit.parent / "report.md"
        for document, message in (
            (evidence, "journal phase"),
            (None, "journal JSON must be an object"),
        ):
            with self.subTest(document=document):
                journal = self._write("journal.json", json.dumps(document))
                args = [
                    "AuditReport.py", str(junit), "--format", "markdown",
                    "--journal-json", str(journal), "--output", str(output),
                ]
                with patch("sys.argv", args), redirect_stderr(io.StringIO()) as stderr:
                    with self.assertRaises(SystemExit) as raised:
                        main()
                self.assertEqual(raised.exception.code, 2)
                self.assertIn(message, stderr.getvalue())
                self.assertFalse(output.exists())

    def test_cli_validates_binary_journal_before_writing_markdown(self) -> None:
        junit = self._junit()
        manifest_data = json.dumps({"fixture_id": "secure-boot-key-audit-v6"}).encode()
        manifest = junit.parent / "manifest.json"
        manifest.write_bytes(manifest_data)
        root = ET.parse(junit).getroot()
        identity = root.find(".//testcase/system-out")
        identity.text = identity.text.replace(
            self._journal()["manifest_sha256"],
            hashlib.sha256(manifest_data).hexdigest(),
        )
        ET.ElementTree(root).write(junit)
        values = [
            JOURNAL_SIGNATURE, JOURNAL_VERSION, struct.calcsize(JOURNAL_FORMAT),
            0, 3, 2, 0, b"secure-boot-key-audit-v6".ljust(32, b"\0"),
            hashlib.sha256(manifest_data).digest(), 1, 0, 0,
            1, 0, 0, 1, 0, 0, 1, 0, 0,
        ]
        data = bytearray(struct.pack(JOURNAL_FORMAT, *values))
        struct.pack_into("<I", data, 8, zlib.crc32(data) & 0xFFFFFFFF)
        journal = junit.parent / "journal.bin"
        journal.write_bytes(data)
        output = junit.parent / "report.md"
        args = [
            "AuditReport.py", str(junit), "--format", "markdown",
            "--journal", str(journal), "--manifest", str(manifest),
            "--output", str(output),
        ]
        with patch("sys.argv", args):
            self.assertEqual(main(), 0)
        self.assertIn("`PERSISTENTLY_VERIFIED` after 2 boots", output.read_text())

        data[-1] ^= 1
        journal.write_bytes(data)
        invalid_output = junit.parent / "invalid.md"
        args[-1] = str(invalid_output)
        with patch("sys.argv", args), redirect_stderr(io.StringIO()) as stderr:
            with self.assertRaises(SystemExit) as raised:
                main()
        self.assertEqual(raised.exception.code, 2)
        self.assertIn("CRC32", stderr.getvalue())
        self.assertFalse(invalid_output.exists())


if __name__ == "__main__":
    unittest.main()
