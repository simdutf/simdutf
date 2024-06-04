#!/bin/bash

cd $SRC/simdutf

cmake -B build
cmake --build build --target simdutf-singleheader-files -j$(nproc)

$CXX $CFLAGS $CXXFLAGS \
     -I build/singleheader \
     -c fuzz/roundtrip.cc -o roundtrip.o

$CXX $CFLAGS $CXXFLAGS $LIB_FUZZING_ENGINE roundtrip.o \
     -o $OUT/roundtrip
