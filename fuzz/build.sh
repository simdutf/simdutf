#!/bin/bash
#
# this is the entry point for oss-fuzz.
#
# it can be invoked locally for development purposes as well
#

set -e

if [ -z $SRC ] ; then
    echo "development mode"
    set -ux

    SCRIPTDIR=$(dirname "$0")
    cd "$SCRIPTDIR/.."

    export CXX=/usr/lib/ccache/clang++-18
    export CXXFLAGS="-fsanitize=fuzzer-no-link,address,undefined -g -O1 -fsanitize-trap=undefined"
    export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
    export OUT=fuzz/out
    export WORK=fuzz/work
    mkdir -p $OUT $WORK
else
    # invoked from oss fuzz
    cd $SRC/simdutf
fi



cmake -B build -S . \
      -DSIMDUTF_TESTS=Off \
      -DSIMDUTF_TOOLS=Off \
      -DCMAKE_BUILD_TYPE=Debug \
      -DSIMDUTF_CXX_STANDARD=20 \
      -DSIMDUTF_ALWAYS_INCLUDE_FALLBACK=On

cmake --build build -j$(nproc)
cmake --install build --prefix $WORK

CXXFLAGSEXTRA=-std=c++20

for fuzzersrc in $(ls fuzz/*.cpp|grep -v fuzz/reproducer.) ; do
    fuzzer=$(basename $fuzzersrc .cpp)

    $CXX $CXXFLAGS $CXXFLAGSEXTRA\
         -I $WORK/include \
         -c $fuzzersrc -o $fuzzer.o

    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE \
         $fuzzer.o \
         $WORK/lib/libsimdutf.a \
         -o $OUT/$fuzzer
done
