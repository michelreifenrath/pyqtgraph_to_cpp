#!/usr/bin/env python3
"""Generate a deterministic inventory of pinned PyQtGraph top-level classes."""

from __future__ import annotations

import argparse
import ast
import json
import subprocess
import sys
import tempfile
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any

import yaml

LOCK_PATH = Path("reference/source.lock")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")
GENERATED_MANIFEST_KEYS = ("classes", "excluded", "summary")
CLASS_SUMMARY_KEYS = (
    "class_count",
    "source_file_count",
    "excluded_example_count",
    "excluded_test_count",
)
TARGET_PATH_KEYS = ("target_header_path", "target_source_path")
STATUS_BY_PRESENT_COUNT = {
    "all": "ported",
    "some": "partial",
    "none": "not_started",
}
COMPLETION_EVIDENCE_KEY = "completion_evidence"
COMPLETION_EVIDENCE_TYPES = {
    "focused_test",
    "numeric_oracle",
    "visual_proof",
    "interaction_proof",
    "completion_report",
    "approved_equivalence",
}


class InventoryError(RuntimeError):
    """Raised when inventory generation cannot be completed safely."""


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a deterministic PyQtGraph class inventory."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="repository root containing reference/source.lock",
    )
    parser.add_argument(
        "--format",
        choices=("yaml", "json"),
        default="yaml",
        help="inventory serialization format for stdout",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check",
        action="store_true",
        help="validate inventory generation without writing persistent files",
    )
    mode.add_argument(
        "--update-manifest",
        action="store_true",
        help="update only port_manifest.yaml with generated class inventory sections",
    )
    return parser.parse_args(argv)


def load_lock(root: Path) -> dict[str, str]:
    lock_file = root / LOCK_PATH
    if not lock_file.is_file():
        raise InventoryError(f"missing reference lock: {LOCK_PATH.as_posix()}")

    data = yaml.safe_load(lock_file.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise InventoryError(f"invalid reference lock: {LOCK_PATH.as_posix()}")

    missing = [key for key in REQUIRED_LOCK_KEYS if not data.get(key)]
    if missing:
        raise InventoryError(
            "reference lock is missing required field(s): " + ", ".join(missing)
        )

    return {key: str(data[key]) for key in REQUIRED_LOCK_KEYS}


def run_git(args: list[str], *, cwd: Path | None = None) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd,
        text=True,
        capture_output=True,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or "git command failed"
        raise InventoryError(f"git {' '.join(args)} failed: {detail}")
    return result.stdout.strip()


def git_rev_parse_head(checkout: Path) -> str:
    try:
        return run_git(["rev-parse", "HEAD"], cwd=checkout)
    except InventoryError as exc:
        raise InventoryError(
            f"checkout {checkout.as_posix()} is not a git repository: {exc}"
        ) from exc


def require_pinned_commit(checkout: Path, lock: dict[str, str]) -> None:
    actual_commit = git_rev_parse_head(checkout)
    expected_commit = lock["pinned_commit"]
    if actual_commit != expected_commit:
        raise InventoryError(
            "checkout pinned_commit mismatch: "
            f"reference/source.lock has {expected_commit}, checkout has {actual_commit}"
        )


def require_clean_checkout(checkout: Path) -> None:
    status = run_git(
        ["--no-optional-locks", "status", "--porcelain", "--untracked-files=all"],
        cwd=checkout,
    )
    if status:
        status_lines = status.splitlines()
        preview = "\n".join(f"  {line}" for line in status_lines[:10])
        if len(status_lines) > 10:
            preview += "\n  ..."
        raise InventoryError(
            f"checkout {checkout.as_posix()} must be clean before inventory generation; "
            "dirty or untracked file(s) would make the inventory non-deterministic:\n"
            f"{preview}"
        )


def clone_pinned_source(lock: dict[str, str], destination: Path) -> None:
    ref = lock["ref"]
    pinned_commit = lock["pinned_commit"]
    try:
        run_git(
            [
                "clone",
                "--quiet",
                "--depth",
                "1",
                "--filter=blob:none",
                "--no-checkout",
                "--branch",
                ref,
                "--single-branch",
                lock["repo"],
                str(destination),
            ]
        )
        run_git(["checkout", "--quiet", "--detach", pinned_commit], cwd=destination)
    except InventoryError as exc:
        raise InventoryError(
            "reference checkout is absent and pinned-source fallback failed: "
            f"could not materialize {ref} at {pinned_commit} from {lock['repo']}: {exc}"
        ) from exc
    require_pinned_commit(destination, lock)


@contextmanager
def source_checkout(root: Path, lock: dict[str, str]) -> Iterator[Path]:
    checkout_rel = Path(lock["checkout_path"])
    checkout = root / checkout_rel
    if checkout.is_dir():
        require_pinned_commit(checkout, lock)
        require_clean_checkout(checkout)
        yield checkout
        return

    with tempfile.TemporaryDirectory(prefix="pyqtgraph-class-inventory-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_source(lock, fallback_checkout)
        yield fallback_checkout


def posix_relative(path: Path, base: Path) -> str:
    return path.relative_to(base).as_posix()


def target_record(upstream_path: str) -> dict[str, str]:
    stem = upstream_path.removesuffix(".py")
    parts = upstream_path.split("/")
    subsystem = parts[1] if len(parts) > 2 else "core"
    return {
        "upstream_path": upstream_path,
        "target_header_path": f"include/{stem}.hpp",
        "target_source_path": f"src/{stem}.cpp",
        "subsystem": subsystem,
    }


def row_identity(row: dict[str, Any]) -> tuple[str, str, str]:
    return ("classes", str(row.get("upstream_path", "")), str(row.get("class_name", "")))


def validate_evidence_artifact(root: Path, prefix: str, value: Any) -> str:
    if not isinstance(value, str) or not value:
        raise InventoryError(f"{prefix}.artifact_path must be a non-empty string")
    relative_path = Path(value)
    if relative_path.is_absolute():
        raise InventoryError(f"{prefix}.artifact_path must be relative: {value}")
    root_path = root.resolve()
    artifact_path = (root_path / relative_path).resolve()
    if not artifact_path.is_relative_to(root_path):
        raise InventoryError(f"{prefix}.artifact_path escapes repository root: {value}")
    if not artifact_path.is_file():
        raise InventoryError(f"{prefix}.artifact_path missing: {value}")
    return value


def parse_completion_evidence(root: Path, manifest: dict[str, Any]) -> dict[tuple[str, str, str], dict[str, str]]:
    records = manifest.get(COMPLETION_EVIDENCE_KEY)
    if records is None:
        return {}
    if not isinstance(records, list):
        raise InventoryError(f"{COMPLETION_EVIDENCE_KEY} must be a list")

    parsed: dict[tuple[str, str, str], dict[str, str]] = {}
    for index, record in enumerate(records):
        prefix = f"{COMPLETION_EVIDENCE_KEY}[{index}]"
        if not isinstance(record, dict):
            raise InventoryError(f"{prefix} must be a mapping")
        section = record.get("section")
        upstream_path = record.get("upstream_path")
        class_name = record.get("class_name")
        evidence_type = record.get("evidence_type")
        if section != "classes":
            continue
        if not isinstance(upstream_path, str) or not upstream_path:
            raise InventoryError(f"{prefix}.upstream_path must be a non-empty string")
        if not isinstance(class_name, str) or not class_name:
            raise InventoryError(f"{prefix}.class_name must be a non-empty string")
        if evidence_type not in COMPLETION_EVIDENCE_TYPES:
            raise InventoryError(f"{prefix}.evidence_type is invalid: {evidence_type}")
        artifact_path = validate_evidence_artifact(root, prefix, record.get("artifact_path"))
        key = ("classes", upstream_path, class_name)
        if key in parsed:
            raise InventoryError(f"duplicate {COMPLETION_EVIDENCE_KEY} record: {key}")
        parsed[key] = {"type": evidence_type, "artifact_path": artifact_path}
    return parsed


def row_status(
    root: Path,
    row: dict[str, Any],
    evidence: dict[tuple[str, str, str], dict[str, str]],
) -> dict[str, Any]:
    target_paths = [row[key] for key in TARGET_PATH_KEYS if key in row]
    present_count = sum(1 for path in target_paths if (root / path).exists())
    if target_paths and present_count == len(target_paths):
        target_presence = "all"
    elif present_count == 0:
        target_presence = "none"
    else:
        target_presence = "some"

    metadata: dict[str, Any] = {
        "status": STATUS_BY_PRESENT_COUNT[target_presence],
        "completion": "missing",
        "target_presence": target_presence,
    }
    row_evidence = evidence.get(row_identity(row))
    if row_evidence is not None:
        if target_presence != "all":
            raise InventoryError(
                f"{COMPLETION_EVIDENCE_KEY} references row without all target files: "
                f"classes {row.get('upstream_path')} class_name={row.get('class_name')}"
            )
        metadata["completion"] = "complete"
        metadata[COMPLETION_EVIDENCE_KEY] = row_evidence
    return metadata


def with_completion_metadata(
    root: Path, rows: list[dict[str, Any]], evidence: dict[tuple[str, str, str], dict[str, str]]
) -> list[dict[str, Any]]:
    return [{**row, **row_status(root, row, evidence)} for row in rows]


def base_expression(base: ast.expr) -> str:
    try:
        return ast.unparse(base)
    except Exception as exc:  # pragma: no cover - defensive for unusual AST nodes.
        raise InventoryError(f"could not render class base expression: {exc}") from exc


def class_records(path: Path, checkout: Path) -> list[dict[str, Any]]:
    upstream_path = posix_relative(path, checkout)
    try:
        source = path.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=upstream_path)
    except SyntaxError as exc:
        raise InventoryError(f"failed to parse {upstream_path}: {exc}") from exc
    except OSError as exc:
        raise InventoryError(f"failed to read {upstream_path}: {exc}") from exc

    file_target = target_record(upstream_path)
    records: list[dict[str, Any]] = []
    for node in tree.body:
        if not isinstance(node, ast.ClassDef):
            continue
        records.append(
            {
                "class_name": node.name,
                **file_target,
                "bases": [base_expression(base) for base in node.bases],
                "line": node.lineno,
            }
        )
    return records


def tracked_files(checkout: Path) -> list[str]:
    return sorted(
        path for path in run_git(["ls-files", "-z"], cwd=checkout).split("\0") if path
    )


def enumerate_inventory(checkout: Path, lock: dict[str, str]) -> dict[str, Any]:
    package_root = checkout / "pyqtgraph"
    if not package_root.is_dir():
        raise InventoryError("checkout does not contain pyqtgraph/ package directory")

    tracked = tracked_files(checkout)
    example_paths: list[str] = []
    source_files: list[Path] = []
    classes: list[dict[str, Any]] = []
    for upstream_path in tracked:
        if not upstream_path.startswith("pyqtgraph/") or not upstream_path.endswith(
            ".py"
        ):
            continue
        if upstream_path.startswith("pyqtgraph/examples/"):
            example_paths.append(upstream_path)
            continue
        path = checkout / upstream_path
        source_files.append(path)
        classes.extend(class_records(path, checkout))

    test_paths = [
        path for path in tracked if path.startswith("tests/") and path.endswith(".py")
    ]

    classes.sort(
        key=lambda record: (
            str(record["upstream_path"]),
            int(record["line"]),
            str(record["class_name"]),
        )
    )
    example_paths.sort()
    test_paths.sort()

    return {
        "reference": {key: lock[key] for key in REQUIRED_LOCK_KEYS},
        "classes": classes,
        "excluded": {
            "examples": example_paths,
            "tests": test_paths,
        },
        "summary": {
            "class_count": len(classes),
            "source_file_count": len(source_files),
            "excluded_example_count": len(example_paths),
            "excluded_test_count": len(test_paths),
        },
    }


def render_inventory(inventory: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(inventory, indent=2, sort_keys=False) + "\n"
    return yaml.safe_dump(inventory, sort_keys=False)


def load_manifest(manifest_path: Path, inventory: dict[str, Any]) -> dict[str, Any]:
    if not manifest_path.exists():
        return {"reference": inventory["reference"]}

    data = yaml.safe_load(manifest_path.read_text(encoding="utf-8"))
    if data is None:
        return {}
    if not isinstance(data, dict):
        raise InventoryError("port_manifest.yaml must contain a YAML mapping")
    return data


def class_summary_subset(summary: Any) -> Any:
    if not isinstance(summary, dict):
        return summary
    return {key: summary.get(key) for key in CLASS_SUMMARY_KEYS}


def update_manifest(root: Path, inventory: dict[str, Any]) -> None:
    manifest_path = root / "port_manifest.yaml"
    manifest = load_manifest(manifest_path, inventory)
    completion_evidence = parse_completion_evidence(root, manifest)
    manifest["classes"] = with_completion_metadata(root, inventory["classes"], completion_evidence)
    manifest["excluded"] = inventory["excluded"]
    summary = manifest.get("summary")
    if not isinstance(summary, dict):
        summary = {}
    for key in CLASS_SUMMARY_KEYS:
        summary[key] = inventory["summary"][key]
    manifest["summary"] = summary
    manifest_path.write_text(
        yaml.safe_dump(manifest, sort_keys=False), encoding="utf-8"
    )


def validate_manifest_current(root: Path, inventory: dict[str, Any]) -> None:
    manifest_path = root / "port_manifest.yaml"
    if not manifest_path.exists():
        return

    manifest = load_manifest(manifest_path, inventory)
    completion_evidence = parse_completion_evidence(root, manifest)
    manifest_sections = {
        "classes": manifest.get("classes"),
        "excluded": manifest.get("excluded"),
        "summary": class_summary_subset(manifest.get("summary")),
    }
    expected_sections = {
        "classes": with_completion_metadata(root, inventory["classes"], completion_evidence),
        "excluded": inventory["excluded"],
        "summary": class_summary_subset(inventory["summary"]),
    }
    stale_keys = [
        key
        for key in GENERATED_MANIFEST_KEYS
        if manifest_sections[key] != expected_sections[key]
    ]
    if stale_keys:
        raise InventoryError(
            "port_manifest.yaml is stale; run --update-manifest "
            "to refresh generated section(s): " + ", ".join(stale_keys)
        )


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = args.root.resolve()

    try:
        lock = load_lock(root)
        with source_checkout(root, lock) as checkout:
            inventory = enumerate_inventory(checkout, lock)
        if args.check:
            # Exercise serialization and validate generated manifest sections in check
            # mode while keeping the command read-only.
            render_inventory(inventory, args.format)
            validate_manifest_current(root, inventory)
            print(
                f"class inventory verified ({inventory['summary']['class_count']} classes)"
            )
        elif args.update_manifest:
            update_manifest(root, inventory)
            print("updated port_manifest.yaml")
        else:
            sys.stdout.write(render_inventory(inventory, args.format))
        return 0
    except InventoryError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
