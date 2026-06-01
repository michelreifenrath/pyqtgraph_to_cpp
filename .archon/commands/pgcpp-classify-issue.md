---
description: Classify a ready PyQtGraph-to-C++ issue for routing.
argument-hint: (reads workflow artifacts)
---

# pgcpp classify issue

Use only these artifacts:
- `$ARTIFACTS_DIR/issue.json`
- `$ARTIFACTS_DIR/readiness.json`
- `$ARTIFACTS_DIR/governance.txt`

Do not adapt `MISSION.md` or `FACTORY_RULES.md`; treat them only as already-captured governance context. Do not use DynaChat/FastAPI/Bun/web-app assumptions.

Return only the structured JSON requested by the workflow schema:
- `issue_type`: one of `bug`, `feature`, `enhancement`, `refactor`, `chore`, `documentation`
- `visual_required`: string `true` or `false`
- `oracle_required`: string `true` or `false`
- `title`
- `reasoning`

Classification guidance:
- `bug`: issue evidence describes a failing behavior/regression.
- `feature`/`enhancement`: new C++/Qt PyQtGraph parity behavior or example support.
- `refactor`: explicitly behavior-preserving internal cleanup.
- `chore`: build/oracle/automation maintenance inside issue scope.
- `documentation`: docs-only.
- Mark visual required when the issue validation levels, proof, acceptance criteria, or example/rendering scope requires visual artifacts or GPT-5.5 semantic visual review.
- Mark oracle required when numeric parity, upstream PyQtGraph probe, fixture, or oracle evidence is required.
