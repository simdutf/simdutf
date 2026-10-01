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

# Build against C++23 so the documentation captures every public function,
# including the std::span overloads (gated on __cpp_lib_span, C++20+) and the
# std::text_encoding-based API (gated on __cpp_lib_text_encoding, C++23). A
# lower standard would silently drop those declarations from the corpus.
CMAKE_ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSIMDUTF_TESTS=OFF \
  -DSIMDUTF_BENCHMARKS=OFF -DCMAKE_BUILD_TYPE=Release -DSIMDUTF_CXX_STANDARD=23"
MRDOCS_ARGS=""

# MrDocs ships its own Clang. On macOS that Clang has no default sysroot, so we
# point it at the active SDK. On Linux the bundled Clang's libc search path
# does not always match the host (e.g. Fedora's /usr/include layout), so ask
# MrDocs to fall back to the system libc headers.
if [ "$(uname)" = "Darwin" ]; then
  SDK=$(xcrun --show-sdk-path)
  CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_OSX_SYSROOT=$SDK"
  MRDOCS_ARGS="--libc-includes=$SDK/usr/include"
else
  MRDOCS_ARGS="--use-system-libc"
fi

echo "==> Configuring (compile_commands.json) in build_mrdocs/"
# shellcheck disable=SC2086
cmake -S . -B build_mrdocs $CMAKE_ARGS

# Wipe any previous output before regenerating. MrDocs derives each page's
# filename from a symbol-ID hash, so when the set of declarations changes
# (e.g. switching C++ standard adds/removes the std::span overloads) the old
# pages are not overwritten -- they linger as orphans. A stale single-overload
# binary_to_base64.html surviving a C++23 rebuild is exactly how an obsolete
# page (missing the span overload) ends up published. Start from a clean slate.
echo "==> Cleaning previous output (doc/mrdocs-html)"
rm -rf doc/mrdocs-html

echo "==> Running MrDocs"
# shellcheck disable=SC2086
mrdocs mrdocs.yml $MRDOCS_ARGS

echo "==> Building sidebar navigation + search (mrdocs-nav.js)"
python3 scripts/mrdocs_nav.py

echo "==> API reference written to doc/mrdocs-html"
