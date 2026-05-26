# Performance benchmark schema

P0.08 defines the first machine-readable performance proof for this port. It is a schema and gate-behavior check, not a product benchmark for a specific pyqtgraph feature.

## Runner

Use `scripts/run_performance`.

Required compare mode:

```sh
scripts/run_performance \
  --mode compare \
  --baseline reports/performance/P0.08/baseline.json \
  --output build/release/reports/performance/P0.08/metrics.json \
  --synthetic-regression-check
```

Modes are deliberately separate:

- `compare`: reads an existing source-tree baseline and writes measured metrics to the requested output path. CTest writes this output under the build tree so validation does not dirty the source tree.
- `create-baseline`: writes a new baseline to `--baseline`.
- `update-baseline`: rewrites `--baseline` and requires `--rationale`.

Compare mode refuses to use the same path for `--baseline` and `--output`, so a run cannot compare against a baseline it is also generating. Baseline creation/update is explicit and separate; update mode records the rationale.

## Workload and dataset

The P0.08 workload is `python-deterministic-transform-sort`:

- dataset size: 4096 deterministic integers
- warmup iterations: 2
- measured repetitions: 9
- operation: numeric transform, sort, and checksum

## Captured report fields

Reports and baselines use `schema_version: p0.08-performance-v1` and include:

- workload name, description, dataset size, warmup count, and repetition count
- environment capture: Python version, platform, machine, processor, CPU count, and current working directory; versioned baseline fixtures scrub `cwd` to `<repo-root>` while transient metrics keep the measured run's full working directory
- raw distribution samples in seconds
- measured stats: min, median, p95, and max seconds
- checksum for workload sanity
- baseline stats and threshold metadata
- threshold decision containing current value, baseline value, ratio, allowed ratio, and pass/fail
- synthetic failure check result when `--synthetic-regression-check` is used

## Threshold

The P0.08 baseline gates `median_seconds` with `max_ratio: 20.0`. This intentionally loose threshold validates the reporting and regression-decision path without treating the Python schema proof as a native C++ product-performance benchmark.
