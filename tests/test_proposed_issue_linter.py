from __future__ import annotations

from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]


def test_factory_control_plane_scripts_are_not_vendored_in_product_repo() -> None:
    assert not (REPO_ROOT / "scripts" / "factory").exists()


def test_factory_repo_notice_is_not_vendored_in_product_repo() -> None:
    assert not (REPO_ROOT / "FACTORY_REPO.md").exists()
