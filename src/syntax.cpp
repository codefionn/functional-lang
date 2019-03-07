#include "func/syntax.hpp"

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg) {
  lexer.reportError(msg);
  return nullptr;
}

Expr *parsePrimary(GCMain &gc, Lexer &lexer) {
  Expr* result = nullptr;

  while (lexer.currentToken() == tok_id
      || lexer.currentToken() == tok_num
      || lexer.currentToken() == tok_obrace
      || lexer.currentToken() == tok_lambda) {
    switch (lexer.currentToken()) {
      case tok_id: {
         IdExpr *idexpr = new IdExpr(gc, lexer.currentIdentifier());
          if (!result)
            result = idexpr;
          else {
            result = new BiOpExpr(gc, ' ', result, idexpr);
          }

          lexer.nextToken(); // eat id
          break;
        } // end case tok_id
      case tok_num: {
          NumExpr *numexpr = new NumExpr(gc, lexer.currentNumber());
          if (!result)
            result = numexpr;
          else
            result = new BiOpExpr(gc, ' ', result, numexpr);

          lexer.nextToken(); // eat num
          break;
        } // end case tok_num
      case tok_obrace: {
          lexer.nextToken(); // eat (
          Expr *oldResult = result;
          result = parse(gc, lexer);
          if (!result || lexer.currentToken() != tok_cbrace) {
            if (result) delete result; // delete result ?

            return reportSyntaxError(lexer, "Expected matching closing bracket )");
          }

          lexer.nextToken(); // eat )

          if (oldResult) {
            result = new BiOpExpr(gc, ' ', oldResult, result);
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

          Expr *expr = parse(gc, lexer);
          if (!expr)
            return nullptr; // Error forwarding

          Expr *lambdaExpr = new LambdaExpr(gc, idname, expr);

          if (!result)
            result = lambdaExpr;
          else
            result = new BiOpExpr(gc, ' ', result, lambdaExpr);

          break;
        } // end case tok_lambda
    }
  }

  if (lexer.currentToken() == tok_err)
    return nullptr;

  if (!result)
    return reportSyntaxError(lexer, "Not a primary expression token!");

  return result;
}

Expr *parse(GCMain &gc, Lexer &lexer) {
  switch(lexer.currentToken()) {
    case tok_err:
    case tok_eof:
    case tok_eol:
      return nullptr;
  }

  Expr *primaryExpr = parsePrimary(gc, lexer);
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

  return parseRHS(gc, lexer, primaryExpr, 0); // 0 = minimum precedence
  // (least binding)
}

Expr *parseRHS(GCMain &gc, Lexer &lexer, Expr *lhs, int prec) {
  while (lexer.currentToken() == tok_op && lexer.currentPrecedence() >= prec) {
    char op = lexer.currentOperator();
    int opprec = lexer.currentPrecedence();
    lexer.nextToken(); // eat op

    // exceptions
    if (op == '=' && lhs->getExpressionType() != expr_id)
      return reportSyntaxError(lexer, "<id> '=' <expr> expected!");

    Expr *rhs = parsePrimary(gc, lexer);
    if (!rhs) {
      delete lhs; // cleanup
      return nullptr; // Error forwarding
    }

    while (lexer.currentToken() == tok_op
        && (lexer.currentPrecedence() > opprec
            || (lexer.currentOperator() == op_asg // left associative
                && lexer.currentPrecedence() == opprec))) {
      rhs = parseRHS(gc, lexer, rhs, lexer.currentPrecedence());
    }

    lhs = new BiOpExpr(gc, op, lhs, rhs);
  }

  return lhs;
}

// interpreter stuff

Expr *BiOpExpr::eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept {
  switch (op) {
  case '=': {
              std::string id = ((IdExpr*) lhs)->getName();
              if (env.find(id) == env.end()) {
                env.insert(std::pair<std::string, Expr*>(id, rhs));
                return this;
              }

              std::cerr << "Variable " << id << " already exists." << std::endl;
              return nullptr;
            }
  case '+':
  case '-':
  case '*':
  case '/':
  case '^': {
              Expr *newlhs = ::eval(gc, env, lhs);
              if (!newlhs) return nullptr; // error forwarding
              Expr *newrhs = ::eval(gc, env, rhs);
              if (!newrhs) return nullptr; // error forwarding

              if (newlhs->getExpressionType() == expr_num
                  && newrhs-> getExpressionType() == expr_num) {
                double num0 = ((NumExpr*) newlhs)->getNumber();
                double num1 = ((NumExpr*) newrhs)->getNumber();
                switch (op) {
                case '+': num0 += num1; break;
                case '-': num0 -= num1; break;
                case '*': num0 *= num1; break;
                case '/': num0 /= num1; break;
                case '^': num0 = pow(num0, num1); break;
                }
                return new NumExpr(gc, num0);
              }

              if (newlhs == lhs && newrhs == rhs)
                return this;

              return new BiOpExpr(gc, op, newlhs, newrhs);
            }
  case ' ': {
              lhs = ::eval(gc, env, lhs);
              if (!lhs)
                return nullptr; // Error forwarding

              if (lhs->getExpressionType() != expr_lambda) {
                std::cerr << "Invalid function call. Callee not a lambda function." << std::endl;
                return nullptr;
              }

              return ((LambdaExpr*)lhs)->replace(gc, rhs);
            }
  }

  std::cerr << "Interpreter error." << std::endl;
  return nullptr;
}

Expr *IdExpr::eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept {
  if (env.find(getName()) == env.end()) {
    std::cerr << "Invalid identifier " << getName() << "." << std::endl;
    return nullptr;
  }

  return env[getName()];
}

Expr *LambdaExpr::replace(GCMain &gc, Expr *newexpr) const noexcept {
  return expr->replace(gc, getName(), newexpr);
}

Expr *LambdaExpr::replace(GCMain &gc, const std::string &name, Expr *newexpr) const noexcept {
  if (name == getName())
    return const_cast<Expr*>(dynamic_cast<const Expr*>(this));

  return new LambdaExpr(gc, getName(), expr->replace(gc, name, newexpr));
}

Expr *BiOpExpr::replace(GCMain &gc, const std::string &name, Expr *newexpr) const noexcept {
  return new BiOpExpr(gc, op,
      lhs->replace(gc, name, newexpr),
      rhs->replace(gc, name, newexpr));
}

Expr *IdExpr::replace(GCMain &gc, const std::string &name, Expr *newexpr) const noexcept {
  if (name == getName())
    return newexpr;

  return const_cast<Expr*>(dynamic_cast<const Expr*>(this));
}

// evaluate
//
Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env, Expr *expr) noexcept {
  Expr *oldExpr = expr;
  while (expr && (expr = expr->eval(gc, env)) != oldExpr) oldExpr = expr;

  return expr;
}
