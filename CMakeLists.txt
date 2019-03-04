cmake_minimum_required(VERSION 3.0)
project(func)

include_directories("${func_SOURCE_DIR}/include")

set(func_SOURCES "${func_SOURCE_DIR}/src/func.cpp"
                 "${func_SOURCE_DIR}/src/lexer.cpp"
                 "${func_SOURCE_DIR}/src/parser.cpp")

add_executable(functional-lang ${func_SOURCES}
                               "${func_SOURCE_DIR}/src/main.cpp")

add_subdirectory("${func_SOURCE_DIR}/test")
