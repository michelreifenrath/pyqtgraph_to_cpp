# Implementation Plan

## Goal
Add a minimal native C++ `pyqtgraph::ColorMap` skeleton, focused construction/accessor tests, CMake registration, and the required implementation report without implementing LUT or rendering behavior.

## Tasks
1. **Confirm issue scope and upstream references before coding**: Verify the issue body/Done definition and, if available, pinned upstream `pyqtgraph/colormap.py` at commit `a20028b98294b9cc8770f2015a92eb342224b788` for naming only.
   - File: `include/pyqtgraph/colormap.hpp`
   - Changes: No code change in this step; confirm the public skeleton can stay limited to constructor plus metadata/stop accessors.
   - Acceptance: Implementer can state that the skeleton excludes `map`, `getLookupTable`, interpolation, and rendering/LUT behavior; if upstream source remains unavailable, proceed only with manifest-confirmed class/file names and call out the gap in `reports/agents/PGCORE-007.md`.

2. **Add the failing ColorMap skeleton test first**: Create a focused self-contained test matching existing `tests/core/test_mkColor.cpp` style.
   - File: `tests/core/test_ColorMap.cpp`
   - Changes: Include `../../include/pyqtgraph/colormap.hpp` and Qt `QColor`; add local `CHECK`/`CHECK_EQ` helpers; test constructing `pyqtgraph::ColorMap` from ordered positions and colors, preserving name, stop count, positions, colors, and default name; test validation errors for mismatched position/color counts and empty stops.
   - Acceptance: Before production files/CMake source registration are complete, the focused test should fail to compile or link for the expected missing `ColorMap` API, establishing TDD evidence.

3. **Define the public ColorMap skeleton API**: Add the header-only declarations and simple value types needed by the test and by future LUT work.
   - File: `include/pyqtgraph/colormap.hpp`
   - Changes: Add source note matching project style; include `<QColor>`, `<QString>`, `<cstddef>`, `<vector>`; declare `namespace pyqtgraph { class ColorMap final { ... }; }`. Expected public API skeleton: `ColorMap(std::vector<double> positions, std::vector<QColor> colors, QString name = {});`, `[[nodiscard]] std::size_t size() const noexcept;`, `[[nodiscard]] bool empty() const noexcept;`, `[[nodiscard]] const std::vector<double>& positions() const noexcept;`, `[[nodiscard]] const std::vector<QColor>& colors() const noexcept;`, `[[nodiscard]] const QString& name() const noexcept;`. Private data: vectors for positions/colors and `QString name_`.
   - Acceptance: Header compiles standalone when included by `tests/core/test_ColorMap.cpp`; no Python wrapper, NumPy dependency, or Qt widget/rendering API is introduced.

4. **Implement constructor/accessors only**: Add simple storage and validation logic.
   - File: `src/pyqtgraph/colormap.cpp`
   - Changes: Include `../../include/pyqtgraph/colormap.hpp`; implement constructor with move storage; throw `std::invalid_argument` when positions are empty or position/color sizes differ; implement accessors as trivial returns. Do not sort, normalize, interpolate, synthesize LUTs, or convert color modes.
   - Acceptance: Focused tests for construction/accessors and validation pass; behavior remains a skeleton suitable for later PGCORE-008 LUT work.

5. **Register ColorMap source and test in CMake**: Wire the new Qt-dependent source and test following the existing `mkColor` pattern.
   - File: `CMakeLists.txt`
   - Changes: Under the existing Qt Core/Gui gate, add `src/pyqtgraph/colormap.cpp` to `target_sources(pyqtgraph_cpp PRIVATE ...)`; add `pyqtgraph_cpp_core_colormap` executable from `tests/core/test_ColorMap.cpp`, link it to `pyqtgraph_cpp`, project options, and warnings, enable sanitizers, and register `add_test(NAME pyqtgraph_cpp.core.ColorMap COMMAND pyqtgraph_cpp_core_colormap)`. Prefer renaming the internal gate from `_pyqtgraph_cpp_has_mkcolor` to a broader Qt-dependent flag only if done consistently in this file; otherwise keep the existing flag to avoid unrelated CMake churn.
   - Acceptance: `cmake --build --preset dev --target pyqtgraph_cpp_core_colormap` builds when Qt6 Core/Gui is available; `ctest --preset dev -R pyqtgraph_cpp.core.ColorMap --output-on-failure` discovers and runs the focused test.

6. **Create the required implementation report as a Done-definition exception**: Add the report only if the issue body/automation explicitly requires `reports/agents/PGCORE-007.md` despite the production/test owned-files list.
   - File: `reports/agents/PGCORE-007.md`
   - Changes: Summarize scope, implemented files, TDD/focused validation results, skipped visual validation rationale, and remaining risks such as missing local pinned upstream checkout. Keep it factual and do not add unrelated report artifacts.
   - Acceptance: Done definition is satisfied with a single report file; if the issue owner rejects report edits outside owned files, stop and ask for explicit approval rather than omitting a required Done artifact silently.

7. **Run focused and configured validation**: Validate the implementation after tests pass.
   - File: `CMakeLists.txt`
   - Changes: No additional code changes unless validation exposes a CMake registration error.
   - Acceptance: Run and record results for:
     - `cmake --preset dev`
     - `cmake --build --preset dev --target pyqtgraph_cpp_core_colormap`
     - `ctest --preset dev -R pyqtgraph_cpp.core.ColorMap --output-on-failure`
     - `cmake --build --preset dev`
     - `ctest --preset dev --output-on-failure`
     - `python3 -m pytest -q`
     - `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md`
     - `git diff --check`

## Files to Modify
- `include/pyqtgraph/colormap.hpp` - new `pyqtgraph::ColorMap` class declaration with constructor, accessors, and stored stop/name data.
- `src/pyqtgraph/colormap.cpp` - constructor validation and trivial accessor definitions.
- `tests/core/test_ColorMap.cpp` - focused TDD coverage for construction, stored values, default/custom names, and invalid inputs.
- `CMakeLists.txt` - add the ColorMap source to the Qt-gated library build and register the focused ColorMap test target.

## New Files
- `include/pyqtgraph/colormap.hpp` - public ColorMap skeleton header.
- `src/pyqtgraph/colormap.cpp` - ColorMap skeleton implementation.
- `tests/core/test_ColorMap.cpp` - focused core test executable source.
- `reports/agents/PGCORE-007.md` - required implementation report; create as a Done-definition exception only after confirming the issue requires it.

## Dependencies
- Task 1 gates the exact scope and whether the report exception is allowed.
- Task 2 must precede Tasks 3-5 for TDD evidence.
- Task 3 must precede Task 4.
- Task 5 depends on Tasks 2-4 so the target has source files to build.
- Task 6 depends on completed implementation and validation results from Task 7, though the file path decision should be confirmed in Task 1.
- Task 7 depends on Tasks 3-5 and should be rerun after any fix.

## Risks
- The full issue body was not available at `/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-26/context.md`; if the actual issue specifies a different public API, owned-files list, or validation command, that issue text overrides this plan.
- Local pinned upstream `pyqtgraph/colormap.py` appears absent; do not invent complex PyQtGraph behavior without an oracle/source check. Keep this issue to a skeleton and record the source gap.
- `reports/agents/PGCORE-007.md` may be outside the issue owned-files list even though Done requires it. Treat it as a narrow Done-required exception; stop for human approval if automation/review insists on strict owned-file compliance.
- `ColorMap` LUT generation, interpolation modes, `map`, `getStops`, `getLookupTable`, color-space conversion, image/rendering output, examples, and visual artifacts are non-goals for this skeleton. Stop if tests or review require these behaviors under PGCORE-007; they belong to later issues such as PGCORE-008.
- Because the skeleton is not pixel-affecting, no visual artifacts or GPT visual review should be required. Stop if the issue’s visual-validation section says otherwise and the needed artifact paths are not owned.
