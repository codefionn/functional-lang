# build test executables
macro(buildtest name)
  add_executable(${name} "${func_SOURCE_DIR}/test/${name}.cpp")
  target_link_libraries(${name} functional-langbase)
endmacro()

buildtest(lexer)
buildtest(parser)
buildtest(slexer)

# testing

enable_testing()

macro(matchtest name app in out)
  add_test(NAME ${name} COMMAND ${app} ${in})
  set_property(TEST ${name} PROPERTY PASS_REGULAR_EXPRESSION ${out})
endmacro()

# lexer
# id
matchtest(slexid0 slexer "hello" "^id")
matchtest(slexid1 slexer "World" "^id")
matchtest(slexid2 slexer "oNE" "^id")
matchtest(slexid3 slexer "one11" "^id")
matchtest(slexid4 slexer "num0123456789" "^id")
# num
matchtest(slexnum0 slexer "10.0" "^num")
matchtest(slexnum1 slexer "0.0" "^num")
matchtest(slexnum2 slexer "12325.123456789" "^num")
# int
matchtest(slexint0 slexer "0" "^int")
matchtest(slexint1 slexer "1234567890" "^int")
# op
matchtest(slexopeq slexer "==" "^op")
matchtest(slexopleq slexer "<=" "^op")
matchtest(slexopgeq slexer ">=" "^op")
matchtest(slexople slexer "<" "^op")
matchtest(slexopgt slexer ">" "^op")
matchtest(slexopland slexer "&&" "^op")
matchtest(slexoplor slexer "||" "^op")
matchtest(slexopadd slexer "+" "^op")
matchtest(slexopsub slexer "-" "^op")
matchtest(slexopmul slexer "*" "^op")
matchtest(slexopdiv slexer "/" "^op")
matchtest(slexoppow slexer "^" "^op")
matchtest(slexopasg slexer "=" "^op")
# brace
matchtest(slexobrace slexer "(" "^obrace")
matchtest(slexcbrace slexer ")" "^cbrace")
matchtest(slexlambda slexer "\\\\" "^lambda")
matchtest(slexatom slexer ".atom" "^atom")
matchtest(slexlit slexer "$" "^literal")
matchtest(slexif slexer "if" "^if")
matchtest(slexthen slexer "then" "^then")
matchtest(slexelse slexer "else" "^else")
matchtest(slexlet slexer "let" "^let")
matchtest(slexin slexer "in" "^in")
matchtest(slexdelim slexer "\\;" "^delim")
matchtest(slexany slexer "_" "^any")
