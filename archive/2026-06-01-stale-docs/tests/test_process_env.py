import subprocess
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


def test_run_retries_transient_gh_401_when_auth_probe_succeeds(monkeypatch):
    calls: list[list[str]] = []

    def fake_subprocess_run(args, **kwargs):
        calls.append(list(args))
        if args == ["gh", "api", "user", "--jq", ".login"]:
            return subprocess.CompletedProcess(args, 0, "michelreifenrath\n", "")
        if len(calls) == 1:
            return subprocess.CompletedProcess(args, 1, "", "gh: Requires authentication (HTTP 401)\n")
        return subprocess.CompletedProcess(args, 0, "ok\n", "")

    monkeypatch.setattr("automation.pi_symphony.process.subprocess.run", fake_subprocess_run)
    monkeypatch.setattr("automation.pi_symphony.process.time.sleep", lambda _seconds: None)

    result = run(["gh", "api", "repos/owner/repo/issues/1/labels"], timeout=30)

    assert result.stdout == "ok\n"
    assert calls == [
        ["gh", "api", "repos/owner/repo/issues/1/labels"],
        ["gh", "api", "user", "--jq", ".login"],
        ["gh", "api", "repos/owner/repo/issues/1/labels"],
    ]


def test_run_does_not_retry_gh_401_when_auth_probe_fails(monkeypatch):
    calls: list[list[str]] = []

    def fake_subprocess_run(args, **kwargs):
        calls.append(list(args))
        if args == ["gh", "api", "user", "--jq", ".login"]:
            return subprocess.CompletedProcess(args, 1, "", "gh: Requires authentication (HTTP 401)\n")
        return subprocess.CompletedProcess(args, 1, "", "gh: Requires authentication (HTTP 401)\n")

    monkeypatch.setattr("automation.pi_symphony.process.subprocess.run", fake_subprocess_run)

    try:
        run(["gh", "api", "repos/owner/repo/issues/1/labels"], timeout=30)
    except RuntimeError as exc:
        assert "Requires authentication" in str(exc)
    else:  # pragma: no cover - assertion guard
        raise AssertionError("expected RuntimeError")

    assert calls == [
        ["gh", "api", "repos/owner/repo/issues/1/labels"],
        ["gh", "api", "user", "--jq", ".login"],
    ]
