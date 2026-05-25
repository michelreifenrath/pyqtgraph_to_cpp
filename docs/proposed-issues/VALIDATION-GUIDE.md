<!-- generated-local-issue -->
# Local issue validation guide

This issue set uses **local validation only**. Do not add GitHub Actions or make completion depend on GitHub infrastructure.

Every issue should prove exactly the behavior it owns: strong enough to catch regressions, but no broad full-suite requirement unless the issue is explicitly a rollup/final gate.

## Required closeout for every issue

- Record exact local command(s) run, exit codes, and relevant artifact paths in the completion report.
- Add or update the smallest focused test/check that proves the task.
- Run or record a changed-file check proving every modified path matches the issue's `## Owned files` selectors and common adjuncts. Extra paths require updating the issue before implementation.
- Update manifest/dashboard status only when the task changes manifest-tracked sources, classes, examples, or assets.
- If a check is not applicable, record the reason in the completion report instead of adding fake tests.
- Run `scripts/check_proposed_issues` before publishing, relabeling, or bulk-editing proposed issues; blocked issues must not carry `ai:ready`.

## Owned files selectors

Each issue declares machine-checkable ownership using compact selectors instead of long hand-expanded file lists.

- **Manifest source selectors** expand through `port_manifest.yaml` to the selected upstream source records and their target C++ header/source paths.
- **Manifest example selectors** expand through `port_manifest.yaml::examples` and `example_validation_levels` to target example paths and validation metadata.
- **Repository path globs** are non-manifest support files owned by the issue.
- **Common adjuncts** name limited support sets below; they never grant broad production-source ownership by themselves.

Common adjunct sets:

- `none`: no additional files.
- `focused-tests`: focused tests/probes/reports for the issue, including `tests/**/<issue-id>*`, `tests/**/*<owned-component>*`, `oracle/probes/<issue-id>/**`, `oracle/**/<issue-id>*`, and `reports/issues/<issue-id>/**`.
- `focused-visual`: `focused-tests` plus `tests/visual/**`, `reports/visual/<issue-id>/**`, and visual renderer hooks/scripts only when the issue validates rendering.
- `focused-examples`: owned example files, `tests/examples/**`, `scripts/run_*examples*`, and `reports/examples/<issue-id>/**`.
- `focused-doc-report`: docs named by the issue plus `reports/issues/<issue-id>/**`; no production source unless separately selected.
- `build-plumbing`: build/local-validation plumbing such as `CMakeLists.txt`, `cmake/**`, `CMakePresets.json`, scripts explicitly named by the issue's repository globs, shared script harness files when the issue directly owns local-validation plumbing, `tests/CMakeLists.txt`, named docs, and reports for P0/P1/P10 infrastructure issues.

For shared build files such as `CMakeLists.txt` and `tests/CMakeLists.txt`, implementation issues may edit them only to wire owned targets/tests whose names match the issue-owned component.

## Validation classes

### decision-doc
Proof is a decision/equivalence document, not a code test. AFK decision issues must either define the governing policy or cite an existing parity-contract/local-validation policy. Human approval is optional; if absent, the document must record the conservative default, rationale, affected manifest entries, accepted C++ equivalence or explicit non-port decision, and follow-up issues for disputed or out-of-scope behavior. Add C++ tests only if the issue changes executable behavior.

### manifest-infra
Proof is a local generator/check command plus at least one failure-mode test for stale, missing, or inconsistent metadata. The check must fail clearly when a manifest entry, dashboard row, ownership selector, or status field is wrong.

### script-infra
Proof is a focused pytest/unit test or deterministic dry-run/smoke test for the script. Cover the normal path and at least one meaningful failure path, such as missing input, stale metadata, invalid arguments, empty selection, or nonzero child command propagation. For command-runner scripts, dry-run alone is insufficient: use a fake runner or fixture to verify subprocess plan, working directory, environment, order, and exit-code propagation, plus a minimal integration smoke when practical.

### oracle-infra
Proof is a reusable pinned-PyQtGraph probe pattern and one sample C++ comparison that fails usefully on mismatch. The probe must record upstream version/commit, fixture inputs, outputs, and tolerance policy.

### core-oracle
Proof is a pinned PyQtGraph oracle or approved C++ equivalent, deterministic C++ unit tests, edge cases, and declared numeric/behavior tolerances. For parity-sensitive behavior, enumerate the minimum representative and edge cases instead of relying on a happy path.

### api-runtime
Proof is focused C++ tests for the public API and runtime behavior owned by the issue: construction/destruction, ownership/lifetime, state changes, signals/callbacks/events where relevant, error/edge inputs, and integration with direct dependencies. Assert observable behavior, not only that the object can be instantiated. For PyQtGraph parity-sensitive API behavior, name either a pinned oracle probe or an approved C++ equivalence rationale.

### pixel-image
Proof is deterministic pixel or image-buffer comparison with explicit format, dtype, stride/copy, color order, levels/LUT behavior, and tolerance rules. Use screenshots only as supplemental render evidence unless the issue also owns presentation.

### visual-render
Proof is deterministic local render output: PyQtGraph reference image or approved non-oracle reference, actual C++ image, diff image, metrics JSON, and local report. Fix viewport size, device pixel ratio/DPI, font/theme, random seeds, antialiasing policy, data inputs, camera/range, and output format. The report must include oracle/reference command, upstream version or approved source, fixture data hash, image dimensions, thresholds, and a guard that fails if reference and actual are both blank/placeholder.

Manual visual inspection is required: open/read the reference, actual, and diff images with an image-capable tool and record a short semantic note describing whether the rendered content matches the intended PyQtGraph behavior. Metrics thresholds still gate completion; manual inspection complements them and does not replace them. If image inspection is unavailable, the issue is blocked rather than complete.

### interaction-ui
Proof is a scripted Qt/user-event replay with assertions on the resulting state, signals, model values, geometry, selection, or view range as applicable. Do not count an app launch or screenshot alone as proof. For parity-sensitive UI behavior, record pre-state, event sequence, post-state, emitted signals/callbacks where applicable, and at least one negative/no-op case when meaningful. Add visual artifacts and apply `visual-render` manual-inspection rules only when rendering changes or screenshots are part of the behavior under test.

### opengl-render
Proof follows the approved local OpenGL backend policy and records renderer/vendor/backend, context profile/version, framebuffer size, and whether the path is software/headless/GPU. Include an offscreen/headless smoke test when applicable. For rendering tasks, produce visual artifacts, metrics, and manual visual inspection using the `visual-render` rules. For camera/navigation/input tasks, include scripted state assertions for camera, transform, selection, or item state and at least one deterministic rendered-frame invariant before/after interaction unless the issue explicitly declares non-rendering scope. Add performance evidence only when the task changes rendering throughput, memory use, or large-scene behavior.

### exporter-io
Proof compares generated files against golden expectations or roundtrips them. Include format-specific tolerances and metadata checks for dimensions, units, embedded resources, data values, and unsupported/export-error cases.

### package-consumer
Proof installs to a clean local prefix, configures/builds/runs a fresh downstream CMake project with `find_package(pyqtgraph-cpp)`, and verifies runtime assets/dependencies.

### resource-assets
Proof checks manifest-listed resources exist, are packaged/installed, and can be loaded through the public runtime path.

### example-port
Proof builds and runs the example locally and applies its manifest validation level: numeric, visual, interaction, or not-applicable. Visual examples require metrics plus manual image inspection. Numeric-only examples do not require screenshots unless the manifest says visual validation is required.

### performance
Proof defines the workload, dataset size, warmup, repetition count, environment capture, measured statistics, baseline file, regression threshold, and baseline-update rule. Report enough distribution data to review stability, such as median plus min/max or p95. Include a fixture or synthetic benchmark check that fails when a metric exceeds the threshold. Distinguish create-baseline from compare-to-baseline modes, and reject comparing a run against a baseline generated in the same invocation unless explicitly in baseline-update mode with rationale.

### review-approval
Proof is a local autonomous review artifact with all blocking findings resolved or explicitly waived. The artifact must include local autoreview plus an independent review pass, name reviewer/tool, scope, date, remaining waivers, and the local validation report bundle. Human approval may be attached but is not required for AFK completion.

### rollup-final
Proof is aggregate only: run the named local commands, collect a report bundle, and verify all child tasks are complete. Rollups must verify source-of-truth counts, missing/duplicate/skipped rows, and links back to child proof. Do not duplicate feature-level oracle/visual tests here.
