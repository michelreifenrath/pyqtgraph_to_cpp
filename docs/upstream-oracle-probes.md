# Upstream oracle probes

P0.06 establishes the reusable pattern for issue-scoped PyQtGraph oracle probes.

## Probe contract

A probe generator lives under `oracle/scripts/` and reads `reference/source.lock`. The lock supplies the upstream repository, tag/ref, pinned commit, documentation URL, and checkout path. The generator must verify or materialize that exact pinned PyQtGraph source before producing fixture data.

Each committed fixture records:

- `schema_version`
- issue id
- upstream `repo`, `ref`, `pinned_commit`, `checkout_path`, `pyqtgraph_version`, and `pyqtgraph_commit`
- deterministic `inputs`
- PyQtGraph-derived `expected` outputs
- an explicit `tolerance` policy

Use exact tolerance (`absolute: 0.0`, `relative: 0.0`) when values are deterministic. If a future issue needs nonzero tolerance, state why in the fixture and the issue report.

## Adding a future probe

1. Add a focused pytest selected by the issue id before writing the generator.
2. Capture the red failure in `reports/issues/<issue-id>/`.
3. Add or update the issue-scoped generator under `oracle/scripts/` or probe assets under `oracle/probes/<issue-id>/`.
4. Generate the fixture under `oracle/fixtures/<issue-id>/`.
5. Include one mismatch example showing the fixture path, JSON path, fixture value, actual probe/C++ value, and tolerance.
6. Add a minimal C++ comparison when the issue requires CTest coverage, and label its CTest entry with the issue id.

P0.06 commands:

```bash
python3 -m pytest -q tests -k P0_06
ctest --preset dev -L P0.06 --output-on-failure
```

The P0.06 fixture is generated with:

```bash
python3 oracle/scripts/generate_P0_06_oracle_probe.py --emit-mismatch-example
```

Check mode fails if the committed fixture differs from the current pinned probe:

```bash
python3 oracle/scripts/generate_P0_06_oracle_probe.py --check
```

Keep normal pytest coverage hermetic by running check mode against a temporary local reference checkout/source lock. Run the real pinned-source check only when the pinned checkout is present locally or network access is explicitly allowed.
