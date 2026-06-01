---
description: Extract exactly one GitHub issue number for pgcpp issue implementation.
argument-hint: <issue-number>
---

# pgcpp extract issue number

Read `$ARGUMENTS` and output only one decimal GitHub issue number, with no prose, quotes, markdown, or trailing explanation.

Rules:
- Accept forms like `42`, `#42`, or `Fix issue #42`.
- If zero or multiple plausible issue numbers are present, output a short `ERROR:` line explaining the ambiguity.
- Do not use tools. Do not read files. Do not mutate GitHub.
