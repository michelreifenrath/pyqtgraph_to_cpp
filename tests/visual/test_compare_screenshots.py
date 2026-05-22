from __future__ import annotations

import binascii
import json
import struct
import subprocess
import sys
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "oracle" / "scripts" / "compare_screenshots.py"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def _chunk(kind: bytes, payload: bytes) -> bytes:
    crc = binascii.crc32(kind + payload) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)


def write_png(
    path: Path, width: int, height: int, pixels: list[tuple[int, ...]]
) -> None:
    if len(pixels) != width * height:
        raise ValueError("pixel count does not match dimensions")
    channel_count = len(pixels[0])
    if channel_count not in (3, 4):
        raise ValueError("test helper supports RGB or RGBA pixels")
    color_type = 2 if channel_count == 3 else 6
    rows = []
    for y in range(height):
        row = bytearray([0])
        for x in range(width):
            row.extend(pixels[y * width + x])
        rows.append(bytes(row))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)
    path.write_bytes(
        PNG_SIGNATURE
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(b"".join(rows)))
        + _chunk(b"IEND", b"")
    )


def read_png_rgba(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    data = path.read_bytes()
    assert data.startswith(PNG_SIGNATURE)
    offset = len(PNG_SIGNATURE)
    width = height = color_type = None
    idat = []
    while offset < len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload = data[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, _bit_depth, color_type, *_rest = struct.unpack(
                ">IIBBBBB", payload
            )
        elif kind == b"IDAT":
            idat.append(payload)
        elif kind == b"IEND":
            break
    assert width is not None and height is not None and color_type is not None
    raw = zlib.decompress(b"".join(idat))
    channels = 4 if color_type == 6 else 3
    pixels = []
    pos = 0
    for _y in range(height):
        assert raw[pos] == 0
        pos += 1
        for _x in range(width):
            values = tuple(raw[pos : pos + channels])
            pos += channels
            if channels == 3:
                pixels.append((values[0], values[1], values[2], 255))
            else:
                pixels.append(values)
    return width, height, pixels


def run_compare(
    reference: Path, candidate: Path, diff: Path, metrics: Path, *extra: str
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            sys.executable,
            str(SCRIPT),
            str(reference),
            str(candidate),
            "--diff",
            str(diff),
            "--metrics",
            str(metrics),
            *extra,
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def test_identical_images_pass_and_write_matching_metrics(tmp_path: Path) -> None:
    reference = tmp_path / "reference.png"
    candidate = tmp_path / "candidate.png"
    diff = tmp_path / "diff.png"
    metrics_path = tmp_path / "metrics.json"
    pixels = [(10, 20, 30), (40, 50, 60), (70, 80, 90), (100, 110, 120)]
    write_png(reference, 2, 2, pixels)
    write_png(candidate, 2, 2, pixels)

    result = run_compare(reference, candidate, diff, metrics_path)

    assert result.returncode == 0, result.stderr
    assert result.stderr == ""
    stdout_metrics = json.loads(result.stdout)
    file_metrics = json.loads(metrics_path.read_text(encoding="utf-8"))
    assert stdout_metrics == file_metrics
    assert set(stdout_metrics) == {
        "bounding_boxes",
        "candidate_path",
        "changed_pixel_percentage",
        "diff_image_path",
        "dimensions",
        "deterministic_verdict",
        "failed_tolerances",
        "max_delta",
        "mean_absolute_delta",
        "passed",
        "reason",
        "reference_path",
        "tolerances",
    }
    assert stdout_metrics["passed"] is True
    assert stdout_metrics["deterministic_verdict"] == "pass"
    assert stdout_metrics["reason"] == "within_tolerance"
    assert stdout_metrics["dimensions"] == {
        "reference": {"width": 2, "height": 2},
        "candidate": {"width": 2, "height": 2},
    }
    assert stdout_metrics["mean_absolute_delta"] == 0
    assert stdout_metrics["max_delta"] == 0
    assert stdout_metrics["changed_pixel_percentage"] == 0
    assert stdout_metrics["bounding_boxes"] == []
    assert stdout_metrics["failed_tolerances"] == []
    assert stdout_metrics["diff_image_path"] == str(diff)
    assert stdout_metrics["tolerances"] == {
        "max_mean_delta": 0.0,
        "max_pixel_delta": 0.0,
        "max_changed_percent": 0.0,
    }
    assert read_png_rgba(diff) == (2, 2, [(0, 0, 0, 255)] * 4)


def test_one_pixel_difference_can_pass_with_configured_tolerances(
    tmp_path: Path,
) -> None:
    reference = tmp_path / "reference.png"
    candidate = tmp_path / "candidate.png"
    diff = tmp_path / "diff.png"
    metrics_path = tmp_path / "metrics.json"
    write_png(reference, 2, 2, [(0, 0, 0, 255)] * 4)
    write_png(
        candidate,
        2,
        2,
        [(0, 0, 0, 255), (10, 0, 0, 255), (0, 0, 0, 255), (0, 0, 0, 255)],
    )

    result = run_compare(
        reference,
        candidate,
        diff,
        metrics_path,
        "--max-mean-delta",
        "0.625",
        "--max-pixel-delta",
        "10",
        "--max-changed-percent",
        "25",
    )

    assert result.returncode == 0, result.stderr
    metrics = json.loads(result.stdout)
    assert metrics["passed"] is True
    assert metrics["mean_absolute_delta"] == 0.625
    assert metrics["max_delta"] == 10
    assert metrics["changed_pixel_percentage"] == 25.0
    assert metrics["bounding_boxes"] == [{"x": 1, "y": 0, "width": 1, "height": 1}]
    assert read_png_rgba(diff)[2][1] == (64, 0, 0, 255)


def test_one_pixel_difference_fails_when_tolerance_is_exceeded(tmp_path: Path) -> None:
    reference = tmp_path / "reference.png"
    candidate = tmp_path / "candidate.png"
    diff = tmp_path / "diff.png"
    metrics_path = tmp_path / "metrics.json"
    write_png(reference, 2, 2, [(0, 0, 0)] * 4)
    write_png(candidate, 2, 2, [(0, 0, 0), (11, 0, 0), (0, 0, 0), (0, 0, 0)])

    result = run_compare(
        reference,
        candidate,
        diff,
        metrics_path,
        "--max-mean-delta",
        "1",
        "--max-pixel-delta",
        "10",
        "--max-changed-percent",
        "25",
    )

    assert result.returncode == 1
    metrics = json.loads(result.stdout)
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert metrics["reason"] == "tolerance_exceeded"
    assert metrics["failed_tolerances"] == ["max_delta"]
    assert metrics["max_delta"] == 11
    assert metrics["bounding_boxes"] == [{"x": 1, "y": 0, "width": 1, "height": 1}]


def test_dimension_mismatch_fails_with_stable_metrics(tmp_path: Path) -> None:
    reference = tmp_path / "reference.png"
    candidate = tmp_path / "candidate.png"
    diff = tmp_path / "diff.png"
    metrics_path = tmp_path / "metrics.json"
    write_png(reference, 2, 1, [(0, 0, 0), (0, 0, 0)])
    write_png(candidate, 1, 2, [(0, 0, 0), (0, 0, 0)])

    result = run_compare(reference, candidate, diff, metrics_path)

    assert result.returncode == 1
    metrics = json.loads(result.stdout)
    assert metrics["passed"] is False
    assert metrics["deterministic_verdict"] == "fail"
    assert metrics["reason"] == "dimension_mismatch"
    assert metrics["dimensions"] == {
        "reference": {"width": 2, "height": 1},
        "candidate": {"width": 1, "height": 2},
    }
    assert metrics["mean_absolute_delta"] is None
    assert metrics["max_delta"] is None
    assert metrics["changed_pixel_percentage"] is None
    assert metrics["failed_tolerances"] == ["dimensions"]
    assert metrics["bounding_boxes"] == []
    assert read_png_rgba(diff) == (1, 1, [(255, 0, 0, 255)])


def test_oversized_png_is_rejected_before_unbounded_decompression(
    tmp_path: Path,
) -> None:
    reference = tmp_path / "huge.png"
    candidate = tmp_path / "candidate.png"
    # Declares >256 MiB of decompressed scanline data but contains only a tiny
    # zlib stream. The CLI must reject from IHDR-derived size limits, not try to
    # inflate arbitrary crafted input first.
    ihdr = struct.pack(">IIBBBBB", 70_000, 1_000, 8, 6, 0, 0, 0)
    reference.write_bytes(
        PNG_SIGNATURE
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(b""))
        + _chunk(b"IEND", b"")
    )
    write_png(candidate, 1, 1, [(0, 0, 0)])

    result = run_compare(
        reference, candidate, tmp_path / "diff.png", tmp_path / "metrics.json"
    )

    assert result.returncode == 2
    assert "decompressed data would exceed limit" in result.stderr


def test_missing_or_malformed_input_exits_two_with_useful_stderr(
    tmp_path: Path,
) -> None:
    reference = tmp_path / "missing.png"
    candidate = tmp_path / "candidate.png"
    candidate.write_text("not a png", encoding="utf-8")

    result = run_compare(
        reference, candidate, tmp_path / "diff.png", tmp_path / "metrics.json"
    )

    assert result.returncode == 2
    assert result.stdout == ""
    assert "compare_screenshots:" in result.stderr
    assert "No such file" in result.stderr or "not a PNG" in result.stderr
