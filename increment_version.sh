#!/bin/bash
set -euo pipefail

if [ "${1:-}" = "" ]; then
  echo "please supply target number to increment to like: 0.1.6"
  exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

current=$(grep '^Version:' packaging/kobiq.spec | awk '{print $2}')
target="$1"

# Caption / binary version comes from CMake PROJECT_VERSION → KOBIQ_VERSION.
# Keep packaging + CMake in sync.
files=(
  CMakeLists.txt
  packaging/kobiq.spec
  packaging/kobiQ.metainfo.xml
  packaging/build-rpm.sh
  packaging/build-rpm-multi.sh
  README.md
  .github/workflows/build-rpms.yml
)

for file in "${files[@]}"; do
  if [ -f "$file" ]; then
    sed -i "s/${current}/${target}/g" "$file"
  fi
done

updated=$(grep '^Version:' packaging/kobiq.spec | awk '{print $2}')
cmake_ver=$(grep -E '^project\(kobiQ VERSION' CMakeLists.txt | awk '{print $3}')
echo "Successfully promoted version from ${current} to ${updated}"
echo "CMake PROJECT_VERSION is now ${cmake_ver}"
if [ "$updated" != "$cmake_ver" ]; then
  echo "ERROR: packaging Version (${updated}) and CMake version (${cmake_ver}) disagree" >&2
  exit 1
fi
