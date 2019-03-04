#ifndef FUNC_LEXER_HPP
#define FUNC_LEXER_HPP

#include "func/global.hpp"

enum Token {
  tok_id,
  tok_num,
  tok_op,

  tok_eol, //!< End of line (new-line character)
  tok_eof, //!< End of file

  tok_obrace, //!< (
  tok_cbrace, //!< )

  tok_err, //!< Error
};

enum Operator {
  op_add, //!< +
  op_sub, //!< -
  op_mul, //!< *
  op_div, //!< /
  op_asg, //!< =

  op_lambda, /*!< \\ */
};

std::size_t getOperatorPrecedence(Operator op);

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

  int nextChar();
  int currentChar() const noexcept ;

  Token nextToken();
  Token currentToken() const noexcept;

  Operator currentOperator() const noexcept;
  double currentNumber() const noexcept;
  const std::string &currentIdentifier() const noexcept;

  Token reportError(const std::string &msg);
};

#endif /* FUNC_LEXER_HPP */
