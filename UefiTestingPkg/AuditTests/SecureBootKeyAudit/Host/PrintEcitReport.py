#!/usr/bin/env python3
"""Print the human-readable ECIT report captured in a JUnit result."""

from __future__ import annotations

import argparse
import re
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    from Host.EcitEvidence import (
        MARKER,
        V2_MARKER,
        Result,
        parse_evidence,
        parse_evidence_v2,
        parse_native_claim,
        render_evidence,
    )
except ModuleNotFoundError:
    from EcitEvidence import (
        MARKER,
        V2_MARKER,
        Result,
        parse_evidence,
        parse_evidence_v2,
        parse_native_claim,
        render_evidence,
    )

REPORT_MARKERS = (
    "[ECIT_SCENARIO_SUMMARY] ",
    "[ECIT_TABLE] ",
    "[ECIT_SUMMARY] ",
)
AUDIT_RUN_MARKER = "[AUDIT_RUN] "
AUDIT_RUN_FIELDS = (
    "app",
    "version",
    "arch",
    "firmware_vendor",
    "firmware_vendor_truncated",
    "firmware_revision",
    "uefi_revision",
    "fixture",
    "manifest_sha256",
)


def _audit_run_identity(root: ET.Element) -> str | None:
    records = []
    for system_out in root.iter("system-out"):
        for line in (system_out.text or "").splitlines():
            if AUDIT_RUN_MARKER in line:
                records.append(line.split(AUDIT_RUN_MARKER, 1)[1].strip())

    if not records:
        return None
    if len(records) != 1:
        raise ValueError("JUnit result must contain exactly one AUDIT_RUN record")

    fields = {}
    for token in records[0].split():
        if "=" not in token:
            raise ValueError(f"malformed AUDIT_RUN token: {token!r}")
        key, value = token.split("=", 1)
        if key in fields:
            raise ValueError(f"duplicate AUDIT_RUN field: {key!r}")
        fields[key] = value

    if set(fields) != set(AUDIT_RUN_FIELDS):
        missing = sorted(set(AUDIT_RUN_FIELDS) - set(fields))
        unknown = sorted(set(fields) - set(AUDIT_RUN_FIELDS))
        raise ValueError(
            f"invalid AUDIT_RUN fields: missing={missing!r} unknown={unknown!r}"
        )

    token_pattern = re.compile(r"[A-Za-z0-9._-]+")
    for key in ("app", "version", "arch", "firmware_vendor", "fixture"):
        if token_pattern.fullmatch(fields[key]) is None:
            raise ValueError(f"invalid AUDIT_RUN {key}: {fields[key]!r}")
    if fields["firmware_vendor_truncated"] not in {"TRUE", "FALSE"}:
        raise ValueError("invalid AUDIT_RUN firmware_vendor_truncated")
    for key in ("firmware_revision", "uefi_revision"):
        if re.fullmatch(r"0x[0-9A-Fa-f]{8}", fields[key]) is None:
            raise ValueError(f"invalid AUDIT_RUN {key}: {fields[key]!r}")
    if re.fullmatch(r"[0-9a-f]{64}", fields["manifest_sha256"]) is None:
        raise ValueError("invalid AUDIT_RUN manifest_sha256")

    return "AUDIT_RUN " + " ".join(
        f"{key}={fields[key]}" for key in AUDIT_RUN_FIELDS
    )


def _native_claim_results(root: ET.Element) -> dict[str, Result]:
    results = {}
    skipped_coverage = {
        "NOT_TESTED",
        "NOT_ADVERTISED",
        "NOT_INDEPENDENTLY_VERIFIABLE",
    }
    failed_coverage = {"OVER_REPORTED", "INCONCLUSIVE"}

    for testcase in root.iter("testcase"):
        if testcase.get("classname") != "EcitClaim":
            continue

        name = testcase.get("name")
        if not name:
            raise ValueError("EcitClaim testcase is missing its name")
        if name in results:
            raise ValueError(f"duplicate EcitClaim testcase: {name!r}")
        result = parse_native_claim(testcase.findtext("system-out") or "", name)
        skipped = testcase.find("skipped") is not None
        failed = (
            testcase.find("failure") is not None or testcase.find("error") is not None
        )

        if result.coverage in skipped_coverage and not skipped:
            raise ValueError(
                f"EcitClaim {name!r} coverage {result.coverage} must be skipped"
            )
        if result.coverage in failed_coverage and not failed:
            raise ValueError(
                f"EcitClaim {name!r} coverage {result.coverage} must fail"
            )
        if result.coverage in {"VERIFIED", "VERIFIED_ABSENT"} and (skipped or failed):
            raise ValueError(
                f"EcitClaim {name!r} coverage {result.coverage} must pass"
            )
        if result.coverage == "UNADVERTISED_SUCCESS" and skipped:
            raise ValueError(
                f"EcitClaim {name!r} UNADVERTISED_SUCCESS cannot be skipped"
            )

        results[name] = result

    return results


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
    audit_run = _audit_run_identity(root)
    if audit_run is not None:
        report.append(audit_run)

    for testcase in root.iter("testcase"):
        if testcase.get("classname") != "EcitCorrelationReport":
            continue

        output = testcase.findtext("system-out") or ""
        if V2_MARKER in output:
            results = _native_claim_results(root)
            return [
                *report,
                *render_evidence(parse_evidence_v2(output, results)),
            ]
        if MARKER in output:
            return [*report, *render_evidence(parse_evidence(output))]

        legacy_report_length = len(report)
        for line in output.splitlines():
            for marker in REPORT_MARKERS:
                if marker in line:
                    report.append(line.split(marker, 1)[1])
                    break

        if len(report) > legacy_report_length:
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
