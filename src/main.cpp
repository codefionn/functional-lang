#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  std::cout << "> ";
  Lexer lexer(std::cin);
  lexer.nextToken();

  GCMain gc;
  std::map<std::string, Expr*> env; // environment
  Expr *expr = nullptr;
  while (expr = parse(gc, lexer)) {
    bool shouldPrint = expr->getExpressionType() != expr_biop
      || ((BiOpExpr*) expr)->getOperator() != '=';

    Expr *oldExpr = expr;
    while (expr && (expr = expr->eval(gc, env)) != oldExpr) {
      oldExpr = expr;
    }

    if (expr && shouldPrint) {
      std::cout << expr->toString() << std::endl;
    }

    std::cout << "> ";

    switch(lexer.currentToken()) {
    case tok_eol:
    case tok_eof:
    case tok_err:
      lexer.nextToken(); // advance to next token
    }

    // mark some stuff
    for (std::pair<std::string, Expr*> _pair : env) {
      _pair.second->mark(gc);
    }

    gc.collect(); // We collect it all
  }

  return 0;
}
