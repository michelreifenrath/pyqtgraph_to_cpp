#!/usr/bin/env python3
"""Generate a deterministic hierarchy manifest of pinned PyQtGraph classes."""

from __future__ import annotations

import argparse
import ast
import json
import subprocess
import sys
import tempfile
from collections import defaultdict
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any

import yaml

LOCK_PATH = Path("reference/source.lock")
FIXTURE_PATH = Path("oracle/fixtures/hierarchy_pyqtgraph.json")
REQUIRED_LOCK_KEYS = ("repo", "ref", "pinned_commit", "docs_url", "checkout_path")


class InventoryError(RuntimeError):
    """Raised when hierarchy generation cannot be completed safely."""


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a deterministic PyQtGraph class hierarchy manifest."
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=Path.cwd(),
        help="repository root containing reference/source.lock",
    )
    parser.add_argument(
        "--format",
        choices=("json", "yaml"),
        default="json",
        help="hierarchy serialization format for stdout",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--check",
        action="store_true",
        help="validate oracle/fixtures/hierarchy_pyqtgraph.json without writing files",
    )
    mode.add_argument(
        "--update-fixture",
        action="store_true",
        help="update only oracle/fixtures/hierarchy_pyqtgraph.json",
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
            f"checkout {checkout.as_posix()} must be clean before hierarchy generation; "
            "dirty or untracked file(s) would make the hierarchy non-deterministic:\n"
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
    checkout = root / Path(lock["checkout_path"])
    if checkout.is_dir():
        require_pinned_commit(checkout, lock)
        require_clean_checkout(checkout)
        yield checkout
        return

    with tempfile.TemporaryDirectory(prefix="pyqtgraph-hierarchy-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_source(lock, fallback_checkout)
        yield fallback_checkout


def posix_relative(path: Path, base: Path) -> str:
    return path.relative_to(base).as_posix()


def base_expression(base: ast.expr) -> str:
    try:
        return ast.unparse(base)
    except Exception as exc:  # pragma: no cover - defensive for unusual AST nodes.
        raise InventoryError(f"could not render class base expression: {exc}") from exc


def qualified_name(upstream_path: str, class_name: str) -> str:
    module = upstream_path.removesuffix(".py").replace("/", ".")
    return f"{module}.{class_name}"


def simple_base_name(base: str) -> str:
    return base.split(".")[-1]


def class_records(path: Path, checkout: Path) -> list[dict[str, Any]]:
    upstream_path = posix_relative(path, checkout)
    try:
        source = path.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=upstream_path)
    except SyntaxError as exc:
        raise InventoryError(f"failed to parse {upstream_path}: {exc}") from exc
    except OSError as exc:
        raise InventoryError(f"failed to read {upstream_path}: {exc}") from exc

    records: list[dict[str, Any]] = []
    for node in tree.body:
        if not isinstance(node, ast.ClassDef):
            continue
        records.append(
            {
                "class_name": node.name,
                "qualified_name": qualified_name(upstream_path, node.name),
                "upstream_path": upstream_path,
                "bases": [base_expression(base) for base in node.bases],
                "resolved_bases": [],
                "children": [],
                "line": node.lineno,
            }
        )
    return records


def tracked_files(checkout: Path) -> list[str]:
    return sorted(
        path for path in run_git(["ls-files", "-z"], cwd=checkout).split("\0") if path
    )


def resolve_hierarchy(classes: list[dict[str, Any]]) -> list[dict[str, str]]:
    by_simple_name: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in classes:
        by_simple_name[str(record["class_name"])].append(record)

    edges: list[dict[str, str]] = []
    for child in classes:
        resolved_bases: list[dict[str, str]] = []
        for base in child["bases"]:
            candidates = by_simple_name.get(simple_base_name(str(base)), [])
            if len(candidates) != 1:
                continue
            parent = candidates[0]
            resolved_bases.append(
                {
                    "base": str(base),
                    "qualified_name": str(parent["qualified_name"]),
                    "upstream_path": str(parent["upstream_path"]),
                }
            )
            edge = {
                "parent": str(parent["qualified_name"]),
                "child": str(child["qualified_name"]),
                "base": str(base),
            }
            edges.append(edge)
            parent["children"].append(str(child["qualified_name"]))
        child["resolved_bases"] = resolved_bases

    for record in classes:
        record["children"] = sorted(record["children"])
        record["resolved_bases"] = sorted(
            record["resolved_bases"],
            key=lambda item: (
                item["qualified_name"],
                item["base"],
                item["upstream_path"],
            ),
        )
    return sorted(edges, key=lambda edge: (edge["parent"], edge["child"], edge["base"]))


def enumerate_hierarchy(checkout: Path, lock: dict[str, str]) -> dict[str, Any]:
    package_root = checkout / "pyqtgraph"
    if not package_root.is_dir():
        raise InventoryError("checkout does not contain pyqtgraph/ package directory")

    tracked = tracked_files(checkout)
    example_paths: list[str] = []
    source_files: list[Path] = []
    classes: list[dict[str, Any]] = []
    for upstream_path in tracked:
        if upstream_path.startswith("pyqtgraph/examples/") and upstream_path.endswith(
            ".py"
        ):
            example_paths.append(upstream_path)
            continue
        if not upstream_path.startswith("pyqtgraph/") or not upstream_path.endswith(
            ".py"
        ):
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
    edges = resolve_hierarchy(classes)
    unresolved_base_count = sum(
        len(record["bases"]) - len(record["resolved_bases"]) for record in classes
    )
    example_paths.sort()
    test_paths.sort()

    return {
        "reference": {key: lock[key] for key in REQUIRED_LOCK_KEYS},
        "classes": classes,
        "edges": edges,
        "excluded": {
            "examples": example_paths,
            "tests": test_paths,
        },
        "summary": {
            "class_count": len(classes),
            "edge_count": len(edges),
            "unresolved_base_count": unresolved_base_count,
            "source_file_count": len(source_files),
            "excluded_example_count": len(example_paths),
            "excluded_test_count": len(test_paths),
        },
    }


def render_hierarchy(hierarchy: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(hierarchy, indent=2, sort_keys=False) + "\n"
    return yaml.safe_dump(hierarchy, sort_keys=False)


def update_fixture(root: Path, hierarchy: dict[str, Any]) -> None:
    fixture_path = root / FIXTURE_PATH
    fixture_path.parent.mkdir(parents=True, exist_ok=True)
    fixture_path.write_text(render_hierarchy(hierarchy, "json"), encoding="utf-8")


def validate_fixture_current(root: Path, hierarchy: dict[str, Any]) -> None:
    fixture_path = root / FIXTURE_PATH
    if not fixture_path.exists():
        raise InventoryError(
            f"{FIXTURE_PATH.as_posix()} is stale; run --update-fixture to refresh it"
        )
    try:
        fixture = json.loads(fixture_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise InventoryError(
            f"{FIXTURE_PATH.as_posix()} is stale; run --update-fixture to refresh it: {exc}"
        ) from exc
    if fixture != hierarchy:
        raise InventoryError(
            f"{FIXTURE_PATH.as_posix()} is stale; run --update-fixture to refresh it"
        )


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = args.root.resolve()

    try:
        lock = load_lock(root)
        with source_checkout(root, lock) as checkout:
            hierarchy = enumerate_hierarchy(checkout, lock)
        if args.check:
            render_hierarchy(hierarchy, args.format)
            validate_fixture_current(root, hierarchy)
            print(
                "hierarchy fixture verified "
                f"({hierarchy['summary']['class_count']} classes, "
                f"{hierarchy['summary']['edge_count']} edges)"
            )
        elif args.update_fixture:
            update_fixture(root, hierarchy)
            print(f"updated {FIXTURE_PATH.as_posix()}")
        else:
            sys.stdout.write(render_hierarchy(hierarchy, args.format))
        return 0
    except InventoryError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
