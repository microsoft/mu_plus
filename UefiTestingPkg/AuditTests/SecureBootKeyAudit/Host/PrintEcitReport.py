#!/usr/bin/env python3
"""Print the human-readable ECIT report captured in a JUnit result."""

from __future__ import annotations

import argparse
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    from Host.EcitEvidence import MARKER, parse_evidence, render_evidence
except ModuleNotFoundError:
    from EcitEvidence import MARKER, parse_evidence, render_evidence

REPORT_MARKERS = (
    "[ECIT_SCENARIO_SUMMARY] ",
    "[ECIT_TABLE] ",
    "[ECIT_SUMMARY] ",
)


def extract_report(junit_path: Path) -> list[str]:
    root = ET.parse(junit_path).getroot()
    suite = next(root.iter("testsuite"), None)
    if suite is None:
        raise ValueError("JUnit result does not contain a testsuite")

    report = [
        (
            "UNIT_TEST_SUMMARY "
            f"tests={suite.get('tests', 'unknown')} "
            f"failures={suite.get('failures', 'unknown')} "
            f"errors={suite.get('errors', 'unknown')} "
            f"skipped={suite.get('skipped', 'unknown')}"
        )
    ]

    for testcase in root.iter("testcase"):
        if testcase.get("classname") != "EcitCorrelationReport":
            continue

        output = testcase.findtext("system-out") or ""
        if MARKER in output:
            return [report[0], *render_evidence(parse_evidence(output))]

        for line in output.splitlines():
            for marker in REPORT_MARKERS:
                if marker in line:
                    report.append(line.split(marker, 1)[1])
                    break

        if len(report) > 1:
            return report

        raise ValueError("EcitCorrelationReport contains no ECIT report records")

    raise ValueError("JUnit result does not contain EcitCorrelationReport")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Print the ECIT coverage table and summaries from JUnit XML."
    )
    parser.add_argument("junit", type=Path, help="SecureBootKeyAuditApp JUnit XML")
    args = parser.parse_args()

    try:
        report = extract_report(args.junit)
    except (ET.ParseError, OSError, ValueError) as error:
        parser.error(str(error))

    print("\n".join(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
