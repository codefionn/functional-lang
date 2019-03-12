#include "func/parser.hpp"

Expr *parse(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel) {
  if (topLevel && lexer.currentToken() == tok_eol)
    return nullptr;

  switch(lexer.currentToken()) {
    case tok_err:
    case tok_eof:
      return nullptr;
  }

  Expr *primaryExpr = parsePrimary(gc, lexer, env);
  if (!primaryExpr)
    return nullptr; // error forwarding

  switch(lexer.currentToken()) {
    case tok_err:
      return nullptr; // Error forwarding
    case tok_eol:
    case tok_eof:
      return primaryExpr;
  }

  // 0 is least binding precedence
  Expr *expr = parseRHS(gc, lexer, env, primaryExpr, 0);
  if (!expr) return nullptr;

  return expr->optimize(gc);
}

Expr *parseRHS(GCMain &gc, Lexer &lexer, Environment &env, Expr *plhs, int prec) {
  StackFrameObj<Expr> lhs(env, plhs);

  while (lexer.currentToken() == tok_op && lexer.currentPrecedence() >= prec) {
    Operator op = lexer.currentOperator();
    int opprec = lexer.currentPrecedence();

    // exceptions
    if (op == op_asg
        && lhs->getExpressionType() != expr_id
        && (lhs->getExpressionType() == expr_biop
          && !dynamic_cast<BiOpExpr*>(*lhs)->isAtomConstructor()
            && !dynamic_cast<BiOpExpr*>(*lhs)->isFunctionConstructor()))
      return reportSyntaxError(lexer, "Expected identifier, atom constructor!", lhs->getTokenPos());

    lexer.nextToken(); // eat op

    StackFrameObj<Expr> rhs(env, parsePrimary(gc, lexer, env));
    if (!rhs) return nullptr; // Error forwarding

    if (rhs->equals(*lhs, true))
      lhs = rhs;

    while (lexer.currentToken() == tok_op
        && (lexer.currentPrecedence() > opprec
            || (lexer.currentOperator() == op_asg // left associative
                && lexer.currentPrecedence() == opprec))) {
      rhs = parseRHS(gc, lexer, env, *rhs, lexer.currentPrecedence());
      if (!rhs) return nullptr; // Errorforwarding
    }

    lhs = new BiOpExpr(gc, op, *lhs, *rhs);
  }

  return *lhs;
}
