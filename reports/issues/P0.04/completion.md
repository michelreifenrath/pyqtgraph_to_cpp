# P0.04 Completion

## Implemented behavior

- Added `scripts/check_manifest_ownership`, a local gate that reads `port_manifest.yaml` and `ownership.yaml` from `--root` and validates the `manifest_owners` section.
- The gate checks manifest sections `source_files`, `classes`, `examples`, `example_assets`, and `example_validation_levels`.
- `source_selectors` cover `source_files` and `classes`; `example_selectors` cover `examples`, `example_assets`, and `example_validation_levels`.
- The gate fails when an issue id does not match `P<digits>.<digits>`, a selector matches no manifest rows, or any checked manifest row is unowned.
- Failure messages include the manifest section and upstream path for unowned rows, and the selector field plus selector text for stale selectors.

## Changed-file ownership notes

- `scripts/check_manifest_ownership`: P0.04-owned manifest ownership gate.
- `tests/test_manifest_ownership.py`: P0.04-owned focused pytest coverage for pass, unowned-row failure, and stale-selector failure.
- `ownership.yaml`: preserved `version: 1` and `claims: []`; added compact deterministic `manifest_owners` selectors using the local P-issue IDs that own the checked-in manifest rows.
- `reports/issues/P0.04/completion.md`: P0.04 completion evidence.

## Manifest/dashboard applicability

- Manifest applicability: yes. The new gate validates ownership coverage for the checked-in `port_manifest.yaml` rows.
- Dashboard applicability: not applicable in this change; no dashboard file or generator was edited.

## Validation

Initial TDD expected failure before implementation:

```text
$ python3 -m pytest -q tests -k P0_04
FFF                                                                      [100%]
... can't open file '/home/michel/code/ai-workspaces/pyqtgraph_to_cpp/issue-98/scripts/check_manifest_ownership': [Errno 2] No such file or directory
3 failed, 295 deselected in 0.47s
exit code: 1
```

Final validation:

```text
$ python3 -m pytest -q tests -k P0_04
...                                                                      [100%]
3 passed, 295 deselected in 0.20s
exit code: 0
```

Independent tester rerun:

```text
$ python3 -m pytest -q tests -k P0_04
...                                                                      [100%]
3 passed, 295 deselected in 0.19s
exit code: 0
```

```text
$ scripts/check_manifest_ownership --root .
manifest ownership check passed
exit code: 0
```

Failure-mode fixture for a missing owner:

```text
$ scripts/check_manifest_ownership --root "$tmp_fixture_with_unowned_row"
unowned manifest row: section=source_files upstream_path=pyqtgraph/Unowned.py
exit code: 1
```

Independent tester rerun:

```text
$ scripts/check_manifest_ownership --root .
manifest ownership check passed
exit code: 0
```

```text
$ python3 -m pytest -q tests/test_claim_ticket.py
..........                                                               [100%]
10 passed in 0.63s
exit code: 0
```

Independent tester rerun:

```text
$ python3 -m pytest -q tests/test_claim_ticket.py
..........                                                               [100%]
10 passed in 0.61s
exit code: 0
```

```text
$ scripts/check_proposed_issues --source github --repo michelreifenrath/pyqtgraph_to_cpp
github-issue-97.md: blocked-by entry does not match a local issue: P0.02
github-issue-98.md: blocked-by entry does not match a local issue: P0.02
github-issue-103.md: blocked-by entry does not match a local issue: P0.02
github-issue-110.md: blocked-by entry does not match a local issue: P0.07
github-issue-111.md: blocked-by entry does not match a local issue: P0.08
github-issue-118.md: blocked-by entry does not match a local issue: P1.04
github-issue-212.md: blocked-by entry does not match a local issue: P1.01
github-issue-120.md: blocked-by entry does not match a local issue: P0.06
github-issue-124.md: blocked-by entry does not match a local issue: P1.03
github-issue-127.md: blocked-by entry does not match a local issue: P1.04
github-issue-129.md: blocked-by entry does not match a local issue: P0.01
github-issue-130.md: blocked-by entry does not match a local issue: P1.01
github-issue-166.md: blocked-by entry does not match a local issue: P0.01
github-issue-180.md: blocked-by entry does not match a local issue: P1.04
github-issue-195.md: blocked-by entry does not match a local issue: P0.01
github-issue-197.md: blocked-by entry does not match a local issue: P0.01
github-issue-201.md: blocked-by entry does not match a local issue: P1.01
github-issue-202.md: blocked-by entry does not match a local issue: P0.02
github-issue-207.md: blocked-by entry does not match a local issue: P0.01
exit code: 1
```

The proposed-issues check failed on pre-existing GitHub blocked-by metadata that does not resolve to local issue ids; no out-of-scope files were edited.

```text
$ git diff --check
exit code: 0
```

Independent tester rerun: exit code 0.

```text
$ git diff --name-only origin/main...HEAD
exit code: 0
```

The branch has no commits beyond `origin/main`, so the exact required range command lists no paths. The active worktree paths are:

```text
$ git status --short --untracked-files=all
 M ownership.yaml
?? reports/issues/P0.04/completion.md
?? scripts/check_manifest_ownership
?? tests/test_manifest_ownership.py
exit code: 0
```
