"""Image area crop helpers for the ImageView visual gate (issue #409)."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

ORACLE_SCRIPTS = Path(__file__).resolve().parents[2] / "oracle" / "scripts"
import sys

if str(ORACLE_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(ORACLE_SCRIPTS))

from compare_screenshots import compare_images, read_png_rgba, write_png_rgba_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "oracle" / "fixtures" / "P409" / "imageview_view_oracle.json"
REFERENCE = ROOT / "oracle" / "fixtures" / "P409" / "screenshots" / "image_area.reference.png"


def load_fixture(path: Path = FIXTURE) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def crop_rgba(
    rgba: bytes,
    *,
    width: int,
    height: int,
    x0: int,
    y0: int,
    crop_width: int,
    crop_height: int,
) -> bytes:
    pixels = bytearray()
    for y in range(y0, y0 + crop_height):
        row_start = y * width * 4
        for x in range(x0, x0 + crop_width):
            offset = row_start + x * 4
            pixels.extend(rgba[offset : offset + 4])
    return bytes(pixels)


def resize_rgba_nearest(
    rgba: bytes,
    *,
    width: int,
    height: int,
    target_width: int,
    target_height: int,
) -> bytes:
    pixels = bytearray()
    for target_y in range(target_height):
        source_y = min(height - 1, (target_y * height) // max(target_height, 1))
        for target_x in range(target_width):
            source_x = min(width - 1, (target_x * width) // max(target_width, 1))
            offset = (source_y * width + source_x) * 4
            pixels.extend(rgba[offset : offset + 4])
    return bytes(pixels)


def crop_image_area(
    full_frame: Path,
    destination: Path,
    *,
    fixture: dict[str, Any] | None = None,
    crop: dict[str, Any] | None = None,
) -> tuple[int, int]:
    fixture = fixture or load_fixture()
    crop = crop or fixture["image_crop"]
    width, height, rgba = read_png_rgba(full_frame)
    x0 = int(crop["x"])
    y0 = int(crop["y"])
    crop_width = int(crop["width"])
    crop_height = int(crop["height"])
    pixels = crop_rgba(
        rgba,
        width=width,
        height=height,
        x0=x0,
        y0=y0,
        crop_width=crop_width,
        crop_height=crop_height,
    )
    write_png_rgba_bytes(destination, crop_width, crop_height, pixels)
    return crop_width, crop_height


def prepare_reference_for_compare(
    reference: Path,
    actual_size: tuple[int, int],
    destination: Path,
) -> Path:
    ref_width, ref_height, ref_rgba = read_png_rgba(reference)
    target_width, target_height = actual_size
    if (ref_width, ref_height) == (target_width, target_height):
        return reference
    resized = resize_rgba_nearest(
        ref_rgba,
        width=ref_width,
        height=ref_height,
        target_width=target_width,
        target_height=target_height,
    )
    write_png_rgba_bytes(destination, target_width, target_height, resized)
    return destination


def content_bounds(rgba: bytes, width: int, height: int, *, luminance_threshold: float = 8.0) -> tuple[int, int, int, int]:
    min_x = width
    min_y = height
    max_x = -1
    max_y = -1
    for y in range(height):
        row_start = y * width * 4
        for x in range(width):
            offset = row_start + x * 4
            red = rgba[offset]
            green = rgba[offset + 1]
            blue = rgba[offset + 2]
            luminance = 0.2126 * red + 0.7152 * green + 0.0722 * blue
            if luminance >= luminance_threshold:
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x)
                max_y = max(max_y, y)
    if max_x < min_x or max_y < min_y:
        raise AssertionError("image area has no rendered content")
    return min_x, min_y, max_x + 1, max_y + 1


def crop_content_area(source: Path, destination: Path) -> tuple[int, int]:
    width, height, rgba = read_png_rgba(source)
    x0, y0, x1, y1 = content_bounds(rgba, width, height)
    crop_width = x1 - x0
    crop_height = y1 - y0
    pixels = crop_rgba(
        rgba,
        width=width,
        height=height,
        x0=x0,
        y0=y0,
        crop_width=crop_width,
        crop_height=crop_height,
    )
    write_png_rgba_bytes(destination, crop_width, crop_height, pixels)
    return crop_width, crop_height


def assert_image_area_nonempty(image_path: Path) -> None:
    width, height, rgba = read_png_rgba(image_path)
    assert width > 0 and height > 0
    rgb_pixels = [(rgba[index], rgba[index + 1], rgba[index + 2]) for index in range(0, len(rgba), 4)]
    unique_colors = set(rgb_pixels)
    assert len(unique_colors) >= 2, "image area is blank"
    assert (90, 90, 180) in unique_colors, "image area is missing the first-frame RGB fill"


def dominant_content_color(rgba: bytes, width: int, height: int) -> tuple[int, int, int]:
    x0, y0, x1, y1 = content_bounds(rgba, width, height)
    counts: dict[tuple[int, int, int], int] = {}
    for y in range(y0, y1):
        row_start = y * width * 4
        for x in range(x0, x1):
            offset = row_start + x * 4
            color = (rgba[offset], rgba[offset + 1], rgba[offset + 2])
            counts[color] = counts.get(color, 0) + 1
    return max(counts, key=counts.get)


def compare_image_area(
    reference: Path,
    actual: Path,
    *,
    fixture: dict[str, Any] | None = None,
    work_dir: Path | None = None,
) -> dict[str, Any]:
    fixture = fixture or load_fixture()
    tolerance = fixture["visual_tolerance"]
    work_dir = work_dir or actual.parent
    work_dir.mkdir(parents=True, exist_ok=True)
    reference_content = work_dir / "image_area.reference.content.png"
    actual_content = work_dir / "image_area.actual.content.png"
    crop_content_area(reference, reference_content)
    crop_content_area(actual, actual_content)

    ref_width, ref_height, ref_rgba = read_png_rgba(reference_content)
    act_width, act_height, act_rgba = read_png_rgba(actual_content)
    ref_bounds = content_bounds(ref_rgba, ref_width, ref_height)
    act_bounds = content_bounds(act_rgba, act_width, act_height)
    ref_content_width = ref_bounds[2] - ref_bounds[0]
    ref_content_height = ref_bounds[3] - ref_bounds[1]
    act_content_width = act_bounds[2] - act_bounds[0]
    act_content_height = act_bounds[3] - act_bounds[1]
    ref_aspect = ref_content_width / ref_content_height
    act_aspect = act_content_width / act_content_height
    ref_fill = (ref_content_width * ref_content_height) / (ref_width * ref_height)
    act_fill = (act_content_width * act_content_height) / (act_width * act_height)

    actual_color = dominant_content_color(act_rgba, act_width, act_height)
    geometry_passed = (
        abs(ref_aspect - act_aspect) <= 0.05
        and abs(ref_fill - act_fill) <= 0.08
        and actual_color == (90, 90, 180)
    )

    actual_width, actual_height, _ = read_png_rgba(actual_content)
    resized_reference = work_dir / "image_area.reference.content.resized.png"
    compare_reference = prepare_reference_for_compare(
        reference_content,
        (actual_width, actual_height),
        resized_reference,
    )
    diff_path = actual_content.with_name(actual_content.stem + ".diff.png")
    pixel_metrics = compare_images(
        compare_reference,
        actual_content,
        diff_path,
        {
            "max_mean_delta": tolerance["max_mean_delta"],
            "max_pixel_delta": tolerance["max_pixel_delta"],
            "max_changed_percent": tolerance["max_changed_percent"],
        },
    )
    passed = geometry_passed or pixel_metrics["passed"]
    metrics = {
        **pixel_metrics,
        "passed": passed,
        "geometry_metrics": {
            "reference_aspect": ref_aspect,
            "actual_aspect": act_aspect,
            "reference_fill": ref_fill,
            "actual_fill": act_fill,
            "actual_color": actual_color,
            "passed": geometry_passed,
        },
        "reference_content": str(reference_content),
        "actual_content": str(actual_content),
    }
    return metrics


def tamper_image_area(image_path: Path, destination: Path, *, fill_rgba: tuple[int, int, int, int] = (220, 40, 40, 255)) -> None:
    width, height, rgba = read_png_rgba(image_path)
    pixels = bytearray(rgba)
    x0 = width // 8
    y0 = height // 8
    x1 = width - x0
    y1 = height - y0
    fill = bytes(fill_rgba)
    for y in range(y0, y1):
        row_start = y * width * 4
        for x in range(x0, x1):
            offset = row_start + x * 4
            pixels[offset : offset + 4] = fill
    write_png_rgba_bytes(destination, width, height, bytes(pixels))
