Did not write `/pgcore-007/scout-context.md` because task also says “Do not edit files”; per instruction, no-edit wins.

Scout findings summary:
- Git: clean worktree. Branch `ai/issue-26-ai-pgcore-007-add-colormap-skeleton` currently at `4a81f10`, same as `origin/main`.
- No existing `ColorMap`/`colormap` C++ files.
- Dependency PGCORE-005 is present: `include/pyqtgraph/functions.hpp`, `src/pyqtgraph/functions.cpp`, `tests/core/test_mkColor.cpp`.
- CMake pattern: Qt-gated source/test registration in root `CMakeLists.txt` lines 20-77.
- Add likely files:
  - `include/pyqtgraph/colormap.hpp`
  - `src/pyqtgraph/colormap.cpp`
  - `tests/core/test_ColorMap.cpp`
  - update `CMakeLists.txt`
  - implementation report `reports/agents/PGCORE-007.md`
- Manifest maps `ColorMap` to upstream `pyqtgraph/colormap.py`, target header/source, subsystem core, base `object`, line 337.
- Local pinned upstream checkout appears absent despite `reference/source.lock`; no local `pyqtgraph/colormap.py` found.