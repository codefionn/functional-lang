cmake_minimum_required(VERSION 3.0)
project(func)

include_directories("${func_SOURCE_DIR}/include")

set(func_SOURCES "${func_SOURCE_DIR}/src/func.cpp"
                 "${func_SOURCE_DIR}/src/lexer.cpp"
                 "${func_SOURCE_DIR}/src/syntax.cpp"
                 "${func_SOURCE_DIR}/src/syntax_equals.cpp"
                 "${func_SOURCE_DIR}/src/syntax_eval.cpp"
                 "${func_SOURCE_DIR}/src/syntax_optimize.cpp"
                 "${func_SOURCE_DIR}/src/syntax_replace.cpp"
                 "${func_SOURCE_DIR}/src/gc.cpp"
                 "${func_SOURCE_DIR}/src/primary_parser.cpp"
                 "${func_SOURCE_DIR}/src/parser.cpp")

add_library(functional-langbase ${func_SOURCES})
add_executable(functional-lang ${func_SOURCES}
                               "${func_SOURCE_DIR}/src/main.cpp")
target_link_libraries(functional-lang functional-langbase)

add_subdirectory("${func_SOURCE_DIR}/test")
