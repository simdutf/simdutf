#!/bin/sh
# Build the simdutf API reference with MrDocs (https://www.mrdocs.com).
#
# Produces doc/mrdocs-html, which the GitHub Pages workflow publishes at
# https://simdutf.github.io/simdutf/api/. Used by both CI and local previews.
#
# Requirements (must be on PATH): mrdocs, cmake, a C++ compiler, python3.
# Install MrDocs from https://github.com/cppalliance/mrdocs/releases
#
# Usage (from the repository root):  ./scripts/build_api_docs.sh
set -e

cd "$(dirname "$0")/.."

CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSIMDUTF_TESTS=OFF \
  -DSIMDUTF_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=Release"
MRDOCS_ARGS=""

# MrDocs ships its own Clang. On macOS that Clang has no default sysroot, so we
# point it at the active SDK; on Linux the system headers are found by default.
if [ "$(uname)" = "Darwin" ]; then
  SDK=$(xcrun --show-sdk-path)
  CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_SYSROOT=$SDK"
  MRDOCS_ARGS="--libc-includes=$SDK/usr/include"
fi

echo "==> Configuring (compile_commands.json) in build_mrdocs/"
# shellcheck disable=SC2086
cmake -S . -B build_mrdocs $CMAKE_ARGS

echo "==> Running MrDocs"
# shellcheck disable=SC2086
mrdocs mrdocs.yml $MRDOCS_ARGS

echo "==> Building sidebar navigation + search (mrdocs-nav.js)"
python3 scripts/mrdocs_nav.py

echo "==> API reference written to doc/mrdocs-html"
