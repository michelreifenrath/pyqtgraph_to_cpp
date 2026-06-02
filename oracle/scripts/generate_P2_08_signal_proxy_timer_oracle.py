#!/usr/bin/env python3
"""Generate/check the P2.08 SignalProxy/ThreadsafeTimer oracle fixture.

The fixture records deterministic behavior from the pinned PyQtGraph 0.14.0
sources without requiring a Qt binding at generation time.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_LOCK = ROOT / "reference" / "source.lock"
SIGNAL_PROXY = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "SignalProxy.py"
THREADSAFE_TIMER = ROOT / "reference" / "pyqtgraph" / "pyqtgraph" / "ThreadsafeTimer.py"
FIXTURE = ROOT / "oracle" / "fixtures" / "P2_08" / "signal_proxy_timer_oracle.json"
PINNED_REF = "pyqtgraph-0.14.0"
PINNED_COMMIT = "a20028b98294b9cc8770f2015a92eb342224b788"


def source_paths_available() -> bool:
    return all(path.exists() for path in (SOURCE_LOCK, SIGNAL_PROXY, THREADSAFE_TIMER))


def require_pinned_sources() -> None:
    missing = [
        path
        for path in (SOURCE_LOCK, SIGNAL_PROXY, THREADSAFE_TIMER)
        if not path.exists()
    ]
    if missing:
        names = ", ".join(str(path.relative_to(ROOT)) for path in missing)
        raise SystemExit(f"Pinned PyQtGraph checkout is unavailable; missing {names}")

    lock_text = SOURCE_LOCK.read_text(encoding="utf-8")
    if PINNED_COMMIT not in lock_text or PINNED_REF not in lock_text:
        raise SystemExit(
            "reference/source.lock does not match the P2.08 pinned PyQtGraph ref/commit"
        )

    signal_text = SIGNAL_PROXY.read_text(encoding="utf-8")
    timer_text = THREADSAFE_TIMER.read_text(encoding="utf-8")
    required_snippets = [
        "self.args = args",
        "self.timer.start(int(self.delay * 1000) + 1)",
        "leakTime = max(0, (lastFlush + (1.0 / self.rateLimit)) - now)",
        "self.sigDelayed.emit(args)",
        "self.blockSignal = True",
        "self.timer.moveToThread(QtCore.QCoreApplication.instance().thread())",
        "self.sigTimerStartRequested.emit(timeout)",
        "self.timeout.emit()",
    ]
    for snippet in required_snippets:
        if snippet not in signal_text and snippet not in timer_text:
            raise SystemExit(
                f"Pinned source did not contain expected behavior snippet: {snippet!r}"
            )


def build_fixture() -> dict[str, object]:
    return {
        "issue": "P2.08",
        "upstream": {
            "project": "pyqtgraph",
            "ref": PINNED_REF,
            "commit": PINNED_COMMIT,
            "files": ["pyqtgraph/SignalProxy.py", "pyqtgraph/ThreadsafeTimer.py"],
        },
        "timing_tolerance_ms": 35,
        "cases": [
            {
                "name": "no_rate_limit_restart_ms",
                "delay_seconds": 0.04,
                "rate_limit": 0,
                "expected_timer_ms": int(0.04 * 1000) + 1,
                "behavior": "each received signal replaces args and restarts the delay timer",
            },
            {
                "name": "rate_limit_first_signal_ms",
                "delay_seconds": 0.2,
                "rate_limit": 20,
                "expected_timer_ms": 1,
                "behavior": "first rate-limited signal has no lastFlushTime, so leakTime is zero",
            },
            {
                "name": "rate_limit_after_flush_ms",
                "delay_seconds": 0.2,
                "rate_limit": 20,
                "expected_minimum_spacing_ms": int((1.0 / 20) * 1000),
                "behavior": "later signals are throttled by max(0, lastFlush + 1/rateLimit - now)",
            },
            {
                "name": "flush_without_args_returns_false",
                "behavior": "flush returns false when no args are queued and emits nothing",
            },
            {
                "name": "flush_with_args_returns_true_and_clears",
                "behavior": "flush stops the timer, clears queued args, updates lastFlushTime, emits latest args, and returns true",
            },
            {
                "name": "disconnect_blocks_future_signals",
                "behavior": "disconnect sets blockSignal true, preventing later signalReceived/flush emissions",
            },
            {
                "name": "threadsafe_timer_worker_requests_are_queued",
                "behavior": "start/stop execute directly on the app thread and are requested through queued signals from other threads",
            },
        ],
        "unsupported_python_only_api": [
            "bound-signal constructor wiring",
            "weakref slot management",
            "SignalBlock context manager returned by block()",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify the fixture is current instead of writing it",
    )
    parser.add_argument(
        "--require-source",
        action="store_true",
        help="fail if the optional pinned PyQtGraph checkout is absent",
    )
    args = parser.parse_args()

    expected = json.dumps(build_fixture(), indent=2, sort_keys=True) + "\n"

    if args.require_source or not args.check or source_paths_available():
        require_pinned_sources()

    if args.check:
        if not FIXTURE.exists():
            raise SystemExit(f"Missing oracle fixture: {FIXTURE.relative_to(ROOT)}")
        actual = FIXTURE.read_text(encoding="utf-8")
        if actual != expected:
            raise SystemExit(
                f"Oracle fixture is stale: regenerate {FIXTURE.relative_to(ROOT)}"
            )
        print(f"P2.08 oracle fixture OK: {FIXTURE.relative_to(ROOT)}")
        return 0

    FIXTURE.parent.mkdir(parents=True, exist_ok=True)
    FIXTURE.write_text(expected, encoding="utf-8")
    print(f"Wrote {FIXTURE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
