#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  Lexer lexer(std::cin);

  GCMain gc;
  std::map<std::string, Expr*> env; // environment
  Expr *expr = nullptr;
  while (true) {
    std::cout << "> ";
  	lexer.nextToken();
	expr = parse(gc, lexer);
	// Just jump if emtpy
   if (!expr) { // empty
	   if (lexer.currentToken() == tok_eof)
		   break;
	   if (lexer.currentToken() == tok_err)
		   std::cout << "Error." << std::endl;
		continue;
	}

    bool shouldPrint = expr->getExpressionType() != expr_biop
      || ((BiOpExpr*) expr)->getOperator() != '=';

    Expr *oldExpr = expr;
    while (expr && (expr = expr->eval(gc, env)) != oldExpr) {
      oldExpr = expr;
    }

    if (expr && shouldPrint) {
      std::cout << expr->toString() << std::endl;
    }

    switch(lexer.currentToken()) {
    case tok_eof:
	  break;
    }

    // mark some stuff
    for (std::pair<std::string, Expr*> _pair : env) {
      _pair.second->mark(gc);
    }

    gc.collect(); // We collect it all
  }

  return 0;
}
