#include "func/global.hpp"
#include "func/lexer.hpp"

int main() {

  Lexer lexer(std::cin);

  std::cout << "> ";
  while (lexer.nextToken() != tok_eof
      && lexer.currentToken() != tok_err) {

    switch (lexer.currentToken()) {
      case tok_id:
        std::cout << "id" << std::endl; break;
      case tok_num:
        std::cout << "num" << std::endl; break;
      case tok_op:
        std::cout << "op" << std::endl; break;
      case tok_eol:
        std::cout << "eol" << std::endl;
        std::cout << "> ";
        break;
      case tok_obrace:
        std::cout <<  "(" << std::endl; break;
      case tok_cbrace:
        std::cout << ")" << std::endl; break;
      default:
        std::cout << "Unknown=" << lexer.currentToken() << std::endl; break;
    }
  }

  if (lexer.currentToken() == tok_err)
    return 1;

  return 0;
}
