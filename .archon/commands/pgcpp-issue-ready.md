# pgcpp issue readiness

Purpose: summarize the actual output from the local readiness checker for one issue.

Issue path:

```text
$ARGUMENTS
```

Readiness checker JSON from `check-readiness`:

```json
$check-readiness.output
```

Instructions:

- Do not rerun shell commands.
- Do not inspect or summarize other issues.
- Report whether this issue is ready based only on the JSON above.
- If `ready` is false, list the blocking errors exactly.
- If `ready` is true, summarize the parsed owned files, dependencies, validation levels, and any protected files.
- Do not mutate GitHub labels or comments.
