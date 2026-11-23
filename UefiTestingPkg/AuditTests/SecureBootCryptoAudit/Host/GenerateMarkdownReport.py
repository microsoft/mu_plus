#!/usr/bin/env python3
"""Generate a Markdown report from SecureBootKeyAudit JUnit evidence."""

from __future__ import annotations

import argparse
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

try:
    from Host.PrintEcitReport import (
        extract_audit_run_identity,
        extract_native_claim_results,
        extract_report,
        validated_junit_totals,
    )
except ModuleNotFoundError:
    from PrintEcitReport import (
        extract_audit_run_identity,
        extract_native_claim_results,
        extract_report,
        validated_junit_totals,
    )

OPERATIONAL_DESCRIPTIONS = {
    "AuditRunIdentity": "Record non-sensitive platform, application, and fixture identity.",
    "EcitClaimsParse": "Parse and validate the platform ECIT before destructive testing.",
    "EcitCorrelationClassifier": (
        "Verify the internal mapping from claims and observations to coverage outcomes."
    ),
    "RecoveryStateClassifier": (
        "Reject unknown or unsafe Secure Boot states before variable mutation."
    ),
    "PersistentJournalReboot": (
        "Prove fixture, ECIT, framework, and baseline state persistence across a cold reboot."
    ),
    "InterruptedStateRecovery": (
        "Preserve interrupted-state evidence before returning a recognized state to baseline."
    ),
    "RsaPkRotation": "Enroll RSA PK A, rotate to RSA PK B, and return to Setup Mode.",
    "MldsaSetupEnrollment": (
        "Enroll an ML-DSA-87 PK directly from Setup Mode and cleanly remove it."
    ),
    "AuthenticatedVariableReplay": (
        "Require replayed authenticated db updates to fail atomically."
    ),
    "RsaToMldsaRotation": (
        "Rotate the update authority from RSA-2048 to ML-DSA-87 and restore baseline."
    ),
    "MldsaImageTbsHashV1": (
        "Authorize and execute an ML-DSA-87 image with a V1 SHA-512 TBS hash."
    ),
    "MldsaImageTbsHashV2": (
        "Authorize and execute an ML-DSA-87 image with a V2 SHA-512 TBS hash."
    ),
    "MultiSignatureRevocation": (
        "Continue with an alternate valid signer until every image signer is revoked."
    ),
    "CumulativeCaRetirement": (
        "Retire RSA, ECDSA, and ML-DSA image CAs cumulatively without breaking later CAs."
    ),
    "CapabilityAuthorityPath": (
        "Traverse every advertised authenticated-update authority and return to baseline."
    ),
    "ImageFormatSweep": (
        "Exercise authorization and revocation for every advertised signature-list format."
    ),
    "EcitCorrelationReport": (
        "Emit ECIT metadata for host correlation with authoritative native claim results."
    ),
}

CLAIM_DESCRIPTIONS = {
    "db-update-authorization": "Database update authorization",
    "db-update-verification": "Database update verification",
    "db-update-verification-profile": "Database update digest/signature profile",
    "image-authorization": "Image authorization signature-list format",
    "image-revocation": "Image revocation signature-list format",
    "image-verification": "Image signature or digest verification",
    "image-verification-profile": "Image verification digest/signature profile",
}

RUN_FIELD_LABELS = {
    "app": "Audit application",
    "version": "Audit application version",
    "arch": "CPU architecture",
    "firmware_vendor": "Firmware vendor",
    "firmware_revision": "Firmware revision",
    "uefi_revision": "UEFI system-table revision",
    "fixture": "Public test-fixture version",
    "manifest_sha256": "Test-fixture manifest SHA-256",
}

CLAIM_DETAIL_NAMES = {
    "x509": "X.509 certificate",
    "x509-v1": "V1 X.509 certificate",
    "x509-v2": "V2 X.509 certificate",
    "rsa-sha256": "RSA signature with SHA-256",
    "ecdsa-sha256": "ECDSA signature with SHA-256",
    "mldsa87": "ML-DSA-87 signature",
    "sha256": "SHA-256 digest",
    "sha512": "SHA-512 digest",
    "sha256-with-rsa": "SHA-256 digest paired with an RSA signature",
    "sha256-with-ecdsa": "SHA-256 digest paired with an ECDSA signature",
    "sha512-with-mldsa87": "SHA-512 digest paired with an ML-DSA-87 signature",
    "hash-v1-sha1": "V1 SHA-1 image hash",
    "hash-v1-sha256": "V1 SHA-256 image hash",
    "hash-v2-sha256": "V2 SHA-256 image hash",
    "hash-v1-sha384": "V1 SHA-384 image hash",
    "hash-v2-sha384": "V2 SHA-384 image hash",
    "hash-v1-sha512": "V1 SHA-512 image hash",
    "hash-v2-sha512": "V2 SHA-512 image hash",
    "tbs-v1-sha256": "V1 SHA-256 certificate TBS hash",
    "tbs-v2-sha256": "V2 SHA-256 certificate TBS hash",
    "tbs-v1-sha384": "V1 SHA-384 certificate TBS hash",
    "tbs-v2-sha384": "V2 SHA-384 certificate TBS hash",
    "tbs-v1-sha512": "V1 SHA-512 certificate TBS hash",
    "tbs-v2-sha512": "V2 SHA-512 certificate TBS hash",
}


def _outcome(testcase: ET.Element) -> str:
    if testcase.find("error") is not None:
        return "ERROR"
    if testcase.find("failure") is not None:
        return "FAIL"
    if testcase.find("skipped") is not None:
        return "SKIP"
    return "PASS"


def _escape(value: object) -> str:
    return str(value).replace("|", r"\|").replace("\n", "<br>")


def _field_map(record: str) -> dict[str, str]:
    return dict(token.split("=", 1) for token in record.split()[1:])


def _operational_description(classname: str, name: str) -> str:
    if classname == "AuthenticatedVariableReplay" and "PK" in name:
        return "Require replayed RSA-authenticated PK updates to fail atomically."
    return OPERATIONAL_DESCRIPTIONS.get(classname, name)


def _operational_finding(
    classname: str, name: str, outcome: str, output: str
) -> str:
    if outcome == "FAIL" or outcome == "ERROR":
        return "The testcase did not complete successfully; inspect its JUnit diagnostics."
    if outcome == "SKIP":
        return "The scenario was not applicable to the advertised platform capabilities."

    findings = {
        "AuditRunIdentity": "Run identity was captured and validated by the host parser.",
        "EcitClaimsParse": "The ECIT parsed successfully.",
        "EcitCorrelationClassifier": "All internal correlation classifier checks passed.",
        "RecoveryStateClassifier": "Safe and unsafe recovery states were classified as expected.",
        "PersistentJournalReboot": (
            "Boot 2 restored framework context, reread a valid ECIT, and matched baseline state."
        ),
        "InterruptedStateRecovery": (
            "Interrupted-state evidence was preserved before successful recovery to baseline."
        ),
        "RsaPkRotation": "RSA PK A rotated to PK B and cleanup restored Setup Mode.",
        "MldsaSetupEnrollment": (
            "ML-DSA-87 established Secure Boot and cleanup restored Setup Mode."
        ),
        "AuthenticatedVariableReplay": (
            "UEFI rejected the replay as a security violation without changing "
            f"{'PK' if 'PK' in name else 'db'}."
        ),
        "RsaToMldsaRotation": (
            "The RSA authority rotated to ML-DSA-87 and cleanup restored baseline."
        ),
        "MldsaImageTbsHashV1": (
            "The image was denied before enrollment and after tampering, then loaded and executed."
        ),
        "MldsaImageTbsHashV2": (
            "The image was denied before enrollment and after tampering, then loaded and executed."
        ),
        "MultiSignatureRevocation": (
            "Both signer orders worked while one signer remained valid and were denied after both revocations."
        ),
        "CumulativeCaRetirement": (
            "Each retired CA was denied while later CAs remained usable until final retirement."
        ),
        "CapabilityAuthorityPath": (
            "The advertised update authorities selected for the transition path completed it."
        ),
        "ImageFormatSweep": "Advertised format probes completed; see native claim results for coverage.",
        "EcitCorrelationReport": (
            "Metadata-only ECIT_V2 evidence was emitted for independent host reconstruction."
        ),
    }
    finding = findings.get(classname, "The testcase completed successfully.")

    if classname == "CapabilityAuthorityPath":
        match = re.search(r"Advertised update-algorithm mask: (0x[0-9a-fA-F]+)", output)
        if match is not None:
            finding = f"{finding} Advertised mask: `{match.group(1)}`."
    elif classname == "CumulativeCaRetirement":
        match = re.search(r"Advertised image-algorithm mask: (0x[0-9a-fA-F]+)", output)
        if match is not None:
            finding = f"{finding} Advertised mask: `{match.group(1)}`."

    return finding


def _claim_description(name: str) -> str:
    prefix, separator, detail = name.partition(":")
    category = CLAIM_DESCRIPTIONS.get(prefix, prefix.replace("-", " "))
    if not separator:
        return category
    detail_name = CLAIM_DETAIL_NAMES.get(detail, detail.replace("-", " "))
    return f"{category}: {detail_name}."


def _feature_name(feature: str) -> str:
    if re.fullmatch(r"[0-9a-fA-F-]{36}", feature) is not None:
        return f"Unrecognized opaque ECIT feature (`{feature}`)"
    return feature


def _uefi_revision(value: str) -> str:
    if re.fullmatch(r"0x[0-9a-fA-F]{8}", value) is None:
        return value
    revision = int(value, 16)
    return f"{revision >> 16}.{revision & 0xFFFF:02d} (`{value}`)"


def _feature_coverage(report: list[str]) -> list[dict[str, str]]:
    rows = []
    feature = None
    for line in report:
        if line.startswith("Feature: "):
            feature = line.removeprefix("Feature: ")
            continue
        if feature is None or "Feature summary: " not in line:
            continue
        values = dict(
            token.split("=", 1)
            for token in line.split("Feature summary: ", 1)[1].split()
            if "=" in token
        )
        rows.append({"feature": feature, **values})
        feature = None
    return rows


def _summary_value(report: list[str], prefix: str) -> str | None:
    return next((line for line in report if line.startswith(prefix)), None)


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
    expected_fields = {
        "journal_version": 3,
        "phase": "PERSISTENTLY_VERIFIED",
        "boot_count": 2,
        "prior_result": "0x0000000000000000",
        "setup_mode": 1,
        "secure_boot": 0,
    }
    for field, expected in expected_fields.items():
        value = journal.get(field)
        if type(value) is not type(expected) or value != expected:
            raise ValueError(f"journal {field} must be {expected!r}")
    for name in ("pk", "db", "dbx"):
        variable = journal.get(name)
        if not isinstance(variable, dict):
            raise ValueError(f"journal {name} must describe an absent variable")
        for field, expected in (
            ("state", 1),
            ("state_name", "ABSENT"),
            ("attributes", "0x00000000"),
            ("size", 0),
        ):
            value = variable.get(field)
            if type(value) is not type(expected) or value != expected:
                raise ValueError(f"journal {name}.{field} must be {expected!r}")


def generate_report(
    junit_path: Path,
    journal_path: Path | None = None,
    restored_variable_store_sha256: str | None = None,
    target_name: str | None = None,
) -> str:
    root = ET.parse(junit_path).getroot()
    totals = validated_junit_totals(root)

    rendered_report = extract_report(junit_path)
    audit_run = extract_audit_run_identity(root)
    native_claims = extract_native_claim_results(root)
    run_fields = _field_map(audit_run) if audit_run is not None else {}
    verdict = _summary_value(rendered_report, "verdict=") or "verdict=UNKNOWN"
    verdict_fields = dict(
        token.split("=", 1) for token in verdict.split() if "=" in token
    )
    coverage_rows = _feature_coverage(rendered_report)
    verified_profiles = sorted(
        name
        for name, result in native_claims.items()
        if "profile:" in name and result.coverage == "VERIFIED"
    )
    skipped_claims = sorted(
        name
        for name, result in native_claims.items()
        if result.coverage
        in {"NOT_TESTED", "NOT_ADVERTISED", "NOT_INDEPENDENTLY_VERIFIABLE"}
    )
    testcases = list(root.iter("testcase"))
    outcome_counts = {
        outcome: sum(_outcome(testcase) == outcome for testcase in testcases)
        for outcome in ("PASS", "SKIP", "FAIL", "ERROR")
    }
    skipped_profiles = sum(
        testcase.get("classname") == "EcitProfileCoverage"
        and _outcome(testcase) == "SKIP"
        for testcase in testcases
    )
    untested_profile_pairs = sum(
        int(match.group(1))
        for testcase in testcases
        if testcase.get("classname") == "EcitProfileCoverage"
        and (
            match := re.search(
                r"\bnot_tested=(\d+)",
                testcase.findtext("system-out") or "",
            )
        )
        is not None
    )
    opaque_features = [
        row for row in coverage_rows if _feature_name(row["feature"]) != row["feature"]
    ]

    journal = None
    if journal_path is not None:
        journal = json.loads(journal_path.read_text(encoding="utf-8"))
        _validate_journal(journal, run_fields)

    lines = [
        "# Secure Boot ECIT Audit Report",
        "",
        f"Generated from `{junit_path.name}`.",
        "",
        (
            "> This report covers the Secure Boot ECIT operations exercised by the "
            "audit. It does not establish whole-platform UEFI conformance."
        ),
        "",
        "## Executive summary",
        "",
        "| Item | Current result |",
        "| --- | --- |",
        (
            f"| Native JUnit | {totals['tests']} tests; "
            f"{totals['failures']} failures; "
            f"{totals['errors']} errors; "
            f"{totals['skipped']} skipped |"
        ),
        f"| ECIT verdict | `{verdict_fields.get('verdict', 'UNKNOWN')}` |",
        (
            f"| ECIT coverage | {verdict_fields.get('verified', 'unknown')} verified "
            f"of {verdict_fields.get('advertised_claims', 'unknown')} advertised rows; "
            f"{verdict_fields.get('not_tested', 'unknown')} not tested |"
        ),
        (
            f"| Mismatches | {verdict_fields.get('over_reported', 'unknown')} "
            f"over-reported; {verdict_fields.get('inconclusive', 'unknown')} "
            "inconclusive |"
        ),
    ]

    if run_fields.get("mode") == "ROBUSTNESS_PROBE_ALL":
        lines[1:1] = [
            "",
            "> **DESTRUCTIVE ROBUSTNESS EXPERIMENT — NOT A CONFORMANCE VERDICT.** "
            "All fixture-backed profiles were selected regardless of advertisement. "
            "Results apply only to this firmware build; they do not establish safety "
            "on deployed devices. Safety preconditions and recovery failures can still "
            "stop a scenario.",
        ]

    if journal is not None:
        lines.append(
            f"| Reboot journal | `{journal.get('phase', 'UNKNOWN')}` after "
            f"{journal.get('boot_count', 'unknown')} boots |"
        )
        lines.append(
            "| Reboot-checkpoint Secure Boot state | "
            f"SetupMode={journal.get('setup_mode', 'unknown')}; "
            f"SecureBoot={journal.get('secure_boot', 'unknown')}; "
            f"PK={journal.get('pk', {}).get('state_name', 'unknown')}; "
            f"db={journal.get('db', {}).get('state_name', 'unknown')}; "
            f"dbx={journal.get('dbx', {}).get('state_name', 'unknown')} |"
        )
    if restored_variable_store_sha256 is not None:
        lines.append(
            f"| Restored variable-store SHA-256 | `{restored_variable_store_sha256}` |"
        )

    lines.extend(
        [
            "",
            "## Plain-language conclusion",
            "",
        ]
    )
    if outcome_counts["FAIL"] == 0 and outcome_counts["ERROR"] == 0:
        lines.append(
            f"**Every operation that the audit actually performed behaved as "
            f"expected.** {outcome_counts['PASS']} tests passed, no test failed, "
            "and the evidence contains no internal error."
        )
    else:
        lines.append(
            "**The audit found one or more failed or incomplete operations.** "
            "Review the failed testcase rows and their JUnit diagnostics."
        )
    if (
        verdict_fields.get("verdict") == "PASS_WITH_LIMITATIONS"
        and outcome_counts["FAIL"] == 0
        and outcome_counts["ERROR"] == 0
    ):
        lines.extend(
            [
                "",
                ("`PASS_WITH_LIMITATIONS` does **not** mean a tested operation failed. "
                "It means that the operations which ran passed, but the platform "
                "advertised additional cryptographic combinations that this fixture "
                "did not exercise. The report must therefore not be read as complete "
                "coverage of every advertised ECIT combination."),
            ]
        )
    if outcome_counts["SKIP"] != 0:
        lines.extend(
            [
                "",
                (f"The {outcome_counts['SKIP']} skipped tests are expected coverage "
                f"results in this run: {len(skipped_claims)} modeled claims were not "
                f"advertised or not tested, and {skipped_profiles} aggregate profile "
                "tests skipped because advertised combinations remain untested. A "
                "skip records a coverage boundary; it is not silently counted as a pass."),
            ]
        )
    total_not_tested = int(verdict_fields.get("not_tested", "0"))
    if untested_profile_pairs != 0 and total_not_tested >= untested_profile_pairs:
        lines.extend(
            [
                "",
                (
                    f"Of the {total_not_tested} untested ECIT rows, "
                    f"{untested_profile_pairs} are exact digest/signature pairings "
                    "implied by advertised algorithm sets. The remaining "
                    f"{total_not_tested - untested_profile_pairs} are individual "
                    "advertised algorithms, formats, or opaque capabilities without "
                    "an exact observation in this fixture."
                ),
            ]
        )

    lines.extend(
        [
            "",
            "## What this audit checks",
            "",
            ("The **EFI Crypto Indicator Table (ECIT)** is firmware-provided data that "
            "advertises which cryptographic formats and algorithms a platform claims "
            "to support. This audit does not trust the table by itself. It performs "
            "real Secure Boot operations and compares the observed behavior with the "
            "advertised claims."),
            "",
            "The report has three complementary kinds of native tests:",
            "",
            ("1. **Operational tests** perform complete behaviors such as rotating the "
            "Platform Key, replaying an authenticated update, authorizing an image, "
            "revoking a signer, rebooting, and recovering the test state."),
            ("2. **Modeled ECIT claim tests** report whether each specifically modeled "
            "advertisement was observed successfully."),
            ("3. **Aggregate profile-coverage tests** compare the exact digest/signature "
            "pairings exercised by the fixture with every pairing implied by ECIT."),
            "",
            ("This is a focused Secure Boot cryptographic-capability audit. It does not "
            "test unrelated UEFI requirements and is not, by itself, a whole-platform "
            "UEFI conformance result."),
            "",
            "## Result terminology",
            "",
            "| Term | Meaning in this report |",
            "| --- | --- |",
            "| **PASS** | The testcase completed and produced its expected result. |",
            ("| **SKIP** | The testcase was not applicable or represents advertised "
            "coverage that remains untested. It is not counted as a pass. |"),
            ("| **FAIL** | Observed behavior contradicted the expected behavior or an "
            "advertised claim. |"),
            ("| `VERIFIED` | ECIT advertised the claim and the matching operation "
            "succeeded. |"),
            ("| `NOT_ADVERTISED` | The fixture models the claim, but this platform did "
            "not advertise it, so the probe was not run. |"),
            ("| `NOT_TESTED` | ECIT advertised the row, but this run made no exact "
            "matching observation. |"),
            ("| `PASS_WITH_LIMITATIONS` | No mismatch was found, but advertised rows "
            "remain untested or otherwise unverifiable. |"),
            "",
            "## Secure Boot terms used below",
            "",
            "| Term | Plain-language meaning |",
            "| --- | --- |",
            ("| **PK (Platform Key)** | The top-level authority that controls Secure "
            "Boot ownership and authenticated key-database updates. |"),
            ("| **db** | The allow database. A matching certificate or hash can "
            "authorize an image to load. |"),
            ("| **dbx** | The deny database. A matching certificate or hash revokes an "
            "image or signer even if it would otherwise be allowed. |"),
            ("| **Setup Mode** | The state with no enrolled PK. The audit requires "
            "`SetupMode=1` and `SecureBoot=0` as its clean baseline. |"),
            ("| **Authenticated update** | A signed update to PK, db, or dbx that must "
            "be authorized by the current update authority. |"),
            ("| **TBS hash** | A hash of the `to-be-signed` part of an X.509 "
            "certificate, used to identify a signer certificate. |"),
            ("| **Fixture** | The public certificates, signed images, authenticated "
            "updates, and expected metadata compiled into the audit application. |"),
            ("| **Profile** | One exact digest/signature pairing, such as SHA-256 with "
            "RSA. Advertising sets of digests and signatures implies many possible "
            "pairings. |"),
        ]
    )

    lines.extend(
        [
            "",
            "## Run identity",
            "",
            "| Field | Value |",
            "| --- | --- |",
        ]
    )
    if target_name is not None:
        lines.append(f"| Test target | {_escape(target_name)} |")
    for field in (
        "app",
        "version",
        "arch",
        "firmware_vendor",
        "firmware_revision",
        "uefi_revision",
        "fixture",
        "manifest_sha256",
    ):
        value = run_fields.get(field, "not recorded")
        if field == "uefi_revision":
            value = _uefi_revision(value)
            lines.append(f"| {RUN_FIELD_LABELS[field]} | {_escape(value)} |")
        else:
            lines.append(f"| {RUN_FIELD_LABELS[field]} | `{_escape(value)}` |")
    lines.extend(
        [
            "",
            (
                "These findings apply only to this target, firmware build, audit "
                "application, and fixture. They must not be generalized to every "
                "platform from the same firmware vendor."
            ),
            "",
            (
                "The UEFI system-table revision is the revision reported in the "
                "firmware table header; it identifies the runtime interface revision "
                "and does not, by itself, prove ECIT conformance or coverage."
            ),
        ]
    )

    lines.extend(
        [
            "",
            "## Current findings",
            "",
            (
                "- All operational scenarios completed without a failure or error."
                if outcome_counts["FAIL"] == 0 and outcome_counts["ERROR"] == 0
                else "- The run contains failed or incomplete tests; inspect their diagnostics."
            ),
        ]
    )
    if verified_profiles:
        profile_pairs = sorted(
            {
                CLAIM_DETAIL_NAMES.get(name.partition(":")[2], name.partition(":")[2])
                for name in verified_profiles
            }
        )
        lines.append(
            f"- {len(verified_profiles)} modeled exact digest/signature profile "
            "claims were advertised and successfully observed across database updates "
            "and image verification. The verified pairings were "
            + ", ".join(profile_pairs)
            + "."
        )
    for feature in (
        "Secure Boot Image Authorization",
        "Secure Boot Image Revocation",
    ):
        rows = [row for row in coverage_rows if row["feature"] == feature]
        if rows:
            lines.append(
                f"- {feature}: {sum(int(row['verified']) for row in rows)} of "
                f"{sum(int(row['advertised']) for row in rows)} advertised rows "
                "were verified."
            )
    if skipped_claims:
        lines.append(
            f"- {len(skipped_claims)} modeled claim tests correctly skipped because "
            "they were unadvertised or not tested: "
            + "; ".join(_claim_description(name).removesuffix(".") for name in skipped_claims)
            + "."
        )
    if opaque_features:
        lines.append(
            f"- ECIT contained {len(opaque_features)} unrecognized opaque feature. "
            "The audit preserved it in the denominator but could not interpret or "
            "exercise it, so it remains `NOT_TESTED`; this is a coverage limitation, "
            "not an observed failure."
        )
    if verdict_fields.get("verdict") == "PASS_WITH_LIMITATIONS":
        lines.append(
            "- The result is `PASS_WITH_LIMITATIONS`, not complete coverage: "
            "advertised ECIT rows remain untested or not independently verifiable."
        )
    lines.append(
        f"- Mismatch findings: {verdict_fields.get('over_reported', 'unknown')} "
        "over-reported, "
        f"{verdict_fields.get('inconclusive', 'unknown')} inconclusive, and "
        f"{verdict_fields.get('unadvertised_success', 'unknown')} "
        "unadvertised-success."
    )
    if journal is not None:
        lines.append(
            "- The cold-reboot journal reached `PERSISTENTLY_VERIFIED` and verified "
            "the absent PK, db, and dbx baseline at the reboot checkpoint. It "
            "precedes later destructive scenarios and does not prove final cleanup "
            "or restoration of the original platform configuration."
        )
    if restored_variable_store_sha256 is not None:
        lines.append(
            "- The complete QEMU variable store was restored to its pre-audit SHA-256."
        )

    if coverage_rows:
        lines.extend(
            [
                "",
                "## ECIT coverage by feature",
                "",
                ("An ECIT coverage row is an advertised capability expectation, not "
                "necessarily a separate native testcase. For features that advertise "
                "sets of digest and signature algorithms, the audit also counts every "
                "possible digest/signature pairing (the Cartesian product). This is "
                "why the 223 advertised rows are greater than the 66 native tests."),
                "",
                "| Feature | Advertised | Verified | Not tested | Limitations | Mismatches |",
                "| --- | ---: | ---: | ---: | ---: | ---: |",
            ]
        )
        for row in coverage_rows:
            lines.append(
                f"| {_escape(_feature_name(row['feature']))} | "
                f"{row.get('advertised', '?')} | "
                f"{row.get('verified', '?')} | {row.get('not_tested', '?')} | "
                f"{row.get('limitations', '?')} | {row.get('mismatches', '?')} |"
            )
        lines.append(
            f"| **Total** | **{verdict_fields.get('advertised_claims', '?')}** | "
            f"**{verdict_fields.get('verified', '?')}** | "
            f"**{verdict_fields.get('not_tested', '?')}** | "
            f"**{verdict_fields.get('unverifiable', '?')}** | "
            f"**{int(verdict_fields.get('over_reported', '0')) + int(verdict_fields.get('inconclusive', '0'))}** |"
        )

    lines.extend(
        [
            "",
            "## Native testcase results",
            "",
            ("The testcase number is its order in the authoritative JUnit result. The "
            "tables below contain every native testcase exactly once: operational "
            "behavior first, aggregate profile coverage second, and individual modeled "
            "ECIT claims last."),
            "",
            "### Operational and reporting tests",
            "",
            "| # | Class | Description | Result | Current finding |",
            "| ---: | --- | --- | :---: | --- |",
        ]
    )

    claim_rows = []
    profile_rows = []
    for index, testcase in enumerate(root.iter("testcase"), start=1):
        classname = testcase.get("classname", "unknown")
        name = testcase.get("name", "unnamed")
        outcome = _outcome(testcase)
        output = testcase.findtext("system-out") or ""
        if classname == "EcitClaim":
            result = native_claims[name]
            claim_rows.append((index, name, result, outcome))
            continue
        if classname == "EcitProfileCoverage":
            profile_rows.append((index, name, output, outcome))
            continue
        finding = _operational_finding(classname, name, outcome, output)
        if classname == "ImageFormatSweep" and outcome == "PASS":
            counts = []
            for prefix, label in (
                ("image-authorization:", "authorization"),
                ("image-revocation:", "revocation"),
            ):
                advertised = [
                    result for claim, result in native_claims.items()
                    if claim.startswith(prefix) and result.claim == "ADVERTISED"
                ]
                verified = sum(result.coverage == "VERIFIED" for result in advertised)
                counts.append(
                    f"{verified} of {len(advertised)} advertised modeled {label} "
                    "claims were verified"
                )
            finding = "; ".join(counts) + ". See individual native claim results."
        lines.append(
            f"| {index} | `{_escape(classname)}` | "
            f"{_escape(_operational_description(classname, name))} | "
            f"**{outcome}** | "
            f"{_escape(finding)} |"
        )

    lines.extend(
        [
            "",
            "### Aggregate profile-coverage tests",
            "",
            ("These tests ask whether the fixture exercised every exact "
            "digest/signature pairing implied by ECIT. A `SKIP` here means the tested "
            "pairings worked but additional advertised pairings remain untested."),
            "",
            "| # | Description | Result | Current finding |",
            "| ---: | --- | :---: | --- |",
        ]
    )
    for index, name, output, outcome in profile_rows:
        match = re.search(
            r"advertised=(\d+) tested=(\d+) not_tested=(\d+) mismatches=(\d+)",
            output,
        )
        finding = "Profile coverage diagnostics were not present."
        if match is not None:
            advertised, tested, not_tested, mismatches = match.groups()
            finding = (
                f"{tested} of {advertised} advertised exact pairings were tested; "
                f"{not_tested} remain untested; {mismatches} mismatches."
            )
        lines.append(
            f"| {index} | {_escape(name)} | **{outcome}** | {_escape(finding)} |"
        )

    lines.extend(
        [
            "",
            "### Modeled ECIT claim tests",
            "",
            ("Each row below is one claim that the audit knows how to correlate with an "
            "observation. **Claim** says whether ECIT advertised it, **Observation** "
            "records what the operation did, and **Coverage** is the audit's conclusion. "
            "The final **Result** maps that conclusion into native JUnit PASS/SKIP/FAIL."),
            "",
            "| # | Test | Description | Claim | Observation | Coverage | Result |",
            "| ---: | --- | --- | --- | --- | --- | :---: |",
        ]
    )
    for index, name, result, outcome in claim_rows:
        lines.append(
            f"| {index} | `{_escape(name)}` | {_escape(_claim_description(name))} | "
            f"`{result.claim}` | `{result.observation}` | `{result.coverage}` | "
            f"**{outcome}** |"
        )

    lines.extend(
        [
            "",
            "## Evidence files",
            "",
            (f"- [`{junit_path.name}`](./{junit_path.name}) - authoritative native "
            "testcase results and logs."),
        ]
    )
    if journal_path is not None:
        lines.append(
            f"- [`{journal_path.name}`](./{journal_path.name}) - decoded persistent "
            "reboot journal."
        )
    lines.append(
        "- [`ecit-report.txt`](./ecit-report.txt) - independently reconstructed ECIT "
        "coverage table."
    )
    lines.append(
        "- [`qemu-console.log`](./qemu-console.log) - complete emulator console "
        "transcript."
    )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate a Markdown Secure Boot ECIT audit report."
    )
    parser.add_argument("junit", type=Path, help="SecureBootKeyAudit JUnit XML")
    parser.add_argument(
        "--journal-json",
        type=Path,
        help="Optional JSON output from InspectJournal.py",
    )
    parser.add_argument(
        "--restored-variable-store-sha256",
        help="Optional independently verified restored variable-store hash",
    )
    parser.add_argument(
        "--target-name",
        help="Optional non-sensitive platform or lab-target label",
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Markdown report output path",
    )
    args = parser.parse_args()

    try:
        report = generate_report(
            args.junit.resolve(),
            args.journal_json.resolve() if args.journal_json is not None else None,
            args.restored_variable_store_sha256,
            args.target_name,
        )
        args.output.write_text(report, encoding="utf-8")
    except (ET.ParseError, OSError, ValueError) as error:
        parser.error(str(error))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
