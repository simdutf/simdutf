# Usage:
# $ cmake -DCMAKE_TOOLCHAIN_FILE=toolchain-loongarch64.cmake
set(CMAKE_SYSTEM_NAME Generic)

set(target       loongarch64-unknown-linux-gnu)
set(c_compiler   gcc)
set(cxx_compiler g++)

set(CMAKE_C_COMPILER   "${target}-${c_compiler}")
set(CMAKE_CXX_COMPILER "${target}-${cxx_compiler}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(LOONGARCH64_ISA "loongarch64")
set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-${LOONGARCH64_ISA}")

# Reason: pk expects static linkage
set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_CXX_FLAGS "-mlsx -mlasx")
