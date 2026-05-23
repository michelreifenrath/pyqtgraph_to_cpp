# PyQtGraph → C++ Port Workflow

This is the canonical project specification for translating PyQtGraph into a native C++ library in this repository.

It adapts the uploaded `pyqtgraph_cpp_port_workflow_en.md` to the current repository and automation state:

- Repository: `michelreifenrath/pyqtgraph_to_cpp`
- Local checkout: `/home/michel/code/pyqtgraph_to_cpp`
- Automation runtime config: `WORKFLOW.md`
- Durable orchestration: GitHub Issues + Hermes Kanban board `pyqtgraph-to-cpp`
- Implementation engine: Pi CLI through the `pi-worker` Hermes profile
- Review/release: `pi-reviewer` + `pi-release-manager`
- Workspaces: `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-<number>`

`WORKFLOW.md` remains the machine-readable Pi Symphony automation config. This document is the human/agent-facing product and engineering specification.

---

## 1. Goal

Translate PyQtGraph into a native C++ library that:

1. can be used directly from C++ applications;
2. keeps PyQtGraph class names, file names, object names, hierarchy, and example names as close as practical;
3. uses Qt/C++ as the primary UI/rendering stack;
4. uses OpenCV and C++ math/data structures instead of NumPy where appropriate;
5. looks and behaves like the pinned PyQtGraph reference;
6. is validated by tests, examples, screenshots, interaction probes, and performance benchmarks.

The project is not a Python wrapper and must not drift into a redesigned plotting API unless a specific issue explicitly authorizes a divergence.

---

## 2. Current automation model

### 2.1 Source of truth

GitHub Issues are the external source of truth.

Hermes Kanban board `pyqtgraph-to-cpp` is the internal durable task graph and audit trail.

Issue labels control automation:

- `ai:ready`: eligible for automation.
- `ai:claimed`: already claimed by automation.
- `ai:blocked`: blocked by missing dependency or failed gate.
- `ai:review`: AI-created PR is ready for human review.
- `ai:failed`: automation failed after retry budget or hard gate.
- `ai:done`: automation completed the issue.
- `ai:ignore`: never automate this issue.
- `human-review`: require explicit human review before merge or further automation.
- `tenant:core`: default tenant.
- `tenant:cpp`: C++ translation tenant.
- `tag:*`: subsystem or phase tag.

### 2.2 Automation profiles

The existing repository workflow uses these Hermes profiles:

- `pi-orchestrator`: intake/reconciliation only.
- `pi-worker`: runs Pi CLI/pi-subagents in isolated worktrees.
- `pi-reviewer`: deterministic checks plus mandatory autoreview/Codex review gate.
- `pi-release-manager`: commits, pushes, opens/updates PRs, never merges.

Automation must never push to `main` and must never enable auto-merge.

### 2.3 Step-by-step issue promotion

Do **not** label the whole backlog `ai:ready` at once.

Recommended policy:

1. Create fine-grained issues for the whole roadmap.
2. Give every issue explicit dependencies in the issue body.
3. Label only dependency-free issues as `ai:ready`.
4. When an issue is merged, promote the next unblocked issue by adding `ai:ready`.
5. Keep `max_concurrent_issues` low while the C++ foundation is unstable.

The current `WORKFLOW.md` already sets `max_concurrent_issues: 2`. For this project, use one active foundation issue at a time until CMake, reference pinning, the manifest, and the oracle harness exist.

---

## 3. Non-negotiable engineering rules

1. The result is a C++ library, not a Python wrapper.
2. Qt/C++ is the primary UI and rendering framework.
3. OpenCV and C++ math/data structures replace NumPy where appropriate.
4. PyQtGraph class names, object names, folder names, and example names remain close to upstream.
5. Every implementation issue is test-driven.
6. Every implementation issue must add or update tests before production implementation.
7. Every PyQtGraph example must eventually have a corresponding C++ example with the same base name.
8. Every completion path must run the local gate and the configured autoreview gate.
9. Agents may edit only explicitly owned files listed in the issue body.
10. The reference PyQtGraph source version must be pinned before translation starts.
11. If behavior is unclear, write a PyQtGraph oracle probe before guessing.
12. Visual parity requires screenshots or an explicit validation level that does not require visuals.
13. Performance work requires a baseline, an optimized measurement, and no visual regression.
14. Merge decisions remain human-controlled.

---

## 4. Reference baseline

The first bootstrap milestone must pin a single PyQtGraph source revision.

Recommended baseline candidate:

```text
pyqtgraph-0.14.0
```

Required files:

```text
reference/PYQTGRAPH_REF
reference/source.lock
port_manifest.yaml
```

Required manifest shape:

```yaml
reference:
  repo: https://github.com/pyqtgraph/pyqtgraph
  ref: pyqtgraph-0.14.0
  pinned_commit: <exact commit sha>
  docs_url: https://pyqtgraph.readthedocs.io/
```

Do not translate from `master` unless a human explicitly changes the baseline policy.

Canonical upstream sources:

- PyQtGraph repository: https://github.com/pyqtgraph/pyqtgraph
- PyQtGraph docs: https://pyqtgraph.readthedocs.io/
- PyQtGraph examples: https://github.com/pyqtgraph/pyqtgraph/tree/master/pyqtgraph/examples
- PyQtGraph graphicsItems: https://github.com/pyqtgraph/pyqtgraph/tree/master/pyqtgraph/graphicsItems
- Qt Graphics View Framework: https://doc.qt.io/qt-6/graphicsview.html
- OpenCV `cv::Mat`: https://docs.opencv.org/4.x/d3/d63/classcv_1_1Mat.html
- Pi: https://pi.dev/
- Autoreview source: https://github.com/steipete/agent-scripts/tree/main/skills/autoreview

### 4.1 License and attribution policy

Translated or adapted source files must include a source note near the top of the file. Required fields:

```text
Source note: translated/adapted from PyQtGraph <upstream-path>
PyQtGraph ref: pyqtgraph-0.14.0
Pinned commit: a20028b98294b9cc8770f2015a92eb342224b788
License: MIT; see THIRD_PARTY_NOTICES.md
```

Generated files must identify their generator and inputs. Required fields:

```text
Generated by <tool/script>
Inputs: <source manifest/upstream path>
Do not edit manually
```

Use `Do not edit manually` when the generated output should only be changed by rerunning the generator. Original project files may instead say:

```text
Original implementation; no PyQtGraph source translation
```

Project-authored tests and benchmarks are under the project license unless a file source note states adaptation from PyQtGraph or another source.

---

## 5. Target repository structure

The first C++ bootstrap issues should create this structure incrementally. Do not create all files as empty placeholders unless the issue says so.

```text
pyqtgraph_to_cpp/
  README.md
  WORKFLOW.md                 # existing automation runtime config
  docs/
    pyqtgraph-cpp-port-workflow.md
    ai-orchestration.md
    classes/
    examples/
  AGENTS.md                   # project instructions for Pi/Hermes workers
  CMakeLists.txt
  CMakePresets.json
  port_manifest.yaml
  ownership.yaml
  reference/
    PYQTGRAPH_REF
    source.lock
    pyqtgraph/                # pinned upstream checkout or linked source mirror
  cmake/
    PyQtGraphCppOptions.cmake
    PyQtGraphCppWarnings.cmake
    PyQtGraphCppSanitizers.cmake
  include/pyqtgraph/
  src/pyqtgraph/
  examples/
  tests/
    core/
    graphicsItems/
    widgets/
    imageview/
    opengl/
    examples/
    hierarchy/
    oracle/
    visual/
    performance/
  oracle/
    fixtures/
    scripts/
  scripts/
    bootstrap_reference
    generate_manifest
    claim_ticket
    gate
    run_autoreview
    run_changed_examples
    run_all_examples
    run_performance
    summarize_status
  prompts/
    port-ticket.md
    review-ticket.md
    fix-review-findings.md
  reports/
    agents/
    gates/
    visual-diffs/
    benchmarks/
```

---

## 6. Naming and mapping rules

For every upstream Python file:

```text
pyqtgraph/<path>/<Name>.py
```

create:

```text
include/pyqtgraph/<path>/<Name>.hpp
src/pyqtgraph/<path>/<Name>.cpp
```

Examples:

```text
pyqtgraph/graphicsItems/PlotCurveItem.py
  -> include/pyqtgraph/graphicsItems/PlotCurveItem.hpp
  -> src/pyqtgraph/graphicsItems/PlotCurveItem.cpp

pyqtgraph/widgets/PlotWidget.py
  -> include/pyqtgraph/widgets/PlotWidget.hpp
  -> src/pyqtgraph/widgets/PlotWidget.cpp
```

Use namespace `pyqtgraph`:

```cpp
namespace pyqtgraph {
class PlotWidget;
class PlotItem;
class ViewBox;
}
```

A convenience alias may be documented:

```cpp
namespace pg = pyqtgraph;
```

Avoid unnecessary renames such as `FastPlotWidget`, `CppViewBox`, or `NativePlotItem`.

---

## 7. Architecture targets

### 7.1 Rendering architecture

Use Qt Graphics View for 2D parity because PyQtGraph is built around scenes, items, transforms, mouse events, and QPainter drawing.

Target ownership model:

```text
PlotWidget QWidget
  owns GraphicsView/QGraphicsView
    owns GraphicsScene/QGraphicsScene
      contains GraphicsItems
        ViewBox
        PlotItem
        AxisItem
        PlotCurveItem
        ScatterPlotItem
        ImageItem
        ROI
        InfiniteLine
```

### 7.2 Numeric/data architecture

Use three data representations:

- `ArrayView<T>`: non-owning NumPy-like shape/stride view for numeric algorithms.
- `std::vector<T>` / `std::span<T>`: fast 1D curve data and zero-copy user input.
- `cv::Mat`: images, dense 2D matrices, multi-channel buffers, LUTs, and image processing.

Core rule: do not copy data unless source lifetime or layout requires it.

### 7.3 NumPy replacement policy

| PyQtGraph / NumPy pattern | C++ replacement |
| --- | --- |
| `np.ndarray` line data | `ArrayView<double>`, `std::span<const double>`, `std::vector<double>` |
| `np.ndarray` image data | `cv::Mat` or `ArrayView<T, 2>` |
| slicing/view | `ArrayView` with shape/stride |
| `np.nanmin`, `np.nanmax` | C++ loop with explicit NaN policy |
| LUT application | `cv::LUT` or SIMD loop depending on dtype/layout |
| `np.clip`, levels | templated C++ loop, SIMD where useful |
| transforms | `QTransform`, `QMatrix4x4`, custom `Transform3D` |

---

## 8. Build and validation baseline

Baseline stack:

- C++20
- CMake 3.26+
- Qt 6 first
- Qt 5 compatibility only if a later issue explicitly requires it
- OpenCV 4.x
- Qt Test for tests
- Google Benchmark or nanobench for benchmarks

Standard commands once CMake exists:

```bash
cmake --preset dev
cmake --build --preset dev --parallel
ctest --preset dev --output-on-failure
```

Visual tests:

```bash
QT_QPA_PLATFORM=offscreen ctest --preset visual --output-on-failure
```

Performance tests:

```bash
cmake --build --preset release --parallel
ctest --preset performance --output-on-failure
```

Until `scripts/gate` exists, the repository-level Python automation tests must continue to pass:

```bash
python3 -m pytest -q
python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md
```

---

## 9. Oracle strategy

The C++ port is validated against the pinned PyQtGraph reference.

Oracle types:

1. Numeric oracle: Python/PyQtGraph produces JSON or binary fixtures; C++ compares values.
2. Visual oracle: Python and C++ render screenshots; a diff tool checks configured tolerances.
3. Interaction oracle: scripted mouse/keyboard events are applied to Python and C++ examples.
4. Performance oracle: benchmark scenarios record FPS, frame time, memory, allocations, and copy counts.

Screenshot comparison reports must include:

- image dimensions;
- mean absolute pixel delta;
- max pixel delta;
- changed pixel percentage;
- bounding boxes of differences;
- diff image path.

Pixel-perfect equality is not required unless an issue explicitly declares it.

### 9.1 Visual validation levels

Every implementation issue whose result can affect pixels must declare a visual validation level in the issue body or `port_manifest.yaml`:

```yaml
validation:
  numeric: required|optional|not_applicable
  visual: required|optional|not_applicable
  interaction: required|optional|not_applicable
  gpt_visual_review: required_for_pr|optional|not_applicable
```

Recommended classification:

- `not_applicable`: pure data structures, hierarchy-only skeletons, numeric helpers with no direct rendered output.
- `optional`: color helpers, pen/brush helpers, LUT generation, early placeholders, or utilities where numeric tests remain authoritative but a visual swatch/gradient catches obvious regressions.
- `required`: image conversion, painter paths, widgets, examples, interaction transforms, layout, axes, and anything where visual parity is part of correctness.

### 9.2 Required visual artifact layout

Visual checks must write reproducible artifacts under `reports/visual-diffs/<case>/`:

```text
reports/visual-diffs/<case>/
  reference.png
  actual.png
  diff.png
  metrics.json
  gpt5_vision_review.md        # when gpt_visual_review != not_applicable
```

`metrics.json` must be machine-readable and include at least:

```json
{
  "case": "SimplePlot",
  "dimensions": [800, 600],
  "mean_abs_delta": 1.8,
  "max_delta": 33,
  "changed_pixel_percent": 0.42,
  "ssim": 0.992,
  "tolerance": {
    "max_changed_pixel_percent": 1.0,
    "min_ssim": 0.98
  },
  "deterministic_verdict": "pass"
}
```

### 9.3 AI-assisted semantic visual review

For `gpt_visual_review: required_for_pr`, deterministic pixel metrics are necessary but not sufficient. A vision-capable reviewer such as GPT-5.5 must compare the reference screenshot, C++ screenshot, diff image, and metrics, and write a structured review:

```yaml
verdict: pass | fail | uncertain
blocking_differences:
  - <differences that must be fixed before PR readiness>
non_blocking_differences:
  - <acceptable antialiasing/font/platform differences>
likely_causes:
  - transform | color | antialiasing | layout | data | other
recommendation: merge_ok | needs_fix | human_review
```

Use this prompt shape for the semantic review:

```text
Compare the pinned PyQtGraph reference screenshot and the C++ port screenshot.
Check plot structure, axes, curve geometry, colors, spacing, labels, clipping, and obvious rendering artifacts.
Ignore tiny antialiasing, font rasterization, and platform differences when metrics are within tolerance.
Return verdict, blocking differences, non-blocking differences, likely causes, and recommendation.
```

If deterministic metrics fail but GPT-5.5 says the result looks acceptable, do not auto-approve; mark the issue `human-review`. If deterministic metrics pass but GPT-5.5 finds a semantic rendering error, block or request human review.

### 9.4 Candidate taxonomy

Prioritize visual validation for:

1. `makeQImage` and image conversion helpers: RGB/BGR swaps, stride mistakes, alpha loss, flips, and contrast errors.
2. `ColorMap` LUTs: gradient bars and stop markers in addition to numeric LUT fixtures.
3. `mkColor`, `mkPen`, `mkBrush`: swatches, line-width/dash samples, fills, and alpha blending.
4. `PlotCurveItem::paint`: line geometry, clipping, transforms, pen behavior, and empty-plot detection.
5. `AxisItem`, `PlotItem`, `PlotWidget`, examples: layout, labels, ticks, spacing, and full-scene parity.
6. `ViewBox` and interactions: before/after screenshots plus numeric range/transform fixtures.

Do not use visual checks as the primary proof for `ArrayView`, `Point`, `Vector`, NaN min/max helpers, or hierarchy-only skeletons.

---

## 10. Issue format for this repository

Every implementation issue must use this structure in its body.

```md
## Goal
<single outcome>

## Dependencies
- #<issue> or `none`

## Owned files
The agent may edit only:
- <exact file path>

## Scope
Implement:
- <small behavior 1>
- <small behavior 2>

Do not implement:
- <excluded behavior>

## TDD plan
Failing tests to add first:
- [ ] <test path or validation command>

Expected initial failure:
- <specific failure before implementation>

Pass condition:
- <observable condition>

## Visual validation
- Level: `required|optional|not_applicable`
- GPT-5.5 semantic review: `required_for_pr|optional|not_applicable`
- Required artifacts, if applicable:
  - `reports/visual-diffs/<case>/reference.png`
  - `reports/visual-diffs/<case>/actual.png`
  - `reports/visual-diffs/<case>/diff.png`
  - `reports/visual-diffs/<case>/metrics.json`
  - `reports/visual-diffs/<case>/gpt5_vision_review.md`

## Validation commands
```bash
<commands>
```

## Done definition
- [ ] tests fail before implementation where applicable
- [ ] tests pass after implementation
- [ ] affected examples pass required validation
- [ ] visual artifacts and GPT-5.5 semantic review are present when visual validation requires them
- [ ] names and paths match upstream where relevant
- [ ] hierarchy check passes if inheritance changed
- [ ] autoreview passed or findings resolved
- [ ] reports/agents/<ticket>.md written when this is an implementation issue
```

Every issue must include `tenant:cpp` unless it is purely automation infrastructure.

Only issues with no unmet dependencies should get `ai:ready`.

---

## 11. Phase plan adapted to current state

The old Phase 0 in the uploaded document included setting up Pi/Hermes automation. That is already present in this repository. The adapted phases start from the C++ port bootstrap.

### Phase A: C++ project bootstrap and governance

Purpose: create the C++ project skeleton and make the automation understand the port rules.

Initial issues:

1. `PGBOOT-001` Create C++ repo skeleton and CMake baseline.
2. `PGBOOT-002` Add `AGENTS.md` and Pi prompt templates for C++ port work.
3. `PGBOOT-003` Add ownership enforcement skeleton and `scripts/claim_ticket`.
4. `PGBOOT-004` Add `scripts/gate` and `scripts/run_autoreview` wrappers.
5. `PGBOOT-005` Pin PyQtGraph reference source.
6. `PGBOOT-006` Add license attribution workflow.
7. `PGBOOT-007` Add CI skeleton.

Exit criteria:

- CMake configures.
- Empty C++ test suite runs.
- Python automation tests still pass.
- `scripts/gate commit` runs.
- `scripts/run_autoreview` fails safely when missing configuration and passes when configured.
- `ownership.yaml` can prevent conflicting claims.

### Phase B: Inventory and manifest

Initial issues:

1. `PGINV-001` Generate upstream file inventory.
2. `PGINV-002` Generate example inventory.
3. `PGINV-003` Generate class inventory.
4. `PGINV-004` Generate PyQtGraph hierarchy manifest.
5. `PGINV-005` Create initial `port_manifest.yaml`.
6. `PGINV-006` Categorize examples by validation level.

Exit criteria:

- Every upstream source file is represented in `port_manifest.yaml`.
- Every upstream example is represented.
- Every top-level class has a target C++ path.

### Phase C: Oracle and visual harness

Initial issues:

1. `PGORACLE-001` Python screenshot renderer.
2. `PGORACLE-002` C++ screenshot renderer placeholder.
3. `PGORACLE-003` Screenshot diff tool.
4. `PGORACLE-004` Numeric oracle runner.
5. `PGORACLE-005` Interaction script runner.
6. `PGORACLE-006` First `SimplePlot` visual oracle.

Exit criteria:

- PyQtGraph `SimplePlot` reference screenshot is generated.
- C++ placeholder screenshot is generated.
- Screenshot diff report is produced.
- Visual diff reports follow the standard artifact layout and metrics schema.
- GPT-5.5 semantic visual review is recorded for cases marked `gpt_visual_review: required_for_pr`.
- Example validation can fail and pass deterministically.

### Phase D: Core data and utilities

Start after Phase A and enough manifest/oracle work exists.

Initial issues:

1. `PGCORE-001` `ArrayView` skeleton and tests.
2. `PGCORE-002` `ArrayView` shape/stride/slice behavior.
3. `PGCORE-003` `Point`.
4. `PGCORE-004` `Vector`.
5. `PGCORE-005` `mkColor`.
6. `PGCORE-006` `mkPen`/`mkBrush`.
7. `PGCORE-007` `ColorMap` skeleton.
8. `PGCORE-008` LUT generation.
9. `PGCORE-009` NaN-aware min/max helpers.
10. `PGCORE-010` `makeQImage`/image conversion helpers.

### Phase E: Qt GraphicsView foundation

Initial issues:

1. `PGGI-001` `GraphicsItem` base.
2. `PGGI-002` `GraphicsObject` base.
3. `PGGI-003` `GraphicsWidget` base.
4. `PGSCENE-001` `GraphicsScene` shell.
5. `PGSCENE-002` mouse event wrappers.
6. `PGVIEW-001` `ViewBox` skeleton.
7. `PGVIEW-002` `ViewBox` range model.
8. `PGVIEW-003` `ViewBox` pan/zoom math.

### Phase F: Basic plotting MVP

Initial issues:

1. `PGPLOT-001` `PlotData`.
2. `PGPLOT-002` `PlotCurveItem` skeleton.
3. `PGPLOT-003` `PlotCurveItem::setData` tests.
4. `PGPLOT-004` `PlotCurveItem` paint.
5. `PGPLOT-005` `AxisItem` skeleton.
6. `PGPLOT-006` `PlotItem` skeleton.
7. `PGPLOT-007` `PlotWidget` skeleton.
8. `PGEXAMPLE-001` Port `SimplePlot.cpp` and validate smoke.

### Later phases

Continue with Images/LUT/OpenCV, ROI/interactive items, widgets/application tools, OpenGL/3D, exporters, packaging, docs, performance, and final acceptance once the MVP is stable.

---

## 12. Agent prompt baseline

`prompts/port-ticket.md` should instruct Pi workers:

```md
You are porting PyQtGraph to C++ in `michelreifenrath/pyqtgraph_to_cpp`.

Follow `AGENTS.md`, `WORKFLOW.md`, and `docs/pyqtgraph-cpp-port-workflow.md` strictly.

Work only on the assigned GitHub issue.
Edit only files listed in the Owned files section.
Use test-driven development.
Keep class names, file names, and hierarchy aligned with PyQtGraph.
Use Qt/C++ for GUI and rendering.
Use OpenCV or C++ math/data structures instead of NumPy.
Validate affected examples.
For any pixel-affecting work, declare the visual validation level, generate `reports/visual-diffs/<case>/` artifacts, and request GPT-5.5 semantic visual review when `gpt_visual_review` is `required_for_pr`.
Do not push to `main`.
Do not merge.
Do not commit manually unless the issue specifically asks you to implement `scripts/agent_commit` behavior.

Before coding:
1. Read the issue body.
2. Read the upstream PyQtGraph source when relevant.
3. Read affected examples/tests.
4. Add or update tests first.
5. Confirm the focused test fails for the expected reason.

After coding:
1. Run focused tests.
2. Run changed examples when relevant.
3. Run `scripts/gate commit` once available.
4. Run `scripts/run_autoreview --mode commit` once available.
5. Write `reports/agents/<ticket>.md`.
```

---

## 13. Completion dashboard

Maintain `reports/status.md` once manifest generation exists. It should show:

- upstream source files total;
- translated files;
- class status counts;
- examples complete/failing;
- visual diffs failing;
- performance benchmarks recorded;
- open autoreview findings;
- blocked tickets.

---

## 14. Final acceptance

The project is done only when:

- every PyQtGraph example has a C++ counterpart;
- every C++ example passes its validation level;
- all core hierarchy checks pass;
- all tests pass on required platforms;
- performance benchmarks are recorded;
- no blocking autoreview findings remain;
- package install works;
- a downstream C++ app builds with `find_package(pyqtgraph-cpp)`;
- a human has reviewed and approved the final state.

Final acceptance command set, once all scripts exist:

```bash
scripts/gate merge
scripts/run_all_examples --visual --interaction --performance
scripts/run_autoreview --mode merge --base origin/main
scripts/summarize_status --require-complete
```
