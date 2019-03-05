#include "func/syntax.hpp"

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg) {
  lexer.reportError(msg);
  return nullptr;
}

Expr *parsePrimary(Lexer &lexer) {
  Expr* result = nullptr;

  while (lexer.currentToken() == tok_id
      || lexer.currentToken() == tok_num
      || lexer.currentToken() == tok_obrace
      || lexer.currentToken() == tok_lambda) {
    switch (lexer.currentToken()) {
      case tok_id: {
          if (!result)
            result = new IdExpr(lexer.currentIdentifier());
          else {
            result = new BiOpExpr(' ', std::shared_ptr<Expr>(result),
                                  std::shared_ptr<Expr>(new IdExpr(lexer.currentIdentifier())));
          }

          lexer.nextToken(); // eat id
          break;
        } // end case tok_id
      case tok_num: {
          if (!result)
            result = new NumExpr(lexer.currentNumber());
          else
            result = new BiOpExpr(' ', std::shared_ptr<Expr>(result),
                std::shared_ptr<Expr>(new NumExpr(lexer.currentNumber())));

          lexer.nextToken(); // eat num
          break;
        } // end case tok_num
      case tok_obrace: {
          lexer.nextToken(); // eat (
          Expr *oldResult = result;
          result = parse(lexer);
          if (lexer.currentToken() != tok_cbrace) {
            if (result) delete result; // delete result ?

            return reportSyntaxError(lexer, "Expected matching closing bracket )");
          }

          lexer.nextToken(); // eat )

          if (oldResult) {
            result = new BiOpExpr(' ', std::shared_ptr<Expr>(oldResult),
                                  std::shared_ptr<Expr>(result));
          }

          break;
        } // end case tok_obrace
      case tok_lambda: {
          lexer.nextToken(); /* eat \ */
          if (lexer.currentToken() != tok_id)
            return reportSyntaxError(lexer, "Expected identifier");

          std::string idname = lexer.currentIdentifier();
          lexer.nextToken(); // eat id

          if (lexer.currentToken() != tok_op
              && lexer.currentOperator() != op_asg)
            return reportSyntaxError(lexer, "Expected assign operator '='!");

          lexer.nextToken(); // eat =

          std::shared_ptr<Expr> expr(parse(lexer));
          if (!expr)
            return nullptr; // Error forwarding

          if (!result)
            result = new LambdaExpr(idname, expr);
          else
            result = new BiOpExpr(' ', std::shared_ptr<Expr>(result),
                                  std::shared_ptr<Expr>(new LambdaExpr(idname, expr)));

          break;
        } // end case tok_lambda
      default:
        return reportSyntaxError(lexer, "Not a primary expression token!");
    }
  }

  return result;
}

Expr *parse(Lexer &lexer) {
  Expr *primaryExpr = parsePrimary(lexer);
  if (!primaryExpr)
    return nullptr; // error forwarding

  switch(lexer.currentToken()) {
    case tok_err:
      delete primaryExpr;
      return nullptr; // Error forwarding
    case tok_eol:
    case tok_eof:
      return primaryExpr;
  }

  return parseRHS(lexer, primaryExpr, 0); // 0 = minimum precedence
  // (least binding)
}

Expr *parseRHS(Lexer &lexer, Expr *lhs, int prec) {
  while (lexer.currentToken() == tok_op && lexer.currentPrecedence() >= prec) {
    char op = lexer.currentOperator();
    int opprec = lexer.currentPrecedence();
    lexer.nextToken(); // eat op

    // exceptions
    if (op == '=' && lhs->getExpressionType() != expr_id)
      return reportSyntaxError(lexer, "<id> '=' <expr> expected!");

    Expr *rhs = parsePrimary(lexer);
    if (!rhs) {
      delete lhs; // cleanup
      return nullptr; // Error forwarding
    }

    while (lexer.currentToken() == tok_op
        && (lexer.currentPrecedence() > opprec
            || (lexer.currentOperator() == op_asg // left associative
                && lexer.currentPrecedence() == opprec))) {
      rhs = parseRHS(lexer, rhs, lexer.currentPrecedence());
    }

    lhs = new BiOpExpr(op, std::shared_ptr<Expr>(lhs),
                       std::shared_ptr<Expr>(rhs));
  }

  return lhs;
}
