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
  tok_num,      //!< Floating-point Number
  tok_int,      //!< Integer number
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

  op_land, //!< &&
  op_lor, //!< ||

  op_add, //!< +
  op_sub, //!< -
  op_mul, //!< *
  op_div, //!< /
  op_pow, //!< ^

  op_asg, //!< =

  op_fn, //!< Not a real operator (functional substition)
};

class TokenPos {
  size_t start, end, lineStart, lineEnd;
public:
  TokenPos(size_t start, size_t end, size_t lineStart, size_t lineEnd)
    : start{start}, end{end}, lineStart{lineStart}, lineEnd{lineEnd} {}
  TokenPos(const TokenPos &pos0, const TokenPos &pos1)
    : start{pos0.start < pos1.start ? pos0.start : pos1.start},
      end{pos0.end > pos1.end ? pos0.end : pos1.end},
      lineStart{pos0.lineStart < pos1.lineStart ? pos0.lineStart : pos1.lineStart},
      lineEnd{pos0.lineEnd > pos1.lineEnd ? pos0.lineEnd : pos1.lineEnd} {}
  virtual ~TokenPos() {}

  size_t getStart() const noexcept { return start; }
  size_t getEnd() const noexcept { return end; }
  size_t getLineStart() const noexcept { return lineStart; }
  size_t getLineEnd() const noexcept { return lineEnd; }

  TokenPos min() const noexcept {
    return TokenPos(start, end - 1, lineStart, lineEnd);
  }
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
  int64_t curint;
  std::string curid;

  int curchar;

  std::istream *input;

  size_t token_start, token_end;
  std::vector<std::string> lines;
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

  /*!\return Returns integer number, which was returned by the latest
   * nextToken() = tok_int call.
   */
  std::int64_t currentInteger() const noexcept;

  size_t currentLine() const noexcept
    { return curchar == -2 || curchar == '\n' || curchar == EOF || curtok == tok_eof ?
        (line == 0 ? 0 : line - 1) : line; }
  size_t getTokenStartPos() const noexcept
    { return curchar < 0 || curchar == '\n' ? token_start :
      (token_start == 0 ? 0 : token_start - 1); }
  size_t getTokenEndPos() const noexcept
    { return curchar < 0 || curchar == '\n' ? token_end :
      (token_end == 0 ? 0 : token_end - 1); }
  const std::vector<std::string> &getLines() { return lines; }

  TokenPos getTokenPos() const noexcept {
    return TokenPos(getTokenStartPos(), getTokenEndPos(),
        currentLine(), currentLine());
  }

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
  Token reportError(const std::string &msg) noexcept;

  Token reportError(const std::string &msg, const TokenPos &pos) noexcept;
  Token reportError(const std::string &msg, size_t start, size_t end) noexcept;

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
