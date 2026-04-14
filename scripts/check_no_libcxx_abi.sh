#!/bin/sh

#
# Verifies the stricter SIMDUTF_NO_LIBCXX ABI contract by auditing
# src/implementation.cpp and src/simdutf.cpp, then linking native smoke tests
# without implicitly adding a C++ standard library or ABI library.

set -eu

rootdir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
builddir=${BUILD_DIR:-$rootdir/build/no-libcxx-build-contract}
cxx=${CXX:-c++}
cc=${CC:-cc}

implementation_obj=$builddir/implementation-no-libcxx-noeh-nortti.o
implementation_undefined=$builddir/implementation-no-libcxx-noeh-nortti.undefined.txt
implementation_abi_runtime=$builddir/implementation-no-libcxx-noeh-nortti.abi-runtime.txt

simdutf_obj=$builddir/simdutf-no-libcxx-noeh-nortti.o
simdutf_undefined=$builddir/simdutf-no-libcxx-noeh-nortti.undefined.txt
simdutf_abi_runtime=$builddir/simdutf-no-libcxx-noeh-nortti.abi-runtime.txt

dispatch_simdutf_obj=$builddir/simdutf-multi-no-libcxx-noeh-nortti.o
dispatch_smoke_src=$builddir/dispatch_smoke.cpp
dispatch_smoke_obj=$builddir/dispatch_smoke.o
dispatch_smoke_bin=$builddir/dispatch_smoke
dispatch_smoke_deps=$builddir/dispatch_smoke.deps.txt

single_impl_smoke_src=$builddir/single_impl_smoke.cpp
single_impl_smoke_obj=$builddir/single_impl_smoke.o
single_impl_simdutf_obj=$builddir/simdutf-fallback-only-no-libcxx-noeh-nortti.o
single_impl_smoke_bin=$builddir/single_impl_smoke
single_impl_smoke_deps=$builddir/single_impl_smoke.deps.txt

smoke_results=$builddir/smoke-tests.txt

mkdir -p "$builddir"

compile_no_libcxx() {
  "$cxx" -std=c++17 \
    -DSIMDUTF_NO_LIBCXX=1 \
    -fno-exceptions \
    -fno-rtti \
    -I"$rootdir/include" \
    -I"$rootdir/src" \
    "$@"
}

compile_fallback_only_no_libcxx() {
  compile_no_libcxx \
    -DSIMDUTF_IMPLEMENTATION_ICELAKE=0 \
    -DSIMDUTF_IMPLEMENTATION_HASWELL=0 \
    -DSIMDUTF_IMPLEMENTATION_WESTMERE=0 \
    -DSIMDUTF_IMPLEMENTATION_ARM64=0 \
    -DSIMDUTF_IMPLEMENTATION_PPC64=0 \
    -DSIMDUTF_IMPLEMENTATION_RVV=0 \
    -DSIMDUTF_IMPLEMENTATION_LASX=0 \
    -DSIMDUTF_IMPLEMENTATION_LSX=0 \
    -DSIMDUTF_IMPLEMENTATION_FALLBACK=1 \
    "$@"
}

compile_multi_backend_no_libcxx() {
  compile_no_libcxx \
    -DSIMDUTF_IMPLEMENTATION_FALLBACK=1 \
    "$@"
}

if command -v rg >/dev/null 2>&1; then
  regex_search() {
    rg "$@"
  }

  regex_search_ignore_case() {
    rg -i "$@"
  }
else
  regex_search() {
    grep -E "$@"
  }

  regex_search_ignore_case() {
    grep -iE "$@"
  }
fi

record_undefined_symbols() {
  obj=$1
  undefined=$2
  abi_runtime=$3

  nm -u "$obj" | c++filt > "$undefined"

  if ! regex_search \
    '(__|__Unwind|__cxxabiv1|std::terminate|vtable for __cxxabiv1|typeinfo (for|name for)|__dynamic_cast)' \
    "$undefined" > "$abi_runtime"; then
    : > "$abi_runtime"
  fi
}

assert_no_forbidden_abi_symbols() {
  undefined=$1

  if regex_search -n \
    '(__cxa_guard_|__cxa_pure_virtual|__gxx_personality|__cxa_begin_catch|__cxa_throw|__cxa_allocate_exception|__cxa_free_exception|typeinfo for |typeinfo name for |__dynamic_cast|vtable for __cxxabiv1)' \
    "$undefined"; then
    echo "unexpected ABI/runtime dependency found in $undefined" >&2
    exit 1
  fi
}

record_link_dependencies() {
  binary=$1
  output=$2

  if command -v otool >/dev/null 2>&1; then
    otool -L "$binary" > "$output"
  elif command -v ldd >/dev/null 2>&1; then
    ldd "$binary" > "$output"
  elif command -v readelf >/dev/null 2>&1; then
    readelf -d "$binary" > "$output"
  else
    printf '%s\n' "dependency inspection unavailable on this host" > "$output"
  fi
}

assert_no_cpp_runtime_dependencies() {
  deps=$1

  if regex_search_ignore_case '(libc\+\+|libc\+\+abi|libstdc\+\+|libsupc\+\+)' "$deps"; then
    echo "unexpected C++ runtime dependency found in $deps" >&2
    exit 1
  fi
}

compile_no_libcxx -c "$rootdir/src/implementation.cpp" -o "$implementation_obj"
record_undefined_symbols "$implementation_obj" "$implementation_undefined" \
  "$implementation_abi_runtime"
assert_no_forbidden_abi_symbols "$implementation_undefined"

compile_no_libcxx -c "$rootdir/src/simdutf.cpp" -o "$simdutf_obj"
record_undefined_symbols "$simdutf_obj" "$simdutf_undefined" \
  "$simdutf_abi_runtime"
assert_no_forbidden_abi_symbols "$simdutf_undefined"

cat > "$dispatch_smoke_src" <<'EOF'
#include "simdutf.h"
#include <stdlib.h>
#include <string.h>

static int check_lookup_round_trip() {
  const simdutf::implementation *fallback =
      simdutf::get_available_implementations()["fallback"];
  if (fallback == nullptr) {
    return 31;
  }
  if (strcmp(fallback->name(), "fallback") != 0) {
    return 32;
  }
  if (simdutf::get_available_implementations()[fallback->name()] != fallback) {
    return 33;
  }
  return 0;
}

static int check_default_detection() {
  if (simdutf::get_available_implementations().size() < 2) {
    return 11;
  }
  const simdutf::implementation *best =
      simdutf::get_available_implementations().detect_best_supported();
  if (best == nullptr) {
    return 12;
  }
  const simdutf::implementation *before = simdutf::get_active_implementation();
  if (before == nullptr) {
    return 13;
  }
  if (before == best) {
    return 14;
  }
  if (!simdutf::validate_utf8("abc", 3)) {
    return 15;
  }
  const simdutf::implementation *after = simdutf::get_active_implementation();
  if (after != best) {
    return 16;
  }
  if (strcmp(after->name(), best->name()) != 0) {
    return 17;
  }
  return check_lookup_round_trip();
}

static int check_force() {
  const simdutf::implementation *forced =
      simdutf::get_available_implementations()["fallback"];
  if (forced == nullptr) {
    return 21;
  }
  if (setenv("SIMDUTF_FORCE_IMPLEMENTATION", "fallback", 1) != 0) {
    return 22;
  }
  if (!simdutf::validate_utf8("abc", 3)) {
    return 23;
  }
  const simdutf::implementation *active = simdutf::get_active_implementation();
  if (active != forced) {
    return 24;
  }
  if (strcmp(active->name(), "fallback") != 0) {
    return 25;
  }
  return check_lookup_round_trip();
}

int main(int argc, char **argv) {
  if (argc != 2) {
    return 2;
  }
  if (strcmp(argv[1], "default") == 0) {
    return check_default_detection();
  }
  if (strcmp(argv[1], "force") == 0) {
    return check_force();
  }
  return 3;
}
EOF

compile_multi_backend_no_libcxx -c "$rootdir/src/simdutf.cpp" \
  -o "$dispatch_simdutf_obj"
compile_multi_backend_no_libcxx -c "$dispatch_smoke_src" \
  -o "$dispatch_smoke_obj"
"$cc" "$dispatch_smoke_obj" "$dispatch_simdutf_obj" -o "$dispatch_smoke_bin"
record_link_dependencies "$dispatch_smoke_bin" "$dispatch_smoke_deps"
assert_no_cpp_runtime_dependencies "$dispatch_smoke_deps"

: > "$smoke_results"
"$dispatch_smoke_bin" default
printf '%s\n' "dispatch_smoke default: ok" >> "$smoke_results"
"$dispatch_smoke_bin" force
printf '%s\n' "dispatch_smoke force: ok" >> "$smoke_results"

cat > "$single_impl_smoke_src" <<'EOF'
#include "simdutf.h"
#include <string.h>

int main() {
  const simdutf::implementation *fallback =
      simdutf::get_available_implementations()["fallback"];
  if (fallback == nullptr) {
    return 41;
  }
  if (simdutf::get_available_implementations().size() != 1) {
    return 42;
  }
  if (simdutf::get_active_implementation() != fallback) {
    return 43;
  }
  if (!simdutf::validate_utf8("abc", 3)) {
    return 44;
  }
  if (simdutf::get_active_implementation() != fallback) {
    return 45;
  }
  if (strcmp(simdutf::get_active_implementation()->name(), "fallback") != 0) {
    return 46;
  }
  return 0;
}
EOF

compile_fallback_only_no_libcxx -c "$rootdir/src/simdutf.cpp" \
  -o "$single_impl_simdutf_obj"
compile_fallback_only_no_libcxx -c "$single_impl_smoke_src" \
  -o "$single_impl_smoke_obj"
"$cc" "$single_impl_smoke_obj" "$single_impl_simdutf_obj" \
  -o "$single_impl_smoke_bin"
record_link_dependencies "$single_impl_smoke_bin" "$single_impl_smoke_deps"
assert_no_cpp_runtime_dependencies "$single_impl_smoke_deps"
"$single_impl_smoke_bin"
printf '%s\n' "single_impl_smoke fallback-only: ok" >> "$smoke_results"

echo "compiled: $implementation_obj"
echo "undefined symbols: $implementation_undefined"
echo "abi/runtime subset: $implementation_abi_runtime"

echo "compiled: $simdutf_obj"
echo "undefined symbols: $simdutf_undefined"
echo "abi/runtime subset: $simdutf_abi_runtime"

echo "smoke results: $smoke_results"
echo "dispatch smoke dependencies: $dispatch_smoke_deps"
echo "single implementation smoke dependencies: $single_impl_smoke_deps"

if [ -s "$implementation_abi_runtime" ]; then
  cat "$implementation_abi_runtime"
else
  echo "(src/implementation.cpp ABI/runtime subset is empty)"
fi

if [ -s "$simdutf_abi_runtime" ]; then
  cat "$simdutf_abi_runtime"
else
  echo "(src/simdutf.cpp ABI/runtime subset is empty)"
fi
