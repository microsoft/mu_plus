import tempfile
import unittest
from pathlib import Path

from Host.PrintEcitReport import extract_report
from Host.test_ecit_evidence import evidence_text


class PrintEcitReportTests(unittest.TestCase):
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
  <testsuite tests="16" failures="0" errors="0" skipped="0">
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
            "UNIT_TEST_SUMMARY tests=16 failures=0 errors=0 skipped=0",
        )
        self.assertEqual(report[1], "ECIT Secure Boot Audit Validation")
        self.assertNotIn("stale legacy table", report)

    def test_rejects_junit_without_correlation_test(self) -> None:
        junit = self._write_junit(
            """<?xml version="1.0"?>
<testsuites><testsuite><testcase classname="Other" /></testsuite></testsuites>
"""
        )

        with self.assertRaisesRegex(ValueError, "does not contain"):
            extract_report(junit)


if __name__ == "__main__":
    unittest.main()
