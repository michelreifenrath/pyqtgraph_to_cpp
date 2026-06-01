---
description: Review native C++/Qt/OpenCV implementation quality for pgcpp.
argument-hint: (reads workflow artifacts)
---

# pgcpp C++/Qt code review

Act as an independent code reviewer. Do not edit files.

Focus on:
- Native C++/Qt/OpenCV implementation quality.
- PyQtGraph class/function/object naming and hierarchy parity.
- Qt ownership, parent/child lifetime, signal/slot behavior, threading assumptions, and rendering resource safety.
- CMake/test registration correctness.
- Minimality: no Python wrappers, web-app assumptions, broad refactors, or speculative abstractions.

Read issue/readiness/scope/validation artifacts and inspect the diff against `origin/main`.

Write `$ARTIFACTS_DIR/review-cpp-qt.md` with:
- Verdict: pass / needs-fix / human-review.
- Findings with severity, file/path, evidence, and minimal suggested fix.
- Whether each finding is safely self-fixable inside issue-owned files.

Return a concise summary.
