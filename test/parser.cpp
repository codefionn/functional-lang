#include "func/func.hpp"

int main() {
  std::cout << "> ";
  Lexer lexer(std::cin);
  lexer.nextToken();

  GCMain gc;
  Expr *expr = nullptr;
  while (expr = parse(gc, lexer)) {
    std::cout << expr->toString() << std::endl;
    gc.collect();

    std::cout << "> ";

    switch(lexer.currentToken()) {
    case tok_eol:
    case tok_eof:
    case tok_err:
      lexer.nextToken(); // advance to next token
    }
  }

  return 0;
}
