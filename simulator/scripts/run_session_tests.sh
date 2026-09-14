#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
simulator_dir="$(cd "$script_dir/.." && pwd)"
build_dir="$simulator_dir/build/headless"
mkdir -p "$build_dir"

g++ -std=c++20 -Wall -Wextra -Werror \
  -I"$simulator_dir/Source/UTSCore/Public" \
  "$simulator_dir/Source/UTSCore/Private/Config/ConfigurationStore.cpp" \
  "$simulator_dir/Source/UTSCore/Private/Session/SessionManager.cpp" \
  "$simulator_dir/Tests/Session/session_manager_tests.cpp" \
  -o "$build_dir/session_manager_tests"

"$build_dir/session_manager_tests" "$simulator_dir/Config/ProfileSets"
