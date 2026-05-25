# P0.08 performance baseline

`baseline.json` is the source-tree fixture used by `scripts/run_performance --mode compare` and the CTest test `P0.08.performance.schema`.

CTest writes measured metrics to:

```text
build/release/reports/performance/P0.08/metrics.json
```

Refresh the baseline only with an explicit rationale, for example:

```sh
scripts/run_performance \
  --mode update-baseline \
  --baseline reports/performance/P0.08/baseline.json \
  --rationale "Explain why the P0.08 schema-proof baseline changed."
```

Do not compare against a baseline generated in the same invocation; create/update mode is separate from compare mode by design.
