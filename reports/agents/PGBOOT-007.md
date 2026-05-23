# PGBOOT-007 Implementation Report

## Summary

Implemented the CI skeleton for the native C++/Qt/OpenCV port. Added GitHub Actions jobs for Linux, macOS, Windows, visual validation, and performance recording; expanded CMake presets to match the workflow; and documented the bootstrap CI contract.

## Changed files

- `.github/workflows/ci.yml` - new CI skeleton with platform, visual, and performance-record jobs.
- `CMakePresets.json` - added `release`, `ci-linux`, `ci-macos`, `ci-windows`, `visual`, and `performance` preset coverage while preserving `dev`; Windows CI uses Ninja to avoid multi-config ambiguity.
- `docs/pyqtgraph-cpp-port-workflow.md` - documented the CI job/preset contract and skeleton-safe artifact behavior.
- `reports/agents/PGBOOT-007.md` - this implementation report.

## CI jobs and presets

- `linux`: `ci-linux` configure/build/test presets.
- `macos`: `ci-macos` configure/build/test presets.
- `windows`: `ci-windows` configure/build/test presets.
- `visual`: `visual` configure/build/test presets with `QT_QPA_PLATFORM=offscreen`; uploads `reports/visual-diffs/` with `if-no-files-found: ignore`.
- `performance-record`: `release` configure/build preset and `performance` test preset; uploads `reports/benchmarks/` with `if-no-files-found: ignore`.

## Validation

- PASS: `git diff --check`
- PASS: `python3 -m pytest -q` - 99 passed in 14.12s.
- PASS: `python3 -m automation.pi_symphony.cli validate-workflow --workflow WORKFLOW.md` - workflow valid.
- PASS: `cmake --list-presets=configure` - listed `dev`, `release`, `ci-linux`, `ci-macos`, `ci-windows`, `visual`.
- PASS: `cmake --list-presets=build` - listed `dev`, `release`, `ci-linux`, `ci-macos`, `ci-windows`, `visual`.
- PASS: `ctest --list-presets` - listed `dev`, `release`, `ci-linux`, `ci-macos`, `ci-windows`, `visual`, `performance`.
- PASS: `cmake --preset dev && cmake --build --preset dev --parallel && ctest --preset dev --output-on-failure` - configured with Qt 6/OpenCV 4, built, and passed 1 smoke test.
- PASS: `cmake --preset visual && ctest --preset visual --output-on-failure` - configured and completed successfully with no `visual` labeled tests yet.
- PASS: `cmake --preset release && ctest --preset performance --output-on-failure` - configured and completed successfully with no `performance` labeled tests yet.

## Known limitations

- The visual and performance jobs are skeleton-safe placeholders. Full screenshot diff, semantic visual review, benchmark harnesses, and labeled tests remain for later issues.
- The Windows dependency installation uses standard setup actions/Chocolatey for the CI skeleton; it may need follow-up tuning once the workflow runs on GitHub-hosted Windows runners.

## PR / handoff note

No changes were made to `WORKFLOW.md`, automation policy scripts, source code, tests, or Python wrapper behavior. The implementation keeps the project direction as a native C++/Qt/OpenCV port.
