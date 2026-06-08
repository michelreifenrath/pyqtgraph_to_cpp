# P4.23 ScaleBar GPT visual review

reviewer: GPT-5.5 visual reviewer (implementation-session semantic inspection)
model: gpt-5.5
review_date: 2026-06-08
cases_reviewed:
- ScaleBar/bottom-right-default
- ScaleBar/top-left-offset
- ScaleBar/wide-range-styled
- ScaleBar/mid-panel-negative-offset

source artifacts inspected:
- reports/visual-diffs/ScaleBar/reference.png
- reports/visual-diffs/ScaleBar/actual.png
- reports/visual-diffs/ScaleBar/diff.png
- reports/visual-diffs/ScaleBar/metrics.json

The reference and C++ actual screenshots render the same ScaleBar cases inside a shared ViewBox panel: bottom-right anchored default bar with SI label, top-left positive-offset bar with distinct pen/brush, wide-range styled bar with negative offset anchoring, and a mid-panel negative-offset bar with thicker geometry. Bar widths track view-coordinate mapping, labels sit centered above each bar, and anchor offsets place each scale bar on the expected parent corner. The diff image is black for all rendered scale-bar pixels, matching metrics.json with changed_pixels=0, max_delta=0, mean_delta=0, and passed=true. No bar-width drift, label misplacement, style mismatch, or anchor offset error is visible.

verdict: pass
recommendation: merge_ok
blocking_findings: none
