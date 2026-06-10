#!/usr/bin/env python3
"""Generate a deterministic inventory of pinned PyQtGraph Python sources."""

from __future__ import annotations

import argparse
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


class InventoryError(RuntimeError):
    """Raised when inventory generation cannot be completed safely."""


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a deterministic PyQtGraph source inventory."
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
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate inventory generation without writing persistent files",
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

    with tempfile.TemporaryDirectory(prefix="pyqtgraph-source-inventory-") as temp_dir:
        fallback_checkout = Path(temp_dir) / "pyqtgraph"
        clone_pinned_source(lock, fallback_checkout)
        yield fallback_checkout


def posix_relative(path: Path, base: Path) -> str:
    return path.relative_to(base).as_posix()


def source_record(upstream_path: str) -> dict[str, str]:
    upstream_stem = upstream_path.removesuffix(".py")
    target_stem = "cppqtgraph/" + upstream_stem.removeprefix("pyqtgraph/")
    parts = upstream_path.split("/")
    subsystem = parts[1] if len(parts) > 2 else "core"
    return {
        "upstream_path": upstream_path,
        "target_header_path": f"include/{target_stem}.hpp",
        "target_source_path": f"src/{target_stem}.cpp",
        "subsystem": subsystem,
    }


def enumerate_inventory(checkout: Path, lock: dict[str, str]) -> dict[str, Any]:
    package_root = checkout / "pyqtgraph"
    if not package_root.is_dir():
        raise InventoryError("checkout does not contain pyqtgraph/ package directory")

    example_paths: list[str] = []
    source_files: list[dict[str, str]] = []
    for path in sorted(package_root.rglob("*.py")):
        upstream_path = posix_relative(path, checkout)
        if upstream_path.startswith("pyqtgraph/examples/"):
            example_paths.append(upstream_path)
            continue
        source_files.append(source_record(upstream_path))

    test_root = checkout / "tests"
    test_paths = (
        [posix_relative(path, checkout) for path in sorted(test_root.rglob("*.py"))]
        if test_root.is_dir()
        else []
    )

    source_files.sort(key=lambda record: record["upstream_path"])
    example_paths.sort()
    test_paths.sort()

    return {
        "reference": {key: lock[key] for key in REQUIRED_LOCK_KEYS},
        "source_files": source_files,
        "excluded": {
            "examples": example_paths,
            "tests": test_paths,
        },
        "summary": {
            "source_file_count": len(source_files),
            "excluded_example_count": len(example_paths),
            "excluded_test_count": len(test_paths),
        },
    }


def render_inventory(inventory: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(inventory, indent=2, sort_keys=False) + "\n"
    return yaml.safe_dump(inventory, sort_keys=False)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)
    root = args.root.resolve()

    try:
        lock = load_lock(root)
        with source_checkout(root, lock) as checkout:
            inventory = enumerate_inventory(checkout, lock)
        if args.check:
            # Exercise serialization in check mode while keeping the command read-only.
            render_inventory(inventory, args.format)
            print(
                "source inventory verified "
                f"({inventory['summary']['source_file_count']} source files)"
            )
        else:
            sys.stdout.write(render_inventory(inventory, args.format))
        return 0
    except InventoryError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
