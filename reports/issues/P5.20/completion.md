# P5.20 JoystickButton and VerticalLabel completion report

- Issue: GitHub #261 / P5.20
- Validation class: interaction-ui

## Summary

Implemented native Qt/C++ `JoystickButton` with checkable press/drag/release interaction, squared-radius normalized state, black spot painting, and `sigStateChanged`. Implemented `VerticalLabel` with vertical/horizontal orientation, rotated text rendering, swapped size hints, and min/max geometry updates.

## Validation commands

| Command | Exit code |
| --- | ---: |
| `cmake --preset dev` | 0 |
| `cmake --build --preset dev --parallel` | 0 |
| `QT_QPA_PLATFORM=offscreen ctest --preset dev -L P5.20 --output-on-failure` | 0 |
| `python3 -m pytest -q` | 0 |
| `git diff --check` | 0 |
| `git diff --name-only origin/main...HEAD` | 0 |
