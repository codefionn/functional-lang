#include "func/func.hpp"

bool interpret(std::istream &input, const std::string &prefix) noexcept {
  bool error = false;

  Lexer lexer(input);

  GCMain gc;
  std::map<std::string, Expr*> env; // environment
  Expr *expr = nullptr;
  while (true) {
    std::cout << prefix; // print prefix
  	lexer.nextToken(); // aquire next token (if first loop, first token)
	  expr = parse(gc, lexer);

    // Just jump if emtpy (error recovery and new-line support)
    if (!expr) { // empty
      if (lexer.currentToken() == tok_eof)
        break;
      if (lexer.currentToken() == tok_err) {
        std::cout << "Error." << std::endl;
        error = true;
      }

      continue;
   	}

    // Should not print if top assignment
    bool shouldPrint = expr->getExpressionType() != expr_biop
      || ((BiOpExpr*) expr)->getOperator() != '=';

    // Evaluate as long as expression is different from the evaluated one
    Expr *oldExpr = expr;
    while (expr && (expr = expr->eval(gc, env)) != oldExpr) {
      oldExpr = expr;
    }

    // Print the expression
    if (expr && shouldPrint) {
      std::cout << expr->toString() << std::endl;
    }

    // Terminate interpreter if eof occured
    if (lexer.currentToken() == tok_eof)
      break;

    // mark some stuff
    for (std::pair<std::string, Expr*> _pair : env) {
      _pair.second->mark(gc);
    }

    gc.collect(); // We collect it all (garbage)
  }

  return !error;
}
