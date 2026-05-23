from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path
from types import SimpleNamespace
from typing import Any

import pytest

SCRIPT = Path("oracle/scripts/run_interaction_script.py")


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


def import_runner() -> Any:
    spec = importlib.util.spec_from_file_location("run_interaction_script", SCRIPT)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def write_valid_files(tmp_path: Path) -> tuple[Path, Path, Path]:
    example = tmp_path / "example.py"
    script = tmp_path / "interaction.yaml"
    output = tmp_path / "nested" / "status.json"
    example.write_text("# no-op\n", encoding="utf-8")
    script.write_text("version: 1\nsteps: []\n", encoding="utf-8")
    return example, script, output


def test_help_exposes_interaction_runner_cli_options() -> None:
    result = run_cli("--help", env={"QT_QPA_PLATFORM": "offscreen"})

    assert result.returncode == 0, result.stderr
    assert "example" in result.stdout
    assert "--script" in result.stdout
    assert "--output" in result.stdout
    assert "--width" in result.stdout
    assert "--height" in result.stdout
    assert "--timeout-ms" in result.stdout
    assert result.stderr == ""


def test_rejects_missing_required_script_and_output(tmp_path: Path) -> None:
    example = tmp_path / "example.py"
    example.write_text("# no-op\n", encoding="utf-8")
    output = tmp_path / "status.json"

    result = run_cli(str(example))

    assert result.returncode == 2
    assert result.stderr.startswith("run_interaction_script:")
    assert "--script" in result.stderr
    assert "--output" in result.stderr
    assert not output.exists()


def test_rejects_non_positive_dimensions_without_creating_output(
    tmp_path: Path,
) -> None:
    example, script, output = write_valid_files(tmp_path)

    result = run_cli(
        str(example), "--script", str(script), "--output", str(output), "--width", "0"
    )

    assert result.returncode == 2
    assert result.stderr.startswith("run_interaction_script:")
    assert "positive width/height" in result.stderr
    assert not output.exists()
    assert not output.parent.exists()


def test_rejects_non_positive_timeout_without_creating_output(tmp_path: Path) -> None:
    example, script, output = write_valid_files(tmp_path)

    result = run_cli(
        str(example),
        "--script",
        str(script),
        "--output",
        str(output),
        "--timeout-ms",
        "0",
    )

    assert result.returncode == 2
    assert result.stderr.startswith("run_interaction_script:")
    assert "positive --timeout-ms" in result.stderr
    assert not output.exists()
    assert not output.parent.exists()


def test_missing_example_file_is_reported_without_creating_output(
    tmp_path: Path,
) -> None:
    _example, script, output = write_valid_files(tmp_path)
    missing_example = tmp_path / "missing.py"

    result = run_cli(
        str(missing_example), "--script", str(script), "--output", str(output)
    )

    assert result.returncode == 2
    assert result.stderr.startswith("run_interaction_script:")
    assert "missing example file" in result.stderr
    assert not output.exists()
    assert not output.parent.exists()


def test_missing_interaction_script_is_reported_without_creating_output(
    tmp_path: Path,
) -> None:
    example, _script, output = write_valid_files(tmp_path)
    missing_script = tmp_path / "missing.yaml"

    result = run_cli(
        str(example), "--script", str(missing_script), "--output", str(output)
    )

    assert result.returncode == 2
    assert result.stderr.startswith("run_interaction_script:")
    assert "missing interaction script file" in result.stderr
    assert not output.exists()
    assert not output.parent.exists()


def test_load_interaction_script_normalizes_supported_actions(tmp_path: Path) -> None:
    runner = import_runner()
    script = tmp_path / "interaction.yaml"
    script.write_text(
        "version: 1\n"
        "steps:\n"
        "  - action: wait\n"
        "    ms: 10\n"
        "  - action: mouse_click\n"
        "    x: 12\n"
        "    y: 34\n"
        "  - action: key_click\n"
        "    key: A\n",
        encoding="utf-8",
    )

    assert runner.load_interaction_script(script) == [
        {"action": "wait", "ms": 10},
        {"action": "mouse_click", "x": 12, "y": 34, "button": "left", "modifiers": []},
        {"action": "key_click", "key": "A", "modifiers": []},
    ]


@pytest.mark.parametrize(
    ("content", "message"),
    [
        ("version: 1\nsteps:\n  - action: wait\n    ms: 0\n", "positive integer"),
        (
            "version: 1\nsteps:\n  - action: wait\n    ms: 1\n    extra: true\n",
            "unknown field",
        ),
        (
            "version: 1\nsteps:\n  - action: mouse_click\n    x: one\n    y: 2\n",
            "must be an integer",
        ),
        (
            "version: 1\nsteps:\n  - action: key_click\n    key: A\n    modifiers: shift\n",
            "list of strings",
        ),
    ],
)
def test_load_interaction_script_rejects_invalid_schema_details(
    tmp_path: Path, content: str, message: str
) -> None:
    runner = import_runner()
    script = tmp_path / "invalid.yaml"
    script.write_text(content, encoding="utf-8")

    with pytest.raises(runner.InteractionError, match=message):
        runner.load_interaction_script(script)


def test_invalid_interaction_scripts_return_2_and_do_not_create_output(
    tmp_path: Path, capsys
) -> None:
    runner = import_runner()
    example = tmp_path / "example.py"
    output = tmp_path / "nested" / "status.json"
    example.write_text("# no-op\n", encoding="utf-8")
    invalid_scripts = [
        "version: 2\nsteps: []\n",
        "version: 1\n",
        "version: 1\nsteps: nope\n",
        "version: 1\nsteps:\n  - action: unknown\n",
        "version: 1\nsteps:\n  - action: wait\n",
        "version: 1\nsteps:\n  - action: mouse_click\n    x: 1\n",
        "version: 1\nsteps:\n  - action: key_click\n",
        "version: 1\nsteps: [\n",
    ]

    for index, content in enumerate(invalid_scripts):
        script = tmp_path / f"invalid-{index}.yaml"
        script.write_text(content, encoding="utf-8")
        code = runner.main(
            [str(example), "--script", str(script), "--output", str(output)]
        )
        captured = capsys.readouterr()

        assert code == 2
        assert captured.err.startswith("run_interaction_script:")
        assert not output.exists()
        assert not output.parent.exists()


def install_fake_runtime(
    runner: Any,
    widget: Any,
    records: dict[str, Any],
    before_load: Callable[[], None] | None = None,
) -> None:
    class FakePoint:
        def __init__(self, x: int, y: int) -> None:
            self.x = x
            self.y = y

    class FakeQt:
        LeftButton = 1
        RightButton = 2
        MiddleButton = 4
        NoModifier = 0
        ShiftModifier = 8
        ControlModifier = 16
        AltModifier = 32
        MetaModifier = 64
        Key_A = 65
        Key_Enter = 16777221

    class FakeQTest:
        @staticmethod
        def wait(ms: int) -> None:
            records.setdefault("qtest", []).append(("wait", ms))

        @staticmethod
        def mouseClick(
            widget_arg: Any, button: int, modifiers: int, point: FakePoint
        ) -> None:
            records.setdefault("qtest", []).append(
                ("mouseClick", widget_arg, button, modifiers, (point.x, point.y))
            )

        @staticmethod
        def keyClick(widget_arg: Any, key: int, modifiers: int) -> None:
            records.setdefault("qtest", []).append(
                ("keyClick", widget_arg, key, modifiers)
            )

    widget_class = type(widget)

    class FakeApplication:
        _instance = None

        def __init__(self, _argv: list[str]) -> None:
            type(self)._instance = self
            self.processed_events = 0

        @classmethod
        def instance(cls):
            return cls._instance

        def topLevelWidgets(self):
            return [widget]

        def processEvents(self) -> None:
            self.processed_events += 1
            records["processed_events"] = self.processed_events

    def load_runtime():
        if before_load is not None:
            before_load()
        return SimpleNamespace(
            QtCore=SimpleNamespace(Qt=FakeQt, QPoint=FakePoint),
            QtWidgets=SimpleNamespace(
                QApplication=FakeApplication, QWidget=widget_class
            ),
            QtTest=SimpleNamespace(QTest=FakeQTest),
        )

    runner._load_runtime = load_runtime


def test_run_interactions_dispatches_fake_qt_actions_and_writes_json(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    runner = import_runner()
    monkeypatch.delenv("QT_QPA_PLATFORM", raising=False)
    example = tmp_path / "example.py"
    script = tmp_path / "interaction.yaml"
    output = tmp_path / "nested" / "status.json"
    example.write_text("# fake example executed through runpy\n", encoding="utf-8")
    script.write_text(
        "version: 1\n"
        "steps:\n"
        "  - action: wait\n"
        "    ms: 5\n"
        "  - action: mouse_click\n"
        "    x: 7\n"
        "    y: 9\n"
        "    modifiers: [shift, control]\n"
        "  - action: key_click\n"
        "    key: A\n",
        encoding="utf-8",
    )
    records: dict[str, Any] = {}

    class FakeWidget:
        def resize(self, width: int, height: int) -> None:
            records["resize"] = (width, height)

        def show(self) -> None:
            records["shown"] = True

        def activateWindow(self) -> None:
            records["activated"] = True

        def setFocus(self) -> None:
            records["focused"] = True

    widget = FakeWidget()
    install_fake_runtime(runner, widget, records)

    def run_path(path: str, run_name: str):
        assert path == str(example)
        assert run_name == runner.EXAMPLE_RUN_NAME
        assert sys.argv == [str(example)]
        return {"widget": widget}

    monkeypatch.setattr(runner.runpy, "run_path", run_path)

    code = runner.main(
        [
            str(example),
            "--script",
            str(script),
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

    assert code == 0, captured.err
    status = json.loads(output.read_text(encoding="utf-8"))
    assert json.loads(captured.out) == status
    assert status == {
        "status": "ok",
        "example": str(example),
        "script": str(script),
        "width": 320,
        "height": 240,
        "steps_executed": 3,
        "actions": ["wait", "mouse_click", "key_click"],
    }
    assert records["resize"] == (320, 240)
    assert records["shown"] is True
    assert records["activated"] is True
    assert records["focused"] is True
    assert records["processed_events"] >= 5
    assert records["qtest"] == [
        ("wait", 5),
        ("mouseClick", widget, 1, 24, (7, 9)),
        ("keyClick", widget, 65, 0),
    ]
    assert os.environ["QT_QPA_PLATFORM"] == "offscreen"
    assert captured.err == ""


def test_runner_isolates_paths_argv_and_prefers_pinned_checkout(
    tmp_path: Path, monkeypatch, capsys
) -> None:
    runner = import_runner()
    checkout_root = tmp_path / "reference" / "pyqtgraph"
    package_dir = checkout_root / "pyqtgraph"
    example_dir = package_dir / "examples" / "nested"
    example_dir.mkdir(parents=True)
    (package_dir / "__init__.py").write_text("# pinned pyqtgraph\n", encoding="utf-8")
    example = example_dir / "example.py"
    script = tmp_path / "interaction.yaml"
    output = tmp_path / "status.json"
    example.write_text("# fake pinned example\n", encoding="utf-8")
    script.write_text("version: 1\nsteps: []\n", encoding="utf-8")
    original_sys_path = sys.path[:]
    leaked_argv = ["run_interaction_script.py", "--leaked"]
    monkeypatch.setattr(sys, "argv", leaked_argv[:])
    records: dict[str, Any] = {}

    class FakeWidget:
        def resize(self, width: int, height: int) -> None:
            pass

        def show(self) -> None:
            pass

    def assert_runtime_path() -> None:
        assert sys.path[0] == str(checkout_root)
        assert str(example_dir) not in sys.path[:1]

    install_fake_runtime(runner, FakeWidget(), records, assert_runtime_path)

    def run_path(path: str, run_name: str):
        assert path == str(example)
        assert run_name == runner.EXAMPLE_RUN_NAME
        assert sys.path[0] == str(example_dir)
        assert sys.path[1] == str(checkout_root)
        assert sys.argv == [str(example)]
        return {}

    monkeypatch.setattr(runner.runpy, "run_path", run_path)

    code = runner.main(
        [
            str(example),
            "--script",
            str(script),
            "--output",
            str(output),
            "--timeout-ms",
            "1",
        ]
    )
    captured = capsys.readouterr()

    assert code == 0, captured.err
    assert sys.path == original_sys_path
    assert sys.argv == leaked_argv
    assert output.exists()
    assert captured.err == ""
