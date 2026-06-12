"""Per-subplot visual helpers for the Plotting example gate (issue #402)."""

from __future__ import annotations

from contextlib import contextmanager
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterator

ORACLE_SCRIPTS = Path(__file__).resolve().parents[2] / "oracle" / "scripts"
import sys

if str(ORACLE_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(ORACLE_SCRIPTS))

from compare_screenshots import compare_images, read_png_rgba, write_png_rgba_bytes  # noqa: E402

PLOT_NAMES = ("p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9")
IMAGE_WIDTH = 1000
IMAGE_HEIGHT = 600
GRID_COLS = 3
GRID_ROWS = 3
CELL_INSET = 10

# Whole-image thresholds are calibrated on pinned PyQtGraph reference vs C++
# output with identical deterministic data (seed 0x504C5454). Numeric values are
# looser than the retired whole-window gate (14/21/0.47); tightening comes from
# per-subplot pixel gates below plus per-cell non-emptiness checks.
WHOLE_IMAGE_TOLERANCE = {
    "max_mean_delta": 20.0,
    "max_pixel_delta": 255.0,
    "max_changed_percent": 25.0,
    "min_ssim": 0.40,
}

# Per-subplot pixel thresholds are measured per cell from the same good render.
# p6 is timer-driven, so it uses non-emptiness only.
SUBPLOT_PIXEL_TOLERANCE: dict[str, dict[str, float] | None] = {
    "p1": {"max_mean_delta": 22.0, "max_changed_percent": 25.0},
    "p2": {"max_mean_delta": 19.0, "max_changed_percent": 27.0},
    "p3": {"max_mean_delta": 36.0, "max_changed_percent": 28.0},
    "p4": {"max_mean_delta": 14.0, "max_changed_percent": 28.0},
    "p5": {"max_mean_delta": 26.0, "max_changed_percent": 42.0},
    "p6": None,
    "p7": {"max_mean_delta": 22.0, "max_changed_percent": 36.0},
    "p8": {"max_mean_delta": 12.0, "max_changed_percent": 12.0},
    "p9": {"max_mean_delta": 8.0, "max_changed_percent": 8.0},
}


@dataclass(frozen=True)
class SubplotCell:
    name: str
    row: int
    col: int


SUBPLOT_CELLS = tuple(
    SubplotCell(name, index // GRID_COLS, index % GRID_COLS)
    for index, name in enumerate(PLOT_NAMES)
)


def _cell_bounds(
    width: int,
    height: int,
    *,
    col: int,
    row: int,
    inset: int = CELL_INSET,
) -> tuple[int, int, int, int]:
    cell_width = width // GRID_COLS
    cell_height = height // GRID_ROWS
    x0 = col * cell_width + inset
    y0 = row * cell_height + inset
    x1 = (col + 1) * cell_width - inset if col < GRID_COLS - 1 else width
    y1 = (row + 1) * cell_height - inset if row < GRID_ROWS - 1 else height
    return x0, y0, x1, y1


def crop_subplot_rgba(
    rgba: bytes,
    width: int,
    height: int,
    *,
    col: int,
    row: int,
) -> tuple[int, int, bytes]:
    x0, y0, x1, y1 = _cell_bounds(width, height, col=col, row=row)
    crop_width = x1 - x0
    crop_height = y1 - y0
    pixels = bytearray()
    for y in range(y0, y1):
        row_start = y * width * 4
        for x in range(x0, x1):
            offset = row_start + x * 4
            pixels.extend(rgba[offset : offset + 4])
    return crop_width, crop_height, bytes(pixels)


def write_subplot_png(
    source: Path,
    destination: Path,
    *,
    col: int,
    row: int,
) -> tuple[int, int]:
    width, height, rgba = read_png_rgba(source)
    crop_width, crop_height, crop_rgba = crop_subplot_rgba(
        rgba, width, height, col=col, row=row
    )
    write_png_rgba_bytes(destination, crop_width, crop_height, crop_rgba)
    return crop_width, crop_height


def assert_subplot_nonempty(
    rgba: bytes,
    *,
    width: int,
    height: int,
    col: int,
    row: int,
    name: str,
) -> None:
    crop_width, crop_height, crop_rgba = crop_subplot_rgba(
        rgba, width, height, col=col, row=row
    )
    rgb_pixels = [
        (crop_rgba[index], crop_rgba[index + 1], crop_rgba[index + 2])
        for index in range(0, len(crop_rgba), 4)
    ]
    unique_colors = set(rgb_pixels)
    assert len(unique_colors) >= 6, f"{name} subplot has too few colors to be rendered"

    luminance = [
        0.2126 * red + 0.7152 * green + 0.0722 * blue
        for red, green, blue in rgb_pixels
    ]
    pixel_count = crop_width * crop_height
    dark_pixels = sum(value < 35 for value in luminance)
    bright_pixels = sum(value > 180 for value in luminance)
    assert dark_pixels / pixel_count > 0.30, f"{name} subplot background is missing"
    assert bright_pixels > max(crop_width, crop_height) // 4, (
        f"{name} subplot curve/axes content is missing"
    )


def assert_all_subplots_nonempty(image_path: Path) -> None:
    width, height, rgba = read_png_rgba(image_path)
    assert (width, height) == (IMAGE_WIDTH, IMAGE_HEIGHT)
    for cell in SUBPLOT_CELLS:
        assert_subplot_nonempty(
            rgba,
            width=width,
            height=height,
            col=cell.col,
            row=cell.row,
            name=cell.name,
        )


def _compare_subplot_crop(
    ref_crop: tuple[int, int, bytes],
    act_crop: tuple[int, int, bytes],
    *,
    tolerance: dict[str, float],
    work_dir: Path,
    name: str,
) -> dict[str, Any]:
    ref_path = work_dir / f"{name}.reference.png"
    act_path = work_dir / f"{name}.actual.png"
    diff_path = work_dir / f"{name}.diff.png"
    write_png_rgba_bytes(ref_path, ref_crop[0], ref_crop[1], ref_crop[2])
    write_png_rgba_bytes(act_path, act_crop[0], act_crop[1], act_crop[2])
    return compare_images(
        ref_path,
        act_path,
        diff_path,
        {
            "max_mean_delta": tolerance["max_mean_delta"],
            "max_pixel_delta": 255.0,
            "max_changed_percent": tolerance["max_changed_percent"],
        },
    )


@contextmanager
def _subplot_work_dir(reports_dir: Path | None) -> Iterator[Path]:
    if reports_dir is not None:
        reports_dir.mkdir(parents=True, exist_ok=True)
        yield reports_dir
        return

    import tempfile

    with tempfile.TemporaryDirectory() as tmp:
        yield Path(tmp)


def compare_subplots(
    reference: Path,
    actual: Path,
    *,
    reports_dir: Path | None = None,
) -> dict[str, Any]:
    ref_width, ref_height, ref_rgba = read_png_rgba(reference)
    act_width, act_height, act_rgba = read_png_rgba(actual)
    assert (ref_width, ref_height) == (act_width, act_height) == (
        IMAGE_WIDTH,
        IMAGE_HEIGHT,
    )

    failed_cells: list[str] = []
    cell_metrics: dict[str, Any] = {}
    with _subplot_work_dir(reports_dir) as work_path:
        for cell in SUBPLOT_CELLS:
            tolerance = SUBPLOT_PIXEL_TOLERANCE[cell.name]
            if tolerance is None:
                continue

            ref_crop = crop_subplot_rgba(
                ref_rgba, ref_width, ref_height, col=cell.col, row=cell.row
            )
            act_crop = crop_subplot_rgba(
                act_rgba, act_width, act_height, col=cell.col, row=cell.row
            )
            metrics = _compare_subplot_crop(
                ref_crop,
                act_crop,
                tolerance=tolerance,
                work_dir=work_path,
                name=cell.name,
            )
            cell_metrics[cell.name] = {
                "tolerance": tolerance,
                "mean_absolute_delta": metrics["mean_absolute_delta"],
                "changed_pixel_percentage": metrics["changed_pixel_percentage"],
                "passed": metrics["passed"],
            }
            if not metrics["passed"]:
                failed_cells.append(cell.name)

    return {
        "passed": not failed_cells,
        "failed_cells": failed_cells,
        "cells": cell_metrics,
    }


def blank_subplot(
    image_path: Path,
    destination: Path,
    *,
    col: int,
    row: int,
    fill_rgba: tuple[int, int, int, int] = (32, 36, 44, 255),
) -> None:
    width, height, rgba = read_png_rgba(image_path)
    pixels = bytearray(rgba)
    x0, y0, x1, y1 = _cell_bounds(width, height, col=col, row=row)
    fill = bytes(fill_rgba)
    for y in range(y0, y1):
        row_start = y * width * 4
        for x in range(x0, x1):
            offset = row_start + x * 4
            pixels[offset : offset + 4] = fill
    write_png_rgba_bytes(destination, width, height, bytes(pixels))


def _plot_area_title_height(cell_height: int) -> int:
    return max(20, cell_height // 5)


def _paste_subplot_plot_region(
    pixels: bytearray,
    *,
    width: int,
    height: int,
    source_col: int,
    source_row: int,
    destination_col: int,
    destination_row: int,
    source_rgba: bytes | None = None,
) -> None:
    source_pixels = source_rgba if source_rgba is not None else bytes(pixels)
    sx0, sy0, sx1, sy1 = _cell_bounds(width, height, col=source_col, row=source_row)
    dx0, dy0, dx1, dy1 = _cell_bounds(
        width, height, col=destination_col, row=destination_row
    )
    source_height = sy1 - sy0
    destination_height = dy1 - dy0
    source_plot_y0 = sy0 + _plot_area_title_height(source_height)
    destination_plot_y0 = dy0 + _plot_area_title_height(destination_height)
    copy_height = min(sy1 - source_plot_y0, dy1 - destination_plot_y0)
    copy_width = min(sx1 - sx0, dx1 - dx0)
    if copy_height <= 0 or copy_width <= 0:
        return

    for row in range(copy_height):
        source_y = source_plot_y0 + row
        destination_y = destination_plot_y0 + row
        source_row_start = source_y * width * 4
        destination_row_start = destination_y * width * 4
        for column in range(copy_width):
            source_offset = source_row_start + (sx0 + column) * 4
            destination_offset = destination_row_start + (dx0 + column) * 4
            pixels[destination_offset : destination_offset + 4] = source_pixels[
                source_offset : source_offset + 4
            ]


def degenerate_axis_subplots(
    image_path: Path,
    destination: Path,
    *,
    reference: Path,
    replacements: tuple[tuple[str, str], ...] = (("p8", "p4"), ("p9", "p1")),
) -> None:
    """Replace plot regions with wrong reference crops to simulate runaway axis/zoom."""
    width, height, rgba = read_png_rgba(image_path)
    pixels = bytearray(rgba)
    _, _, reference_rgba = read_png_rgba(reference)
    cells_by_name = {cell.name: cell for cell in SUBPLOT_CELLS}

    for destination_name, source_name in replacements:
        destination_cell = cells_by_name[destination_name]
        source_cell = cells_by_name[source_name]
        _paste_subplot_plot_region(
            pixels,
            width=width,
            height=height,
            source_col=source_cell.col,
            source_row=source_cell.row,
            destination_col=destination_cell.col,
            destination_row=destination_cell.row,
            source_rgba=reference_rgba,
        )

    write_png_rgba_bytes(destination, width, height, bytes(pixels))
