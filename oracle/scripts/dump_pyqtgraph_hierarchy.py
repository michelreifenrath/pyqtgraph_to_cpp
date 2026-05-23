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


def module_name(upstream_path: str) -> str:
    module = upstream_path.removesuffix(".py").replace("/", ".")
    return module.removesuffix(".__init__")


def package_name(upstream_path: str) -> str:
    module = module_name(upstream_path)
    if upstream_path.endswith("/__init__.py"):
        return module
    return module.rsplit(".", 1)[0]


def qualified_name(upstream_path: str, class_name: str) -> str:
    return f"{module_name(upstream_path)}.{class_name}"


def simple_base_name(base: str) -> str:
    return base.split(".")[-1]


def resolve_import_module(upstream_path: str, *, level: int, module: str | None) -> str:
    if level == 0:
        return module or ""
    parts = package_name(upstream_path).split(".")
    if level > len(parts):
        return module or ""
    base_parts = parts[: len(parts) - level + 1]
    if module:
        base_parts.extend(module.split("."))
    return ".".join(part for part in base_parts if part)


def import_aliases(upstream_path: str, tree: ast.Module) -> dict[str, str]:
    aliases: dict[str, str] = {}
    for node in tree.body:
        if isinstance(node, ast.Import):
            for alias in node.names:
                alias_name = alias.asname or alias.name.split(".", 1)[0]
                aliases[alias_name] = alias.name if alias.asname else alias.name.split(".", 1)[0]
        elif isinstance(node, ast.ImportFrom):
            base_module = resolve_import_module(
                upstream_path,
                level=node.level,
                module=node.module,
            )
            for alias in node.names:
                if alias.name == "*":
                    continue
                alias_name = alias.asname or alias.name
                aliases[alias_name] = f"{base_module}.{alias.name}" if base_module else alias.name
    return aliases


def parse_source_module(path: Path, checkout: Path) -> tuple[str, ast.Module]:
    upstream_path = posix_relative(path, checkout)
    try:
        source = path.read_text(encoding="utf-8")
        return upstream_path, ast.parse(source, filename=upstream_path)
    except SyntaxError as exc:
        raise InventoryError(f"failed to parse {upstream_path}: {exc}") from exc
    except OSError as exc:
        raise InventoryError(f"failed to read {upstream_path}: {exc}") from exc


def file_import_aliases(path: Path, checkout: Path) -> dict[str, str]:
    upstream_path, tree = parse_source_module(path, checkout)
    return import_aliases(upstream_path, tree)


def class_records(path: Path, checkout: Path) -> list[dict[str, Any]]:
    upstream_path, tree = parse_source_module(path, checkout)
    aliases = import_aliases(upstream_path, tree)
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
                "_import_aliases": aliases,
            }
        )
    return records


def tracked_files(checkout: Path) -> list[str]:
    return sorted(
        path for path in run_git(["ls-files", "-z"], cwd=checkout).split("\0") if path
    )


def resolve_hierarchy(
    classes: list[dict[str, Any]], module_aliases: dict[str, str] | None = None
) -> list[dict[str, str]]:
    by_simple_name: dict[str, list[dict[str, Any]]] = defaultdict(list)
    by_qualified_name: dict[str, dict[str, Any]] = {}
    all_module_aliases: dict[str, str] = dict(module_aliases or {})
    for record in classes:
        by_simple_name[str(record["class_name"])].append(record)
        by_qualified_name[str(record["qualified_name"])] = record
        module = module_name(str(record["upstream_path"]))
        aliases = record.get("_import_aliases", {})
        if isinstance(aliases, dict):
            for alias, target in aliases.items():
                all_module_aliases[f"{module}.{alias}"] = str(target)

    def expand_alias_prefix(name: str, aliases: dict[str, str]) -> str | None:
        for alias, target in sorted(aliases.items(), key=lambda item: len(item[0]), reverse=True):
            if name == alias:
                return target
            if name.startswith(f"{alias}."):
                return f"{target}{name[len(alias):]}"
        return None

    def expand_module_aliases(name: str) -> str:
        expanded = name
        for _ in range(10):
            replacement = expand_alias_prefix(expanded, all_module_aliases)
            if replacement is None or replacement == expanded:
                return expanded
            expanded = replacement
        return expanded

    def candidate_qualified_bases(child: dict[str, Any], base: str) -> list[str]:
        candidates = [base]
        aliases = child.get("_import_aliases", {})
        if isinstance(aliases, dict):
            expanded = expand_alias_prefix(base, {str(key): str(value) for key, value in aliases.items()})
            if expanded is not None:
                candidates.append(expanded)
        candidates.extend(expand_module_aliases(candidate) for candidate in list(candidates))
        unique: list[str] = []
        for candidate in candidates:
            if candidate not in unique:
                unique.append(candidate)
        return unique

    def resolve_parent(child: dict[str, Any], base: str) -> dict[str, Any] | None:
        """Resolve a base expression without inventing ambiguous inheritance edges."""
        simple_name = simple_base_name(base)
        child_module = module_name(str(child["upstream_path"]))
        is_qualified = "." in base

        for candidate in candidate_qualified_bases(child, base):
            exact = by_qualified_name.get(candidate)
            if exact is not None:
                return exact

        # Prefer a class defined beside the child only for unqualified bases.
        # Qualified expressions such as ptree.types.ColorMapParameter must not
        # collapse to a same-module SimpleName match before alias resolution.
        if not is_qualified:
            same_module = by_qualified_name.get(f"{child_module}.{simple_name}")
            if same_module is not None:
                return same_module

        # Handle import-qualified expressions such as GraphicsView.GraphicsView
        # when the manifest has a unique suffix match.
        if is_qualified:
            suffix_matches = [
                record
                for qualified, record in by_qualified_name.items()
                if qualified.endswith(f".{base}")
            ]
            if len(suffix_matches) == 1:
                return suffix_matches[0]
            return None

        # Fall back to globally unique simple class names only after the more
        # specific resolution strategies above fail.
        candidates = by_simple_name.get(simple_name, [])
        if len(candidates) == 1:
            return candidates[0]
        return None

    edges: list[dict[str, str]] = []
    for child in classes:
        resolved_bases: list[dict[str, str]] = []
        for base in child["bases"]:
            parent = resolve_parent(child, str(base))
            if parent is None:
                continue
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
    module_aliases: dict[str, str] = {}
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
        for alias, target in file_import_aliases(path, checkout).items():
            module_aliases[f"{module_name(upstream_path)}.{alias}"] = target
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
    edges = resolve_hierarchy(classes, module_aliases)
    for record in classes:
        record.pop("_import_aliases", None)
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
