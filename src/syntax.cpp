#include "func/syntax.hpp"

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg) {
  lexer.reportError(msg);
  return nullptr;
}

Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env) {
  Expr* result = nullptr;

  while (lexer.currentToken() == tok_id
      || lexer.currentToken() == tok_num
      || lexer.currentToken() == tok_obrace
      || lexer.currentToken() == tok_lambda
      || lexer.currentToken() == tok_atom
      || lexer.currentToken() == tok_if
      || lexer.currentToken() == tok_literal) {
    switch (lexer.currentToken()) {
      case tok_id: {
         IdExpr *idexpr = new IdExpr(gc, lexer.currentIdentifier());
          if (!result)
            result = idexpr;
          else {
            result = new BiOpExpr(gc, op_fn, result, idexpr);
          }

          lexer.nextToken(); // eat id
          break;
        } // end case tok_id
      case tok_num: {
          NumExpr *numexpr = new NumExpr(gc, lexer.currentNumber());
          if (!result)
            result = numexpr;
          else
            result = new BiOpExpr(gc, op_fn, result, numexpr);

          lexer.nextToken(); // eat num
          break;
        } // end case tok_num
      case tok_obrace: {
          lexer.nextToken(); // eat (
          Expr *oldResult = result;
          result = parse(gc, lexer, env);
          if (!result || lexer.currentToken() != tok_cbrace) {
            return reportSyntaxError(lexer, "Expected matching closing bracket )");
          }

          lexer.nextToken(); // eat )

          if (oldResult) {
            result = new BiOpExpr(gc, op_fn, oldResult, result);
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

          Expr *expr = parse(gc, lexer, env);
          if (!expr)
            return nullptr; // Error forwarding

          Expr *lambdaExpr = new LambdaExpr(gc, idname, expr);

          if (!result)
            result = lambdaExpr;
          else
            result = new BiOpExpr(gc, op_fn, result, lambdaExpr);

          break;
        } // end case tok_lambda
      case tok_atom: {
          lexer.nextToken(); // eat .
          if (lexer.currentToken() != tok_id)
            return reportSyntaxError(lexer, "Expected identifier!");

          std::string idname = lexer.currentIdentifier();
          lexer.nextToken(); // eat id

          Expr *atom = new AtomExpr(gc, idname);
          if (!result)
            result = atom;
          else
            result = new BiOpExpr(gc, op_fn, result, atom);

          break;
        } //end case tok_atom
      case tok_if: {
          lexer.nextToken(); // eat if

          Expr *condition = parse(gc, lexer, env);
          if (!condition)
            return nullptr;

          if (lexer.currentToken() != tok_then)
            return reportSyntaxError(lexer, "Expected keyword 'then'.");
          lexer.nextToken(); // eat then

          Expr *exprTrue = parse(gc, lexer, env);
          if (!exprTrue)
            return nullptr;

          if (lexer.currentToken() != tok_else)
            return reportSyntaxError(lexer, "Expected keyword 'else'.");
          lexer.nextToken(); // eat else

          Expr *exprFalse = parse(gc, lexer, env);
          if (!exprFalse)
            return nullptr;

          Expr *ifExpr = new IfExpr(gc, condition, exprTrue, exprFalse);
          if (!result)
            result = ifExpr;
          else
            result = new BiOpExpr(gc, op_fn, result, ifExpr);

          break;
        } // end case tok_if
      case tok_literal: {
          lexer.nextToken(); // eat $

          Expr *expr = parse(gc, lexer, env);
          if (!expr) return nullptr;

          return ::eval(gc, env, expr);
        } // end case tok_literal
    }
  }

  if (lexer.currentToken() == tok_err)
    return nullptr;

  if (!result)
    return reportSyntaxError(lexer, "Not a primary expression token!");

  return result;
}

Expr *parse(GCMain &gc, Lexer &lexer, Environment &env) {
  switch(lexer.currentToken()) {
    case tok_err:
    case tok_eof:
    case tok_eol:
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

  return parseRHS(gc, lexer, env, primaryExpr, 0); // 0 = minimum precedence
  // (least binding)
}

Expr *parseRHS(GCMain &gc, Lexer &lexer, Environment &env, Expr *lhs, int prec) {
  while (lexer.currentToken() == tok_op && lexer.currentPrecedence() >= prec) {
    Operator op = lexer.currentOperator();
    int opprec = lexer.currentPrecedence();
    lexer.nextToken(); // eat op

    // exceptions
    if (op == '=' && lhs->getExpressionType() != expr_id)
      return reportSyntaxError(lexer, "<id> '=' <expr> expected!");

    Expr *rhs = parsePrimary(gc, lexer, env);
    if (!rhs) {
      return nullptr; // Error forwarding
    }

    while (lexer.currentToken() == tok_op
        && (lexer.currentPrecedence() > opprec
            || (lexer.currentOperator() == op_asg // left associative
                && lexer.currentPrecedence() == opprec))) {
      rhs = parseRHS(gc, lexer, env, rhs, lexer.currentPrecedence());
    }

    lhs = new BiOpExpr(gc, op, lhs, rhs);
  }

  return lhs;
}

// interpreter stuff

static std::string boolToAtom(bool b) {
  return b ? "true" : "false";
}

Expr *BiOpExpr::eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept {
  switch (op) {
  case op_asg: {
              std::string id = ((IdExpr*) lhs)->getName();
              if (env.find(id) == env.end()) {
                env.insert(std::pair<std::string, Expr*>(id, rhs));
                return this;
              }

              std::cerr << "Variable " << id << " already exists." << std::endl;
              return nullptr;
            }
  case op_eq:
  case op_leq:
  case op_geq:
  case op_le:
  case op_gt:
  case op_add:
  case op_sub:
  case op_mul:
  case op_div:
  case op_pow: {
              Expr *newlhs = ::eval(gc, env, lhs);
              if (!newlhs) return nullptr; // error forwarding
              Expr *newrhs = ::eval(gc, env, rhs);
              if (!newrhs) return nullptr; // error forwarding

              if (newlhs->getExpressionType() == expr_num
                  && newrhs-> getExpressionType() == expr_num) {
                double num0 = ((NumExpr*) newlhs)->getNumber();
                double num1 = ((NumExpr*) newrhs)->getNumber();
                switch (op) {
                case op_add: num0 += num1; break;
                case op_sub: num0 -= num1; break;
                case op_mul: num0 *= num1; break;
                case op_div: num0 /= num1; break;
                case op_pow: num0 = pow(num0, num1); break;
                case op_eq: return new AtomExpr(gc, boolToAtom(num0 == num1));
                case op_leq: return new AtomExpr(gc, boolToAtom(num0 <= num1));
                case op_geq: return new AtomExpr(gc, boolToAtom(num0 >= num1));
                case op_le: return new AtomExpr(gc, boolToAtom(num0 < num1));
                case op_gt: return new AtomExpr(gc, boolToAtom(num0 > num1));
                }
                return new NumExpr(gc, num0);
              }

              if (newlhs->getExpressionType() == expr_atom
                  && newrhs->getExpressionType() == expr_atom) {
                const std::string &atom0 = ((AtomExpr*) newlhs)->getName();
                const std::string &atom1 = ((AtomExpr*) newrhs)->getName();
                switch(op) {
                case op_eq:
                  return new AtomExpr(gc, boolToAtom(atom0 == atom1));
                }
              }

              if (newlhs == lhs && newrhs == rhs)
                return this;

              return new BiOpExpr(gc, op, newlhs, newrhs);
            }
  case op_fn: {
              lhs = ::eval(gc, env, lhs);
              if (!lhs)
                return nullptr; // Error forwarding

              if (lhs->getExpressionType() != expr_lambda) {
                return this; // Just return this (without substitution)
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

Expr *IfExpr::eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept {
  Expr *resCondition = ::eval(gc, env, condition);
  if (!resCondition)
    return nullptr;

  if (resCondition->getExpressionType() != expr_atom) {
    std::cerr << "Invalid if condition. Doesn't evaluate to atom." << std::endl;
    return this;
  }

  AtomExpr *cond = dynamic_cast<AtomExpr*>(resCondition);

  if (cond->getName() != "false")
    return ::eval(gc, env, exprTrue);
  else
    return ::eval(gc, env, exprFalse);
}

// replace/substitute

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

Expr *IfExpr::replace(GCMain &gc, const std::string &name, Expr *newexpr) const noexcept {
  return new IfExpr(gc,
      condition->replace(gc, name, newexpr),
      exprTrue->replace(gc, name, newexpr),
      exprFalse->replace(gc, name, newexpr));
}

// evaluate

Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env, Expr *expr) noexcept {
  Expr *oldExpr = expr;
  while (expr && (expr = expr->eval(gc, env)) != oldExpr) oldExpr = expr;

  return expr;
}
