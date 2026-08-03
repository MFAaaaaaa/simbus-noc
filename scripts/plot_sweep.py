#!/usr/bin/env python3
"""Plot noc_benchmark sweep CSV files.

The script intentionally depends only on the Python standard library plus
matplotlib.  It does not require pandas, so it is easy to run on lab machines.
"""

import argparse
import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean


def parse_args():
    parser = argparse.ArgumentParser(description="Plot simbus_noc benchmark sweep results")
    parser.add_argument("csv_file", help="Input CSV produced by run_sweep.sh or noc_benchmark")
    parser.add_argument("--out-dir", default="plots", help="Directory for generated PNG files")
    parser.add_argument(
        "--metrics",
        nargs="*",
        default=[
            "avg_packet_latency",
            "throughput_bytes_per_tick",
            "blocked_cycles",
            "congested_cycles",
            "delivery_ratio",
            "source_blocked_messages",
            "undelivered_messages",
        ],
        help="CSV metric columns to plot",
    )
    return parser.parse_args()


def load_rows(path):
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        raise SystemExit(f"no rows found in {path}")
    return rows


def to_float(row, key):
    try:
        return float(row[key])
    except KeyError as exc:
        raise SystemExit(f"CSV is missing required column: {key}") from exc
    except ValueError as exc:
        raise SystemExit(f"CSV column {key} contains non-numeric value: {row[key]!r}") from exc


def group_rows(rows, metric):
    grouped = defaultdict(list)
    for row in rows:
        key = (
            row.get("topology", "unknown"),
            row.get("pattern", "unknown"),
            row.get("congestion", "unknown"),
        )
        grouped[key].append((to_float(row, "offered_load"), to_float(row, metric)))

    # Average repeated runs with the same load, for example different seeds.
    series = {}
    for key, pts in grouped.items():
        by_load = defaultdict(list)
        for load, value in pts:
            by_load[load].append(value)
        series[key] = sorted((load, mean(values)) for load, values in by_load.items())
    return series


def safe_name(s):
    return "".join(ch if ch.isalnum() or ch in "._-" else "_" for ch in s)


def plot_metric(rows, metric, out_dir):
    try:
        import matplotlib.pyplot as plt
    except ImportError as exc:
        raise SystemExit(
            "matplotlib is required for plotting. Install it with: python3 -m pip install matplotlib"
        ) from exc

    series = group_rows(rows, metric)
    patterns = sorted({key[1] for key in series})
    outputs = []

    for pattern in patterns:
        fig, ax = plt.subplots(figsize=(8, 5))
        used = False
        for (topology, pat, congestion), pts in sorted(series.items()):
            if pat != pattern:
                continue
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            label = f"{topology}, congestion={congestion}"
            ax.plot(xs, ys, marker="o", label=label)
            used = True

        if not used:
            plt.close(fig)
            continue

        ax.set_xlabel("offered_load")
        ax.set_ylabel(metric)
        ax.set_title(f"{metric} vs offered_load ({pattern})")
        ax.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
        ax.legend()
        fig.tight_layout()
        out = out_dir / f"{safe_name(metric)}_{safe_name(pattern)}.png"
        fig.savefig(out, dpi=160)
        plt.close(fig)
        outputs.append(out)

    return outputs


def print_summary(rows):
    # Compact textual summary that is useful in terminals without image viewers.
    important = [
        "pattern",
        "congestion",
        "offered_load",
        "throughput_bytes_per_tick",
        "avg_packet_latency",
        "blocked_cycles",
        "congested_cycles",
        "delivery_ratio",
        "undelivered_messages",
        "drain_timeout",
    ]
    available = [k for k in important if k in rows[0]]
    print(",".join(available))
    for row in rows:
        print(",".join(row.get(k, "") for k in available))


def main():
    args = parse_args()
    rows = load_rows(args.csv_file)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    print_summary(rows)

    outputs = []
    for metric in args.metrics:
        outputs.extend(plot_metric(rows, metric, out_dir))

    print(f"generated {len(outputs)} plot(s) in {out_dir}")
    for out in outputs:
        print(out)


if __name__ == "__main__":
    main()
