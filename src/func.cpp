#include "func/func.hpp"

bool interpret(std::istream &input, GCMain &gc,
    std::vector<std::string> &lines,
    Environment *env,
    bool interpret_mode) noexcept {
  bool error = false;

  Lexer lexer(input, lines);
  if (interpret_mode)
    lexer.skippedNewLinePrefix = "..";

  if (!env)
    env = new Environment(gc, &lexer); // environment
  else
    env->lexer = &lexer;

  Expr *expr = nullptr;
  while (true) {
    if (interpret_mode)
      std::cout << "> "; // print prefix
  	lexer.nextToken(); // aquire next token (if first loop, first token)
	  expr = parse(gc, lexer, *env);

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
      || ((BiOpExpr*) expr)->getOperator() != op_asg;

    // Evaluate as long as expression is different from the evaluated one
    expr = eval(gc, *env, expr);

    // Print the expression
    if (expr && shouldPrint) {
      std::cout << "=> " << expr->toString() << std::endl;
    }

    // Terminate interpreter if eof occured
    if (lexer.currentToken() == tok_eof)
      break;

    env->mark(gc); // mark main scope/environemnt
  }

  return !error;
}
