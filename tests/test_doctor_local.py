"""Tests for issue #118 local environment doctor."""

from __future__ import annotations

import json
import os
import stat
import subprocess
import sys
from pathlib import Path

import pytest


REPO_ROOT = Path(__file__).resolve().parents[1]


def run_doctor(
    *args: str, env: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    merged_env = os.environ.copy()
    if env is None or "CXX" not in env:
        merged_env.pop("CXX", None)
    merged_env.update(env or {})
    return subprocess.run(
        [sys.executable, str(REPO_ROOT / "scripts/doctor_local"), *args],
        cwd=REPO_ROOT,
        env=merged_env,
        text=True,
        capture_output=True,
        timeout=30,
    )


def make_executable(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def write_fake_tool(bin_dir: Path, name: str, order_file: Path, marker: str) -> None:
    make_executable(
        bin_dir / name,
        f"#!{sys.executable}\n"
        "import os, sys\n"
        "from pathlib import Path\n"
        f"order_file = Path({json.dumps(str(order_file))})\n"
        f"root = {json.dumps(str(REPO_ROOT))}\n"
        f"marker = {json.dumps(marker)}\n"
        "name = Path(sys.argv[0]).name\n"
        "entry = f\"{name} {' '.join(sys.argv[1:])}|cwd={Path.cwd()}|env={os.environ.get(marker, '')}\"\n"
        "order_file.open('a', encoding='utf-8').write(entry + '\\n')\n"
        "if Path.cwd().as_posix() != root:\n"
        "    raise SystemExit(31)\n"
        "if os.environ.get(marker) != 'propagated':\n"
        "    raise SystemExit(32)\n"
        "if name == 'pkg-config' and sys.argv[1:] == ['--modversion', 'Qt6Core', 'Qt6Gui', 'Qt6Widgets', 'Qt6Test']:\n"
        "    print('6.6.0')\n"
        "elif name == 'pkg-config' and sys.argv[1:] == ['--modversion', 'opencv4']:\n"
        "    print('4.8.0')\n"
        "elif name == 'pkg-config' and sys.argv[1:] == ['--modversion', 'gl']:\n"
        "    print('1.2')\n"
        "else:\n"
        "    print(name + ' ok')\n",
    )


def test_doctor_local_P1_14_help_smoke() -> None:
    result = run_doctor("--help")

    assert result.returncode == 0
    assert "local environment doctor" in result.stdout
    assert "compiler" in result.stdout


def test_doctor_local_P1_14_reports_required_checks_in_order_with_cwd_and_env(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    order_file = tmp_path / "order.txt"
    marker = "P1_14_ENV_MARKER"
    monkeypatch.setenv("CXX", str(tmp_path / "external-cxx"))
    for tool in ["git", "cmake", "ctest", "pkg-config", "c++"]:
        write_fake_tool(bin_dir, tool, order_file, marker)

    result = run_doctor(
        env={
            "PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}",
            marker: "propagated",
        }
    )

    assert result.returncode == 0, result.stderr + result.stdout
    assert order_file.read_text(encoding="utf-8").splitlines() == [
        f"git --version|cwd={REPO_ROOT}|env=propagated",
        f"cmake --version|cwd={REPO_ROOT}|env=propagated",
        f"ctest --version|cwd={REPO_ROOT}|env=propagated",
        f"pkg-config --version|cwd={REPO_ROOT}|env=propagated",
        f"c++ --version|cwd={REPO_ROOT}|env=propagated",
        f"pkg-config --modversion Qt6Core Qt6Gui Qt6Widgets Qt6Test|cwd={REPO_ROOT}|env=propagated",
        f"pkg-config --modversion opencv4|cwd={REPO_ROOT}|env=propagated",
        f"pkg-config --modversion gl|cwd={REPO_ROOT}|env=propagated",
    ]
    for section in ["Tools", "Compiler", "Qt", "OpenCV", "OpenGL"]:
        assert section in result.stdout
    assert "doctor_local: PASS" in result.stdout


def test_doctor_local_P1_14_propagates_failure_and_skips_later_checks(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    order_file = tmp_path / "order.txt"
    marker = "P1_14_ENV_MARKER"
    monkeypatch.setenv("CXX", str(tmp_path / "external-cxx"))
    for tool in ["git", "cmake", "ctest", "c++"]:
        write_fake_tool(bin_dir, tool, order_file, marker)
    make_executable(
        bin_dir / "pkg-config",
        f"#!{sys.executable}\n"
        "import sys\n"
        "from pathlib import Path\n"
        f"order_file = Path({json.dumps(str(order_file))})\n"
        "order_file.open('a', encoding='utf-8').write('pkg-config ' + ' '.join(sys.argv[1:]) + '\\n')\n"
        "if sys.argv[1:] == ['--modversion', 'Qt6Core', 'Qt6Gui', 'Qt6Widgets', 'Qt6Test']:\n"
        "    print('Qt6 missing', file=sys.stderr)\n"
        "    raise SystemExit(17)\n"
        "print('pkg-config ok')\n",
    )

    result = run_doctor(
        env={
            "PATH": f"{bin_dir}{os.pathsep}{os.environ.get('PATH', '')}",
            marker: "propagated",
        }
    )

    assert result.returncode == 17
    assert order_file.read_text(encoding="utf-8").splitlines() == [
        f"git --version|cwd={REPO_ROOT}|env=propagated",
        f"cmake --version|cwd={REPO_ROOT}|env=propagated",
        f"ctest --version|cwd={REPO_ROOT}|env=propagated",
        "pkg-config --version",
        f"c++ --version|cwd={REPO_ROOT}|env=propagated",
        "pkg-config --modversion Qt6Core Qt6Gui Qt6Widgets Qt6Test",
    ]
    assert "Qt" in result.stdout
    assert "FAIL" in result.stdout
    assert "Qt6 missing" in result.stdout
    assert "opencv4" not in result.stdout
