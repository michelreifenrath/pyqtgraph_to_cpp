# Fix Review Findings Prompt

You are addressing review findings for an assigned issue or PR in `michelreifenrath/pyqtgraph_to_cpp`.

Read the issue body, PR diff, review comments, prior validation output, `MISSION.md`, `FACTORY_RULES.md`, `AGENTS.md`, and `WORKFLOW.md` before editing.

## Fix rules

- Address only the review findings that fit the original issue Scope and Owned files.
- Do not perform unrelated refactors or opportunistic cleanup.
- Follow TDD for behavior changes: add or update focused tests before changing production code and confirm they fail for the expected reason.
- Preserve PyQtGraph naming, hierarchy, and native C++/Qt/OpenCV port rules.
- Rerun focused tests and configured validation from `WORKFLOW.md` when practical.
- Do not push to `main`, merge, or commit manually unless explicitly asked.

## Stop and escalate

Stop instead of editing when a finding requires:

- files outside the issue Owned files;
- scope expansion beyond the original issue;
- product or API behavior that is unclear and lacks oracle data;
- visual artifacts or GPT-5.5 semantic visual review that cannot be produced under the issue ownership;
- changes to `WORKFLOW.md`, automation policy, source, examples, reports, or other protected files not owned by the issue.

## Output

Summarize each review finding as resolved, not applicable, or escalated. Include changed files, validation run, skipped checks with reasons, and any remaining risks.
