<!-- generated-local-issue-policy -->
# Proposed GitHub issue readiness policy

GitHub issues are the single source of truth for the proposed full native C++ PyQtGraph port backlog.

The former generated local issue files were published to GitHub and removed to avoid split-brain edits. Do not reintroduce local issue-body mirrors as an editable source of truth.

Policy: **local validation only**. Do not add GitHub Actions or make completion depend on GitHub infrastructure.

Validation rules are centralized in `docs/proposed-issues/VALIDATION-GUIDE.md`. Each proposed GitHub issue must declare a validation class, owned-file selectors, task-specific proof, and the readiness contract below.

## Required issue contract

Each proposed issue body must include:

- `## Goal`
- `## Current evidence`
- `## Scope`
- `## Owned files`
- `## Required local proof`
- `## TDD plan`
- `## Validation commands`
- `## Acceptance criteria`
- `## Done definition`
- `## Scope boundaries`

Additional gate rules:

- `Blocked by` entries must be explicit issue IDs such as `P4.01`; no shorthand ranges such as `P4.01-P4.14` and no prose placeholders such as `Required modules`.
- Owned-file selectors must be concrete enough for automation; prose-heavy selectors are rejected by `scripts/check_proposed_issues`.
- Only dependency-free issues may carry `ai:ready`.
- Issues with any `Blocked by` entry must carry `ai:blocked` until their blockers are complete and the readiness linter passes.

## Commands

Lint live GitHub proposed issues:

```bash
scripts/check_proposed_issues --source github
```

Show readiness label plan from live GitHub issues:

```bash
scripts/check_proposed_issues --source github --github-label-plan
```

Apply readiness labels from live GitHub issues:

```bash
scripts/check_proposed_issues --source github --apply-github-labels
```

`--source auto` uses local generated issue files only if they exist; otherwise it lints GitHub directly.
