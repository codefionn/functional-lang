add_executable(lex "${func_SOURCE_DIR}/test/lexer.cpp")
target_link_libraries(lex functional-langbase)
add_executable(parse "${func_SOURCE_DIR}/test/parser.cpp")
target_link_libraries(parse functional-langbase)
