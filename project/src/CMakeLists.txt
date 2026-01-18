# src/CMakelists.txt
# generate lexer/parser
set(LEXER_SRC frontend/sysy.lx)
set(YACC_SRC frontend/sysy.y)

# generate the lexer and parser files
flex_target(Lexer ${LEXER_SRC} ${CMAKE_CURRENT_BINARY_DIR}/sysy.lex.cpp)
bison_target(Parser ${YACC_SRC} ${CMAKE_CURRENT_BINARY_DIR}/sysy.tab.cpp COMPILE_FLAGS "-Wcounterexamples")
add_flex_bison_dependency(Lexer Parser)
message(STATUS "[INFO]  Generated lexer: ${CMAKE_CURRENT_BINARY_DIR}/sysy.lex.cpp")
message(STATUS "[INFO]  Generated parser: ${CMAKE_CURRENT_BINARY_DIR}/sysy.tab.cpp")
message(STATUS "[INFO]  Generated lexer outpus ${FLEX_Lexer_OUTPUTS}")
message(STATUS "[INFO]  Generated parser outpus ${BISON_Parser_OUTPUT_SOURCE}")

set(CORE_SOURCES
    ir/ast.cpp
    ir/codegen.cpp
    backend/backend.cpp
    ${FLEX_Lexer_OUTPUTS}
    ${BISON_Parser_OUTPUT_SOURCE}
)

add_library(compiler_core STATIC ${CORE_SOURCES})

target_include_directories(compiler_core PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}  # cmake-build/src/* for generated lexer/parser
)

target_sources(compiler_core PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES
    ${PROJECT_SOURCE_DIR}/include/backend/koopawrapper.cppm
    ${PROJECT_SOURCE_DIR}/include/backend/backend.cppm
    ${PROJECT_SOURCE_DIR}/include/ir/ast.cppm
    ${PROJECT_SOURCE_DIR}/include/ir/type.cppm
    ${PROJECT_SOURCE_DIR}/include/ir/symbol_table.cppm
    ${PROJECT_SOURCE_DIR}/include/ir/ir_builder.cppm
    ${PROJECT_SOURCE_DIR}/include/Log/log.cppm
)

# compiler core link libraries
target_link_libraries(compiler_core PUBLIC
    koopa
    pthread
    dl
    fmt::fmt
    headers
)

# complie options
target_compile_options(compiler_core PRIVATE -O2 -Wall -Wno-register -Wextra)

# executable
add_executable(compiler main.cpp)
target_compile_options(compiler PRIVATE -O2 -Wall -Wno-register -Wextra)
target_include_directories(compiler PRIVATE $ENV{CDE_INCLUDE_PATH})

# compiler link libraries
target_link_libraries(compiler PRIVATE compiler_core)
target_link_directories(compiler PRIVATE $ENV{CDE_LIBRARY_PATH}/native)