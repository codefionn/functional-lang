#include "func/func.hpp"

int main() {
  std::cout << "> ";
  Lexer lexer(std::cin);
  lexer.nextToken();

  GCMain gc;
  Expr *expr = nullptr;
  Environment env;
  while (expr = parse(gc, lexer, env)) {
    std::cout << expr->toString() << std::endl;
    gc.collect(); // We collect it all

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
