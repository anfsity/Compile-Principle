# include/CMakeLists.txt
add_library(headers INTERFACE)

target_include_directories(headers INTERFACE
    # include/
    ${CMAKE_CURRENT_SOURCE_DIR} 
)

message(STATUS "[INFO]  Compiler Headers Target created: headers")