#!/usr/bin/env python3
"""
coverage_to_kusto.py — Parse Cobertura XML and produce Kusto-ready coverage data.

Usage
-----
# Write JSON for later Kusto ingestion:
  python3 scripts/coverage_to_kusto.py \\
      --input out/coverage/Cobertura.xml \\
      --output out/coverage/coverage-kusto.json \\
      --repo "azure/iot-hub-device-update" \\
      --branch main --commit abc123 --build-id 12345

# Direct ingestion into Azure Data Explorer:
  python3 scripts/coverage_to_kusto.py \\
      --input out/coverage/Cobertura.xml \\
      --cluster "https://adukusto.westus2.kusto.windows.net" \\
      --database "DevMetrics" \\
      --table "CodeCoverage" \\
      --repo "azure/iot-hub-device-update" \\
      --branch main --commit abc123 --build-id 12345

# Fail CI when coverage drops below a threshold:
  python3 scripts/coverage_to_kusto.py \\
      --input out/coverage/Cobertura.xml \\
      --output out/coverage/coverage-kusto.json \\
      --min-coverage 60.0 \\
      --repo "azure/iot-hub-device-update" \\
      --branch main --commit abc123 --build-id 12345

The Cobertura XML is the format produced by gcovr (see scripts/run_coverage.sh).
Only files under ``src/`` are included; tests, cmake artifacts, and third-party
code are automatically excluded.

For direct Kusto ingestion the script authenticates with
``azure.identity.DefaultAzureCredential`` and streams the payload via the ADX
REST streaming-ingestion endpoint.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from datetime import datetime, timezone
from typing import Any


# ---------------------------------------------------------------------------
# Cobertura XML parsing
# ---------------------------------------------------------------------------

def _derive_module(filepath: str) -> str:
    """Return ``src/{first_component}`` from a file path, e.g. ``src/extension_sdk``."""
    parts = filepath.split("/")
    if len(parts) >= 2 and parts[0] == "src":
        return f"src/{parts[1]}"
    return "src/unknown"


def parse_cobertura(xml_path: str) -> list[dict[str, Any]]:
    """Parse a Cobertura XML file and return per-file coverage records.

    Only files whose path starts with ``src/`` are included.  Duplicate class
    entries for the same file are merged (highest coverage wins, counters are
    summed where appropriate).
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()

    # Accumulate per-file data; a file may appear in multiple <class> elements.
    files: dict[str, dict[str, Any]] = {}

    for package in root.findall(".//package"):
        for cls in package.findall(".//class"):
            filepath = cls.get("filename", "")
            if not filepath.startswith("src/"):
                continue

            # Lines
            lines_hit = 0
            lines_total = 0
            lines_elem = cls.find("lines")
            if lines_elem is not None:
                for line in lines_elem.findall("line"):
                    lines_total += 1
                    if int(line.get("hits", "0")) > 0:
                        lines_hit += 1

            # Branches (from line-level branch attributes)
            branches_hit = 0
            branches_total = 0
            if lines_elem is not None:
                for line in lines_elem.findall("line"):
                    if line.get("branch") == "true":
                        cond = line.get("condition-coverage", "")
                        # Format: "75% (3/4)"
                        if "(" in cond and "/" in cond:
                            inner = cond.split("(")[1].rstrip(")")
                            hit_s, total_s = inner.split("/")
                            branches_hit += int(hit_s)
                            branches_total += int(total_s)

            # Functions / methods
            funcs_hit = 0
            funcs_total = 0
            methods_elem = cls.find("methods")
            if methods_elem is not None:
                for method in methods_elem.findall("method"):
                    funcs_total += 1
                    if float(method.get("line-rate", "0")) > 0:
                        funcs_hit += 1

            if filepath in files:
                # Merge: sum counters
                prev = files[filepath]
                prev["LinesHit"] += lines_hit
                prev["LinesTotal"] += lines_total
                prev["BranchesHit"] += branches_hit
                prev["BranchesTotal"] += branches_total
                prev["FunctionsHit"] += funcs_hit
                prev["FunctionsTotal"] += funcs_total
            else:
                files[filepath] = {
                    "FilePath": filepath,
                    "Module": _derive_module(filepath),
                    "LinesHit": lines_hit,
                    "LinesTotal": lines_total,
                    "BranchesHit": branches_hit,
                    "BranchesTotal": branches_total,
                    "FunctionsHit": funcs_hit,
                    "FunctionsTotal": funcs_total,
                }

    # Compute CoveragePct
    for rec in files.values():
        total = rec["LinesTotal"]
        rec["CoveragePct"] = round(rec["LinesHit"] / total * 100, 2) if total > 0 else 0.0

    return sorted(files.values(), key=lambda r: r["FilePath"])


# ---------------------------------------------------------------------------
# Metadata enrichment
# ---------------------------------------------------------------------------

def enrich_records(
    records: list[dict[str, Any]],
    *,
    repo: str,
    branch: str,
    commit: str,
    build_id: str,
    timestamp: str | None = None,
) -> list[dict[str, Any]]:
    """Prepend CI metadata columns to each record."""
    ts = timestamp or datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    enriched = []
    for rec in records:
        row = {
            "Timestamp": ts,
            "Repo": repo,
            "Branch": branch,
            "CommitId": commit,
            "BuildId": build_id,
        }
        row.update(rec)
        enriched.append(row)
    return enriched


# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

def print_summary(records: list[dict[str, Any]], min_coverage: float | None) -> float:
    """Print a human-readable summary and return overall line-coverage %."""
    total_hit = sum(r["LinesHit"] for r in records)
    total_lines = sum(r["LinesTotal"] for r in records)
    overall_pct = round(total_hit / total_lines * 100, 2) if total_lines > 0 else 0.0

    modules: dict[str, dict[str, int]] = defaultdict(lambda: {"hit": 0, "total": 0, "files": 0})
    for r in records:
        m = modules[r["Module"]]
        m["hit"] += r["LinesHit"]
        m["total"] += r["LinesTotal"]
        m["files"] += 1

    print(f"\n{'='*60}")
    print(f"  Coverage Summary — {len(records)} files, {overall_pct}% line coverage")
    print(f"  Lines: {total_hit}/{total_lines}")
    print(f"{'='*60}")
    print(f"  {'Module':<35} {'Files':>5}  {'Lines':>12}  {'Cov%':>6}")
    print(f"  {'-'*35} {'-'*5}  {'-'*12}  {'-'*6}")
    for mod_name in sorted(modules):
        m = modules[mod_name]
        pct = round(m["hit"] / m["total"] * 100, 2) if m["total"] > 0 else 0.0
        print(f"  {mod_name:<35} {m['files']:>5}  {m['hit']:>5}/{m['total']:<5}  {pct:>6.2f}")
    print(f"{'='*60}\n")

    if min_coverage is not None:
        if overall_pct < min_coverage:
            print(f"FAIL: overall coverage {overall_pct}% is below threshold {min_coverage}%")
        else:
            print(f"PASS: overall coverage {overall_pct}% meets threshold {min_coverage}%")

    return overall_pct


# ---------------------------------------------------------------------------
# Kusto direct ingestion
# ---------------------------------------------------------------------------

def ingest_to_kusto(
    records: list[dict[str, Any]],
    *,
    cluster: str,
    database: str,
    table: str,
) -> None:
    """Stream *records* into ADX via the REST streaming-ingestion endpoint."""
    try:
        import requests  # noqa: F811
    except ImportError:
        print("ERROR: 'requests' package is required for direct Kusto ingestion.", file=sys.stderr)
        print("       Install it with: pip install requests", file=sys.stderr)
        sys.exit(1)

    try:
        from azure.identity import DefaultAzureCredential  # noqa: F811
    except ImportError:
        print("ERROR: 'azure-identity' package is required for direct Kusto ingestion.", file=sys.stderr)
        print("       Install it with: pip install azure-identity", file=sys.stderr)
        sys.exit(1)

    credential = DefaultAzureCredential()
    # ADX resource scope
    scope = cluster.rstrip("/") + "/.default"
    token = credential.get_token(scope)

    # Build newline-delimited JSON payload
    payload = "\n".join(json.dumps(r) for r in records)

    url = (
        f"{cluster.rstrip('/')}/v1/rest/ingest/{database}/{table}"
        "?streamFormat=JSON&mappingName=CoverageMappingV1"
    )
    headers = {
        "Authorization": f"Bearer {token.token}",
        "Content-Type": "application/json; charset=utf-8",
    }

    print(f"Ingesting {len(records)} records into {cluster} / {database}.{table} ...")
    resp = requests.post(url, headers=headers, data=payload.encode("utf-8"), timeout=60)

    if resp.status_code < 200 or resp.status_code >= 300:
        print(f"ERROR: Kusto ingestion failed (HTTP {resp.status_code})", file=sys.stderr)
        print(resp.text, file=sys.stderr)
        sys.exit(1)

    print(f"Kusto ingestion succeeded (HTTP {resp.status_code}).")


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Parse Cobertura XML coverage and produce Kusto-ready JSON or ingest directly.",
    )
    p.add_argument("--input", required=True, help="Path to Cobertura XML report")
    p.add_argument("--output", default=None, help="Path for output JSON file (default mode)")

    # Metadata
    p.add_argument("--repo", required=True, help="Repository slug, e.g. azure/iot-hub-device-update")
    p.add_argument("--branch", required=True, help="Git branch name")
    p.add_argument("--commit", required=True, help="Git commit SHA")
    p.add_argument("--build-id", required=True, help="CI build identifier")
    p.add_argument("--timestamp", default=None, help="ISO-8601 timestamp (default: now)")

    # Quality gate
    p.add_argument(
        "--min-coverage",
        type=float,
        default=None,
        help="Minimum overall line-coverage %%. Exit non-zero if not met.",
    )

    # Direct Kusto ingestion
    kusto = p.add_argument_group("Kusto direct ingestion")
    kusto.add_argument("--cluster", default=None, help="ADX cluster URL")
    kusto.add_argument("--database", default=None, help="ADX database name")
    kusto.add_argument("--table", default=None, help="ADX table name")

    return p


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    # Validate arguments
    kusto_args = [args.cluster, args.database, args.table]
    if any(kusto_args) and not all(kusto_args):
        parser.error("--cluster, --database, and --table must all be provided together")

    if not any(kusto_args) and args.output is None:
        parser.error("Either --output or --cluster/--database/--table must be provided")

    if not os.path.isfile(args.input):
        print(f"ERROR: input file not found: {args.input}", file=sys.stderr)
        return 1

    # Parse
    records = parse_cobertura(args.input)
    if not records:
        print("WARNING: no src/ files found in coverage report.", file=sys.stderr)

    # Enrich
    records = enrich_records(
        records,
        repo=args.repo,
        branch=args.branch,
        commit=args.commit,
        build_id=args.build_id,
        timestamp=args.timestamp,
    )

    # Summary
    overall_pct = print_summary(records, args.min_coverage)

    # Output JSON file
    if args.output:
        os.makedirs(os.path.dirname(args.output) or ".", exist_ok=True)
        with open(args.output, "w", encoding="utf-8") as fh:
            json.dump(records, fh, indent=2)
        print(f"Wrote {len(records)} records to {args.output}")

    # Direct Kusto ingestion
    if all(kusto_args):
        ingest_to_kusto(records, cluster=args.cluster, database=args.database, table=args.table)

    # Quality gate
    if args.min_coverage is not None and overall_pct < args.min_coverage:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
