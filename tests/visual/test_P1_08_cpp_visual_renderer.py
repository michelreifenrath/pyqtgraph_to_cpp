from __future__ import annotations

import binascii
import json
import os
import struct
import subprocess
import sys
import zlib
from pathlib import Path

import pytest
from test_compare_screenshots import write_png

ROOT = Path(__file__).resolve().parents[2]
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
REFERENCE = ROOT / "oracle" / "fixtures" / "screenshots" / "SimplePlot.reference.png"
CHECK_VISUAL_ARTIFACTS = ROOT / "scripts" / "check_visual_artifacts"
REPORTS_ROOT = Path(
    os.environ.get("PG_VISUAL_REPORTS_ROOT", ROOT / "reports" / "visual" / "P1.08")
)
CASE_DIR = REPORTS_ROOT / "SimplePlot"


def _run_renderer(
    renderer: Path, output: Path, width: int = 800, height: int = 600
) -> dict[str, object]:
    result = subprocess.run(
        [
            str(renderer),
            "SimplePlot",
            "--output",
            str(output),
            "--width",
            str(width),
            "--height",
            str(height),
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
        env={**os.environ, "QT_QPA_PLATFORM": "offscreen"},
    )
    assert result.returncode == 0, result.stderr
    return json.loads(result.stdout)


def _paeth(left: int, up: int, up_left: int) -> int:
    estimate = left + up - up_left
    left_distance = abs(estimate - left)
    up_distance = abs(estimate - up)
    up_left_distance = abs(estimate - up_left)
    if left_distance <= up_distance and left_distance <= up_left_distance:
        return left
    if up_distance <= up_left_distance:
        return up
    return up_left


def _read_png_rgba(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    assert data.startswith(PNG_SIGNATURE)
    offset = len(PNG_SIGNATURE)
    width = height = color_type = None
    idat_parts = []
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        crc_expected = struct.unpack(
            ">I", data[offset + 8 + length : offset + 12 + length]
        )[0]
        assert binascii.crc32(chunk_type + payload) & 0xFFFFFFFF == crc_expected
        offset += 12 + length
        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, *_rest = struct.unpack(
                ">IIBBBBB", payload
            )
            assert bit_depth == 8
            assert color_type in (2, 6)
        elif chunk_type == b"IDAT":
            idat_parts.append(payload)
        elif chunk_type == b"IEND":
            break

    assert width is not None and height is not None and color_type is not None
    channels = 4 if color_type == 6 else 3
    row_length = width * channels
    raw = zlib.decompress(b"".join(idat_parts))
    rows = []
    previous = bytearray(row_length)
    position = 0
    for _row in range(height):
        filter_type = raw[position]
        position += 1
        scanline = bytearray(raw[position : position + row_length])
        position += row_length
        reconstructed = bytearray(row_length)
        for index, value in enumerate(scanline):
            left = reconstructed[index - channels] if index >= channels else 0
            up = previous[index]
            up_left = previous[index - channels] if index >= channels else 0
            if filter_type == 0:
                reconstructed[index] = value
            elif filter_type == 1:
                reconstructed[index] = (value + left) & 0xFF
            elif filter_type == 2:
                reconstructed[index] = (value + up) & 0xFF
            elif filter_type == 3:
                reconstructed[index] = (value + ((left + up) // 2)) & 0xFF
            elif filter_type == 4:
                reconstructed[index] = (value + _paeth(left, up, up_left)) & 0xFF
            else:
                raise AssertionError(f"unsupported PNG filter: {filter_type}")
        rows.append(bytes(reconstructed))
        previous = reconstructed

    rgba = bytearray()
    for row in rows:
        for index in range(0, len(row), channels):
            rgba.extend(row[index : index + 3])
            rgba.append(row[index + 3] if channels == 4 else 255)
    return width, height, bytes(rgba)


def _assert_semantic_plot_image(path: Path, *, width: int, height: int) -> None:
    actual_width, actual_height, rgba = _read_png_rgba(path)
    assert (actual_width, actual_height) == (width, height)

    rgb_pixels = [
        (rgba[index], rgba[index + 1], rgba[index + 2])
        for index in range(0, len(rgba), 4)
    ]
    unique_colors = set(rgb_pixels)
    assert len(unique_colors) >= 8, "image has too few colors to be a rendered plot"

    luminance = [
        0.2126 * red + 0.7152 * green + 0.0722 * blue for red, green, blue in rgb_pixels
    ]
    dark_pixels = sum(value < 35 for value in luminance)
    bright_pixels = sum(value > 180 for value in luminance)
    total_pixels = width * height
    assert dark_pixels / total_pixels > 0.35, "plot background is not present"
    assert bright_pixels > max(width, height), "plot curve/axes are not present"

    # The retired placeholder renderer used a tiny palette with border/diagonal stripes.
    # A semantic plot render must contain real antialiasing/axis/curve variation.
    assert len(unique_colors) > 12 or bright_pixels / total_pixels > 0.015


def _write_blank(path: Path) -> None:
    write_png(path, 16, 12, [(0, 0, 0, 255)] * 16 * 12)


def _write_placeholder_like(path: Path) -> None:
    width = 48
    height = 36
    background = (32, 36, 44, 255)
    accent = (180, 190, 200, 255)
    pixels = []
    for y in range(height):
        for x in range(width):
            on_border = x < 2 or y < 2 or x >= width - 2 or y >= height - 2
            on_diagonal = (x + y) % 8 < 2
            in_marker = 6 <= x < 14 and 5 <= y < 13
            pixels.append(
                accent if on_border or on_diagonal or in_marker else background
            )
    write_png(path, width, height, pixels)


def _existing_gpt_visual_review(tmp_path: Path) -> Path:
    review_path = Path(
        os.environ.get("PG_VISUAL_REVIEW_REPORT", CASE_DIR / "gpt5_vision_review.md")
    )
    assert review_path.is_file(), (
        "P1.08 requires an existing GPT visual review report; set "
        f"PG_VISUAL_REVIEW_REPORT or provide {review_path}"
    )

    review_copy = tmp_path / "gpt5_vision_review.md"
    review_copy.write_text(review_path.read_text(encoding="utf-8"), encoding="utf-8")
    return review_copy


def test_P1_08_blank_and_placeholder_guards_reject_non_semantic_images(
    tmp_path: Path,
) -> None:
    blank = tmp_path / "blank.png"
    placeholder = tmp_path / "placeholder.png"
    _write_blank(blank)
    _write_placeholder_like(placeholder)

    with pytest.raises(AssertionError, match="too few colors"):
        _assert_semantic_plot_image(blank, width=16, height=12)
    with pytest.raises(AssertionError, match="too few colors"):
        _assert_semantic_plot_image(placeholder, width=48, height=36)


def test_P1_08_native_renderer_writes_canonical_simpleplot_artifacts(
    tmp_path: Path,
) -> None:
    renderer_env = os.environ.get("PG_CPP_VISUAL_RENDERER")
    if not renderer_env:
        pytest.skip(
            "PG_CPP_VISUAL_RENDERER is provided by CTest; run "
            "`ctest --preset visual -L P1.08 --output-on-failure` for native proof"
        )
    renderer = Path(renderer_env)
    assert renderer.is_file(), f"PG_CPP_VISUAL_RENDERER does not exist: {renderer}"

    actual_source = tmp_path / "SimplePlot.actual.png"
    status = _run_renderer(renderer, actual_source)
    assert status["example"] == "SimplePlot"
    assert status["dimensions"] == {"width": 800, "height": 600}
    assert status["placeholder"] is False
    assert status["render_path"] == "QWidget::grab"
    assert Path(str(status["output"])).resolve() == actual_source.resolve()
    assert actual_source.is_file()
    _assert_semantic_plot_image(actual_source, width=800, height=600)

    CASE_DIR.mkdir(parents=True, exist_ok=True)
    review_source = _existing_gpt_visual_review(tmp_path)
    for artifact_name in (
        "reference.png",
        "actual.png",
        "diff.png",
        "metrics.json",
    ):
        (CASE_DIR / artifact_name).unlink(missing_ok=True)
    result = subprocess.run(
        [
            sys.executable,
            str(CHECK_VISUAL_ARTIFACTS),
            "--case",
            "SimplePlot",
            "--reference",
            str(REFERENCE),
            "--actual",
            str(actual_source),
            "--reports-root",
            str(REPORTS_ROOT),
            "--gpt-visual-review",
            "required_for_pr",
            "--review-report",
            str(review_source),
            "--max-mean-delta",
            "6",
            "--max-pixel-delta",
            "220",
            "--max-changed-percent",
            "5",
            "--min-ssim",
            "0.8",
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr

    reference = CASE_DIR / "reference.png"
    actual = CASE_DIR / "actual.png"
    diff = CASE_DIR / "diff.png"
    metrics_path = CASE_DIR / "metrics.json"
    review = CASE_DIR / "gpt5_vision_review.md"
    for artifact in (reference, actual, diff, metrics_path, review):
        assert artifact.is_file(), f"missing artifact: {artifact}"
    assert review.read_text(encoding="utf-8") == review_source.read_text(
        encoding="utf-8"
    )

    _assert_semantic_plot_image(actual, width=800, height=600)
    metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert metrics["case"] == "SimplePlot"
    assert metrics["dimensions"] == [800, 600]
    assert metrics["passed"] is True
    assert metrics["deterministic_verdict"] == "pass"
    assert metrics["review_report_path"] == str(review)
    assert metrics["semantic_review"] == {
        "mode": "required_for_pr",
        "verdict": "pass",
        "recommendation": "merge_ok",
        "accepted": True,
        "blocks_gate": False,
        "failure_reason": None,
    }
    assert metrics["failed_checks"] == []
    assert metrics["tolerance"] == {
        "max_mean_delta": 6.0,
        "max_pixel_delta": 220.0,
        "max_changed_pixel_percent": 5.0,
        "min_ssim": 0.8,
    }
    assert metrics["artifact_paths"] == {
        "reference": str(reference),
        "actual": str(actual),
        "diff": str(diff),
        "metrics": str(metrics_path),
    }
