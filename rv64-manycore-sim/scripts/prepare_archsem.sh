#!/usr/bin/env bash
set -Eeuo pipefail

source_root="$1"
patched_root="$2"

case "$patched_root" in
  */rv64-archsem-patched) ;;
  *) echo "refusing unexpected patched destination: $patched_root" >&2; exit 2 ;;
esac

rm -rf -- "$patched_root"
mkdir -p -- "$patched_root"
cp -a -- "$source_root/CMakeLists.txt" "$patched_root/"
cp -a -- "$source_root/include" "$patched_root/"
cp -a -- "$source_root/riscv64" "$patched_root/"
cp -a -- "$source_root/softfloat" "$patched_root/"

perl -0pi -e 's/struct(?: alignas\(1\))? ExpdMaskT\s*\{\s*uint8_t value;\s*inline operator bool\(\) const\s*\{\s*return value != 0;\s*\}\s*inline ExpdMaskT& operator=\(const bool& b\)\s*\{\s*value = b \? 1 : 0;\s*return \*this;\s*\}\s*\};/using ExpdMaskT = int;/s' \
  "$patched_root/riscv64/rvv/vcommon.hpp"

find "$patched_root/riscv64" -name '*.cpp' -exec sed -i 's/#include <bit>/#include <cstring>/' {} +
sed -i 's/inst\.id/inst.instName/g' "$patched_root/riscv64/rvv/rvv64.cpp"
sed -i 's/static std::array<ExpdMaskT, MAX_VL> AllTrueExpdMask{1};/static std::array<ExpdMaskT, MAX_VL> AllTrueExpdMask{}; AllTrueExpdMask.fill(1);/' \
  "$patched_root/riscv64/rvv/rvv64.cpp"
sed -i -E 's/bit_cast<([_[:alnum:]]+)>\(([^()]*)\)/static_cast<\1>(\2)/g' \
  "$patched_root/riscv64/rvi/rvi64.cpp" "$patched_root/riscv64/rva/rva64.cpp"
