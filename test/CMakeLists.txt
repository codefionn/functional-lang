add_executable(lex ${func_SOURCES}
                   "${func_SOURCE_DIR}/test/lexer.cpp")
add_executable(parse ${func_SOURCES}
                   "${func_SOURCE_DIR}/test/parser.cpp")
