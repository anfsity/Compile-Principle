# root CMakelists.txt
cmake_minimum_required(VERSION 3.20)

project(
    compiler
    LANGUAGES CXX
    DESCRIPTION "PKU Compile Principle LABs."
    VERSION 0.1.0
)

# c++ settings
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# binary_dir : the output dir like build/cmake-build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# library fmt
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG         12.1.0
)
FetchContent_MakeAvailable(fmt)

# Flex & Bsion
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

add_subdirectory(include)
add_subdirectory(src)
