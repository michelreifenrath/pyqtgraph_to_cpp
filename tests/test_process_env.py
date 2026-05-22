import sys

from automation.pi_symphony.process import run


def test_run_loads_profile_hermes_env_for_subprocesses(tmp_path, monkeypatch):
    hermes_home = tmp_path / "profile-home"
    hermes_home.mkdir()
    (hermes_home / ".env").write_text("GITHUB_TOKEN=token-from-profile\n", encoding="utf-8")
    monkeypatch.setenv("HERMES_HOME", str(hermes_home))
    monkeypatch.delenv("GITHUB_TOKEN", raising=False)

    result = run(
        [sys.executable, "-c", "import os; print(os.environ.get('GITHUB_TOKEN', ''))"],
        timeout=30,
    )

    assert result.stdout.strip() == "token-from-profile"


def test_run_does_not_override_existing_environment_with_dotenv(tmp_path, monkeypatch):
    hermes_home = tmp_path / "profile-home"
    hermes_home.mkdir()
    (hermes_home / ".env").write_text("GITHUB_TOKEN=token-from-profile\n", encoding="utf-8")
    monkeypatch.setenv("HERMES_HOME", str(hermes_home))
    monkeypatch.setenv("GITHUB_TOKEN", "already-present")

    result = run(
        [sys.executable, "-c", "import os; print(os.environ.get('GITHUB_TOKEN', ''))"],
        timeout=30,
    )

    assert result.stdout.strip() == "already-present"


def test_run_exposes_github_token_as_gh_token_for_github_cli(tmp_path, monkeypatch):
    hermes_home = tmp_path / "profile-home"
    hermes_home.mkdir()
    (hermes_home / ".env").write_text("GITHUB_TOKEN=token-from-profile\n", encoding="utf-8")
    monkeypatch.setenv("HERMES_HOME", str(hermes_home))
    monkeypatch.delenv("GITHUB_TOKEN", raising=False)
    monkeypatch.delenv("GH_TOKEN", raising=False)

    result = run(
        [sys.executable, "-c", "import os; print(os.environ.get('GH_TOKEN', ''))"],
        timeout=30,
    )

    assert result.stdout.strip() == "token-from-profile"
