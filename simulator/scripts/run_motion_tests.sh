#!/usr/bin/env bash
set -euo pipefail
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
simulator_dir="$(cd "$script_dir/.." && pwd)"
build_dir="$simulator_dir/build/headless"
mkdir -p "$build_dir"
g++ -std=c++20 -Wall -Wextra -Werror \
  -I"$simulator_dir/Source/UTSCore/Public" \
  "$simulator_dir/Tests/Vehicle/synthetic_tractor_motion_tests.cpp" \
  -o "$build_dir/synthetic_tractor_motion_tests"
"$build_dir/synthetic_tractor_motion_tests"
