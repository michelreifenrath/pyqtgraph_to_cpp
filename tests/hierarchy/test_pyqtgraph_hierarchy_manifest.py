from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pytest
import yaml

SCRIPT = Path("oracle/scripts/dump_pyqtgraph_hierarchy.py")
REF = "pyqtgraph-0.14.0"
DOCS_URL = "https://pyqtgraph.readthedocs.io/"
CHECKOUT_PATH = "reference/pyqtgraph"
FIXTURE_PATH = Path("oracle/fixtures/hierarchy_pyqtgraph.json")


def run_cli(*args: str, root: Path | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), *args],
        cwd=root,
        text=True,
        capture_output=True,
    )


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        check=True,
        text=True,
        capture_output=True,
    )
    return result.stdout.strip()


def write_fixture_file(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def populate_fixture_repo(repo: Path) -> str:
    repo.mkdir(parents=True)
    git(repo, "init")
    git(repo, "config", "user.email", "test@example.invalid")
    git(repo, "config", "user.name", "Test User")

    write_fixture_file(
        repo / "pyqtgraph" / "PlotData.py",
        """class PlotData(object):
    class NestedInClass:
        pass


def factory():
    class NestedInFunction:
        pass
    return NestedInFunction
""",
    )
    write_fixture_file(
        repo / "pyqtgraph" / "widgets" / "PlotWidget.py",
        """class HelperMixin:
    pass


class BaseWidget(HelperMixin):
    pass


class PlotWidget(BaseWidget, QtWidgets.QWidget):
    pass
""",
    )
    write_fixture_file(
        repo / "pyqtgraph" / "widgets" / "AliasWidget.py",
        """class AliasWidget(PlotWidget):
    pass
""",
    )
    write_fixture_file(
        repo / "pyqtgraph" / "graphicsItems" / "ScatterPlotItem.py",
        """class ScatterPlotItem(GraphicsObject):
    pass
""",
    )
    write_fixture_file(
        repo / "pyqtgraph" / "examples" / "Example.py",
        """class ExampleOnly:
    pass
""",
    )
    write_fixture_file(
        repo / "tests" / "test_x.py",
        """class TestOnly:
    pass
""",
    )
    git(repo, "add", ".")
    git(repo, "commit", "-m", "fixture")
    git(repo, "tag", REF)
    return git(repo, "rev-parse", "HEAD")


def write_source_lock(root: Path, *, repo: str, commit: str) -> None:
    (root / "reference").mkdir(parents=True, exist_ok=True)
    (root / "reference" / "source.lock").write_text(
        yaml.safe_dump(
            {
                "repo": repo,
                "ref": REF,
                "pinned_commit": commit,
                "docs_url": DOCS_URL,
                "checkout_path": CHECKOUT_PATH,
            },
            sort_keys=False,
        ),
        encoding="utf-8",
    )


def make_inventory_root(tmp_path: Path) -> tuple[Path, str]:
    root = tmp_path / "workspace"
    checkout = root / CHECKOUT_PATH
    commit = populate_fixture_repo(checkout)
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)
    return root, commit


def snapshot_tree(root: Path) -> dict[str, tuple[int, int]]:
    snapshot: dict[str, tuple[int, int]] = {}
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        stat = path.stat()
        snapshot[path.relative_to(root).as_posix()] = (stat.st_size, stat.st_mtime_ns)
    return snapshot


def expected_hierarchy(
    commit: str, *, repo: str = "fixture://pyqtgraph"
) -> dict[str, object]:
    return {
        "reference": {
            "repo": repo,
            "ref": REF,
            "pinned_commit": commit,
            "docs_url": DOCS_URL,
            "checkout_path": CHECKOUT_PATH,
        },
        "classes": [
            {
                "class_name": "PlotData",
                "qualified_name": "pyqtgraph.PlotData.PlotData",
                "upstream_path": "pyqtgraph/PlotData.py",
                "bases": ["object"],
                "resolved_bases": [],
                "children": [],
                "line": 1,
            },
            {
                "class_name": "ScatterPlotItem",
                "qualified_name": "pyqtgraph.graphicsItems.ScatterPlotItem.ScatterPlotItem",
                "upstream_path": "pyqtgraph/graphicsItems/ScatterPlotItem.py",
                "bases": ["GraphicsObject"],
                "resolved_bases": [],
                "children": [],
                "line": 1,
            },
            {
                "class_name": "AliasWidget",
                "qualified_name": "pyqtgraph.widgets.AliasWidget.AliasWidget",
                "upstream_path": "pyqtgraph/widgets/AliasWidget.py",
                "bases": ["PlotWidget"],
                "resolved_bases": [
                    {
                        "base": "PlotWidget",
                        "qualified_name": "pyqtgraph.widgets.PlotWidget.PlotWidget",
                        "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                    }
                ],
                "children": [],
                "line": 1,
            },
            {
                "class_name": "HelperMixin",
                "qualified_name": "pyqtgraph.widgets.PlotWidget.HelperMixin",
                "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                "bases": [],
                "resolved_bases": [],
                "children": ["pyqtgraph.widgets.PlotWidget.BaseWidget"],
                "line": 1,
            },
            {
                "class_name": "BaseWidget",
                "qualified_name": "pyqtgraph.widgets.PlotWidget.BaseWidget",
                "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                "bases": ["HelperMixin"],
                "resolved_bases": [
                    {
                        "base": "HelperMixin",
                        "qualified_name": "pyqtgraph.widgets.PlotWidget.HelperMixin",
                        "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                    }
                ],
                "children": ["pyqtgraph.widgets.PlotWidget.PlotWidget"],
                "line": 5,
            },
            {
                "class_name": "PlotWidget",
                "qualified_name": "pyqtgraph.widgets.PlotWidget.PlotWidget",
                "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                "bases": ["BaseWidget", "QtWidgets.QWidget"],
                "resolved_bases": [
                    {
                        "base": "BaseWidget",
                        "qualified_name": "pyqtgraph.widgets.PlotWidget.BaseWidget",
                        "upstream_path": "pyqtgraph/widgets/PlotWidget.py",
                    }
                ],
                "children": ["pyqtgraph.widgets.AliasWidget.AliasWidget"],
                "line": 9,
            },
        ],
        "edges": [
            {
                "parent": "pyqtgraph.widgets.PlotWidget.BaseWidget",
                "child": "pyqtgraph.widgets.PlotWidget.PlotWidget",
                "base": "BaseWidget",
            },
            {
                "parent": "pyqtgraph.widgets.PlotWidget.HelperMixin",
                "child": "pyqtgraph.widgets.PlotWidget.BaseWidget",
                "base": "HelperMixin",
            },
            {
                "parent": "pyqtgraph.widgets.PlotWidget.PlotWidget",
                "child": "pyqtgraph.widgets.AliasWidget.AliasWidget",
                "base": "PlotWidget",
            },
        ],
        "excluded": {
            "examples": ["pyqtgraph/examples/Example.py"],
            "tests": ["tests/test_x.py"],
        },
        "summary": {
            "class_count": 6,
            "edge_count": 3,
            "unresolved_base_count": 3,
            "source_file_count": 4,
            "excluded_example_count": 1,
            "excluded_test_count": 1,
        },
    }


def test_help_exposes_hierarchy_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "--root" in result.stdout
    assert "--format" in result.stdout
    assert "--check" in result.stdout
    assert "--update-fixture" in result.stdout


def test_json_hierarchy_is_deterministic_sorted_and_excludes_nested_example_and_tests(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)

    first = run_cli("--root", str(root))
    second = run_cli("--root", str(root))

    assert first.returncode == 0, first.stderr
    assert second.returncode == 0, second.stderr
    assert first.stdout == second.stdout
    hierarchy = json.loads(first.stdout)
    assert hierarchy == expected_hierarchy(commit)
    assert [
        (record["upstream_path"], record["line"], record["class_name"])
        for record in hierarchy["classes"]
    ] == sorted(
        (record["upstream_path"], record["line"], record["class_name"])
        for record in hierarchy["classes"]
    )
    assert [
        (edge["parent"], edge["child"], edge["base"]) for edge in hierarchy["edges"]
    ] == sorted(
        (edge["parent"], edge["child"], edge["base"]) for edge in hierarchy["edges"]
    )
    assert {record["class_name"] for record in hierarchy["classes"]}.isdisjoint(
        {"NestedInClass", "NestedInFunction", "ExampleOnly", "TestOnly"}
    )


def test_yaml_stdout_matches_json_shape(tmp_path: Path) -> None:
    root, commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--format", "yaml")

    assert result.returncode == 0, result.stderr
    assert yaml.safe_load(result.stdout) == expected_hierarchy(commit)


def test_check_mode_validates_existing_fixture_without_writes(tmp_path: Path) -> None:
    root, commit = make_inventory_root(tmp_path)
    write_fixture_file(
        root / FIXTURE_PATH, json.dumps(expected_hierarchy(commit), indent=2) + "\n"
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "hierarchy fixture verified (6 classes, 3 edges)" in result.stdout
    assert snapshot_tree(root) == before


@pytest.mark.parametrize("fixture_text", [None, '{"stale": true}\n'])
def test_check_mode_rejects_missing_or_stale_fixture_without_writes(
    tmp_path: Path, fixture_text: str | None
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    if fixture_text is not None:
        write_fixture_file(root / FIXTURE_PATH, fixture_text)
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode != 0
    assert "oracle/fixtures/hierarchy_pyqtgraph.json is stale" in result.stderr
    assert "--update-fixture" in result.stderr
    assert not (root / FIXTURE_PATH).exists() if fixture_text is None else True
    assert snapshot_tree(root) == before


def test_check_mode_uses_read_only_fallback_when_checkout_absent(
    tmp_path: Path,
) -> None:
    upstream = tmp_path / "upstream"
    commit = populate_fixture_repo(upstream)
    root = tmp_path / "workspace"
    write_source_lock(root, repo=str(upstream), commit=commit)
    write_fixture_file(
        root / FIXTURE_PATH,
        json.dumps(expected_hierarchy(commit, repo=str(upstream)), indent=2) + "\n",
    )
    before = snapshot_tree(root)

    result = run_cli("--root", str(root), "--check")

    assert result.returncode == 0, result.stderr
    assert "hierarchy fixture verified (6 classes, 3 edges)" in result.stdout
    assert not (root / CHECKOUT_PATH).exists()
    assert snapshot_tree(root) == before


def test_rejects_checkout_at_wrong_commit(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    lock_path = root / "reference" / "source.lock"
    lock = yaml.safe_load(lock_path.read_text(encoding="utf-8"))
    lock["pinned_commit"] = "0" * 40
    lock_path.write_text(yaml.safe_dump(lock, sort_keys=False), encoding="utf-8")

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "pinned_commit" in result.stderr


@pytest.mark.parametrize("dirty_state", ["untracked", "modified", "deleted"])
def test_rejects_dirty_local_checkout_at_pinned_commit(
    tmp_path: Path, dirty_state: str
) -> None:
    root, commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    assert git(checkout, "rev-parse", "HEAD") == commit

    if dirty_state == "untracked":
        write_fixture_file(checkout / "pyqtgraph" / "Bogus.py", "class Bogus: pass\n")
    elif dirty_state == "modified":
        write_fixture_file(checkout / "pyqtgraph" / "PlotData.py", "# changed\n")
    else:
        (checkout / "pyqtgraph" / "PlotData.py").unlink()

    result = run_cli("--root", str(root))

    assert result.returncode != 0
    assert "must be clean" in result.stderr
    assert "non-deterministic" in result.stderr


def test_ignored_local_python_files_do_not_contaminate_output(tmp_path: Path) -> None:
    root, _commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    write_fixture_file(checkout / ".gitignore", "pyqtgraph/Ignored.py\n")
    git(checkout, "add", ".gitignore")
    git(checkout, "commit", "-m", "ignore local artifacts")
    commit = git(checkout, "rev-parse", "HEAD")
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)
    write_fixture_file(
        checkout / "pyqtgraph" / "Ignored.py", "class IgnoredArtifact:\n    pass\n"
    )

    result = run_cli("--root", str(root))

    assert result.returncode == 0, result.stderr
    hierarchy = json.loads(result.stdout)
    assert "IgnoredArtifact" not in {
        record["class_name"] for record in hierarchy["classes"]
    }


def test_resolves_same_module_base_when_simple_name_is_duplicated(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    write_fixture_file(
        checkout / "pyqtgraph" / "jupyter" / "GraphicsView.py",
        """class GraphicsView:
    pass


class GraphicsLayoutWidget(GraphicsView):
    pass


class PlotWidget(GraphicsView):
    pass
""",
    )
    write_fixture_file(
        checkout / "pyqtgraph" / "widgets" / "GraphicsView.py",
        """class GraphicsView:
    pass
""",
    )
    git(checkout, "add", ".")
    git(checkout, "commit", "-m", "add duplicate graphics view names")
    commit = git(checkout, "rev-parse", "HEAD")
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)

    result = run_cli("--root", str(root))

    assert result.returncode == 0, result.stderr
    hierarchy = json.loads(result.stdout)
    by_qualified_name = {
        record["qualified_name"]: record for record in hierarchy["classes"]
    }
    expected_base = [
        {
            "base": "GraphicsView",
            "qualified_name": "pyqtgraph.jupyter.GraphicsView.GraphicsView",
            "upstream_path": "pyqtgraph/jupyter/GraphicsView.py",
        }
    ]
    assert (
        by_qualified_name[
            "pyqtgraph.jupyter.GraphicsView.GraphicsLayoutWidget"
        ]["resolved_bases"]
        == expected_base
    )
    assert by_qualified_name[
        "pyqtgraph.jupyter.GraphicsView.PlotWidget"
    ]["resolved_bases"] == expected_base
    assert by_qualified_name[
        "pyqtgraph.jupyter.GraphicsView.GraphicsView"
    ]["children"] == [
        "pyqtgraph.jupyter.GraphicsView.GraphicsLayoutWidget",
        "pyqtgraph.jupyter.GraphicsView.PlotWidget",
    ]


def test_package_initializer_classes_use_package_qualified_name(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    write_fixture_file(
        checkout / "pyqtgraph" / "icons" / "__init__.py",
        """class GraphIcon:
    pass
""",
    )
    git(checkout, "add", ".")
    git(checkout, "commit", "-m", "add package initializer class")
    commit = git(checkout, "rev-parse", "HEAD")
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)

    result = run_cli("--root", str(root))

    assert result.returncode == 0, result.stderr
    hierarchy = json.loads(result.stdout)
    qualified_names = {record["qualified_name"] for record in hierarchy["classes"]}
    assert "pyqtgraph.icons.GraphIcon" in qualified_names
    assert "pyqtgraph.icons.__init__.GraphIcon" not in qualified_names


def test_qualified_import_alias_base_does_not_resolve_to_same_module_duplicate(
    tmp_path: Path,
) -> None:
    root, _commit = make_inventory_root(tmp_path)
    checkout = root / CHECKOUT_PATH
    write_fixture_file(
        checkout / "pyqtgraph" / "parametertree" / "__init__.py",
        """from . import parameterTypes as types


class ParameterTree:
    pass
""",
    )
    write_fixture_file(
        checkout / "pyqtgraph" / "parametertree" / "parameterTypes" / "__init__.py",
        """from .colormap import ColorMapParameter
""",
    )
    write_fixture_file(
        checkout / "pyqtgraph" / "parametertree" / "parameterTypes" / "colormap.py",
        """class ColorMapParameter:
    pass
""",
    )
    write_fixture_file(
        checkout / "pyqtgraph" / "widgets" / "ColorMapWidget.py",
        """from .. import parametertree as ptree


class ColorMapParameter:
    pass


class RangeColorMapItem(ptree.types.ColorMapParameter):
    pass
""",
    )
    git(checkout, "add", ".")
    git(checkout, "commit", "-m", "add qualified alias hierarchy case")
    commit = git(checkout, "rev-parse", "HEAD")
    write_source_lock(root, repo="fixture://pyqtgraph", commit=commit)

    result = run_cli("--root", str(root))

    assert result.returncode == 0, result.stderr
    hierarchy = json.loads(result.stdout)
    by_qualified_name = {
        record["qualified_name"]: record for record in hierarchy["classes"]
    }
    assert by_qualified_name[
        "pyqtgraph.widgets.ColorMapWidget.RangeColorMapItem"
    ]["resolved_bases"] == [
        {
            "base": "ptree.types.ColorMapParameter",
            "qualified_name": "pyqtgraph.parametertree.parameterTypes.colormap.ColorMapParameter",
            "upstream_path": "pyqtgraph/parametertree/parameterTypes/colormap.py",
        }
    ]
    assert by_qualified_name[
        "pyqtgraph.widgets.ColorMapWidget.ColorMapParameter"
    ]["children"] == []


def test_update_fixture_writes_deterministic_json_and_is_idempotent(
    tmp_path: Path,
) -> None:
    root, commit = make_inventory_root(tmp_path)

    result = run_cli("--root", str(root), "--update-fixture")

    assert result.returncode == 0, result.stderr
    assert result.stdout == "updated oracle/fixtures/hierarchy_pyqtgraph.json\n"
    fixture_path = root / FIXTURE_PATH
    after_first = fixture_path.read_text(encoding="utf-8")
    assert json.loads(after_first) == expected_hierarchy(commit)

    second = run_cli("--root", str(root), "--update-fixture")

    assert second.returncode == 0, second.stderr
    assert fixture_path.read_text(encoding="utf-8") == after_first


def test_check_and_update_fixture_are_mutually_exclusive() -> None:
    result = run_cli("--check", "--update-fixture")

    assert result.returncode != 0
    assert "not allowed with argument" in result.stderr
