# src/CMakelists.txt
set(FB_EXT ".cpp")
message(STATUS "Flex/Bison generated source file extension: ${FB_EXT}")

# options about libraries and includes
set(LIB_DIR "$ENV{CDE_LIBRARY_PATH}/native" CACHE STRING "directory of libraries")
set(INC_DIR "$ENV{CDE_INCLUDE_PATH}" CACHE STRING "directory of includes")
message(STATUS "Library directory: ${LIB_DIR}")
message(STATUS "Include directory: ${INC_DIR}")

# library fmt
include(FetchContent)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG         12.1.0
)
FetchContent_MakeAvailable(fmt)

# require flex/bison
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# generate lexer/parser
file(GLOB_RECURSE L_SOURCES "*.lx")
file(GLOB_RECURSE Y_SOURCES "*.y")
if(NOT (L_SOURCES STREQUAL "" AND Y_SOURCES STREQUAL ""))
    string(REGEX REPLACE ".*/(.*)\\.lx" "${CMAKE_CURRENT_BINARY_DIR}/\\1.lex${FB_EXT}" L_OUTPUTS "${L_SOURCES}")
    string(REGEX REPLACE ".*/(.*)\\.y" "${CMAKE_CURRENT_BINARY_DIR}/\\1.tab${FB_EXT}" Y_OUTPUTS "${Y_SOURCES}")
    flex_target(Lexer ${L_SOURCES} ${L_OUTPUTS})
    bison_target(Parser ${Y_SOURCES} ${Y_OUTPUTS})
    add_flex_bison_dependency(Lexer Parser)
endif()

# project link directories
link_directories(${LIB_DIR})

set(CXX_SOURCES main.cpp ast.cpp)
set(SOURCES ${CXX_SOURCES}
${FLEX_Lexer_OUTPUTS} ${BISON_Parser_OUTPUT_SOURCE})

add_executable(compiler ${SOURCES})

# project include directories
target_include_directories(compiler PRIVATE
    .                                       # current dir
    ${CMAKE_CURRENT_BINARY_DIR}             # generated header by flex/bison
    ${INC_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# link libraries
target_link_libraries(compiler PRIVATE
    koopa
    pthread
    dl
    fmt::fmt
)

# complie options
target_compile_options(compiler PRIVATE -O2 -Wall -Wno-register -Wextra)
