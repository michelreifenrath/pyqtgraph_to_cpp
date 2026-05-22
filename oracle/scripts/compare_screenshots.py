#!/usr/bin/env python3
"""Compare two PNG screenshots and emit deterministic visual-diff metrics."""

from __future__ import annotations

import argparse
import binascii
import json
import struct
import sys
import zlib
from collections.abc import Iterable
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MAX_DECOMPRESSED_BYTES = 256 * 1024 * 1024


class PngError(ValueError):
    """Raised when a PNG cannot be parsed by this lightweight reader."""


def _bytes_per_pixel(color_type: int) -> int:
    if color_type == 0:
        return 1
    if color_type == 2:
        return 3
    if color_type == 4:
        return 2
    if color_type == 6:
        return 4
    raise PngError(f"unsupported PNG color type: {color_type}")


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa = abs(p - a)
    pb = abs(p - b)
    pc = abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png_rgba(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise PngError("not a PNG file")

    offset = len(PNG_SIGNATURE)
    width = height = color_type = None
    idat_parts: list[bytes] = []
    seen_iend = False

    while offset < len(data):
        if offset + 8 > len(data):
            raise PngError("truncated PNG chunk header")
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        chunk_type = data[offset + 4 : offset + 8]
        offset += 8
        if offset + length + 4 > len(data):
            raise PngError("truncated PNG chunk data")
        payload = data[offset : offset + length]
        crc_expected = struct.unpack(">I", data[offset + length : offset + length + 4])[
            0
        ]
        crc_actual = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
        if crc_actual != crc_expected:
            raise PngError(
                f"CRC mismatch in {chunk_type.decode('ascii', 'replace')} chunk"
            )
        offset += length + 4

        if chunk_type == b"IHDR":
            if width is not None:
                raise PngError("duplicate IHDR chunk")
            if length != 13:
                raise PngError("invalid IHDR length")
            (
                width,
                height,
                bit_depth,
                color_type,
                compression,
                filter_method,
                interlace,
            ) = struct.unpack(">IIBBBBB", payload)
            if width <= 0 or height <= 0:
                raise PngError("invalid PNG dimensions")
            if bit_depth != 8:
                raise PngError("only 8-bit PNG images are supported")
            if color_type not in (0, 2, 4, 6):
                raise PngError(f"unsupported PNG color type: {color_type}")
            if compression != 0 or filter_method != 0 or interlace != 0:
                raise PngError(
                    "unsupported PNG compression, filter, or interlace method"
                )
        elif chunk_type == b"IDAT":
            idat_parts.append(payload)
        elif chunk_type == b"IEND":
            seen_iend = True
            break

    if width is None or height is None or color_type is None:
        raise PngError("missing IHDR chunk")
    if not idat_parts:
        raise PngError("missing IDAT chunk")
    if not seen_iend:
        raise PngError("missing IEND chunk")

    bpp = _bytes_per_pixel(color_type)
    row_len = width * bpp
    expected = height * (row_len + 1)
    if expected > MAX_DECOMPRESSED_BYTES:
        raise PngError(
            f"PNG decompressed data would exceed limit: {expected} > {MAX_DECOMPRESSED_BYTES} bytes"
        )

    try:
        decompressor = zlib.decompressobj()
        raw = decompressor.decompress(b"".join(idat_parts), expected + 1)
        remaining = max(0, expected + 1 - len(raw))
        raw += decompressor.flush(remaining)
    except zlib.error as exc:
        raise PngError(f"invalid PNG zlib stream: {exc}") from exc
    if len(raw) > expected:
        raise PngError("PNG decompressed data exceeds expected image size")
    if not decompressor.eof:
        raise PngError("PNG zlib stream exceeds expected image size or is truncated")

    if len(raw) != expected:
        raise PngError("unexpected decompressed image data length")

    rows: list[bytes] = []
    previous = bytearray(row_len)
    pos = 0
    for _y in range(height):
        filter_type = raw[pos]
        pos += 1
        scanline = bytearray(raw[pos : pos + row_len])
        pos += row_len
        reconstructed = bytearray(row_len)
        for x in range(row_len):
            left = reconstructed[x - bpp] if x >= bpp else 0
            up = previous[x]
            up_left = previous[x - bpp] if x >= bpp else 0
            value = scanline[x]
            if filter_type == 0:
                recon = value
            elif filter_type == 1:
                recon = value + left
            elif filter_type == 2:
                recon = value + up
            elif filter_type == 3:
                recon = value + ((left + up) // 2)
            elif filter_type == 4:
                recon = value + _paeth(left, up, up_left)
            else:
                raise PngError(f"unsupported PNG scanline filter: {filter_type}")
            reconstructed[x] = recon & 0xFF
        rows.append(bytes(reconstructed))
        previous = reconstructed

    pixels: list[tuple[int, int, int, int]] = []
    for row in rows:
        for x in range(0, len(row), bpp):
            if color_type == 0:
                g = row[x]
                pixels.append((g, g, g, 255))
            elif color_type == 2:
                pixels.append((row[x], row[x + 1], row[x + 2], 255))
            elif color_type == 4:
                g = row[x]
                pixels.append((g, g, g, row[x + 1]))
            else:
                pixels.append((row[x], row[x + 1], row[x + 2], row[x + 3]))
    return width, height, pixels


def _png_chunk(chunk_type: bytes, payload: bytes) -> bytes:
    crc = binascii.crc32(chunk_type + payload) & 0xFFFFFFFF
    return (
        struct.pack(">I", len(payload)) + chunk_type + payload + struct.pack(">I", crc)
    )


def write_png_rgba(
    path: Path, width: int, height: int, pixels: Iterable[tuple[int, int, int, int]]
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    iterator = iter(pixels)
    for _y in range(height):
        row = bytearray([0])
        for _x in range(width):
            try:
                row.extend(next(iterator))
            except StopIteration as exc:
                raise ValueError("not enough pixels for PNG") from exc
        rows.append(bytes(row))
    compressed = zlib.compress(b"".join(rows), level=9)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    path.write_bytes(
        PNG_SIGNATURE
        + _png_chunk(b"IHDR", ihdr)
        + _png_chunk(b"IDAT", compressed)
        + _png_chunk(b"IEND", b"")
    )


def _dimension(width: int, height: int) -> dict[str, int]:
    return {"width": width, "height": height}


def compare_images(
    reference_path: Path,
    candidate_path: Path,
    diff_path: Path,
    tolerances: dict[str, float],
) -> dict[str, object]:
    ref_w, ref_h, ref_pixels = read_png_rgba(reference_path)
    cand_w, cand_h, cand_pixels = read_png_rgba(candidate_path)

    base: dict[str, object] = {
        "reference_path": str(reference_path),
        "candidate_path": str(candidate_path),
        "diff_image_path": str(diff_path),
        "dimensions": {
            "reference": _dimension(ref_w, ref_h),
            "candidate": _dimension(cand_w, cand_h),
        },
        "tolerances": tolerances,
    }

    if (ref_w, ref_h) != (cand_w, cand_h):
        write_png_rgba(diff_path, 1, 1, [(255, 0, 0, 255)])
        return {
            **base,
            "passed": False,
            "deterministic_verdict": "fail",
            "reason": "dimension_mismatch",
            "failed_tolerances": ["dimensions"],
            "mean_absolute_delta": None,
            "max_delta": None,
            "changed_pixel_percentage": None,
            "bounding_boxes": [],
        }

    total_delta = 0
    max_delta = 0
    changed_pixels = 0
    min_x = min_y = None
    max_x = max_y = None
    diff_pixels: list[tuple[int, int, int, int]] = []

    for idx, (ref_px, cand_px) in enumerate(zip(ref_pixels, cand_pixels, strict=True)):
        deltas = [abs(a - b) for a, b in zip(ref_px, cand_px, strict=True)]
        pixel_max = max(deltas)
        total_delta += sum(deltas)
        max_delta = max(max_delta, pixel_max)
        if pixel_max > 0:
            changed_pixels += 1
            x = idx % ref_w
            y = idx // ref_w
            min_x = x if min_x is None else min(min_x, x)
            min_y = y if min_y is None else min(min_y, y)
            max_x = x if max_x is None else max(max_x, x)
            max_y = y if max_y is None else max(max_y, y)
            intensity = max(64, pixel_max)
            diff_pixels.append((intensity, 0, 0, 255))
        else:
            diff_pixels.append((0, 0, 0, 255))

    pixel_count = ref_w * ref_h
    channel_count = pixel_count * 4
    mean_delta = total_delta / channel_count if channel_count else 0.0
    changed_percentage = (changed_pixels / pixel_count * 100.0) if pixel_count else 0.0
    boxes = []
    if (
        min_x is not None
        and min_y is not None
        and max_x is not None
        and max_y is not None
    ):
        boxes.append(
            {
                "x": min_x,
                "y": min_y,
                "width": max_x - min_x + 1,
                "height": max_y - min_y + 1,
            }
        )

    write_png_rgba(diff_path, ref_w, ref_h, diff_pixels)
    passed = (
        mean_delta <= tolerances["max_mean_delta"]
        and max_delta <= tolerances["max_pixel_delta"]
        and changed_percentage <= tolerances["max_changed_percent"]
    )
    failed = []
    if mean_delta > tolerances["max_mean_delta"]:
        failed.append("mean_absolute_delta")
    if max_delta > tolerances["max_pixel_delta"]:
        failed.append("max_delta")
    if changed_percentage > tolerances["max_changed_percent"]:
        failed.append("changed_pixel_percentage")

    return {
        **base,
        "passed": passed,
        "deterministic_verdict": "pass" if passed else "fail",
        "reason": "within_tolerance" if passed else "tolerance_exceeded",
        "failed_tolerances": failed,
        "mean_absolute_delta": mean_delta,
        "max_delta": max_delta,
        "changed_pixel_percentage": changed_percentage,
        "bounding_boxes": boxes,
    }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare two PNG screenshots and write visual diff metrics."
    )
    parser.add_argument("reference", type=Path, help="Reference PNG screenshot path")
    parser.add_argument("candidate", type=Path, help="Candidate PNG screenshot path")
    parser.add_argument(
        "--diff",
        type=Path,
        default=Path("reports/visual-diffs/diff.png"),
        help="Output diff PNG path",
    )
    parser.add_argument(
        "--metrics",
        type=Path,
        default=Path("reports/visual-diffs/metrics.json"),
        help="Output metrics JSON path",
    )
    parser.add_argument(
        "--max-mean-delta",
        type=float,
        default=0.0,
        help="Maximum allowed mean absolute channel delta",
    )
    parser.add_argument(
        "--max-pixel-delta",
        type=float,
        default=0.0,
        help="Maximum allowed per-channel pixel delta",
    )
    parser.add_argument(
        "--max-changed-percent",
        type=float,
        default=0.0,
        help="Maximum allowed percentage of changed pixels",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    tolerances = {
        "max_mean_delta": args.max_mean_delta,
        "max_pixel_delta": args.max_pixel_delta,
        "max_changed_percent": args.max_changed_percent,
    }
    try:
        metrics = compare_images(args.reference, args.candidate, args.diff, tolerances)
        args.metrics.parent.mkdir(parents=True, exist_ok=True)
        json_text = json.dumps(metrics, sort_keys=True, indent=2) + "\n"
        args.metrics.write_text(json_text, encoding="utf-8")
        sys.stdout.write(json_text)
        return 0 if metrics["passed"] else 1
    except (OSError, PngError, ValueError) as exc:
        print(f"compare_screenshots: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
