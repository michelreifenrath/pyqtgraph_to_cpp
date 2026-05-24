# Example validation levels

PGINV-006 records the validation policy for every checked-in PyQtGraph example in `port_manifest.yaml` under the non-generated top-level `example_validation_levels` section.

The generated `examples` inventory remains unchanged. Keeping validation metadata in a separate top-level section lets `scripts/generate_manifest --check` continue to verify only generated inventory fields while preserving reviewable per-example validation policy.

## Schema

Each record mirrors one `examples` entry by `upstream_path`, `name`, and `category`, then declares:

- `numeric`: `required`, `optional`, or `not_applicable`
- `visual`: `required`, `optional`, or `not_applicable`
- `interaction`: `required`, `optional`, or `not_applicable`
- `gpt_visual_review`: `required_for_pr`, `optional`, or `not_applicable`

## Classification policy

- Static-render examples use visual parity as the primary gate: `visual: required`, `interaction: optional`, `gpt_visual_review: required_for_pr`.
- Interactive examples require scripted interaction coverage in addition to visual parity: `interaction: required`.
- Math/helper modules used by demos but not rendered directly use `numeric: required` and visual/interaction as `not_applicable`.
- Loader templates, packaging setup files, package initializers, and test/helper modules with no direct C++ example rendering use `not_applicable` for all validation channels.

## Current inventory summary

- 64 examples: visual required, interaction optional.
- 47 examples: visual required, interaction required.
- 4 examples: numeric required only.
- 14 examples: validation not applicable.

The focused test `tests/oracle/test_example_validation_levels.py` enforces full coverage, schema validity, identity consistency with `examples`, and policy invariants.
