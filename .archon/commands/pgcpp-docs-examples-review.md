---
description: Review docs and examples impact for pgcpp changes.
argument-hint: (reads workflow artifacts)
---

# pgcpp docs/examples review

Act as an independent reviewer for docs and examples. Do not edit files.

Run only when docs/examples/user-facing names changed or the issue requires examples. Check:
- PyQtGraph example names and hierarchy are preserved unless the issue explicitly approves a C++ equivalent.
- Example code is native C++/Qt and fits existing style.
- Docs/comments are necessary, accurate, and not speculative.
- Required example validation or visual evidence exists.

Write `$ARTIFACTS_DIR/review-docs-examples.md` with verdict, findings, and self-fixability.
