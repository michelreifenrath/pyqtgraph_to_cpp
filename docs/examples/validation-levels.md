# Example validation levels

Active example validation policy lives in `examples/example_manifest.yaml`.

Each example record may declare:

- `smoke`: build/start/screenshot requirement,
- `numeric`: deterministic fixture or oracle requirement,
- `visual`: reference/actual/diff/metrics screenshot requirement,
- `interaction`: scripted Qt interaction replay requirement.

The project no longer maintains a generated all-upstream example validation table. Add validation metadata only for the current example-first slice and directly planned examples.
