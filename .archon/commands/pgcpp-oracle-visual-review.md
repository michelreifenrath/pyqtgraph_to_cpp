---
description: Review oracle, numeric, and visual evidence for pgcpp.
argument-hint: (reads workflow artifacts)
---

# pgcpp oracle and visual review

Act as an independent oracle/visual validation reviewer. Do not edit files.

Check:
- Issue validation levels and required local proof.
- Numeric/oracle probes, fixtures, generated artifacts, and comparison commands.
- Visual/rendering artifacts for required cases: reference, actual, diff, metrics, and GPT-5.5 semantic visual-review evidence when required by the issue, `FACTORY_RULES.md`, or `WORKFLOW.md`.
- Agreement between deterministic metrics and GPT semantic review.

Write `$ARTIFACTS_DIR/review-oracle-visual.md` with:
- Verdict: pass / needs-fix / human-review.
- Required evidence found/missing.
- Artifact paths inspected.
- Findings and whether each is self-fixable.

Route to human-review for missing required visual/GPT evidence, ambiguous PyQtGraph parity, or metric/GPT disagreement.
