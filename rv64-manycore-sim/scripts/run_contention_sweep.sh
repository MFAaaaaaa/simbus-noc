#!/usr/bin/env bash
set -euo pipefail

build_dir="${1:-build}"
csv_path="${2:-${build_dir}/results/contention.csv}"
simulator="${build_dir}/rv64_manycore_sim"
example_dir="${build_dir}/examples"

if [[ ! -x "${simulator}" ]]; then
  echo "simulator not found: ${simulator}" >&2
  exit 1
fi
if [[ -e "${csv_path}" ]]; then
  echo "refusing to overwrite existing result: ${csv_path}" >&2
  exit 1
fi
mkdir -p "$(dirname "${csv_path}")"

workloads=(vector_stream_private vector_stream_hotspot vector_stream_gap)
for workload in "${workloads[@]}"; do
  binary="${example_dir}/${workload}.bin"
  if [[ ! -f "${binary}" ]]; then
    echo "workload not found: ${binary}" >&2
    exit 1
  fi
  for cores in 1 2 4 8; do
    echo "running workload=${workload} cores=${cores}"
    "${simulator}" "${binary}" \
      --cores "${cores}" \
      --tohost 0x80001000 \
      --link-width 4 \
      --route-latency 3 \
      --router-buffer 2 \
      --memory-latency 8 \
      --memory-queue 2 \
      --max-ticks 2000000 \
      --workload "${workload}" \
      --csv "${csv_path}"
  done
done

echo "wrote ${csv_path}"
