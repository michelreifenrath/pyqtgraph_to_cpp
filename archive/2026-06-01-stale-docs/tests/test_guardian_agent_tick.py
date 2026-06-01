from automation.pi_symphony import guardian_agent_tick as guardian


def test_build_prompt_is_advisory_and_uses_persistent_session_state():
    snapshot = {
        "timestamp_utc": "2026-05-25T00:00:00Z",
        "repo_root": "/repo",
        "board": "pyqtgraph-to-cpp",
        "mode": "test",
    }
    prompt = guardian.build_prompt(snapshot, {"last_output_hash": "abc123"})

    assert "persistent advisory Guardian agent" in prompt
    assert "Always produce a compact Telegram-suitable status" in prompt
    assert "Advisory only" in prompt
    assert "do not mutate state" in prompt
    assert "Worker layer" in prompt
    assert "Supervisor layer" in prompt
    assert "Meta layer" in prompt
    assert "Synthetic regression candidates" in prompt
    assert "must propose" in prompt
    assert "last_output_hash" in prompt
    assert "pyqtgraph-to-cpp" in prompt


def test_save_guardian_report_writes_local_markdown_and_json_index(tmp_path, monkeypatch):
    monkeypatch.setattr(guardian, "REPORT_DIR", tmp_path / "reports")

    path = guardian.save_guardian_report(
        snapshot={"timestamp_utc": "2026-05-25T00:00:00Z", "board": "pyqtgraph-to-cpp"},
        output="Guardian tick: pyqtgraph_to_cpp\nHealth: OK",
        result=guardian.CommandResult("hermes", 0, "Guardian tick: pyqtgraph_to_cpp\nHealth: OK"),
        state={"last_output_hash": "abc123"},
    )

    assert path == tmp_path / "reports" / "2026" / "05" / "25" / "20260525T000000Z-guardian-report.md"
    assert path.exists()
    content = path.read_text(encoding="utf-8")
    assert "# Guardian report: pyqtgraph_to_cpp" in content
    assert "Guardian tick: pyqtgraph_to_cpp" in content
    assert "## Snapshot" in content
    index = tmp_path / "reports" / "latest.json"
    assert index.exists()
    assert "20260525T000000Z-guardian-report.md" in index.read_text(encoding="utf-8")


def test_invoke_guardian_resumes_same_named_profile_and_session(monkeypatch):
    calls = []

    def fake_run_command(args, *, cwd=guardian.REPO_ROOT, timeout=guardian.DEFAULT_TIMEOUT):
        calls.append((args, cwd, timeout))
        return guardian.CommandResult("hermes", 0, "Guardian tick: pyqtgraph_to_cpp")

    monkeypatch.setattr(guardian, "run_command", fake_run_command)
    monkeypatch.setattr(guardian, "hermes_executable", lambda: "hermes")

    result = guardian.invoke_guardian("snapshot prompt", timeout=123)

    assert result.exit_code == 0
    args, cwd, timeout = calls[0]
    assert timeout == 123
    assert cwd == guardian.REPO_ROOT
    assert args[:5] == ["hermes", "-p", guardian.PROFILE, "--continue", guardian.SESSION_NAME]
    assert "--toolsets" in args
    assert "safe" in args
    assert "-q" in args
    assert args[-1] == "snapshot prompt"


def test_collect_snapshot_uses_supported_kanban_list_arguments(monkeypatch):
    commands = []

    def fake_run_command(args, *, cwd=guardian.REPO_ROOT, timeout=guardian.DEFAULT_TIMEOUT):
        commands.append(args)
        return guardian.CommandResult(" ".join(args), 0, "[]" if "gh" in args else "ok")

    monkeypatch.setattr(guardian, "run_command", fake_run_command)
    monkeypatch.setattr(guardian, "matching_processes", lambda: "<none>")
    monkeypatch.setattr(guardian, "recent_logs", lambda: "<none>")

    guardian.collect_snapshot()

    kanban_list_commands = [
        args
        for args in commands
        if args[:4] == ["hermes", "kanban", "--board", guardian.BOARD] and "list" in args
    ]
    assert kanban_list_commands
    assert all("--limit" not in args for args in kanban_list_commands)


def test_main_dry_run_does_not_invoke_guardian(monkeypatch, tmp_path, capsys):
    monkeypatch.setattr(guardian, "SCRIPT_STATE_DIR", tmp_path)
    monkeypatch.setattr(guardian, "STATE_FILE", tmp_path / "state.json")
    monkeypatch.setattr(guardian, "LOCK_FILE", tmp_path / "lock")
    monkeypatch.setattr(guardian, "collect_snapshot", lambda: {"timestamp_utc": "2026-05-25T00:00:00Z", "board": "pyqtgraph-to-cpp"})

    def fail_invoke(_prompt, *, timeout=300):
        raise AssertionError("dry-run must not invoke Guardian")

    monkeypatch.setattr(guardian, "invoke_guardian", fail_invoke)

    assert guardian.main(["--dry-run"]) == 0
    output = capsys.readouterr().out
    assert "Fresh snapshot" in output
    assert "pyqtgraph-to-cpp" in output


def test_main_real_tick_saves_report_and_mentions_path(monkeypatch, tmp_path, capsys):
    monkeypatch.setattr(guardian, "SCRIPT_STATE_DIR", tmp_path / "state")
    monkeypatch.setattr(guardian, "STATE_FILE", tmp_path / "state" / "state.json")
    monkeypatch.setattr(guardian, "LOCK_FILE", tmp_path / "state" / "lock")
    monkeypatch.setattr(guardian, "REPORT_DIR", tmp_path / "reports")
    monkeypatch.setattr(guardian, "collect_snapshot", lambda: {"timestamp_utc": "2026-05-25T00:00:00Z", "board": "pyqtgraph-to-cpp"})
    monkeypatch.setattr(
        guardian,
        "invoke_guardian",
        lambda _prompt, *, timeout=300: guardian.CommandResult(
            "hermes",
            0,
            "Guardian tick: pyqtgraph_to_cpp\nHealth: OK\nWorkflow: idle\nFindings:\n- none\nAction for Michel: none",
        ),
    )

    assert guardian.main([]) == 0
    output = capsys.readouterr().out

    assert "Guardian tick: pyqtgraph_to_cpp" in output
    assert "Local report:" in output
    report_path = tmp_path / "reports" / "2026" / "05" / "25" / "20260525T000000Z-guardian-report.md"
    assert report_path.exists()
    assert str(report_path) in output
    state = guardian.load_state()
    assert state["last_report_path"] == str(report_path)


def test_redact_masks_common_secret_shapes():
    text = "token=abc123xyzlongenough and Authorization: Bearer secret-value-long-enough"
    redacted = guardian.redact(text)

    assert "[REDACTED]" in redacted
    assert "abc123xyzlongenough" not in redacted
    assert "secret-value-long-enough" not in redacted
