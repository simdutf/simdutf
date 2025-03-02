set(CMAKE_SYSTEM_NAME PowerPC)

set(target       powerpc64-linux-gnu)
set(version      14)
set(c_compiler   gcc)
set(cxx_compiler g++)

set(CMAKE_C_COMPILER   "${target}-${c_compiler}-${version}")
set(CMAKE_CXX_COMPILER "${target}-${cxx_compiler}-${version}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-ppc64-static")
