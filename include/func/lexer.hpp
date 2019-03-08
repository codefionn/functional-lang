#ifndef FUNC_LEXER_HPP
#define FUNC_LEXER_HPP

/*!\file func/lexer.hpp
 * \brief Lexical Analysis/Tokenizer.
 */

#include "func/global.hpp"

/*!\brief Tokens for Tokenizer/Lexical Analysis.
 * \see Lexer::nextToken, Lexer::currentToken
 */
enum Token : int {
  tok_id = 256, //!< Identifier
  tok_num,      //!< Number
  tok_op,       //!< Binary-Operator

  tok_eol, //!< End of line (new-line character)
  tok_eof, //!< End of file

  tok_obrace, //!< (
  tok_cbrace, //!< )
  tok_lambda, /*!< \\ */
  tok_atom, //!< .
  tok_literal, //!< $

  tok_if,   //!< 'if'
  tok_then, //!< 'then'
  tok_else, //!< 'else'

  tok_let, //!< 'let'
  tok_in, //!< 'in'

  tok_delim, //!< ';'

  tok_any,  //!< '_'

  tok_err, //!< Error
};

/*!\brief Operator type.
 * \see Lexer::currentOperator()
 */
enum Operator {
  op_eq, //!< ==
  op_leq, //!< \<=
  op_geq, //!< \>=
  op_le, //!< \<
  op_gt, //!< \>

  op_add, //!< +
  op_sub, //!< -
  op_mul, //!< *
  op_div, //!< /
  op_pow, //!< ^

  op_asg, //!< =

  op_fn, //!< Not a real operator (functional substition)
};

namespace std {
  std::string to_string(Operator op) noexcept;
};

/*!\return Returns operator precedence of binary operator op
 */
int getOperatorPrecedence(Operator op);

class Lexer {
  std::size_t line;
  std::size_t column;

  std::string lineStr;

  Token curtok;
  Operator curop;
  double curnum;
  std::string curid;

  int curchar;

  std::istream *input;
public:
  Lexer(std::istream &input);
  virtual ~Lexer();

  /*!\brief Aquire next char.
   * \return Returns next char.
   *
   * Gets next character in file-stream. Also generates a string for the
   * current line, count current column and line.
   */
  int nextChar();

  /*!\return Returns char returned by the latest nextChar() call.
   * \see nextChar
   */
  int currentChar() const noexcept ;

  /*!\return Returns next token.
   * \see Token, nextToken
   */
  Token nextToken();
  
  /*!\return Returns token, which was returned by the latest nextToken() call.
   * \see nextToken
   */
  Token currentToken() const noexcept;

  /*!\return Returns operator, which was returned by the lastest
   * nextToken() == tok_op call.
   */
  Operator currentOperator() const noexcept;

  /*!\return Returns floating-point number, which was returned by the latest
   * nextToken() == tok_num call.
   */
  double currentNumber() const noexcept;

  /*!\return Returns identifier, which was returned by the latest
   * nextToken() == tok_id call.
   */
  const std::string &currentIdentifier() const noexcept;

  /*!\brief Returns precedence of current token.
   */
  int currentPrecedence();

  /*!\brief Prints error to console (std::cerr).
   * \return Returns tok_err.
   */
  Token reportError(const std::string &msg);

  /*!\brief True if lines should be ignored/skipped by nextToken.
   * \see nextToken
   */
  bool skipNewLine = false;

  /*!\brief Prefix to print if line was ignored/skipped by nextToken.
   * \see nextToken, skipNewLine
   */
  std::string skippedNewLinePrefix = "";
};

#endif /* FUNC_LEXER_HPP */
