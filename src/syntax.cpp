#include "func/syntax.hpp"

// Environment

bool Environment::contains(const std::string &name) const noexcept {
  if (variables.count(name) > 0)
    return true;

  return parent ? parent->contains(name) : false;
}

Expr *Environment::get(const std::string &name) const noexcept {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }

  return parent ? parent->get(name) : nullptr;
}

void Environment::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;
  
  markSelf(gc);
  for (std::pair<std::string, Expr*> var : variables) {
    var.second->mark(gc);
  }

  if (parent) parent->mark(gc);
}

// Expressions

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg) {
  lexer.skipNewLine = false; // reset new line skip
  lexer.reportError(msg);
  return nullptr;
}


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

Expr *BiOpExpr::eval(GCMain &gc, Environment &env) noexcept {
  switch (op) {
  case op_asg: {
              if (lhs->getExpressionType() == expr_id) {
                std::string id = ((IdExpr*) lhs)->getName();
                if (env.getVariables().count(id) == 0) {
                  env.getVariables().insert(std::pair<std::string, Expr*>(id, rhs));
                  return this;
                }

                std::cerr << "Variable " << id << " already exists." << std::endl;
                return nullptr;
              }
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

              if (op == op_eq)
                return new AtomExpr(gc, boolToAtom(newlhs->equals(newrhs)));

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
                case op_leq: return new AtomExpr(gc, boolToAtom(num0 <= num1));
                case op_geq: return new AtomExpr(gc, boolToAtom(num0 >= num1));
                case op_le: return new AtomExpr(gc, boolToAtom(num0 < num1));
                case op_gt: return new AtomExpr(gc, boolToAtom(num0 > num1));
                }
                return new NumExpr(gc, num0);
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
                // Evaluate rhs
                Expr *newrhs = ::eval(gc, env, rhs);
                if (newrhs == rhs)
                  return this;
                else
                  return new BiOpExpr(gc, op_fn, lhs, newrhs);
              }

              return ((LambdaExpr*)lhs)->replace(gc, rhs);
            }
  }

  std::cerr << "Interpreter error." << std::endl;
  return nullptr;
}

Expr *IdExpr::eval(GCMain &gc, Environment &env) noexcept {
  Expr *val = env.get(getName());
  if (!val) {
    std::cerr << "Invalid identifier " << getName() << "." << std::endl;
    return nullptr;
  }

  return val;
}

Expr *IfExpr::eval(GCMain &gc, Environment &env) noexcept {
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

Expr *LetExpr::eval(GCMain &gc, Environment &env) noexcept {
  // create new scope
  Environment *scope = new Environment(gc, &env /* == parent */);
  // iterate through assignments and eval them
  for (BiOpExpr *expr : assignments)
    expr->eval(gc, *scope); // only one execution required (because asg)

  // Evaluate body with the new scope
  return ::eval(gc, *scope, body);
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

Expr *eval(GCMain &gc, Environment &env, Expr *expr) noexcept {
  Expr *oldExpr = expr;
  while (expr && (expr = expr->eval(gc, env)) != oldExpr) {
    oldExpr = expr;
  }

  return expr;
}
