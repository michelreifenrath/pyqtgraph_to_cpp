from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path
from types import SimpleNamespace
from typing import Any

SCRIPT = Path("oracle/scripts/render_pyqtgraph_example.py")
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def run_cli(
    *args: str, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    command_env = os.environ.copy()
    if env:
        command_env.update(env)
    return subprocess.run(
        [sys.executable, str(SCRIPT.resolve()), *args],
        text=True,
        capture_output=True,
        env=command_env,
    )


def import_renderer() -> Any:
    spec = importlib.util.spec_from_file_location("render_pyqtgraph_example", SCRIPT)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def install_fake_runtime_with_top_level_widget(
    renderer: Any, before_load: Callable[[], None] | None = None
) -> None:
    class FakePixmap:
        def save(self, path: str, fmt: str) -> bool:
            assert fmt == "PNG"
            Path(path).write_bytes(PNG_SIGNATURE + b"fake-png")
            return True

    class FakeWidget:
        def resize(self, _width: int, _height: int) -> None:
            pass

        def show(self) -> None:
            pass

        def grab(self) -> FakePixmap:
            return FakePixmap()

    class FakeApplication:
        _instance = None

        def __init__(self, _argv: list[str]) -> None:
            type(self)._instance = self

        @classmethod
        def instance(cls):
            return cls._instance

        def topLevelWidgets(self):
            return [FakeWidget()]

        def processEvents(self) -> None:
            pass

    def load_runtime():
        if before_load is not None:
            before_load()
        return SimpleNamespace(
            QtCore=SimpleNamespace(),
            QtWidgets=SimpleNamespace(QApplication=FakeApplication, QWidget=FakeWidget),
        )

    renderer._load_runtime = load_runtime


def test_help_exposes_renderer_cli_options() -> None:
    result = run_cli("--help", env={"QT_QPA_PLATFORM": "offscreen"})

    assert result.returncode == 0, result.stderr
    assert "--output" in result.stdout
    assert "--width" in result.stdout
    assert "--height" in result.stdout
    assert "example" in result.stdout
    assert result.stderr == ""


def test_rejects_missing_output_argument(tmp_path: Path) -> None:
    example = tmp_path / "example.py"
    example.write_text("# no-op\n", encoding="utf-8")
    output = tmp_path / "reference.png"

    result = run_cli(str(example))

    assert result.returncode == 2
    assert "--output" in result.stderr
    assert not output.exists()


def test_rejects_non_positive_dimensions_without_creating_output(
    tmp_path: Path,
) -> None:
    example = tmp_path / "example.py"
    example.write_text("# no-op\n", encoding="utf-8")
    output = tmp_path / "nested" / "reference.png"

    result = run_cli(str(example), "--output", str(output), "--width", "0")

    assert result.returncode == 2
    assert "positive width/height" in result.stderr
    assert not output.exists()
    assert not output.parent.exists()


def test_missing_runtime_dependencies_are_reported_clearly(
    tmp_path: Path, capsys
) -> None:
    renderer = import_renderer()
    example = tmp_path / "example.py"
    example.write_text("# no-op\n", encoding="utf-8")
    output = tmp_path / "reference.png"

    def fail_load_runtime():
        raise ImportError("No module named 'pyqtgraph'")

    renderer._load_runtime = fail_load_runtime

    code = renderer.main([str(example), "--output", str(output)])
    captured = capsys.readouterr()

    assert code == 2
    assert captured.err.startswith("render_pyqtgraph_example:")
    assert "missing required runtime dependency" in captured.err
    assert not output.exists()


def test_render_skips_example_main_guard_to_avoid_blocking_event_loop(
    tmp_path: Path, capsys
) -> None:
    renderer = import_renderer()
    sentinel = tmp_path / "main-guard-ran"
    example = tmp_path / "example.py"
    example.write_text(
        "from pathlib import Path\n"
        "if __name__ == '__main__':\n"
        f"    Path({str(sentinel)!r}).write_text('blocked', encoding='utf-8')\n",
        encoding="utf-8",
    )
    output = tmp_path / "reference.png"

    class FakePixmap:
        def save(self, path: str, fmt: str) -> bool:
            assert fmt == "PNG"
            Path(path).write_bytes(PNG_SIGNATURE + b"fake-png")
            return True

    class FakeWidget:
        def resize(self, _width: int, _height: int) -> None:
            pass

        def show(self) -> None:
            pass

        def grab(self) -> FakePixmap:
            return FakePixmap()

    class FakeApplication:
        _instance = None

        def __init__(self, _argv: list[str]) -> None:
            type(self)._instance = self

        @classmethod
        def instance(cls):
            return cls._instance

        def topLevelWidgets(self):
            return [FakeWidget()]

        def processEvents(self) -> None:
            pass

    def load_runtime():
        return SimpleNamespace(
            QtCore=SimpleNamespace(),
            QtWidgets=SimpleNamespace(QApplication=FakeApplication, QWidget=FakeWidget),
        )

    renderer._load_runtime = load_runtime

    code = renderer.main([str(example), "--output", str(output), "--timeout-ms", "1"])
    captured = capsys.readouterr()

    assert code == 0
    assert output.read_bytes().startswith(PNG_SIGNATURE)
    assert not sentinel.exists()
    assert captured.err == ""


def test_render_isolates_example_argv_from_renderer_cli_args(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    renderer = import_renderer()
    example = tmp_path / "example.py"
    output = tmp_path / "reference.png"
    leaked_argv = [
        "render_pyqtgraph_example.py",
        str(example),
        "--output",
        str(output),
        "--width",
        "320",
    ]
    example.write_text(
        "import sys\n"
        f"expected = [{str(example)!r}]\n"
        "if sys.argv != expected:\n"
        "    raise RuntimeError(f'leaked argv: {sys.argv!r}')\n",
        encoding="utf-8",
    )
    monkeypatch.setattr(sys, "argv", leaked_argv[:])
    install_fake_runtime_with_top_level_widget(renderer)

    code = renderer.main([str(example), "--output", str(output), "--timeout-ms", "1"])
    captured = capsys.readouterr()

    assert code == 0, captured.err
    assert output.read_bytes().startswith(PNG_SIGNATURE)
    assert sys.argv == leaked_argv
    assert captured.err == ""


def test_render_prepends_example_directory_for_sibling_imports(
    tmp_path: Path, capsys
) -> None:
    renderer = import_renderer()
    sibling = tmp_path / "example_ui.py"
    example = tmp_path / "example.py"
    output = tmp_path / "reference.png"
    sibling.write_text("SENTINEL = 'loaded sibling'\n", encoding="utf-8")
    example.write_text(
        "import example_ui\n"
        "if example_ui.SENTINEL != 'loaded sibling':\n"
        "    raise RuntimeError('unexpected sibling module')\n",
        encoding="utf-8",
    )
    original_sys_path = sys.path[:]
    assert str(tmp_path) not in original_sys_path
    install_fake_runtime_with_top_level_widget(renderer)

    code = renderer.main([str(example), "--output", str(output), "--timeout-ms", "1"])
    captured = capsys.readouterr()

    assert code == 0, captured.err
    assert output.read_bytes().startswith(PNG_SIGNATURE)
    assert sys.path == original_sys_path
    assert captured.err == ""


def test_render_prepends_pinned_checkout_before_loading_runtime(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    renderer = import_renderer()
    checkout_root = tmp_path / "reference" / "pyqtgraph"
    package_dir = checkout_root / "pyqtgraph"
    example_dir = package_dir / "examples" / "nested"
    example_dir.mkdir(parents=True)
    (package_dir / "__init__.py").write_text("# pinned pyqtgraph\n", encoding="utf-8")
    example = example_dir / "example.py"
    example.write_text("# fake pinned example\n", encoding="utf-8")
    output = tmp_path / "reference.png"
    original_sys_path = sys.path[:]

    def assert_runtime_path() -> None:
        assert sys.path[0] == str(checkout_root)
        assert str(example_dir) not in sys.path[:1]

    def run_path(path: str, run_name: str):
        assert path == str(example)
        assert run_name == renderer.EXAMPLE_RUN_NAME
        assert sys.path[0] == str(example_dir)
        assert sys.path[1] == str(checkout_root)
        return {}

    install_fake_runtime_with_top_level_widget(renderer, assert_runtime_path)
    monkeypatch.setattr(renderer.runpy, "run_path", run_path)

    code = renderer.main([str(example), "--output", str(output), "--timeout-ms", "1"])
    captured = capsys.readouterr()

    assert code == 0, captured.err
    assert output.read_bytes().startswith(PNG_SIGNATURE)
    assert sys.path == original_sys_path
    assert captured.err == ""


def test_render_creates_parent_dirs_and_writes_png_with_fake_runtime(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    renderer = import_renderer()
    monkeypatch.delenv("QT_QPA_PLATFORM", raising=False)
    example = tmp_path / "example.py"
    example.write_text("# fake example executed through runpy\n", encoding="utf-8")
    output = tmp_path / "nested" / "reference.png"

    class FakePixmap:
        def save(self, path: str, fmt: str) -> bool:
            assert fmt == "PNG"
            Path(path).write_bytes(PNG_SIGNATURE + b"fake-png")
            return True

    class FakeWidget:
        def __init__(self) -> None:
            self.size = None
            self.shown = False

        def resize(self, width: int, height: int) -> None:
            self.size = (width, height)

        def show(self) -> None:
            self.shown = True

        def grab(self) -> FakePixmap:
            assert self.size == (320, 240)
            assert self.shown
            return FakePixmap()

    widget = FakeWidget()

    class FakeApplication:
        _instance = None

        def __init__(self, _argv: list[str]) -> None:
            type(self)._instance = self
            self.processed_events = 0

        @classmethod
        def instance(cls):
            return cls._instance

        @classmethod
        def topLevelWidgets(cls):
            return []

        def processEvents(self) -> None:
            self.processed_events += 1

    def load_runtime():
        assert os.environ["QT_QPA_PLATFORM"] == "offscreen"
        return SimpleNamespace(
            QtCore=SimpleNamespace(),
            QtWidgets=SimpleNamespace(QApplication=FakeApplication, QWidget=FakeWidget),
        )

    def run_path(path: str, run_name: str):
        assert path == str(example)
        assert run_name == renderer.EXAMPLE_RUN_NAME
        return {"widget": widget}

    renderer._load_runtime = load_runtime
    monkeypatch.setattr(renderer.runpy, "run_path", run_path)

    code = renderer.main(
        [
            str(example),
            "--output",
            str(output),
            "--width",
            "320",
            "--height",
            "240",
            "--timeout-ms",
            "1",
        ]
    )
    captured = capsys.readouterr()

    assert code == 0
    assert output.read_bytes().startswith(PNG_SIGNATURE)
    assert f"wrote screenshot: {output}" in captured.out
    assert captured.err == ""
    assert os.environ["QT_QPA_PLATFORM"] == "offscreen"
