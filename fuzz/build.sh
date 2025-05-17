#!/bin/sh
#
# this is the entry point for oss-fuzz.
#
# it can be invoked locally for development purposes as well
#

set -e

fuzzer_src_files=

if [ -z $SRC ] ; then
    echo "development mode"
    set -ux

    SCRIPTDIR=$(dirname "$0")
    cd "$SCRIPTDIR/.."

    # Check if /usr/lib/ccache/clang++-18 exists
    if [ -f /usr/lib/ccache/clang++-18 ]; then
        export CXX=/usr/lib/ccache/clang++-18
    else
        # Check if clang++ exists
        if command -v clang++ >/dev/null 2>&1; then
            # Get clang++ version
            CLANG_VERSION=$(clang++ --version | grep -oP 'version \K[0-9]+' | head -1)
            
            # Check if version is less than 18
            if [ "$CLANG_VERSION" -lt 18 ]; then
                echo "Warning: clang++ version $CLANG_VERSION is less than 18."
            fi
            
            export CXX=clang++
        else
            echo "Error: clang++ not found."
            exit 1
        fi
    fi
    export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined -g -O1 -fsanitize-trap=undefined -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1"
    export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
    export OUT=fuzz/out
    export WORK=fuzz/work
    BUILD=$WORK/build
    mkdir -p $OUT $WORK
    fuzzer_src_files=$(ls fuzz/*.cpp|grep -v -E "fuzz/(reproducer.|main)")
else
    # invoked from oss fuzz
    cd $SRC/simdutf
    # temporary: exclude atomic from oss-fuzz, libc++ 18 used there does not support atomic ref
    fuzzer_src_files=$(ls fuzz/*.cpp|grep -v -E "fuzz/(reproducer.|main|atomic.)")
    BUILD=build
fi



cmake -B $BUILD -S . \
      -DSIMDUTF_TESTS=Off \
      -DSIMDUTF_TOOLS=Off \
      -DSIMDUTF_FUZZERS=Off \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSIMDUTF_CXX_STANDARD=20 \
      -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=On

cmake --build $BUILD -j$(nproc)
cmake --install $BUILD --prefix $WORK

CXXFLAGSEXTRA=-std=c++20

for fuzzersrc in $fuzzer_src_files ; do
    fuzzer=$(basename $fuzzersrc .cpp)

    $CXX $CXXFLAGS $CXXFLAGSEXTRA\
         -I $WORK/include \
         -c $fuzzersrc -o $fuzzer.o

    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE \
         $fuzzer.o \
         $WORK/lib/libsimdutf.a \
         -o $OUT/$fuzzer
done
