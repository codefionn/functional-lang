#include "func/func.hpp"

int main() {
  std::cout << "> ";
  std::vector<std::string> lines;
  Lexer lexer(std::cin, lines);
  lexer.nextToken();
  lexer.skippedNewLinePrefix = "..";

  GCMain gc;
  Expr *expr = nullptr;
  Environment *env = new Environment(gc, &lexer);
  while (expr = parse(gc, lexer, *env)) {
    std::cout << expr->toString() << std::endl;
    env->mark(gc);
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
