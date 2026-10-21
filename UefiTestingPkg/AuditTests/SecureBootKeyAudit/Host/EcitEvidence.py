"""Strict parser and renderer for the ECIT_V1 JUnit evidence contract."""

from __future__ import annotations

import re
from dataclasses import dataclass
from enum import Enum
from uuid import UUID

MARKER = "[ECIT_V1]"
TABLE_RULE = "-" * 108

FEATURES = {
    "08324cfc-efe6-4211-a858-d4cac8915aef": (
        "Secure Boot Image Verification",
        "OIDS",
    ),
    "335f880f-180f-43d9-8ed9-ce584ed9b6f0": (
        "Secure Boot Image Authorization",
        "GUIDS",
    ),
    "02913331-2f71-43db-8277-7be88ecc651c": (
        "Secure Boot Image Revocation",
        "GUIDS",
    ),
    "03092d2c-9a52-4c5c-8bf5-eaf04f45229d": (
        "Secure Boot Database Update Verification",
        "OIDS",
    ),
    "304b3849-4906-40ea-8ade-751d6da7d4f9": (
        "Secure Boot Database Update Authorization",
        "GUIDS",
    ),
    "8417f337-8e42-4657-aeae-9b21a4b90258": (
        "System Firmware Update",
        "OIDS",
    ),
    "41c7bd17-6bd4-4df5-aaad-8987164ead4c": (
        "ESRT Device Firmware Update",
        "OPAQUE",
    ),
}

SIGNATURE_LIST_NAMES = {
    "a5c059a1-94e4-4aa7-87b5-ab155c2bf072": "X.509 certificate",
    "79518039-4715-4393-bdf8-d8e3c67fbac7": "X.509 certificate (V2)",
    "826ca512-cf10-4ac9-b187-be01496631bd": "SHA-1 image hash",
    "c1c41626-504c-4092-aca9-41f936934328": "SHA-256 image hash",
    "65325437-ca92-441e-943d-4e003faeda97": "SHA-256 image hash (V2)",
    "ff3e5307-9fd0-48c9-85f1-8ad56c701e01": "SHA-384 image hash",
    "45db3553-f9a6-4026-8e48-eb1e4b07185c": "SHA-384 image hash (V2)",
    "093e0fae-a6c4-4f50-9f1b-d41e2b89c19a": "SHA-512 image hash",
    "d2291d97-f066-4abf-9c57-6363e5fd6903": "SHA-512 image hash (V2)",
    "3bd2a492-96c0-4079-b420-fcf98ef103ed": "X.509 TBS hash (SHA-256)",
    "af4d21cd-7627-4b81-9e96-d4f3bbc3fae9": "X.509 TBS hash (SHA-256, V2)",
    "7076876e-80c2-4ee6-aad2-28b349a6865b": "X.509 TBS hash (SHA-384)",
    "78860e15-f2eb-4d16-8156-231518fb7688": "X.509 TBS hash (SHA-384, V2)",
    "446dbf63-2502-4cda-bcfa-2465d2b0fe9d": "X.509 TBS hash (SHA-512)",
    "4d46147a-fa79-43a9-8836-00341c708a70": "X.509 TBS hash (SHA-512, V2)",
}

OID_NAMES = {
    "1.2.840.113549.1.1.4": ("md5WithRSAEncryption", False),
    "1.2.840.113549.1.1.5": ("sha1WithRSAEncryption", False),
    "1.2.840.113549.1.1.11": ("sha256WithRSAEncryption", False),
    "1.2.840.113549.1.1.12": ("sha384WithRSAEncryption", False),
    "1.2.840.113549.1.1.13": ("sha512WithRSAEncryption", False),
    "1.2.840.113549.1.1.14": ("sha224WithRSAEncryption", False),
    "1.2.840.10045.4.1": ("ecdsa-with-SHA1", False),
    "1.2.840.10045.4.3.1": ("ecdsa-with-SHA224", False),
    "1.2.840.10045.4.3.2": ("ecdsa-with-SHA256", False),
    "1.2.840.10045.4.3.3": ("ecdsa-with-SHA384", False),
    "1.2.840.10045.4.3.4": ("ecdsa-with-SHA512", False),
    "2.16.840.1.101.3.4.3.17": ("id-ml-dsa-44", False),
    "2.16.840.1.101.3.4.3.18": ("id-ml-dsa-65", False),
    "2.16.840.1.101.3.4.3.19": ("id-ml-dsa-87", False),
    "1.3.14.3.2.26": ("sha1", True),
    "2.16.840.1.101.3.4.2.1": ("sha256", True),
    "2.16.840.1.101.3.4.2.2": ("sha384", True),
    "2.16.840.1.101.3.4.2.3": ("sha512", True),
    "2.16.840.1.101.3.4.2.4": ("sha224", True),
    "1.2.840.113549.2.5": ("md5", True),
    "1.2.156.10197.1.401": ("sm3", True),
}


class IdentifierKind(Enum):
    GUID = "GUID"
    OID = "OID"
    OID_PAIR = "OID_PAIR"


@dataclass(frozen=True)
class ClaimIdentity:
    feature: str
    kind: IdentifierKind
    identifier: str
    paired_identifier: str | None = None


IMAGE_VERIFICATION = "08324cfc-efe6-4211-a858-d4cac8915aef"
IMAGE_AUTHORIZATION = "335f880f-180f-43d9-8ed9-ce584ed9b6f0"
IMAGE_REVOCATION = "02913331-2f71-43db-8277-7be88ecc651c"
DB_VERIFICATION = "03092d2c-9a52-4c5c-8bf5-eaf04f45229d"
DB_AUTHORIZATION = "304b3849-4906-40ea-8ade-751d6da7d4f9"


def _oid(feature: str, identifier: str) -> ClaimIdentity:
    return ClaimIdentity(feature, IdentifierKind.OID, identifier)


def _pair(feature: str, signature: str, digest: str) -> ClaimIdentity:
    return ClaimIdentity(feature, IdentifierKind.OID_PAIR, signature, digest)


def _guid(feature: str, identifier: str) -> ClaimIdentity:
    return ClaimIdentity(feature, IdentifierKind.GUID, identifier)


CLAIMS = {
    "db-update-authorization:x509": _guid(
        DB_AUTHORIZATION, "a5c059a1-94e4-4aa7-87b5-ab155c2bf072"
    ),
    "db-update-verification:rsa-sha256": _oid(DB_VERIFICATION, "1.2.840.113549.1.1.11"),
    "db-update-verification:ecdsa-sha256": _oid(DB_VERIFICATION, "1.2.840.10045.4.3.2"),
    "db-update-verification:sha256": _oid(DB_VERIFICATION, "2.16.840.1.101.3.4.2.1"),
    "db-update-verification:mldsa87": _oid(DB_VERIFICATION, "2.16.840.1.101.3.4.3.19"),
    "db-update-verification:sha512": _oid(DB_VERIFICATION, "2.16.840.1.101.3.4.2.3"),
    "db-update-verification-profile:sha256-with-rsa": _pair(
        DB_VERIFICATION,
        "1.2.840.113549.1.1.11",
        "2.16.840.1.101.3.4.2.1",
    ),
    "db-update-verification-profile:sha256-with-ecdsa": _pair(
        DB_VERIFICATION, "1.2.840.10045.4.3.2", "2.16.840.1.101.3.4.2.1"
    ),
    "db-update-verification-profile:sha512-with-mldsa87": _pair(
        DB_VERIFICATION, "2.16.840.1.101.3.4.3.19", "2.16.840.1.101.3.4.2.3"
    ),
}

_GUID_CLAIMS = (
    ("x509-v1", "a5c059a1-94e4-4aa7-87b5-ab155c2bf072"),
    ("x509-v2", "79518039-4715-4393-bdf8-d8e3c67fbac7"),
    ("hash-v1-sha1", "826ca512-cf10-4ac9-b187-be01496631bd"),
    ("hash-v1-sha256", "c1c41626-504c-4092-aca9-41f936934328"),
    ("hash-v2-sha256", "65325437-ca92-441e-943d-4e003faeda97"),
    ("hash-v1-sha384", "ff3e5307-9fd0-48c9-85f1-8ad56c701e01"),
    ("hash-v2-sha384", "45db3553-f9a6-4026-8e48-eb1e4b07185c"),
    ("hash-v1-sha512", "093e0fae-a6c4-4f50-9f1b-d41e2b89c19a"),
    ("hash-v2-sha512", "d2291d97-f066-4abf-9c57-6363e5fd6903"),
    ("tbs-v1-sha256", "3bd2a492-96c0-4079-b420-fcf98ef103ed"),
    ("tbs-v2-sha256", "af4d21cd-7627-4b81-9e96-d4f3bbc3fae9"),
    ("tbs-v1-sha384", "7076876e-80c2-4ee6-aad2-28b349a6865b"),
    ("tbs-v2-sha384", "78860e15-f2eb-4d16-8156-231518fb7688"),
    ("tbs-v1-sha512", "446dbf63-2502-4cda-bcfa-2465d2b0fe9d"),
    ("tbs-v2-sha512", "4d46147a-fa79-43a9-8836-00341c708a70"),
)
for _prefix, _feature in (
    ("image-authorization", IMAGE_AUTHORIZATION),
    ("image-revocation", IMAGE_REVOCATION),
):
    CLAIMS.update(
        {
            f"{_prefix}:{suffix}": _guid(_feature, identifier)
            for suffix, identifier in _GUID_CLAIMS
        }
    )

CLAIMS.update(
    {
        "image-verification:rsa-sha256": _oid(
            IMAGE_VERIFICATION, "1.2.840.113549.1.1.11"
        ),
        "image-verification:ecdsa-sha256": _oid(
            IMAGE_VERIFICATION, "1.2.840.10045.4.3.2"
        ),
        "image-verification:sha256": _oid(IMAGE_VERIFICATION, "2.16.840.1.101.3.4.2.1"),
        "image-verification:mldsa87": _oid(
            IMAGE_VERIFICATION, "2.16.840.1.101.3.4.3.19"
        ),
        "image-verification:sha512": _oid(IMAGE_VERIFICATION, "2.16.840.1.101.3.4.2.3"),
        "image-verification-profile:sha256-with-rsa": _pair(
            IMAGE_VERIFICATION,
            "1.2.840.113549.1.1.11",
            "2.16.840.1.101.3.4.2.1",
        ),
        "image-verification-profile:sha256-with-ecdsa": _pair(
            IMAGE_VERIFICATION,
            "1.2.840.10045.4.3.2",
            "2.16.840.1.101.3.4.2.1",
        ),
        "image-verification-profile:sha512-with-mldsa87": _pair(
            IMAGE_VERIFICATION,
            "2.16.840.1.101.3.4.3.19",
            "2.16.840.1.101.3.4.2.3",
        ),
    }
)

CLAIM_STATES = {"UNKNOWN", "ABSENT", "ADVERTISED"}
OBSERVATION_STATES = {"NOT_TESTED", "SUCCESS", "FAILURE", "INCONCLUSIVE"}
COVERAGE_STATES = {
    "VERIFIED",
    "OVER_REPORTED",
    "UNADVERTISED_SUCCESS",
    "VERIFIED_ABSENT",
    "NOT_ADVERTISED",
    "NOT_INDEPENDENTLY_VERIFIABLE",
    "NOT_TESTED",
    "INCONCLUSIVE",
}
VERDICTS = {"PASS", "PASS_WITH_LIMITATIONS", "FAIL"}
UINT_RE = re.compile(r"(?:0|[1-9][0-9]*)\Z")
HEX_RE = re.compile(r"0x[0-9A-Fa-f]+\Z")
OID_RE = re.compile(r"[0-9]+(?:\.[0-9]+)+\Z")


@dataclass(frozen=True)
class Result:
    name: str
    claim: str
    observation: str
    coverage: str
    status: int


@dataclass
class Feature:
    index: int
    identifier: str
    kind: str
    advertised: list[str]
    derived: dict[str, int] | None = None
    summary: dict[str, int] | None = None


@dataclass(frozen=True)
class Row:
    claim: str
    identifier: str
    observation: str
    coverage: str


@dataclass
class Evidence:
    parse_status: int
    features: list[Feature]
    results: dict[str, Result]
    rows: dict[int, list[Row]]
    summary: dict[str, int | str]


SCHEMAS = {
    "begin": {"t", "features", "results", "parse"},
    "feature": {"t", "i", "id", "kind"},
    "ads": {"t", "f", "k", "v"},
    "result": {"t", "n", "c", "o", "v", "s"},
    "derived": {"t", "f", "advertised", "tested", "not_tested"},
    "feature_summary": {
        "t",
        "f",
        "advertised",
        "verified",
        "not_tested",
        "limitations",
        "mismatches",
    },
    "summary": {
        "t",
        "verdict",
        "advertised",
        "verified",
        "not_tested",
        "unverifiable",
        "over_reported",
        "unadvertised_success",
        "inconclusive",
        "verified_absent",
    },
    "end": {"t", "features", "results", "records"},
}


def _uint(value: str, field: str) -> int:
    if UINT_RE.fullmatch(value) is None:
        raise ValueError(f"{field} is not an unsigned decimal integer: {value!r}")
    return int(value)


def _hex(value: str, field: str) -> int:
    if HEX_RE.fullmatch(value) is None:
        raise ValueError(f"{field} is not a hexadecimal status: {value!r}")
    return int(value, 16)


def _guid(value: str, field: str) -> str:
    try:
        parsed = str(UUID(value))
    except ValueError as error:
        raise ValueError(f"{field} is not a GUID: {value!r}") from error
    if value.lower() != parsed:
        raise ValueError(f"{field} is not a canonical GUID: {value!r}")
    return parsed


def _records(output: str) -> list[dict[str, str]]:
    records = []
    for line_number, line in enumerate(output.splitlines(), 1):
        if MARKER not in line:
            continue
        payload = line.split(MARKER, 1)[1].strip()
        if not payload:
            raise ValueError(f"empty {MARKER} record on line {line_number}")
        record: dict[str, str] = {}
        for token in payload.split():
            if token.count("=") != 1:
                raise ValueError(f"malformed token on line {line_number}: {token!r}")
            key, value = token.split("=", 1)
            if not key or not value:
                raise ValueError(f"empty key or value on line {line_number}")
            if key in record:
                raise ValueError(f"duplicate key {key!r} on line {line_number}")
            record[key] = value
        record_type = record.get("t")
        if record_type not in SCHEMAS:
            raise ValueError(f"unsupported ECIT_V1 record type: {record_type!r}")
        if set(record) != SCHEMAS[record_type]:
            missing = sorted(SCHEMAS[record_type] - set(record))
            extra = sorted(set(record) - SCHEMAS[record_type])
            raise ValueError(
                f"invalid {record_type} schema; missing={missing}, extra={extra}"
            )
        records.append(record)
    return records


def _enum(value: str, choices: set[str], field: str) -> str:
    if value not in choices:
        raise ValueError(f"unsupported {field}: {value!r}")
    return value


def _control(record: dict[str, str], fields: tuple[str, ...]) -> dict[str, int]:
    return {field: _uint(record[field], field) for field in fields}


def _claim_lookup() -> dict[ClaimIdentity, str]:
    return {identity: name for name, identity in CLAIMS.items()}


def _derive(
    features: list[Feature], results: dict[str, Result]
) -> tuple[dict[int, list[Row]], dict[int, dict[str, int]], dict[str, int]]:
    lookup = _claim_lookup()
    rows: dict[int, list[Row]] = {}
    feature_totals: dict[int, dict[str, int]] = {}
    global_totals = {state: 0 for state in COVERAGE_STATES}
    global_totals["advertised"] = 0

    def add_row(
        feature: Feature,
        identity: ClaimIdentity | None,
        claim: str,
        identifier: str,
        *,
        render: bool = True,
    ) -> None:
        result = results.get(lookup.get(identity, "")) if identity is not None else None
        observation = result.observation if result is not None else "NOT_TESTED"
        coverage = result.coverage if result is not None else "NOT_TESTED"
        totals = feature_totals[feature.index]
        totals["advertised"] += 1
        totals[coverage] += 1
        global_totals["advertised"] += 1
        global_totals[coverage] += 1
        if render:
            rows[feature.index].append(Row(claim, identifier, observation, coverage))

    for feature in features:
        rows[feature.index] = []
        feature_totals[feature.index] = {state: 0 for state in COVERAGE_STATES}
        feature_totals[feature.index]["advertised"] = 0
        if feature.kind == "OIDS":
            for oid in feature.advertised:
                add_row(
                    feature,
                    ClaimIdentity(feature.identifier, IdentifierKind.OID, oid),
                    OID_NAMES.get(oid, (oid, False))[0],
                    oid,
                )
            signatures = [
                oid
                for oid in feature.advertised
                if oid in OID_NAMES and not OID_NAMES[oid][1]
            ]
            digests = [
                oid
                for oid in feature.advertised
                if oid in OID_NAMES and OID_NAMES[oid][1]
            ]
            tested = 0
            for signature in signatures:
                for digest in digests:
                    identity = ClaimIdentity(
                        feature.identifier,
                        IdentifierKind.OID_PAIR,
                        signature,
                        digest,
                    )
                    modeled = identity in lookup
                    if modeled:
                        tested += 1
                    add_row(
                        feature,
                        identity,
                        "digest/signature profile",
                        f"{OID_NAMES[digest][0]} + {OID_NAMES[signature][0]}",
                        render=modeled,
                    )
            expected = {
                "advertised": len(signatures) * len(digests),
                "tested": tested,
                "not_tested": len(signatures) * len(digests) - tested,
            }
            if feature.derived != expected:
                raise ValueError(
                    f"feature {feature.index} derived control mismatch: "
                    f"expected {expected}, got {feature.derived}"
                )
        elif feature.kind == "GUIDS":
            for identifier in feature.advertised:
                add_row(
                    feature,
                    ClaimIdentity(feature.identifier, IdentifierKind.GUID, identifier),
                    SIGNATURE_LIST_NAMES.get(identifier, "Unknown signature-list type"),
                    identifier,
                )
        else:
            add_row(
                feature,
                None,
                "Opaque feature payload",
                feature.identifier,
            )

    return rows, feature_totals, global_totals


def parse_evidence(output: str) -> Evidence:
    """Parse, cross-check, and derive an ECIT_V1 evidence stream."""
    records = _records(output)
    if not records:
        raise ValueError("EcitCorrelationReport contains no ECIT_V1 records")
    if records[0]["t"] != "begin":
        raise ValueError("ECIT_V1 evidence must begin with t=begin")
    if records[-1]["t"] != "end":
        raise ValueError("ECIT_V1 evidence is missing its final t=end")
    if any(record["t"] in {"begin", "end"} for record in records[1:-1]):
        raise ValueError("duplicate or misplaced begin/end record")

    begin, end = records[0], records[-1]
    expected_features = _uint(begin["features"], "begin features")
    expected_results = _uint(begin["results"], "begin results")
    parse_status = _hex(begin["parse"], "parse")
    if _uint(end["records"], "end records") != len(records):
        raise ValueError(
            f"record count mismatch: end says {end['records']}, parsed {len(records)}"
        )
    if _uint(end["features"], "end features") != expected_features:
        raise ValueError("begin/end feature count mismatch")
    if _uint(end["results"], "end results") != expected_results:
        raise ValueError("begin/end result count mismatch")

    features_by_index: dict[int, Feature] = {}
    results: dict[str, Result] = {}
    summary: dict[str, int | str] | None = None
    for record in records[1:-1]:
        record_type = record["t"]
        if record_type == "feature":
            index = _uint(record["i"], "feature index")
            if index == 0 or index in features_by_index:
                raise ValueError(f"duplicate or invalid feature index: {index}")
            identifier = _guid(record["id"], "feature id")
            kind = _enum(record["kind"], {"OIDS", "GUIDS", "OPAQUE"}, "kind")
            known = FEATURES.get(identifier)
            if known is not None and kind != known[1]:
                raise ValueError(
                    f"feature {index} kind {kind} disagrees with known kind {known[1]}"
                )
            features_by_index[index] = Feature(index, identifier, kind, [])
        elif record_type == "ads":
            index = _uint(record["f"], "ads feature")
            feature = features_by_index.get(index)
            if feature is None:
                raise ValueError(f"ads references unknown feature {index}")
            advertised_kind = _enum(record["k"], {"OID", "GUID"}, "ads kind")
            expected_kind = "OID" if feature.kind == "OIDS" else "GUID"
            if feature.kind == "OPAQUE" or advertised_kind != expected_kind:
                raise ValueError(f"ads kind is invalid for feature {index}")
            values = record["v"].split(",")
            if any(not value for value in values):
                raise ValueError(f"ads feature {index} contains an empty identifier")
            for value in values:
                normalized = (
                    _guid(value, "advertised GUID")
                    if advertised_kind == "GUID"
                    else value
                )
                if advertised_kind == "OID" and OID_RE.fullmatch(value) is None:
                    raise ValueError(f"invalid advertised OID: {value!r}")
                if normalized in feature.advertised:
                    raise ValueError(
                        f"duplicate advertised identifier for feature {index}: "
                        f"{normalized}"
                    )
                feature.advertised.append(normalized)
        elif record_type == "derived":
            index = _uint(record["f"], "derived feature")
            feature = features_by_index.get(index)
            if feature is None:
                raise ValueError(f"derived references unknown feature {index}")
            if feature.kind != "OIDS" or feature.derived is not None:
                raise ValueError(
                    f"duplicate or invalid derived record for feature {index}"
                )
            feature.derived = _control(record, ("advertised", "tested", "not_tested"))
        elif record_type == "feature_summary":
            index = _uint(record["f"], "summary feature")
            feature = features_by_index.get(index)
            if feature is None:
                raise ValueError(f"feature_summary references unknown feature {index}")
            if feature.summary is not None:
                raise ValueError(f"duplicate feature_summary for feature {index}")
            feature.summary = _control(
                record,
                (
                    "advertised",
                    "verified",
                    "not_tested",
                    "limitations",
                    "mismatches",
                ),
            )
        elif record_type == "result":
            name = record["n"]
            if name not in CLAIMS:
                raise ValueError(f"unknown ECIT claim name: {name!r}")
            if name in results:
                raise ValueError(f"duplicate ECIT result: {name!r}")
            results[name] = Result(
                name,
                _enum(record["c"], CLAIM_STATES, "claim state"),
                _enum(record["o"], OBSERVATION_STATES, "observation state"),
                _enum(record["v"], COVERAGE_STATES, "coverage state"),
                _hex(record["s"], "result status"),
            )
        elif record_type == "summary":
            if summary is not None:
                raise ValueError("duplicate global summary")
            summary = {
                "verdict": _enum(record["verdict"], VERDICTS, "verdict"),
                **_control(
                    record,
                    (
                        "advertised",
                        "verified",
                        "not_tested",
                        "unverifiable",
                        "over_reported",
                        "unadvertised_success",
                        "inconclusive",
                        "verified_absent",
                    ),
                ),
            }

    if sorted(features_by_index) != list(range(1, expected_features + 1)):
        raise ValueError("feature indexes are not the declared contiguous 1-based set")
    feature_identifiers = [feature.identifier for feature in features_by_index.values()]
    if len(set(feature_identifiers)) != len(feature_identifiers):
        raise ValueError("duplicate feature identifier")
    if len(results) != expected_results:
        raise ValueError(
            f"result count mismatch: begin says {expected_results}, parsed {len(results)}"
        )
    if expected_results != len(CLAIMS) or set(results) != set(CLAIMS):
        missing = sorted(set(CLAIMS) - set(results))
        raise ValueError(f"result set does not match claim registry; missing={missing}")
    if summary is None:
        raise ValueError("ECIT_V1 evidence is missing t=summary")

    features = [features_by_index[index] for index in sorted(features_by_index)]
    for feature in features:
        if feature.summary is None:
            raise ValueError(f"feature {feature.index} is missing feature_summary")
        if feature.kind == "OIDS" and feature.derived is None:
            raise ValueError(f"OID feature {feature.index} is missing derived control")
        if feature.kind != "OIDS" and feature.derived is not None:
            raise ValueError(f"non-OID feature {feature.index} has derived control")

    advertised_by_feature = {
        feature.identifier: set(feature.advertised) for feature in features
    }
    parse_error = bool(parse_status & (1 << 63))
    for name, result in results.items():
        identity = CLAIMS[name]
        advertised = advertised_by_feature.get(identity.feature)
        if advertised is None:
            expected_claim = "UNKNOWN"
        elif identity.kind == IdentifierKind.OID_PAIR:
            expected_claim = (
                "ADVERTISED"
                if identity.identifier in advertised
                and identity.paired_identifier in advertised
                else "ABSENT"
            )
        else:
            expected_claim = (
                "ADVERTISED" if identity.identifier in advertised else "ABSENT"
            )
        if result.claim != expected_claim:
            raise ValueError(
                f"result {name} claim state mismatch: "
                f"expected {expected_claim}, got {result.claim}"
            )

        if (
            parse_error
            or result.observation == "INCONCLUSIVE"
            or result.claim == "UNKNOWN"
        ):
            expected_coverage = "INCONCLUSIVE"
        elif result.claim == "ADVERTISED":
            if result.observation == "NOT_TESTED":
                expected_coverage = "NOT_TESTED"
            elif result.observation == "SUCCESS":
                expected_coverage = "VERIFIED"
            else:
                expected_coverage = "OVER_REPORTED"
        elif result.observation == "NOT_TESTED":
            expected_coverage = "NOT_ADVERTISED"
        elif result.observation == "SUCCESS":
            expected_coverage = "UNADVERTISED_SUCCESS"
        else:
            expected_coverage = "VERIFIED_ABSENT"
        if result.coverage != expected_coverage:
            raise ValueError(
                f"result {name} coverage mismatch: "
                f"expected {expected_coverage}, got {result.coverage}"
            )

    rows, feature_totals, global_totals = _derive(features, results)
    for feature in features:
        totals = feature_totals[feature.index]
        calculated = {
            "advertised": totals["advertised"],
            "verified": totals["VERIFIED"],
            "not_tested": totals["NOT_TESTED"],
            "limitations": totals["NOT_INDEPENDENTLY_VERIFIABLE"],
            "mismatches": totals["OVER_REPORTED"],
        }
        if feature.summary != calculated:
            raise ValueError(
                f"feature {feature.index} summary mismatch: "
                f"expected {calculated}, got {feature.summary}"
            )

    verdict = (
        "FAIL"
        if global_totals["OVER_REPORTED"] or global_totals["INCONCLUSIVE"]
        else (
            "PASS_WITH_LIMITATIONS"
            if global_totals["NOT_INDEPENDENTLY_VERIFIABLE"]
            or global_totals["NOT_TESTED"]
            else "PASS"
        )
    )
    calculated_summary: dict[str, int | str] = {
        "verdict": verdict,
        "advertised": global_totals["advertised"],
        "verified": global_totals["VERIFIED"],
        "not_tested": global_totals["NOT_TESTED"],
        "unverifiable": global_totals["NOT_INDEPENDENTLY_VERIFIABLE"],
        "over_reported": global_totals["OVER_REPORTED"],
        "unadvertised_success": global_totals["UNADVERTISED_SUCCESS"],
        "inconclusive": global_totals["INCONCLUSIVE"],
        "verified_absent": global_totals["VERIFIED_ABSENT"],
    }
    if summary != calculated_summary:
        raise ValueError(
            f"global summary mismatch: expected {calculated_summary}, got {summary}"
        )
    return Evidence(parse_status, features, results, rows, summary)


def render_evidence(evidence: Evidence) -> list[str]:
    """Render validated evidence in the former firmware table style."""
    output = ["ECIT Secure Boot Audit Validation"]
    row_number = 1
    for feature in evidence.features:
        name = FEATURES.get(feature.identifier, (feature.identifier, feature.kind))[0]
        output.extend(
            [
                TABLE_RULE,
                f"Feature: {name}",
                TABLE_RULE,
                (
                    " #  Claim                      Identifier                  "
                    "Observation  Coverage"
                ),
            ]
        )
        for row in evidence.rows[feature.index]:
            output.append(
                f"{row_number:2d}  {row.claim:<26} {row.identifier:<27} "
                f"{row.observation:<12} {row.coverage:<30}"
            )
            row_number += 1
        if feature.derived is not None:
            output.append(
                "    Derived profile summary: "
                f"advertised={feature.derived['advertised']} "
                f"tested={feature.derived['tested']} "
                f"not_tested={feature.derived['not_tested']}"
            )
        control = feature.summary
        assert control is not None
        output.append(
            "    Feature summary: "
            f"advertised={control['advertised']} "
            f"verified={control['verified']} "
            f"not_tested={control['not_tested']} "
            f"limitations={control['limitations']} "
            f"mismatches={control['mismatches']}"
        )
    output.extend(
        [
            TABLE_RULE,
            (
                f"verdict={evidence.summary['verdict']} "
                f"advertised_claims={evidence.summary['advertised']} "
                f"verified={evidence.summary['verified']} "
                f"not_tested={evidence.summary['not_tested']} "
                f"unverifiable={evidence.summary['unverifiable']} "
                f"over_reported={evidence.summary['over_reported']} "
                f"unadvertised_success={evidence.summary['unadvertised_success']} "
                f"inconclusive={evidence.summary['inconclusive']} "
                f"verified_absent={evidence.summary['verified_absent']}"
            ),
        ]
    )
    return output
