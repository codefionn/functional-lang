#ifndef FUNC_PARSER_HPP
#define FUNC_PARSER_HPP

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/syntax.hpp"

/*!\brief Parses primary expression(s). Also parses lambda function
 * substitutions (so also expressions, not only one primary one).
 * \param gc
 * \param lexer
 * \param env Environment to access variables.
 * \param topLevel Is the primary a top level primary expression (typically
 * true).
 * \return Returns nullptr on error, otherwise primary expression(s).
 */
Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel = true);

/*!\return Returns nullptr on error, otherwise parsed tokens from
 * lexer.nextToken().
 *
 * \param gc
 * \param lexer
 * \param env Environment to access variables.
 * \param topLevel If top level, nullptr is returned if eol occured.
 */
Expr *parse(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel = true);

/*!\brief Parse right-hand-side
 * \param gc
 * \param lexer
 * \param lhs Already parsed Left-hand-side
 * \param prec current minimum precedence
 * \return Returns nullptr on error, otherwise parsed RHS.
 */
Expr *parseRHS(GCMain &gc, Lexer &lexer, Environment &env, Expr *lhs, int prec);

#endif /* FUNC_PARSER_HPP */
