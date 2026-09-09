#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 SOURCE.S OUTPUT_DIRECTORY" >&2
  exit 2
fi

source_file=$1
output_dir=$2
project_root=$(cd "$(dirname "$0")/.." && pwd)
tool_prefix=${RISCV_TOOL_PREFIX:-riscv64-unknown-elf-}
gcc=${tool_prefix}gcc
objcopy=${tool_prefix}objcopy
nm=${tool_prefix}nm

for tool in "$gcc" "$objcopy" "$nm"; do
  command -v "$tool" >/dev/null || {
    echo "required RISC-V tool not found: $tool" >&2
    exit 1
  }
done

mkdir -p "$output_dir"
name=$(basename "$source_file" .S)
elf="$output_dir/$name.elf"
binary="$output_dir/$name.bin"

"$gcc" -march=rv64im -mabi=lp64 -mno-relax -nostdlib -nostartfiles \
  -Wl,--build-id=none -T "$project_root/examples/rv64_raw.ld" \
  -o "$elf" "$source_file"
"$objcopy" -O binary "$elf" "$binary"

tohost=$($nm -n "$elf" | awk '$3 == "tohost" { print "0x" $1; exit }')
if [[ -z "$tohost" ]]; then
  echo "tohost symbol is missing from $elf" >&2
  exit 1
fi

echo "binary=$binary"
echo "tohost=$tohost"
