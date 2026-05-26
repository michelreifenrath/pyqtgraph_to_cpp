from __future__ import annotations

import os
from pathlib import Path

import pytest


MARKER_NAME = "P1_06_visual_smoke_marker.txt"
ENV_NAME = "PG_VISUAL_ARTIFACT_DIR"


def resolve_artifact_dir(value: str | os.PathLike[str] | None) -> Path:
    if value is None:
        raise ValueError(f"{ENV_NAME} must name a non-empty artifact directory")
    path_text = os.fspath(value).strip()
    if not path_text:
        raise ValueError(f"{ENV_NAME} must name a non-empty artifact directory")
    return Path(path_text)


def write_visual_smoke_marker(artifact_dir: Path) -> Path:
    artifact_dir.mkdir(parents=True, exist_ok=True)
    marker = artifact_dir / MARKER_NAME
    marker.write_text("P1.06 visual smoke fixture executed\n", encoding="utf-8")
    return marker


def test_P1_06_visual_smoke_writes_marker(tmp_path: Path) -> None:
    artifact_dir = resolve_artifact_dir(os.environ.get(ENV_NAME, str(tmp_path)))

    marker = write_visual_smoke_marker(artifact_dir)

    assert marker.is_file()
    assert marker.read_text(encoding="utf-8") == "P1.06 visual smoke fixture executed\n"


@pytest.mark.parametrize("bad_value", [None, "", "   "])
def test_P1_06_visual_smoke_rejects_empty_artifact_dir(
    bad_value: str | None,
) -> None:
    with pytest.raises(ValueError, match=ENV_NAME):
        resolve_artifact_dir(bad_value)
