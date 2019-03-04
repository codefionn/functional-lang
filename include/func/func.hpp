#ifndef FUNC_FUNC_HPP
#define FUNC_FUNC_HPP

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/syntax.hpp"
#include "func/parser.hpp"

/*!\file func/func.hpp
 * \brief Main file of the project
 */

/*!\mainpage Functional-Language
 *
 * Defining a primitive functional language and implementing an interpreter.
 *
 *     <program> := e | <expr> | <expr> <program>
 *     <expr> := <id>
 *             | <num>
 *             | '(' <expr> ')'
 *             | <id> '=' <expr>
 *             | <expr> '+' <expr>
 *             | <expr> '-' <expr>
 *             | <expr> '*' <expr>
 *             | <expr> '/' <expr>
 *             | <expr> <expr>
 *
 *  Precedence:
 *
 *  - '=': 1
 *  - '+', '-': 2
 *  - '*', '/': 3
 *  - \<expr\> \<expr\>: 4
 */

#endif /* FUNC_FUNC_HPP */
