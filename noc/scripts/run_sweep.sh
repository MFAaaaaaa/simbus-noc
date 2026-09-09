#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./build/noc_benchmark}"
OUT="${2:-noc_sweep.csv}"

# Keep this default sweep intentionally small so it finishes quickly on older lab machines.
# Increase ticks/nodes/loads manually for publication-quality experiments.
loads=(0.10 0.50)
patterns=(uniform hotspot)
cc_modes=(0 1)

: > "$OUT"
first=1
for pattern in "${patterns[@]}"; do
  for cc in "${cc_modes[@]}"; do
    for load in "${loads[@]}"; do
      common_args=(
        --topology mesh --nodes 9 --mesh-x 3 --mesh-y 3
        --pattern "$pattern" --offered-load "$load"
        --ticks 300 --drain-ticks 600
        --payload-bytes 32 --channel-width 8 --link-width 16
        --route-latency 2 --congestion "$cc"
        --buffer-limit 8 --high-watermark 6 --low-watermark 2
        --seed 1
      )
      if [[ "$first" -eq 1 ]]; then
        "$BIN" "${common_args[@]}" >> "$OUT"
        first=0
      else
        "$BIN" "${common_args[@]}" --no-header >> "$OUT"
      fi
    done
  done
done

echo "wrote $OUT"
