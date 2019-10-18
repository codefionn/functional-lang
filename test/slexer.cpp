/**
 * test/slexer.cpp
 * -----------------------------------------------------------------------------
 * Writes type of token of the program first argument
 */

#include "func/global.hpp"
#include "func/lexer.hpp"
#include <sstream>

int main(int vargsc, char * vargs[]) {
  if (vargsc != 2)
    return 1;

  std::istringstream istrstream(vargs[1]);
  std::vector<std::string> lines;
  Lexer lexer(istrstream, lines);
  switch (lexer.nextToken()) {
  case tok_id:
    std::cout << "id"; break;
  case tok_num:
    std::cout << "num"; break;
  case tok_op:
    std::cout << "op"; break;
  case tok_eol:
    std::cout << "eol"; break;
  case tok_eof:
    std::cout << "eof"; break;
  case tok_int:
    std::cout << "int"; break;
  case tok_obrace:
    std::cout << "obrace"; break;
  case tok_cbrace:
    std::cout << "cbrace"; break;
  case tok_lambda:
    std::cout << "lambda"; break;
  case tok_atom:
    std::cout << "atom"; break;
  case tok_literal:
    std::cout << "literal"; break;
  case tok_if:
    std::cout << "if"; break;
  case tok_then:
    std::cout << "then"; break;
  case tok_else:
    std::cout << "else"; break;
  case tok_let:
    std::cout << "let"; break;
  case tok_in:
    std::cout << "in"; break;
  case tok_delim:
    std::cout << "delim"; break;
  case tok_any:
    std::cout << "any"; break;
  case tok_err:
    std::cout << "err"; break;
  default:
    std::cout << "Unknown=" << lexer.currentToken(); break;
  }

  return 0;
}
