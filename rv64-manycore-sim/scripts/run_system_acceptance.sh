#!/usr/bin/env bash
set -euo pipefail
build_dir="${1:-build}"
result_dir="${2:-${build_dir}/results/system-acceptance}"
if [[ -e "${result_dir}" ]]; then
  echo "refusing to overwrite existing results: ${result_dir}" >&2
  exit 1
fi
mkdir -p "${result_dir}"
bash "$(dirname "$0")/run_contention_sweep.sh" "${build_dir}" "${result_dir}/rvv.csv"
for cores in 1 2 4 8; do
  "${build_dir}/rv64_manycore_sim" "${build_dir}/examples/mmio_bus_smoke.bin" \
    --cores "${cores}" --tohost 0x80001000 --receive-slots 1 \
    --memory-latency 100 --memory-queue 1 --router-buffer 2 --link-width 4 \
    --workload mmio_roundtrip --csv "${result_dir}/mmio.csv"
done
# Same slow-memory workload, toggle destination reservations only.
for slots in 0 1; do
  "${build_dir}/rv64_manycore_sim" "${build_dir}/examples/vector_stream_private.bin" \
    --cores 8 --tohost 0x80001000 --receive-slots "${slots}" \
    --memory-latency 100 --memory-queue 1 --router-buffer 2 --link-width 4 \
    --max-ticks 2000000 --workload slow_memory_credit_comparison \
    --csv "${result_dir}/credit-comparison.csv"
done
