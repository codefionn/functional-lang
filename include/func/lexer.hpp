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

  tok_if,
  tok_then,
  tok_else,

  tok_err, //!< Error
};

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
