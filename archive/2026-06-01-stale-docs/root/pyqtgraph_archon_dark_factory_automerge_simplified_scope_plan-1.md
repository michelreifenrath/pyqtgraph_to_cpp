# PyQtGraph → C++ with Archon + Dark Factory

## Auto-Merge + Self-Healing Plan with Simplified Product Scope

Repository target: `michelreifenrath/pyqtgraph_to_cpp`  
Primary goal: build a native C++ library that **functions and looks like PyQtGraph from the outside**.  
Non-goal: line-by-line translation of every PyQtGraph internal Python module.

---

## 1. Core decision

Use the **Dark Factory / Archon operating model**, including:

- automatic issue implementation;
- independent holdout PR validation;
- automatic fix-and-retry;
- automatic squash merge when all gates pass;
- post-merge and scheduled self-healing regression workflows.

Do **not** use a full public-repo triage workflow. This is a private repository and the owner controls issue creation. Replace full triage with a lightweight **issue readiness gate** that answers one question:

> Is this issue small, testable, dependency-free, and suitable for autonomous implementation?

This keeps the automation powerful while removing unnecessary public-repo complexity.

---

## 2. Simplified product definition

The C++ library is successful when a C++ user can use PyQtGraph-like classes, names, examples, and visual behavior without caring that the implementation is not Python.

### 2.1 Build exactly this

Build a native C++ / Qt library that:

1. exposes PyQtGraph-like public classes and names;
2. keeps examples close to PyQtGraph examples in structure and naming;
3. renders plots, axes, curves, images, colors, interactions, and common widgets with matching visual style;
4. provides deterministic C++ tests and visual/oracle tests against the pinned PyQtGraph reference;
5. installs cleanly and works from a downstream C++ project with `find_package(pyqtgraph-cpp)`.

### 2.2 Do not build more than this

Do not port complexity that is not visible to a C++ library user.

Avoid or defer:

- Python import machinery;
- Python module `__init__.py` behavior unless it maps to a useful C++ header API;
- Python-only debugging helpers;
- Python console / REPL tools unless a C++ equivalent has clear user value;
- Jupyter-specific behavior;
- multiprocessing / remote graphics internals;
- Matplotlib integration unless explicitly required by a user-facing example;
- Qt5 compatibility during the first working version;
- OpenGL / 3D until the 2D plotting library is stable;
- performance optimization beyond basic smoke thresholds until visual/function parity works.

### 2.3 The guiding rule

For every upstream PyQtGraph file or feature, ask:

> Does this affect how an external C++ user creates, views, styles, interacts with, or embeds a plot?

If yes, port the externally visible behavior.  
If no, mark it `not_applicable`, `deferred`, or `covered_by_equivalent` in the manifest.

---

## 3. Recommended architecture: Dark Factory Lite, not Dark Factory Heavy

Use four workflows only. Borrow two ideas from WorkOS Case and OpenAI Symphony without copying their full systems:

- from Case: split work into clear roles and require evidence before PR creation or merge;
- from Symphony: treat GitHub issues/labels as the control plane and make run state visible.

```text
Human-created issue
  -> issue readiness gate
  -> Archon implementation workflow
       -> scout/read-only refusal check
       -> failing test or oracle
       -> minimal implementation
       -> evidence packet
  -> holdout validation workflow
       -> independent validation
       -> auto-fix once or twice if needed
       -> auto-merge if green
       -> human escalation if not green
  -> self-healing regression workflow after merge / nightly, introduced gradually
```

### 3.1 Workflow 1 — issue readiness gate

Purpose: prevent oversized or vague work from entering the factory.

This is not public triage. It does not decide product strategy. It checks mechanical readiness.

Recommended workflow name:

```text
.archon/workflows/pgcpp-issue-ready.yaml
```

Recommended deterministic script:

```text
scripts/factory/check_issue_ready.py
```

The gate should verify:

- issue has one externally observable outcome;
- issue names the PyQtGraph reference class, function, example, or behavior;
- issue has dependencies listed as `none` or resolved issue numbers;
- issue lists owned files;
- issue includes a TDD plan;
- issue includes validation commands;
- issue declares whether visual or interaction validation is required;
- issue is under the size budget;
- issue does not request protected automation/governance changes unless explicitly marked as an automation issue.

Recommended labels:

```text
ai:ready             # eligible for implementation
ai:blocked           # not ready or dependency missing
human-review         # human decision needed
factory:ready-checked
factory:from-regression
```

Do not add more readiness labels unless a workflow truly needs them. Runtime state labels are listed separately below.

### 3.2 Workflow 2 — implementation

Purpose: implement exactly one small issue.

Recommended workflow name:

```text
.archon/workflows/pgcpp-fix-issue.yaml
```

High-level phases:

```text
1. Fetch issue
2. Verify readiness stamp
3. Scout the task in read-only mode
4. Refuse early if scope, owned files, dependency state, or validation is unclear
5. Read governance files
6. Read pinned PyQtGraph reference source/example
7. Add or update failing test/oracle first
8. Implement minimal C++ behavior
9. Run focused validation
10. Run local gate
11. Create PR with validation evidence
12. Label PR for validation
```

The scout step is allowed to stop the run before coding. A stopped scout is a success if it prevents an unsafe or oversized autonomous change.

The implementer must not merge. The implementer must not modify governance files. The implementer must not expand the issue scope.

Each implementation PR must include a compact evidence packet:

```md
## Factory evidence
- Issue: #<number>
- Scope: <one-sentence external behavior>
- Baseline failure: <command/output or not_applicable>
- Files changed: <summary>
- Validation commands run:
  - <command>: <pass/fail>
- Visual/oracle artifacts: <path or not_applicable>
- Known limitations: <none or short note>
```

The evidence packet is not a long agent transcript. It is the minimum proof a fresh validator needs.

### 3.3 Workflow 3 — holdout validation + auto-merge

Purpose: independently validate the PR without reading the implementer’s plan or reasoning.

Recommended workflow name:

```text
.archon/workflows/pgcpp-validate-pr.yaml
```

This workflow should:

1. fetch the PR diff;
2. fetch the linked issue only;
3. fetch governance files from `origin/main` before checking out the PR;
4. verify the PR evidence packet exists;
5. run deterministic checks;
6. run external behavior validation;
7. run visual/oracle validation when required;
8. run security/protected-file checks;
9. run one fresh-context fix pass if validation fails for fixable reasons;
10. re-run the full validation;
11. write an independent validator verdict;
12. squash merge automatically if green;
13. escalate to `human-review` if still failing.

Auto-merge is allowed only when every gate is green and the validator verdict says the PR solves the linked issue.

Recommended merge command:

```bash
gh pr merge "$PR_NUMBER" --squash --delete-branch
```

If branch deletion causes local worktree issues, drop `--delete-branch` and rely on GitHub server-side branch cleanup.

### 3.4 Run state and stale-run rule

Keep run state visible with simple GitHub labels or comments. Do not build a separate dashboard at first.

Recommended states:

```text
factory:running
factory:pr-opened
factory:validating
human-review
factory:merged
factory:failed-with-evidence
```

If an automation run appears stale, do not guess that it is failed or abandoned. Comment with the last known state and ask the human to resume, retry, or abandon it.

### 3.5 Workflow 4 — self-healing regression

Purpose: continuously test the merged library and file small repair issues automatically.

Recommended workflow name:

```text
.archon/workflows/pgcpp-comprehensive-test.yaml
```

Introduce triggers gradually:

1. first: post-merge smoke only;
2. later: nightly broader visual/example suite;
3. last: weekly full status, examples, install, and performance smoke.

Do not build the full self-healing system before the basic issue -> PR -> validation -> merge loop has proven itself.

The workflow should file issues only when it has real evidence:

- failing command;
- expected result;
- actual result;
- logs;
- screenshot diff artifacts when visual;
- suspected subsystem;
- proposed small owned-file set if it can infer one.

Regression issues should be labelled:

```text
factory:from-regression
ai:ready              # only if readiness gate can prove the issue is small
ai:blocked            # if the failure is broad or ambiguous
human-review          # if the failure suggests architecture or environment trouble
```

---

## 4. Minimal governance files

Create three clear governance files. Keep them short.

```text
MISSION.md
FACTORY_RULES.md
AGENTS.md
```

Your repo already has `AGENTS.md`, `WORKFLOW.md`, and `docs/pyqtgraph-cpp-port-workflow.md`. The migration should not duplicate everything. Instead:

- `MISSION.md`: short product contract and hard non-goals;
- `FACTORY_RULES.md`: automation rules, auto-merge gates, protected files, retry limits;
- `AGENTS.md`: coding and porting rules for AI agents;
- `docs/pyqtgraph-cpp-port-workflow.md`: detailed engineering reference;
- `WORKFLOW.md`: retire or freeze once Archon becomes the active engine.

### 4.1 `MISSION.md` minimal contents

```md
# Mission

Build a native C++ Qt library that functions and looks like PyQtGraph from the outside.

The project succeeds when C++ users can:

- create plots with PyQtGraph-like class names and structure;
- run C++ examples corresponding to important PyQtGraph examples;
- get visually similar output for plots, axes, curves, images, colors, and interactions;
- install the library and use it from a downstream C++ application.

## In scope

- Qt 6 C++ plotting widgets and graphics items
- PyQtGraph-like public classes and examples
- Numeric, visual, and interaction oracle tests
- CMake package installation
- Documentation for the C++ API and examples

## Out of scope for the first working version

- Python wrappers
- Python import/module compatibility
- Jupyter integration
- remote graphics / multiprocessing
- Qt5 support
- OpenGL / 3D
- Matplotlib integration
- complete line-by-line internal translation

## Hard invariants

- Native C++ library, not a Python wrapper
- Qt 6 first
- External behavior and visual parity are the acceptance criteria
- Issues must be small and testable
- Auto-merge requires holdout validation
- Governance files cannot be modified by normal implementation issues
```

### 4.2 `FACTORY_RULES.md` minimal contents

```md
# Factory Rules

## Issue readiness

An issue may be implemented automatically only if it is small, dependency-free, and testable.

## Auto-merge gates

A PR may auto-merge only if:

- it links exactly one issue;
- issue readiness passed;
- changed files are inside the issue-owned files plus allowed shared integration files;
- diff size is below the cap;
- tests pass;
- CMake configure/build/test pass when applicable;
- visual/oracle artifacts pass when required;
- the PR includes a compact evidence packet;
- protected files are untouched;
- holdout validation says the PR solves the issue;
- no high-severity review finding remains.

## Retry policy

- One automatic fix pass by default.
- Two automatic fix passes maximum.
- If still failing, label `human-review`.

## Protected files

- `MISSION.md`
- `FACTORY_RULES.md`
- `AGENTS.md`
- `.archon/**` unless the issue is explicitly automation work
- `scripts/factory/**` unless the issue is explicitly automation work
- `WORKFLOW.md` during migration
- credential files and `.env*`
```

---

## 5. Strip complexity from the C++ port plan

The implementation plan should move from **source-file-completion thinking** to **external-contract completion thinking**.

### 5.1 Replace “port every file” with a parity matrix

Current manifest-style tracking is useful, but it can push the project toward porting Python internals that do not matter externally.

Add or generate a simplified parity matrix:

```yaml
features:
  - id: simple-plot
    external_contract: "C++ SimplePlot example renders like PyQtGraph SimplePlot"
    public_api:
      - PlotWidget
      - PlotItem
      - PlotCurveItem
      - AxisItem
      - ViewBox
    examples:
      - examples/SimplePlot.cpp
    validation:
      numeric: required
      visual: required
      interaction: not_applicable
    status: passing
```

Each upstream file can still be tracked, but implementation priority should come from features/examples, not from file count.

### 5.2 Use four implementation categories

For each PyQtGraph component, classify it as exactly one of:

```text
public_port             # user-facing C++ API or visible behavior
example_port            # example needed to prove outside behavior
oracle_only             # only needed to generate/compare reference behavior
not_applicable          # Python-specific or no useful C++ equivalent
```

Avoid vague states such as `maybe`, `partial`, or `unknown` for active issues. If the status is unknown, create a small investigation issue first.

### 5.3 Prefer example-driven issues

High-success issue shape:

```text
Make <one example> compile, run, and visually match the pinned PyQtGraph reference within tolerance.
```

Lower-success issue shape:

```text
Port all of PlotItem.
```

Best issue shape:

```text
[PLOT-014] Make SimplePlot render one blue curve with visible axes and matching view range
```

This lets the agent implement only the necessary part of `PlotWidget`, `PlotItem`, `ViewBox`, `AxisItem`, and `PlotCurveItem` for that externally visible behavior.

---

## 6. Size budget for autonomous issues

Use small issue budgets. Auto-merge should be strict.

Recommended default caps:

```text
Production files changed:       max 4
Test/oracle files changed:      max 4
Shared integration files:       max 3
Total files changed:            max 10
Diff line soft target:          50–400
Diff line hard cap:             800
New public classes per issue:   max 2
New examples per issue:         max 1
New dependencies:               forbidden unless issue is dependency-specific
```

For visual or interaction work, allow slightly more files only if the issue owns the example and oracle artifact path.

Oversized issues should not run implementation. They should be split.

---

## 7. Auto-merge gates in detail

A PR is auto-mergeable only if all of the following pass.

### 7.1 Issue and scope gates

- PR body contains `Closes #N`, `Fixes #N`, or `Resolves #N`.
- Linked issue has `factory:ready-checked`.
- PR touches only owned files plus allowed shared integration files.
- PR does not modify protected files.
- Diff is under size caps.
- No unrelated cleanup, refactor, or “drive-by improvement”.

### 7.2 Build and test gates

Run the fastest reliable local gate first:

```bash
git diff --check
python3 -m pytest -q
```

Once CMake is stable, include:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

For package/install changes:

```bash
cmake --install build/dev --prefix /tmp/pgcpp-install
cmake -S tests/downstream -B /tmp/pgcpp-downstream-build \
  -DCMAKE_PREFIX_PATH=/tmp/pgcpp-install
cmake --build /tmp/pgcpp-downstream-build --parallel
```

### 7.3 Oracle and visual gates

Pure numeric helpers:

```text
numeric oracle required
visual not applicable
interaction not applicable
```

Rendering work:

```text
numeric oracle optional or required
visual required
interaction optional or required
```

Interaction work:

```text
numeric required for ranges/transforms
visual required for before/after screenshots
interaction required
```

Required artifact layout:

```text
reports/visual-diffs/<case>/reference.png
reports/visual-diffs/<case>/actual.png
reports/visual-diffs/<case>/diff.png
reports/visual-diffs/<case>/metrics.json
```

Defer semantic vision review at first:

- use deterministic pixel/metric visual checks as the normal gate;
- if metrics are ambiguous, escalate to `human-review` instead of asking a vision model to decide;
- add vision review later only if deterministic visual checks repeatedly miss severe semantic mismatches.

### 7.4 Holdout validation rule

The validator must not read:

- implementation plans;
- implementer scratch notes;
- implementer reasoning;
- previous implementation workflow artifacts.

The validator may read:

- linked issue;
- PR diff;
- current tests and validation output;
- pinned PyQtGraph reference;
- governance files from `origin/main`;
- generated visual/oracle artifacts.

---

## 8. Self-healing strategy

Self-healing should repair regressions, not create chaos.

### 8.1 Quick post-merge smoke

Run after each auto-merge:

```bash
git fetch origin main
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

Also run changed or high-priority examples:

```bash
scripts/run_changed_examples --visual
```

If it fails, file a regression issue with evidence.

### 8.2 Nightly comprehensive suite

Run nightly:

```bash
scripts/gate merge
scripts/run_all_examples --visual
scripts/summarize_status
```

During the early project phase, `run_all_examples` should only require examples classified as active or implemented. Missing/deferred examples should not fail the suite.

### 8.3 Weekly full library acceptance

Run weekly:

```bash
scripts/gate merge
scripts/run_all_examples --visual --interaction
scripts/run_performance --smoke
scripts/summarize_status --require-active-complete
```

Do not require all PyQtGraph examples to be complete until the project explicitly enters final acceptance mode.

### 8.4 Regression issue template

Self-healing issues should use this structure:

```md
## Goal
Fix regression found by `<workflow>` in `<case>`.

## Evidence
- Failing command: `<command>`
- Failing log: `<path or excerpt>`
- Reference artifact: `<path>`
- Actual artifact: `<path>`
- Diff artifact: `<path>`
- Metrics: `<path>`

## Expected
<external behavior expected from pinned PyQtGraph reference>

## Actual
<observed C++ behavior>

## Dependencies
none

## Owned files
The agent may edit only:
- <inferred file path>
- <test or oracle path>

## Scope
Implement:
- the smallest fix that restores this external behavior

Do not implement:
- unrelated cleanup
- broader refactors
- new features

## Validation commands
```bash
<focused failing command>
scripts/gate commit
```
```

If the workflow cannot infer owned files, label the issue `human-review` instead of `ai:ready`.

---

## 9. Recommended milestones for a working library

The project should reach a usable C++ library in layers. Each milestone should have a small set of examples and outside-facing acceptance tests.

### Milestone 0 — Factory foundation with auto-merge

Goal: automation can safely implement, validate, auto-fix, auto-merge, and file regression issues.

Deliverables:

- `MISSION.md`
- `FACTORY_RULES.md`
- `.archon/workflows/pgcpp-issue-ready.yaml`
- `.archon/workflows/pgcpp-fix-issue.yaml`
- `.archon/workflows/pgcpp-validate-pr.yaml`
- `.archon/workflows/pgcpp-comprehensive-test.yaml`
- `.archon/commands/pgcpp-*.md`
- `scripts/factory/check_issue_ready.py`
- `scripts/factory/check_pr_scope.py`
- `scripts/factory/apply_pr_verdict.py`

Exit criteria:

- one README/docs issue auto-merges;
- one tiny code issue auto-merges;
- one intentionally broken PR is rejected or fixed;
- one synthetic regression issue is filed by the comprehensive-test workflow.

### Milestone 1 — Installable C++ package smoke

Goal: the library builds, installs, and links from a downstream C++ project.

External acceptance:

```cpp
#include <pyqtgraph/PlotWidget.hpp>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    pyqtgraph::PlotWidget widget;
    widget.show();
    return 0;
}
```

Exit criteria:

- `find_package(pyqtgraph-cpp)` works;
- downstream smoke app builds;
- `PlotWidget` can be instantiated;
- no visual parity required yet beyond window creation smoke.

### Milestone 2 — SimplePlot MVP

Goal: one basic PyQtGraph-style plot renders correctly.

External acceptance:

- C++ `SimplePlot.cpp` compiles and runs;
- window contains visible plot area;
- axes visible;
- one curve rendered;
- view range is correct;
- screenshot diff within tolerance against pinned PyQtGraph reference.

Minimum public surface:

- `PlotWidget`
- `PlotItem`
- `ViewBox`
- `AxisItem`
- `PlotCurveItem`
- `PlotData`
- `mkPen`
- basic color handling

Do not implement advanced interactions yet.

### Milestone 3 — Basic interactive plotting

Goal: simple plots behave like PyQtGraph for normal user interaction.

External acceptance:

- pan works;
- zoom works;
- auto-range works;
- mouse range changes match reference within tolerance;
- before/after screenshots and numeric range fixtures pass.

Minimum public surface:

- `ViewBox` range model;
- mouse event wrappers;
- basic scene/view plumbing.

### Milestone 4 — Common plot styling

Goal: common PyQtGraph visual style is recognizable.

External acceptance:

- line colors;
- line widths;
- dashed pens;
- symbols where required;
- background/foreground behavior;
- axis tick/label style.

Minimum public surface:

- `mkColor`
- `mkPen`
- `mkBrush`
- `ColorMap` basic LUT
- style-related `PlotCurveItem` options.

### Milestone 5 — Scatter and image MVP

Goal: cover the next most common PyQtGraph use cases.

External acceptance:

- scatter plot example renders;
- image view or image item example renders;
- LUT/levels behavior visually matches reference within tolerance.

Minimum public surface:

- `ScatterPlotItem`
- `ImageItem`
- color maps / LUT application;
- OpenCV or C++ image buffer path.

### Milestone 6 — ROI / InfiniteLine / measurement tools

Goal: support common interactive analysis items.

External acceptance:

- `InfiniteLine` renders and can move;
- basic ROI item renders and updates geometry;
- interaction probes pass.

### Milestone 7 — Examples-driven completion loop

Goal: expand by examples, not by internal file count.

For each issue:

```text
Make one specific C++ example look/function like the pinned PyQtGraph example.
```

Examples should be ordered by user value and dependency readiness.

Recommended order:

1. SimplePlot
2. Multiple curves
3. Scatter plot
4. ImageItem
5. Histogram/LUT-style image example
6. InfiniteLine
7. ROI basics
8. PlotWidget embedding
9. Axis/date/log examples if needed
10. Exporter-related examples only after rendering is stable

### Milestone 8 — Stable beta

Goal: usable external library for real C++ users.

Exit criteria:

- install works;
- downstream app builds;
- core examples pass;
- active visual examples pass;
- active interaction probes pass;
- package docs exist;
- self-healing workflow has created and resolved at least one real regression issue;
- auto-merge has a safe track record.

---

## 10. Issue template for high-success autonomous work

Use this as the default issue body.

```md
## Goal
<One externally visible behavior or example outcome.>

## PyQtGraph reference
- Upstream class/function/example: `<path or name>`
- Reference behavior: `<short description>`

## Dependencies
- none

## Owned files
The agent may edit only:
- <production file 1>
- <production file 2>
- <test/oracle file>
- <example file if applicable>

Allowed shared integration files:
- CMakeLists.txt
- port_manifest.yaml
- reports/agents/<issue-code>.md

## Scope
Implement:
- <small behavior 1>
- <small behavior 2>

Do not implement:
- unrelated PyQtGraph features
- unrelated examples
- broad refactors
- new dependencies

## TDD plan
Failing tests to add or update first:
- [ ] <test path or oracle path>

Expected initial failure:
- <specific failure before implementation>

Pass condition:
- <specific observable condition>

## Validation level
- Numeric: `required|not_applicable`
- Visual: `required|not_applicable`
- Interaction: `required|not_applicable`

## Validation commands
```bash
git diff --check
python3 -m pytest -q
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

## Done definition
- [ ] focused test fails before implementation where applicable
- [ ] focused test passes after implementation
- [ ] local gate passes
- [ ] visual/oracle artifacts exist when required
- [ ] C++ names remain close to PyQtGraph
- [ ] no unrelated files changed
- [ ] PR links this issue
```

---

## 11. Recommended simplifications to apply now

### 11.1 Choose one active automation engine

Do not run Pi/Hermes and Archon as competing implementation engines long term.

Recommended path:

1. Keep current Pi/Hermes setup frozen while Archon is introduced.
2. Run Archon only on a few low-risk issues.
3. After several green auto-merges, make Archon the active factory engine.
4. Move Pi/Hermes docs/config into `docs/legacy-pi-symphony/` or remove them if no longer needed.

This reduces operational complexity and makes failures easier to diagnose.

### 11.2 Reduce validation levels

Use only three validation dimensions:

```text
numeric
visual
interaction
```

Each is either:

```text
required
not_applicable
```

Avoid `optional` in issue bodies. Optional validation confuses agents. If it matters, require it. If it does not matter, skip it.

### 11.3 Defer performance

Performance should not block the first working library unless a PR causes catastrophic slowdown.

Use only a smoke threshold early:

```text
The example must render within a reasonable timeout and must not allocate unbounded memory.
```

Add serious benchmarks after visual/function parity is stable.

### 11.4 Avoid full internal hierarchy perfection early

Class hierarchy should be close enough for external use and examples.

Do not block early milestones on exact internal inheritance if:

- public behavior matches;
- visual output matches;
- examples compile;
- downstream code can use the API naturally.

Record intentional deviations in the manifest.

### 11.5 Treat Python-only files as resolved, not missing

A Python-only upstream file should not remain as `missing` forever if no external C++ equivalent is needed.

Use:

```yaml
status: not_applicable
reason: python_import_or_runtime_only
external_equivalent: none
```

or:

```yaml
status: covered_by_equivalent
external_equivalent: include/pyqtgraph/<SomePublicHeader>.hpp
```

This prevents the factory from wasting effort on non-user-facing internals.

### 11.6 Keep orchestration simple

Use GitHub issues, PRs, labels, and comments as the control plane. Do not add Linear, a dashboard, or a custom scheduler until the simple GitHub-based loop is reliable.

A run only needs to answer:

```text
What issue is being worked?
What state is it in?
What evidence exists?
What should happen next?
```

### 11.7 Defer semantic vision review

Start with deterministic visual diff metrics. If the metric result is unclear, label the PR `human-review`.

Only add vision-model review after there is repeated evidence that deterministic metrics are too weak.

### 11.8 Model policy

Use Composer 2.5 for all implementation work. Keep GPT/Codex models for scouting, validation, verdicts, and escalation.

Prefer reasoning that is slightly too high rather than too low. The cost is acceptable because the factory should run small issues with strict gates.

Recommended default routing:

```text
Issue readiness gate:        GPT-5.5 high
Scout / refusal check:       GPT-5.5 high
Implementation:              Composer 2.5 via Cursor CLI
Implementation retry:        Composer 2.5 via Cursor CLI, fresh context
Evidence summary:            GPT-5.5 high
Holdout validation:          GPT-5.5 high
Final merge verdict:         GPT-5.5 xhigh for risky or visual PRs, high otherwise
Self-healing triage:         GPT-5.5 high
Self-healing repair:         GPT-5.5 high
Automation/governance work:  GPT-5.5 xhigh plus human review, no auto-merge at first
```

Rules:

- Do not use the implementation run as the final validator.
- Composer may implement normal issues and update tests, but it should not make the final auto-merge decision.
- If Cursor CLI does not expose an explicit reasoning-effort knob for Composer 2.5, use the strongest/default Composer mode available and compensate with stricter prompts, small issues, and holdout validation.
- Use `xhigh` only for final risky verdicts, repeated failures, or protected automation/governance decisions.
- Protected-file or workflow changes require human review regardless of model.

---

## 12. Concrete first 12 issues

These issues should be small enough for auto-merge pilots and useful for the final goal.

### DF-001 — Add minimal mission and factory rules

Owned files:

```text
MISSION.md
FACTORY_RULES.md
```

Goal: define the external-only C++ PyQtGraph parity contract and auto-merge rules.

Auto-merge: yes, if docs-only validation passes.

### DF-002 — Add issue readiness checker

Owned files:

```text
scripts/factory/check_issue_ready.py
tests/factory/test_check_issue_ready.py
```

Goal: validate issue size, owned files, dependencies, TDD plan, and validation level.

Auto-merge: yes.

### DF-003 — Add PR scope checker

Owned files:

```text
scripts/factory/check_pr_scope.py
tests/factory/test_check_pr_scope.py
```

Goal: verify PR changes stay within issue-owned files and shared integration exceptions.

Auto-merge: yes.

### DF-004 — Add Archon issue readiness workflow

Owned files:

```text
.archon/workflows/pgcpp-issue-ready.yaml
.archon/commands/pgcpp-issue-ready.md
```

Goal: run the readiness checker and apply labels/comments.

Auto-merge: yes, after manual review of this automation PR unless you are comfortable auto-merging workflow changes.

### DF-005 — Add Archon implementation workflow skeleton

Owned files:

```text
.archon/workflows/pgcpp-fix-issue.yaml
.archon/commands/pgcpp-*.md
```

Goal: scout one ready issue, refuse if unsafe, implement only if clear, and open a PR with a compact evidence packet. No auto-merge logic here.

Auto-merge: probably no for first workflow PR; human review recommended.

### DF-006 — Add holdout validation and auto-merge workflow

Owned files:

```text
.archon/workflows/pgcpp-validate-pr.yaml
scripts/factory/apply_pr_verdict.py
tests/factory/test_apply_pr_verdict.py
```

Goal: validate PRs independently, verify the evidence packet, write a validator verdict, and auto-merge only when gates pass.

Auto-merge: no for this workflow PR; human review recommended.

### DF-007 — Add synthetic regression workflow

Owned files:

```text
.archon/workflows/pgcpp-comprehensive-test.yaml
scripts/factory/file_regression_issue.py
tests/factory/test_file_regression_issue.py
```

Goal: prove self-healing issue creation with a synthetic failing fixture.

Auto-merge: no for first version; human review recommended.

### DF-008 — Reclassify Python-only manifest entries

Owned files:

```text
port_manifest.yaml
scripts/generate_manifest
<manifest tests>
```

Goal: stop treating Python-only internals as missing C++ work.

Auto-merge: yes if generated-file checks pass.

### DF-009 — Add external parity matrix generator

Owned files:

```text
port_manifest.yaml
scripts/generate_parity_matrix
reports/status.md
<tests>
```

Goal: track user-visible features/examples instead of only source files.

Auto-merge: yes.

### DF-010 — Downstream install smoke

Owned files:

```text
tests/downstream/CMakeLists.txt
tests/downstream/main.cpp
scripts/gate
<tests>
```

Goal: prove `find_package(pyqtgraph-cpp)` works.

Auto-merge: yes.

### DF-011 — SimplePlot external contract issue

Owned files:

```text
examples/SimplePlot.cpp
include/pyqtgraph/<needed headers>
src/pyqtgraph/<needed sources>
tests/visual/<case>
reports/visual-diffs/SimplePlot/
```

Goal: one simple PyQtGraph-style plot looks correct.

Auto-merge: yes only when visual gate is reliable.

### DF-012 — Self-healing real regression pilot

Goal: intentionally break or simulate one known small visual/numeric regression, file an issue automatically, fix it through the factory, validate, and auto-merge.

Auto-merge: yes for the repair PR if gates pass.

---

## 13. Final recommended operating mode

After the migration is complete, the normal loop should be:

```text
1. You create a small external-behavior issue.
2. You add `ai:ready`.
3. Readiness gate stamps `factory:ready-checked` or blocks it.
4. Archon scouts the issue in read-only mode and refuses if it is unsafe or unclear.
5. Archon implements the issue in an isolated worktree only after the scout passes.
6. Archon opens a PR with a compact evidence packet.
7. Holdout validator runs deterministic, oracle, visual, scope, and review gates.
8. Validator fixes once if needed.
9. Validator writes a verdict and auto-merges only if all gates pass.
10. Post-merge smoke runs.
11. Nightly self-healing is added later, after the basic loop is reliable.
```

The project remains simple because:

- issues are small;
- scope is external behavior only;
- Python-only internals are not treated as missing work;
- validation is deterministic wherever possible;
- visual parity is tested by examples;
- every PR carries a compact evidence packet;
- auto-merge is allowed only after independent holdout validation;
- stale or ambiguous runs ask for human action instead of guessing;
- self-healing files evidence-backed issues, not vague tasks.

---

## 14. One-sentence project rule

> Build the smallest native C++ Qt library that makes PyQtGraph-style examples compile, run, behave, and look like the pinned PyQtGraph reference — and let Archon automatically merge only evidence-backed, holdout-validated improvements.
