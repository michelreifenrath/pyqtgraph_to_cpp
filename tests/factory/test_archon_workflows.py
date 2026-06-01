from __future__ import annotations

from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]

EXPECTED_COMMANDS = {
    "pgcpp-issue-ready.yaml": "pgcpp-issue-ready",
    "pgcpp-fix-issue.yaml": "pgcpp-fix-issue",
    "pgcpp-validate-pr.yaml": "pgcpp-validate-pr",
    "pgcpp-comprehensive-test.yaml": "pgcpp-comprehensive-test",
}

PI_MODEL = "openai-codex/gpt-5.5"
CURSOR_MODEL = "composer-2.5"
PI_XHIGH = "xhigh"


def load_workflow(name: str) -> dict[str, object]:
    workflow = ROOT / ".archon" / "workflows" / name
    assert workflow.exists(), workflow
    data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
    assert isinstance(data, dict)
    return data


def test_archon_workflows_use_dag_schema() -> None:
    for workflow in (ROOT / ".archon" / "workflows").glob("*.yaml"):
        data = yaml.safe_load(workflow.read_text(encoding="utf-8"))
        assert isinstance(data.get("name"), str), workflow
        assert isinstance(data.get("description"), str), workflow
        assert "nodes" in data, workflow
        assert isinstance(data["nodes"], list), workflow
        assert data["nodes"], workflow
        for node in data["nodes"]:
            node_types = [key for key in ("command", "prompt", "bash", "script", "loop", "approval", "cancel") if key in node]
            assert len(node_types) == 1, (workflow, node)


def test_archon_command_nodes_reference_existing_commands() -> None:
    for workflow_name, command_name in EXPECTED_COMMANDS.items():
        data = load_workflow(workflow_name)
        commands = [node["command"] for node in data["nodes"] if "command" in node]
        assert command_name in commands
        assert (ROOT / ".archon" / "commands" / f"{command_name}.md").exists()


def test_pgcpp_workflows_route_ai_nodes_to_pi_and_cursor_cli() -> None:
    for workflow_name in EXPECTED_COMMANDS:
        data = load_workflow(workflow_name)
        assert data["provider"] == "pi"
        assert data["model"] == PI_MODEL

    fix_issue = {node["id"]: node for node in load_workflow("pgcpp-fix-issue.yaml")["nodes"]}
    validate_pr = {node["id"]: node for node in load_workflow("pgcpp-validate-pr.yaml")["nodes"]}

    for nodes, cursor_ids in (
        (fix_issue, {"implement", "self-fix", "simplify"}),
        (validate_pr, {"fix-pr-issues"}),
    ):
        for node_id in cursor_ids:
            assert nodes[node_id]["provider"] == "cursor"
            assert nodes[node_id]["model"] == CURSOR_MODEL
            assert "effort" not in nodes[node_id]

    for node_id in {"plan", "synthesize-review"}:
        assert fix_issue[node_id]["effort"] == PI_XHIGH
    for node_id in {"synthesize-pass1", "holdout-review"}:
        assert validate_pr[node_id]["effort"] == PI_XHIGH


def test_archon_command_prompts_use_supported_artifact_variables() -> None:
    issue_ready = (ROOT / ".archon" / "commands" / "pgcpp-issue-ready.md").read_text(encoding="utf-8")
    fix_issue = (ROOT / ".archon" / "commands" / "pgcpp-fix-issue.md").read_text(encoding="utf-8")
    assert "$check-readiness.output" not in issue_ready
    assert "$label-plan.output" not in issue_ready
    assert "$maybe-apply-labels.output" not in issue_ready
    assert "$ISSUE_FILE" not in fix_issue
    assert "$CHANGED_FILES_FILE" not in fix_issue
    assert "$ARTIFACTS_DIR/issue.json" in fix_issue



def test_archon_workflow_bash_arguments_are_quoted() -> None:
    combined = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / ".archon" / "workflows").glob("*.yaml")
    )
    assert "--issue-file \"$ARTIFACTS_DIR/issue.json\"" in combined
    assert "--input \"$ARGUMENTS\"" in combined
    assert "--issue-file $ARGUMENTS" not in combined
    assert "--input $ARGUMENTS" not in combined


def test_archon_workflows_reference_existing_factory_scripts() -> None:
    scripts = {
        "scripts/factory/check_issue_ready.py",
        "scripts/factory/check_pr_scope.py",
        "scripts/factory/file_regression_issue.py",
    }
    combined = "\n".join(
        path.read_text(encoding="utf-8")
        for path in (ROOT / ".archon" / "workflows").glob("*.yaml")
    )
    for script in scripts:
        assert script in combined
        assert (ROOT / script).exists()


def assert_pre_commit_changed_file_collection(snippet: str, artifact: str) -> None:
    assert "git diff --name-only origin/main...HEAD" in snippet
    assert "git diff --name-only --cached" in snippet
    assert "git diff --name-only\n" in snippet
    assert "git ls-files --others --exclude-standard" in snippet
    assert "sort -u" in snippet
    assert f'tee "{artifact}"' in snippet or f'> "{artifact}"' in snippet
    assert f'git diff --name-only origin/main...HEAD | tee "{artifact}"' not in snippet
    assert f'git diff --name-only origin/main...HEAD > "{artifact}"' not in snippet


def test_fix_issue_workflow_collects_uncommitted_changed_files_for_scope_gates() -> None:
    nodes = {node["id"]: node for node in load_workflow("pgcpp-fix-issue.yaml")["nodes"]}
    assert_pre_commit_changed_file_collection(nodes["pre-pr-gates"]["bash"], "$ARTIFACTS_DIR/changed-files.txt")
    assert_pre_commit_changed_file_collection(nodes["final-evidence"]["bash"], "$ARTIFACTS_DIR/changed-files-final.txt")


def test_pre_commit_command_prompts_collect_uncommitted_changed_files() -> None:
    command_artifacts = {
        "pgcpp-fix-issue.md": "$ARTIFACTS_DIR/changed-files.txt",
        "pgcpp-validate-implementation.md": "$ARTIFACTS_DIR/changed-files.txt",
        "pgcpp-fix-pr-issues.md": "$ARTIFACTS_DIR/changed-files-after-fix.txt",
    }
    for command, artifact in command_artifacts.items():
        content = (ROOT / ".archon" / "commands" / command).read_text(encoding="utf-8")
        assert_pre_commit_changed_file_collection(content, artifact)


def test_validate_pr_command_documents_governed_auto_merge_verdict() -> None:
    command = (ROOT / ".archon" / "commands" / "pgcpp-validate-pr.md").read_text(encoding="utf-8")
    assert "scripts/factory/apply_pr_verdict.py" in command
    assert "$ARTIFACTS_DIR/verdict.json" in command
    assert "--allow-merge" in command
    assert "--match-head-commit" in command
    assert "GPT-5.5 semantic visual-review" in command


def test_fix_issue_workflow_uses_dark_factory_issue_to_pr_topology() -> None:
    data = load_workflow("pgcpp-fix-issue.yaml")
    nodes = {node["id"]: node for node in data["nodes"]}

    expected_nodes = {
        "extract-issue-number",
        "fetch-issue",
        "verify-readiness",
        "claim-issue",
        "fetch-governance",
        "classify-issue",
        "repo-research",
        "investigate",
        "plan",
        "bridge-artifacts",
        "implement",
        "validate",
        "pre-pr-gates",
        "create-pr",
        "capture-pr-number",
        "review-scope",
        "review-classify",
        "cpp-qt-code-review",
        "oracle-visual-review",
        "test-coverage-review",
        "scope-policy-review",
        "docs-examples-review",
        "synthesize-review",
        "self-fix",
        "revalidate-after-self-fix",
        "simplify",
        "final-evidence",
        "workflow-integrity-audit",
        "report",
    }
    assert expected_nodes <= nodes.keys()

    expected_commands = {
        "extract-issue-number": "pgcpp-extract-issue-number",
        "classify-issue": "pgcpp-classify-issue",
        "repo-research": "pgcpp-research-issue",
        "investigate": "pgcpp-investigate-issue",
        "plan": "pgcpp-create-plan",
        "implement": "pgcpp-fix-issue",
        "validate": "pgcpp-validate-implementation",
        "create-pr": "pgcpp-create-pr",
        "review-scope": "pgcpp-pr-review-scope",
        "review-classify": "pgcpp-review-classify",
        "cpp-qt-code-review": "pgcpp-cpp-qt-code-review",
        "oracle-visual-review": "pgcpp-oracle-visual-review",
        "test-coverage-review": "pgcpp-test-coverage-review",
        "scope-policy-review": "pgcpp-scope-policy-review",
        "docs-examples-review": "pgcpp-docs-examples-review",
        "synthesize-review": "pgcpp-synthesize-review",
        "self-fix": "pgcpp-self-fix-review-findings",
        "revalidate-after-self-fix": "pgcpp-validate-implementation",
        "simplify": "pgcpp-simplify-changes",
        "report": "pgcpp-issue-completion-report",
    }
    for node_id, command in expected_commands.items():
        assert nodes[node_id]["command"] == command

    assert nodes["claim-issue"]["depends_on"] == ["verify-readiness"]
    assert nodes["fetch-governance"]["depends_on"] == ["claim-issue"]
    assert "ai:claimed" in nodes["claim-issue"]["bash"]
    assert nodes["bridge-artifacts"]["trigger_rule"] == "one_success"
    assert nodes["implement"]["depends_on"] == ["bridge-artifacts"]
    assert nodes["create-pr"]["depends_on"] == ["pre-pr-gates"]
    assert nodes["synthesize-review"]["trigger_rule"] == "one_success"
    assert nodes["self-fix"]["depends_on"] == ["synthesize-review"]
    assert nodes["final-evidence"]["depends_on"] == ["simplify"]
    assert nodes["workflow-integrity-audit"]["depends_on"] == ["final-evidence"]
    assert nodes["report"]["depends_on"] == ["workflow-integrity-audit"]
    assert "workflow-integrity-audit.json" in nodes["workflow-integrity-audit"]["bash"]


def test_validate_pr_workflow_is_governed_merge_controller() -> None:
    workflow = (ROOT / ".archon" / "workflows" / "pgcpp-validate-pr.yaml").read_text(encoding="utf-8")
    data = load_workflow("pgcpp-validate-pr.yaml")
    nodes = {node["id"]: node for node in data["nodes"]}
    fetch_pr = nodes["fetch-pr-metadata"]["bash"]
    assert "gh pr view" in workflow
    assert "gh pr diff" in workflow
    assert "gh pr checkout" in workflow
    assert "headRefOid" in workflow
    assert "fixes|closes|resolves" in workflow
    assert 'r"(?i)\\b(?:fixes|closes|resolves)\\s+#(\\d+)"' in workflow
    assert 'r"(?i)\\\\b(?:fixes|closes|resolves)\\\\s+#(\\\\d+)"' not in workflow
    assert "closingIssuesReferences" not in fetch_pr
    assert "commits" not in fetch_pr
    assert "origin/main" in workflow
    assert "scripts/factory/check_pr_scope.py" in workflow
    assert "git diff --check origin/main...HEAD" in workflow
    assert "scripts/gate merge" in workflow
    assert "scripts/gate commit --dry-run" not in workflow
    assert "scripts/run_autoreview --mode merge" in workflow
    assert "apply_pr_verdict.py --input \"$ARTIFACTS_DIR/verdict.json\" --allow-merge" in workflow
    assert "--match-head-commit" in workflow
    assert "from scripts.factory.workflow_config import load_workflow" in workflow
    assert "load_workflow(\"WORKFLOW.md\").policy.auto_merge" in workflow
    assert 'payload["auto_merge_enabled"] = True' not in workflow
    assert 'payload.get("auto_merge_enabled", True)' not in workflow
    assert 'payload.get("auto_merge_enabled") is True' in workflow
    assert nodes["deterministic-verdict-guard"]["depends_on"] == ["holdout-review"]
    assert nodes["final-head-check"]["depends_on"] == ["deterministic-verdict-guard"]
    assert nodes["workflow-integrity-audit"]["depends_on"] == ["final-head-check"]
    assert nodes["governed-auto-merge"]["depends_on"] == ["workflow-integrity-audit"]
    assert "workflow-integrity-audit.json" in nodes["workflow-integrity-audit"]["bash"]


def test_validate_pr_workflow_has_dark_factory_pass1_fix_pass2_topology() -> None:
    data = load_workflow("pgcpp-validate-pr.yaml")
    nodes = {node["id"]: node for node in data["nodes"]}

    expected_nodes = {
        "verify-holdout-clean",
        "readiness-check-pass1",
        "scope-check-pass1",
        "evidence-packet-check-pass1",
        "diff-check-pass1",
        "local-merge-gate-pass1",
        "visual-oracle-gate-pass1",
        "autoreview-merge-gate-pass1",
        "reviewer-pass1-cpp-qt",
        "reviewer-pass1-oracle-visual",
        "reviewer-pass1-scope-governance",
        "synthesize-pass1",
        "fix-pr-issues",
        "refresh-after-fix",
        "mark-no-fix",
        "checkout-pass2-head",
        "readiness-check-pass2",
        "scope-check-pass2",
        "diff-check-pass2",
        "local-merge-gate-pass2",
        "visual-oracle-gate-pass2",
        "autoreview-merge-gate-pass2",
        "holdout-review",
        "deterministic-verdict-guard",
        "workflow-integrity-audit",
    }
    assert expected_nodes <= nodes.keys()

    expected_commands = {
        "reviewer-pass1-cpp-qt": "pgcpp-pr-review-cpp-qt",
        "reviewer-pass1-oracle-visual": "pgcpp-pr-review-oracle-visual",
        "reviewer-pass1-scope-governance": "pgcpp-pr-review-scope-governance",
        "synthesize-pass1": "pgcpp-synthesize-pr-validation",
        "fix-pr-issues": "pgcpp-fix-pr-issues",
        "holdout-review": "pgcpp-validate-pr",
    }
    for node_id, command in expected_commands.items():
        assert nodes[node_id]["command"] == command

    assert nodes["synthesize-pass1"]["depends_on"] == [
        "reviewer-pass1-cpp-qt",
        "reviewer-pass1-oracle-visual",
        "reviewer-pass1-scope-governance",
    ]
    assert nodes["synthesize-pass1"]["trigger_rule"] == "one_success"
    assert nodes["fix-pr-issues"]["depends_on"] == ["synthesize-pass1"]
    assert nodes["fix-pr-issues"]["when"] == "$synthesize-pass1.output.action == 'fix'"
    assert nodes["mark-no-fix"]["depends_on"] == ["synthesize-pass1"]
    assert set(nodes["checkout-pass2-head"]["depends_on"]) == {"refresh-after-fix", "mark-no-fix"}
    assert nodes["checkout-pass2-head"]["trigger_rule"] == "one_success"
    assert nodes["holdout-review"]["depends_on"] == ["autoreview-merge-gate-pass2"]
    assert nodes["deterministic-verdict-guard"]["depends_on"] == ["holdout-review"]



def test_all_pgcpp_workflows_have_integrity_audit_before_final_side_effects() -> None:
    expected_edges = {
        "pgcpp-fix-issue.yaml": ("final-evidence", "report"),
        "pgcpp-validate-pr.yaml": ("final-head-check", "governed-auto-merge"),
        "pgcpp-issue-ready.yaml": ("apply-label-plan", "summarize-readiness"),
        "pgcpp-comprehensive-test.yaml": ("optionally-file-deduped-regression-issues", "comprehensive-guidance"),
    }
    for workflow_name, (upstream, downstream) in expected_edges.items():
        nodes = {node["id"]: node for node in load_workflow(workflow_name)["nodes"]}
        audit = nodes["workflow-integrity-audit"]
        assert audit["depends_on"] == [upstream]
        assert nodes[downstream]["depends_on"] == ["workflow-integrity-audit"]
        assert "workflow-integrity-audit.json" in audit["bash"]
        assert "workflow-integrity-audit.md" in audit["bash"]
        assert "workflow-log:error-events" in audit["bash"]
