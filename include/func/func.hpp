#ifndef FUNC_FUNC_HPP
#define FUNC_FUNC_HPP

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/syntax.hpp"

/*!\file func/func.hpp
 * \brief Main file of the project. You want to include this, nothing else.
 */

/*!\mainpage Functional-Language
 *
 * Defining a primitive functional language and implementing an interpreter.
 *
 *     <program> := e | <expr> | <expr> <newline> <program>
 *     <expr> := <id>
 *             | <num>
 *             | '(' <expr> ')'
 *             | <asg-expr> '=' <expr>
 *             | <expr> '==' <expr>
 *             | <expr> '<=' <expr>
 *             | <expr> '>=' <expr>
 *             | <expr> '<' <expr>
 *             | <expr> '>' <expr>
 *             | <expr> '+' <expr>
 *             | <expr> '-' <expr>
 *             | <expr> '*' <expr>
 *             | <expr> '/' <expr>
 *             | <expr> '^' <expr>
 *             | <expr> <expr>
 *             | '\' <id> '=' <expr>
 *             | '.' <id>
 *             | '$' <expr>
 *             | '_'
 *             | 'let' <let-expr> 'in' <expr>
 *             | 'if' <expr> 'then' <expr> 'else' <expr>
 *     <let-expr> := <asg-expr> '=' <expr>
 *                 | <asg-expr> '=' <expr> ';' <let-expr>
 *                 | <asg-expr> '=' <expr> <newline> <let-expr>
 *     <asg-expr> := <id> | <atom-asg-expr> | <id> <asg-expr>
 *     <atom-asg-expr> := '.'<id> <asg-expr>
 *
 * Precedence:
 *
 * - '=': 1
 * - '==', '<=', '>=', '<', '>': 2
 * - '+', '-': 3
 * - '*', '/': 4
 * - '^': 5
 * - \<expr\> \<expr\>: 6
 *
 * ## Semantics
 *
 *     '(' <expr> ')' evaluates to <expr>.
 *
 *     <id> '=' <expr> adds <id> to environment (current scope), which points
 *     to <expr> . Danger: Evaluates to itself.
 *
 *     <expr_0> <expr_1> evaluates <expr_0>. If <expr_0> is a lambda function
 *     then to substitution (variable of lambda function substituted with
 *     <expr_1>). If <expr_0> isn't a lambda function, <expr_1> is evaluated
 *     and <expr_0> (evaluated <expr_1>) is returned.
 *
 *     '==' evalutes both expression and then checks if their structures are
 *     equal (except when '_' is used, where '==' always evaluates to .true).
 *     If both evaluated expression structures are equal, then .true is
 *     returned otherwise .false.
 *
 *     '$' evaluates <expr> while parsing syntax ("immediatly").
 *
 *     '\' <id> '=' <expr> is an lambda function. When substituting every
 *     identifier in <expr> which is equal to <id> will be substituted, except
 *     if the <id> is in another lambda function, which has the same <id> as
 *     the "current" one.
 *
 *     '.' <id> is an atom. And I'm already sorry to say this, but an atom is
 *     an atom. It evaluates to itself and doesn't do anything else.
 *
 * ## License
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

/*!\brief Interpret characters streamed from input.
 * \param input
 * \param interpret_mode Prints some pretty helpers (line prefixes) if true.
 * \return Returns true on success, false if error occured.
 */
bool interpret(std::istream &input, bool interpret_mode = false) noexcept;

#endif /* FUNC_FUNC_HPP */
