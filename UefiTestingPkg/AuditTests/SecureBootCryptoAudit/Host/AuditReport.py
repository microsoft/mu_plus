#!/usr/bin/env python3
"""Validate Secure Boot audit evidence and report it as text or Markdown."""

from __future__ import annotations

import argparse
import json
import re
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path

try:
    from Host.EcitEvidence import (
        MARKER,
        V2_MARKER,
        FEATURES,
        Evidence,
        Result,
        parse_evidence,
        parse_evidence_v2,
        parse_native_claim,
        render_evidence,
    )
    from Host.InspectJournal import inspect_journal
except ModuleNotFoundError:
    from EcitEvidence import (
        MARKER,
        V2_MARKER,
        FEATURES,
        Evidence,
        Result,
        parse_evidence,
        parse_evidence_v2,
        parse_native_claim,
        render_evidence,
    )
    from InspectJournal import inspect_journal

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


def extract_audit_run_identity(root: ET.Element) -> str | None:
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

    if set(fields) - {"mode"} != set(AUDIT_RUN_FIELDS):
        missing = sorted(set(AUDIT_RUN_FIELDS) - set(fields))
        unknown = sorted(set(fields) - set(AUDIT_RUN_FIELDS) - {"mode"})
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

    if re.fullmatch(r"[0-9]+(?:\.[0-9]+)*", fields["version"]) is None:
        raise ValueError("invalid AUDIT_RUN version")
    version = tuple(int(part) for part in fields["version"].split("."))
    if version >= (0, 12) and "mode" not in fields:
        raise ValueError("AUDIT_RUN mode is required for version 0.12 and later")

    if "mode" in fields and fields["mode"] not in {
        "CAPABILITY_GATED", "ROBUSTNESS_PROBE_ALL"
    }:
        raise ValueError("invalid AUDIT_RUN mode")
    return "AUDIT_RUN " + " ".join(
        f"{key}={fields[key]}" for key in AUDIT_RUN_FIELDS
    ) + (f" mode={fields['mode']}" if "mode" in fields else "")


def extract_native_claim_results(root: ET.Element) -> dict[str, Result]:
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


def validated_junit_totals(root: ET.Element) -> dict[str, int]:
    suites = list(root.iter("testsuite"))
    if not suites:
        raise ValueError("JUnit result does not contain a testsuite")
    testcases = list(root.iter("testcase"))
    if not testcases:
        raise ValueError("JUnit result does not contain any testcases")
    if set(testcases) != {
        testcase for suite in suites for testcase in suite.iter("testcase")
    }:
        raise ValueError("JUnit testcase is outside a testsuite")
    for testcase in testcases:
        if sum(child.tag in {"failure", "error", "skipped"} for child in testcase) > 1:
            raise ValueError("JUnit testcase contains multiple outcome elements")

    def count(element: ET.Element) -> dict[str, int]:
        cases = list(element.iter("testcase"))
        return {
            "tests": len(cases),
            "failures": sum(case.find("failure") is not None for case in cases),
            "errors": sum(case.find("error") is not None for case in cases),
            "skipped": sum(case.find("skipped") is not None for case in cases),
        }

    for element in root.iter():
        if element.tag not in {"testsuite", "testsuites"}:
            continue
        totals = count(element)
        for field, expected in totals.items():
            declared = element.get(field)
            if declared is None and element.tag == "testsuites":
                continue
            if declared is None or re.fullmatch(r"[0-9]+", declared) is None:
                raise ValueError(f"JUnit {element.tag} has invalid {field} total")
            if int(declared) != expected:
                raise ValueError(
                    f"JUnit {element.tag} {field} total is {declared}, "
                    f"but testcase outcomes give {expected}"
                )
    return count(root)


@dataclass
class AuditResult:
    root: ET.Element
    totals: dict[str, int]
    identity: str | None
    claims: dict[str, Result]
    evidence: Evidence | None
    legacy_report: list[str]


def read_result(junit_path: Path) -> AuditResult:
    root = ET.parse(junit_path).getroot()
    totals = validated_junit_totals(root)
    audit_run = extract_audit_run_identity(root)
    claims = extract_native_claim_results(root)

    for testcase in root.iter("testcase"):
        if testcase.get("classname") != "EcitCorrelationReport":
            continue

        output = testcase.findtext("system-out") or ""
        if V2_MARKER in output:
            evidence = parse_evidence_v2(output, claims)
            return AuditResult(root, totals, audit_run, claims, evidence, [])
        if MARKER in output:
            evidence = parse_evidence(output)
            return AuditResult(root, totals, audit_run, claims, evidence, [])

        report = []
        for line in output.splitlines():
            for marker in REPORT_MARKERS:
                if marker in line:
                    report.append(line.split(marker, 1)[1])
                    break

        if report:
            return AuditResult(root, totals, audit_run, claims, None, report)

        raise ValueError("EcitCorrelationReport contains no ECIT report records")

    raise ValueError("JUnit result does not contain EcitCorrelationReport")


def render_text(result: AuditResult) -> list[str]:
    report = [
        "UNIT_TEST_SUMMARY " + " ".join(
            f"{field}={result.totals[field]}"
            for field in ("tests", "failures", "errors", "skipped")
        )
    ]
    if result.identity is not None:
        report.append(result.identity)
    report.extend(
        render_evidence(result.evidence)
        if result.evidence is not None else result.legacy_report
    )
    return report


def extract_report(junit_path: Path) -> list[str]:
    return render_text(read_result(junit_path))


def _run_fields(result: AuditResult) -> dict[str, str]:
    return (
        dict(token.split("=", 1) for token in result.identity.split()[1:])
        if result.identity is not None else {}
    )


def _validate_journal(journal: object, run_fields: dict[str, str]) -> None:
    if not isinstance(journal, dict):
        raise ValueError("journal JSON must be an object")
    if not run_fields:
        raise ValueError("journal requires a JUnit AUDIT_RUN identity")
    for field, identity_field in (
        ("fixture_id", "fixture"),
        ("manifest_sha256", "manifest_sha256"),
    ):
        if journal.get(field) != run_fields[identity_field]:
            raise ValueError(f"journal {field} does not match JUnit AUDIT_RUN")
    for field, expected in {
        "journal_version": 3,
        "phase": "PERSISTENTLY_VERIFIED",
        "boot_count": 2,
        "prior_result": "0x0000000000000000",
        "setup_mode": 1,
        "secure_boot": 0,
    }.items():
        value = journal.get(field)
        if type(value) is not type(expected) or value != expected:
            raise ValueError(f"journal {field} must be {expected!r}")
    for name in ("pk", "db", "dbx"):
        variable = journal.get(name)
        if not isinstance(variable, dict):
            raise ValueError(f"journal {name} must describe an absent variable")
        for field, expected in (
            ("state", 1), ("state_name", "ABSENT"),
            ("attributes", "0x00000000"), ("size", 0),
        ):
            value = variable.get(field)
            if type(value) is not type(expected) or value != expected:
                raise ValueError(f"journal {name}.{field} must be {expected!r}")


def _outcome(testcase: ET.Element) -> str:
    for tag, outcome in (("error", "ERROR"), ("failure", "FAIL"), ("skipped", "SKIP")):
        if testcase.find(tag) is not None:
            return outcome
    return "PASS"


def _escape(value: object) -> str:
    return str(value).replace("|", r"\|").replace("\n", "<br>")


def render_markdown(
    result: AuditResult,
    junit_path: Path,
    journal: dict[str, object] | None = None,
    journal_path: Path | None = None,
    restored_variable_store_sha256: str | None = None,
    target_name: str | None = None,
) -> str:
    totals = result.totals
    fields = _run_fields(result)
    if result.evidence is not None:
        summary = result.evidence.summary
    else:
        record = next(
            (line for line in result.legacy_report if line.startswith("verdict=")),
            "",
        )
        summary = dict(token.split("=", 1) for token in record.split() if "=" in token)
        if "advertised_claims" in summary:
            summary["advertised"] = summary["advertised_claims"]

    def value(field: str) -> object:
        return summary.get(field, "unknown")

    lines = [
        "# Secure Boot ECIT Audit Report", "",
        f"Generated from `{_escape(junit_path.name)}`.", "",
    ]
    if fields.get("mode") == "ROBUSTNESS_PROBE_ALL":
        lines.extend([
            "> **DESTRUCTIVE ROBUSTNESS EXPERIMENT - NOT A CONFORMANCE VERDICT.**",
            "> Results apply only to this firmware build, not deployed devices.", "",
        ])
    lines.extend([
        "## Executive summary", "",
        "| Item | Current result |", "| --- | --- |",
        f"| Native JUnit | {totals['tests']} tests; {totals['failures']} failures; "
        f"{totals['errors']} errors; {totals['skipped']} skipped |",
        f"| ECIT verdict | `{value('verdict')}` |",
        f"| ECIT coverage | {value('verified')} verified of {value('advertised')} "
        f"advertised rows; {value('not_tested')} not tested |",
        f"| Mismatches | {value('over_reported')} over-reported; "
        f"{value('inconclusive')} inconclusive |",
    ])
    if journal is not None:
        lines.extend([
            f"| Reboot journal | `{journal['phase']}` after {journal['boot_count']} boots |",
            "| Reboot-checkpoint Secure Boot state | SetupMode=1; SecureBoot=0; "
            "PK=ABSENT; db=ABSENT; dbx=ABSENT |",
        ])
    if restored_variable_store_sha256 is not None:
        lines.append(
            f"| Restored variable-store SHA-256 | `{restored_variable_store_sha256}` |"
        )
    failed = totals["failures"] != 0 or totals["errors"] != 0
    lines.extend([
        "", "## Plain-language conclusion", "",
        "The audit found one or more failed or incomplete operations."
        if failed else "All operational scenarios completed without a failure or error.",
        "A passing result does not establish complete ECIT coverage or whole-platform "
        "UEFI conformance. Skipped and untested capabilities are not counted as passes.",
        "", "## What this audit checks", "",
        "Real Secure Boot operations are compared with firmware's ECIT advertisements.",
        "", "## Result terminology", "",
        "PASS means the expected behavior was observed; FAIL or ERROR requires "
        "investigation; SKIP records an applicability or coverage boundary.",
        "", "## Secure Boot terms used below", "",
        "PK controls Secure Boot ownership, db authorizes images, and dbx revokes "
        "images or signers. See the audit README for the operation and recovery details.",
        "", "## Run identity", "", "| Field | Value |", "| --- | --- |",
    ])
    if target_name is not None:
        lines.append(f"| Test target | {_escape(target_name)} |")
    for field, entry in fields.items():
        lines.append(f"| {_escape(field)} | `{_escape(entry)}` |")
    if journal is not None:
        lines.extend([
            "", "The cold-reboot journal reached `PERSISTENTLY_VERIFIED` and verified "
            "the absent-variable baseline at the reboot checkpoint. It precedes later "
            "destructive scenarios and does not prove final cleanup or restoration "
            "of the original platform configuration.",
        ])
    if result.evidence is not None:
        lines.extend([
            "", "## ECIT coverage by feature", "",
            "Coverage rows include advertised digest/signature combinations, so they "
            "need not equal the number of native testcases.",
            "", "| Feature | Advertised | Verified | Not tested | Limitations | Over-reported |",
            "| --- | ---: | ---: | ---: | ---: | ---: |",
        ])
        for feature in result.evidence.features:
            name = FEATURES.get(feature.identifier, (feature.identifier, feature.kind))[0]
            control = feature.summary
            assert control is not None
            lines.append(
                f"| {_escape(name)} | " + " | ".join(
                    str(control[field]) for field in (
                        "advertised", "verified", "not_tested", "limitations", "mismatches"
                    )
                ) + " |"
            )
        lines.append(
            f"| **Total** | **{value('advertised')}** | **{value('verified')}** | "
            f"**{value('not_tested')}** | **{value('unverifiable')}** | "
            f"**{value('over_reported')}** |"
        )
    sections = (
        ("Operational and reporting tests", None),
        ("Aggregate profile-coverage tests", "EcitProfileCoverage"),
        ("Modeled ECIT claim tests", "EcitClaim"),
    )
    for heading, selected_class in sections:
        lines.extend([
            "", f"### {heading}", "",
            "| # | Class | Test | Result | Evidence |",
            "| ---: | --- | --- | :---: | --- |",
        ])
        for index, testcase in enumerate(result.root.iter("testcase"), 1):
            classname = testcase.get("classname", "unknown")
            if selected_class is None:
                if classname in {"EcitClaim", "EcitProfileCoverage"}:
                    continue
            elif classname != selected_class:
                continue
            name = testcase.get("name", "unnamed")
            finding = "See the authoritative JUnit diagnostics."
            if classname == "EcitClaim":
                claim = result.claims[name]
                finding = f"`{claim.claim}`; `{claim.observation}`; `{claim.coverage}`"
            elif classname == "EcitProfileCoverage":
                match = re.search(
                    r"advertised=(\d+) tested=(\d+) not_tested=(\d+) mismatches=(\d+)",
                    testcase.findtext("system-out") or "",
                )
                if match is not None:
                    advertised, tested, not_tested, mismatches = match.groups()
                    finding = (
                        f"{tested} of {advertised} advertised exact pairings were "
                        f"tested; {not_tested} remain untested; {mismatches} mismatches."
                    )
            elif classname == "ImageFormatSweep" and _outcome(testcase) == "PASS":
                counts = []
                for prefix, label in (
                    ("image-authorization:", "authorization"),
                    ("image-revocation:", "revocation"),
                ):
                    advertised = [
                        claim for name, claim in result.claims.items()
                        if name.startswith(prefix) and claim.claim == "ADVERTISED"
                    ]
                    verified = sum(claim.coverage == "VERIFIED" for claim in advertised)
                    counts.append(
                        f"{verified} of {len(advertised)} advertised modeled "
                        f"{label} claims were verified"
                    )
                finding = "; ".join(counts)
            lines.append(
                f"| {index} | `{_escape(classname)}` | `{_escape(name)}` | "
                f"**{_outcome(testcase)}** | {_escape(finding)} |"
            )
    lines.extend([
        "", "## Evidence files", "",
        f"- [`{_escape(junit_path.name)}`](./{junit_path.name}) - authoritative results.",
    ])
    if journal_path is not None:
        lines.append(
            f"- [`{_escape(journal_path.name)}`](./{journal_path.name}) - reboot journal."
        )
    return "\n".join([*lines, ""])


def generate_report(
    junit_path: Path,
    journal_path: Path | None = None,
    restored_variable_store_sha256: str | None = None,
    target_name: str | None = None,
) -> str:
    result = read_result(junit_path)
    journal = None
    if journal_path is not None:
        journal = json.loads(journal_path.read_text(encoding="utf-8"))
        _validate_journal(journal, _run_fields(result))
    return render_markdown(
        result, junit_path, journal, journal_path,
        restored_variable_store_sha256, target_name,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__
    )
    parser.add_argument("junit", type=Path, help="SecureBootCryptoAuditApp JUnit XML")
    parser.add_argument("--format", choices=("text", "markdown"), default="text")
    journal_args = parser.add_mutually_exclusive_group()
    journal_args.add_argument("--journal", type=Path, help="Binary reboot journal")
    journal_args.add_argument("--journal-json", type=Path, help="Previously decoded journal")
    parser.add_argument("--manifest", type=Path, help="Manifest for the binary journal")
    parser.add_argument("--restored-variable-store-sha256")
    parser.add_argument("--target-name")
    parser.add_argument("--output", type=Path, help="Write the report instead of stdout")
    args = parser.parse_args()

    try:
        if args.manifest is not None and args.journal is None:
            raise ValueError("--manifest requires --journal")
        result = read_result(args.junit)
        journal = None
        journal_path = args.journal or args.journal_json
        if args.journal is not None:
            manifest = args.manifest
            if manifest is None:
                arch = _run_fields(result).get("arch")
                if arch not in {"X64", "AARCH64"}:
                    raise ValueError("binary journal requires an X64/AARCH64 run or --manifest")
                manifest = Path(__file__).resolve().parent / (
                    "TestDataAArch64.manifest.json" if arch == "AARCH64"
                    else "TestData.manifest.json"
                )
            journal = inspect_journal(args.journal, manifest)
        elif args.journal_json is not None:
            journal = json.loads(args.journal_json.read_text(encoding="utf-8"))
        if journal_path is not None:
            _validate_journal(journal, _run_fields(result))
        if (
            args.restored_variable_store_sha256 is not None
            and re.fullmatch(r"[0-9a-fA-F]{64}", args.restored_variable_store_sha256) is None
        ):
            raise ValueError("restored variable-store SHA-256 must contain 64 hex digits")
        report = (
            render_markdown(
                result, args.junit, journal, journal_path,
                args.restored_variable_store_sha256, args.target_name,
            )
            if args.format == "markdown" else "\n".join(render_text(result)) + "\n"
        )
        if args.output is not None:
            args.output.write_text(report, encoding="utf-8")
        else:
            print(report, end="")
    except (ET.ParseError, OSError, ValueError) as error:
        parser.error(str(error))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
