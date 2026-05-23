#!/usr/bin/env python3
"""Render deterministic placeholder screenshots for future C++ examples."""

from __future__ import annotations

import argparse
import binascii
import hashlib
import json
import struct
import sys
import zlib
from pathlib import Path

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
DEFAULT_WIDTH = 800
DEFAULT_HEIGHT = 600


def _chunk(kind: bytes, payload: bytes) -> bytes:
    crc = binascii.crc32(kind + payload) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", crc)


def _palette(example: str) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
    digest = hashlib.sha256(example.encode("utf-8")).digest()
    background = (
        24 + digest[0] % 48,
        28 + digest[1] % 48,
        36 + digest[2] % 48,
    )
    accent = (
        128 + digest[3] % 96,
        128 + digest[4] % 96,
        128 + digest[5] % 96,
    )
    return background, accent


def write_placeholder_png(path: Path, width: int, height: int, example: str) -> None:
    """Write a deterministic RGBA placeholder PNG."""
    background, accent = _palette(example)
    rows: list[bytes] = []
    stripe_period = max(16, min(width, height) // 6 or 16)

    for y in range(height):
        row = bytearray([0])
        for x in range(width):
            on_border = x < 4 or y < 4 or x >= width - 4 or y >= height - 4
            on_diagonal = (x + y) % stripe_period < max(2, stripe_period // 12)
            in_marker = width // 8 <= x < width // 8 + max(
                8, width // 12
            ) and height // 8 <= y < height // 8 + max(8, height // 12)
            if on_border or on_diagonal or in_marker:
                red, green, blue = accent
            else:
                red, green, blue = background
            row.extend((red, green, blue, 255))
        rows.append(bytes(row))

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        PNG_SIGNATURE
        + _chunk(b"IHDR", ihdr)
        + _chunk(b"IDAT", zlib.compress(b"".join(rows), level=9))
        + _chunk(b"IEND", b"")
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Render a deterministic placeholder screenshot for a future C++ "
            "example. This dependency-free oracle helper stands in until the "
            "native C++ renderer is available."
        )
    )
    parser.add_argument("example", help="C++ example name to render")
    parser.add_argument(
        "--output",
        required=True,
        type=Path,
        help="PNG screenshot path to write",
    )
    parser.add_argument(
        "--width",
        type=int,
        default=DEFAULT_WIDTH,
        help=f"placeholder image width in pixels (default: {DEFAULT_WIDTH})",
    )
    parser.add_argument(
        "--height",
        type=int,
        default=DEFAULT_HEIGHT,
        help=f"placeholder image height in pixels (default: {DEFAULT_HEIGHT})",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if args.width <= 0:
        print("error: --width must be a positive integer", file=sys.stderr)
        return 1
    if args.height <= 0:
        print("error: --height must be a positive integer", file=sys.stderr)
        return 1

    try:
        write_placeholder_png(args.output, args.width, args.height, args.example)
    except OSError as exc:
        print(f"error: failed to write output: {exc}", file=sys.stderr)
        return 1

    status = {
        "example": args.example,
        "output": str(args.output),
        "dimensions": {"width": args.width, "height": args.height},
        "placeholder": True,
    }
    print(json.dumps(status, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
