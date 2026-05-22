# Review Ticket Prompt

You are independently reviewing an assigned issue or PR for `michelreifenrath/pyqtgraph_to_cpp`.

Review against the issue body, its Owned files, `AGENTS.md`, `WORKFLOW.md`, and `docs/pyqtgraph-cpp-port-workflow.md`. Do not fix code directly unless explicitly assigned to implement fixes.

## Review scope

- Confirm the diff stays within the issue Scope and Owned files.
- Verify the change preserves the native C++/Qt/OpenCV port direction and does not introduce Python wrappers or NumPy-style runtime dependencies.
- Check PyQtGraph naming, file layout, object hierarchy, and example naming consistency where applicable.
- Confirm unclear behavior was handled with an oracle probe or explicit escalation.
- Check that safety policy was followed: no main push, no merge, and no manual commit unless explicitly authorized.

## Checklist

1. Owned-file compliance: every modified repository file is listed in the issue Owned files.
2. Scope compliance: no unrelated refactors, placeholders, or speculative scaffolding.
3. TDD evidence: behavior changes include tests added or updated before implementation, with focused validation results.
4. Validation: configured checks from `WORKFLOW.md` and relevant focused tests/examples are reported or justified if skipped.
5. Visual validation: pixel-affecting work declares its validation level and includes required artifacts and GPT-5.5 semantic review when required.
6. Native-port rules: Qt/C++ and OpenCV/C++ choices are appropriate for the changed area.
7. Documentation and prompts: guidance changes do not claim automation wiring that does not exist.

## Output

Report findings as actionable items. Mark each item as blocking or non-blocking and include file/line references when possible. If no issues are found, state the checks performed and any residual risks.
