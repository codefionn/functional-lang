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
 *
 *  ## License
 *
 * Copyright (c) 2019 Fionn Langhans
 * 
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *        claim that you wrote the original software. If you use this software in
 *        a product, an acknowledgment in the product documentation would be
 *        appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not
 *        be misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *        distribution.
 */

#endif /* FUNC_FUNC_HPP */
