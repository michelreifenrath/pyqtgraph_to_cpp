from __future__ import annotations

import json
import struct
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "oracle" / "scripts" / "render_cpp_example.py"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def run_cli(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def read_png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    assert data.startswith(PNG_SIGNATURE)
    length = struct.unpack(">I", data[len(PNG_SIGNATURE) : len(PNG_SIGNATURE) + 4])[0]
    kind = data[len(PNG_SIGNATURE) + 4 : len(PNG_SIGNATURE) + 8]
    assert length == 13
    assert kind == b"IHDR"
    width, height, bit_depth, color_type, compression, filter_method, interlace = (
        struct.unpack(
            ">IIBBBBB", data[len(PNG_SIGNATURE) + 8 : len(PNG_SIGNATURE) + 21]
        )
    )
    assert bit_depth == 8
    assert color_type == 6
    assert compression == 0
    assert filter_method == 0
    assert interlace == 0
    return width, height


def test_help_exposes_renderer_cli_options() -> None:
    result = run_cli("--help")

    assert result.returncode == 0, result.stderr
    assert "example" in result.stdout
    assert "--output" in result.stdout
    assert "--width" in result.stdout
    assert "--height" in result.stdout
    assert "C++" in result.stdout
    assert "placeholder" in result.stdout


def test_placeholder_png_is_deterministic_for_same_arguments(tmp_path: Path) -> None:
    first = tmp_path / "first.png"
    second = tmp_path / "second.png"

    first_result = run_cli(
        "SimplePlot", "--output", str(first), "--width", "320", "--height", "240"
    )
    second_result = run_cli(
        "SimplePlot", "--output", str(second), "--width", "320", "--height", "240"
    )

    assert first_result.returncode == 0, first_result.stderr
    assert second_result.returncode == 0, second_result.stderr
    assert first_result.stderr == ""
    assert second_result.stderr == ""
    assert first.exists()
    assert second.exists()
    assert first.read_bytes() == second.read_bytes()
    assert read_png_dimensions(first) == (320, 240)
    status = json.loads(first_result.stdout)
    assert status["example"] == "SimplePlot"
    assert status["dimensions"] == {"width": 320, "height": 240}
    assert status["placeholder"] is True


def test_default_dimensions_and_nested_output_directory(tmp_path: Path) -> None:
    output = tmp_path / "nested" / "actual.png"

    result = run_cli("SimplePlot", "--output", str(output))

    assert result.returncode == 0, result.stderr
    assert result.stderr == ""
    assert output.exists()
    assert read_png_dimensions(output) == (800, 600)


@pytest.mark.parametrize(
    ("args", "filename"),
    [
        (("--width", "0"), "zero-width.png"),
        (("--height", "-1"), "negative-height.png"),
    ],
)
def test_invalid_dimensions_fail_without_writing(
    tmp_path: Path, args: tuple[str, str], filename: str
) -> None:
    output = tmp_path / filename

    result = run_cli("SimplePlot", "--output", str(output), *args)

    assert result.returncode != 0
    assert "error:" in result.stderr
    assert not output.exists()
