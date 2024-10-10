# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(datalab C)

# Set common C flags
set(CMAKE_C_FLAGS "-O -Wall -m32")

# Find math library
find_library(M_LIB m)
if(NOT M_LIB)
    message(FATAL_ERROR "Math library not found!")
endif()

# Add executable for btest
add_executable(btest bits.c btest.c decl.c tests.c)
target_link_libraries(btest ${M_LIB})

# Add executable for fshow
add_executable(fshow fshow.c)

# Add executable for ishow
add_executable(ishow ishow.c)

# Clean target: equivalent to 'make clean'
add_custom_target(clean
        COMMAND ${CMAKE_COMMAND} -E rm -f btest fshow ishow
        COMMAND ${CMAKE_COMMAND} -E rm -f CMakeCache.txt
)
