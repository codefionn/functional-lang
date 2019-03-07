#ifndef FUNC_LEXER_HPP
#define FUNC_LEXER_HPP

#include "func/global.hpp"

enum Token : int {
  tok_id = 256,
  tok_num,
  tok_op,

  tok_eol, //!< End of line (new-line character)
  tok_eof, //!< End of file

  tok_obrace, //!< (
  tok_cbrace, //!< )
  tok_lambda, /*!< \\ */
  tok_atom, //!< .

  tok_err, //!< Error
};

enum Operator : char {
  op_add = '+', //!< +
  op_sub = '-', //!< -
  op_mul = '*', //!< *
  op_div = '/', //!< /
  op_pow = '^', //!< ^
  op_asg = '=', //!< =
};

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

  int nextChar();
  int currentChar() const noexcept ;

  Token nextToken();
  Token currentToken() const noexcept;

  Operator currentOperator() const noexcept;
  double currentNumber() const noexcept;
  const std::string &currentIdentifier() const noexcept;

  int currentPrecedence();

  Token reportError(const std::string &msg);
};

#endif /* FUNC_LEXER_HPP */
