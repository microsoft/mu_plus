import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

from Host.PrintEcitReport import extract_audit_run_identity, extract_report
from Host.test_ecit_evidence import (
    evidence_text,
    representative_results,
    v2_evidence_text,
)


class PrintEcitReportTests(unittest.TestCase):
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

    def test_v2_combines_native_claims_with_metadata(self) -> None:
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


if __name__ == "__main__":
    unittest.main()
