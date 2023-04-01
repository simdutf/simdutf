#!/bin/bash

cd $SRC/simdutf

cmake -B build
cmake --build build -j$(nproc)

for f in fuzz/roundtrip_*.cc
do
  $CXX $CFLAGS $CXXFLAGS $LIB_FUZZING_ENGINE \
     -I build/singleheader \
     $f \
     -o $OUT/$(basename ${f%.cc})  
done

