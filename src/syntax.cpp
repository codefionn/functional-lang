#include "func/syntax.hpp"

// Environment

bool Environment::contains(const std::string &name) const noexcept {
  if (variables.count(name) > 0)
    return true;

  return parent ? parent->contains(name) : false;
}

const Expr *Environment::get(const std::string &name) const noexcept {
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
  for (std::pair<std::string, const Expr*> var : variables) {
    const_cast<Expr*>(var.second)->mark(gc);
  }

  if (parent) parent->mark(gc);
}

// BiOpExpr

bool BiOpExpr::isAtomConstructor() const noexcept {
  if (getOperator() != op_fn) return false;

  if (getRHS().getExpressionType() != expr_id
      && getRHS().getExpressionType() != expr_any
      && (getRHS().getExpressionType() == expr_biop
        && !dynamic_cast<const BiOpExpr*>(rhs)->isAtomConstructor()))
      return false;

  if (getLHS().getExpressionType() == expr_atom) return true;
  if (getLHS().getExpressionType() != expr_biop) return false;

  return dynamic_cast<const BiOpExpr*>(lhs)->isAtomConstructor();
}

const AtomExpr *BiOpExpr::getAtomConstructor() const noexcept {
  if (getOperator() != op_fn) return nullptr;

  if (getLHS().getExpressionType() == expr_atom)
    return dynamic_cast<const AtomExpr*>(lhs);

  if (getLHS().getExpressionType() == expr_biop)
    return dynamic_cast<const BiOpExpr*>(lhs)->getAtomConstructor();

  return nullptr;
}

// Expressions

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg, const TokenPos &pos) {
  lexer.skipNewLine = false; // reset new line skip
  lexer.reportError(msg, pos);
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
    if (op == '='
        && lhs->getExpressionType() != expr_id
        && (lhs->getExpressionType() == expr_biop
          && !dynamic_cast<BiOpExpr*>(lhs)->isAtomConstructor()))
      return reportSyntaxError(lexer, "<id> '=' <expr> expected!", lhs->getTokenPos());

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

const Expr *assignExpressions(GCMain &gc, Environment &env,
    const Expr *thisExpr,
    const Expr *lhs, const Expr *rhs) noexcept {

  if (lhs->getExpressionType() == expr_id) {
    std::string id = ((IdExpr*) lhs)->getName();
    if (env.getVariables().count(id) == 0) {
      env.getVariables().insert(std::pair<std::string, const Expr*>(id, rhs));
      return thisExpr;
    }

    return reportSyntaxError(env.lexer, "Variable " + id + " already exists.",
        lhs->getTokenPos());
  }

  if (lhs->getExpressionType() == expr_any)
    return thisExpr;

  if (lhs->getExpressionType() == expr_biop
      && dynamic_cast<const BiOpExpr*>(lhs)->isAtomConstructor()) {

    // Evaluate rhs and check for right expression type
    Expr *newrhs = ::eval(gc, env, const_cast<Expr*>(rhs));
    if (!newrhs) return nullptr;
    if (newrhs->getExpressionType() != expr_biop
        || dynamic_cast<BiOpExpr*>(newrhs)->getOperator() != op_fn) {
      return reportSyntaxError(env.lexer,
          "RHS must be a substitution expression!",
          newrhs->getTokenPos());
    }

    const BiOpExpr *bioplhs = dynamic_cast<const BiOpExpr*>(lhs);
    const BiOpExpr *bioprhs = dynamic_cast<const BiOpExpr*>(newrhs);

    const Expr *exprlhs = bioplhs;
    const Expr *exprrhs = bioprhs;
    while (exprlhs->getExpressionType() == expr_biop
        && exprrhs->getExpressionType() == expr_biop
        && dynamic_cast<const BiOpExpr*>(exprlhs)->getOperator() == op_fn
        && dynamic_cast<const BiOpExpr*>(exprrhs)->getOperator() == op_fn) {
      if (!assignExpressions(gc, env,
          thisExpr,
          &dynamic_cast<const BiOpExpr*>(exprlhs)->getRHS(),
          &dynamic_cast<const BiOpExpr*>(exprrhs)->getRHS()))
        return nullptr; // error forwarding

      // assigned, now next expressions
      exprlhs = &dynamic_cast<const BiOpExpr*>(exprlhs)->getLHS();
      exprrhs = &dynamic_cast<const BiOpExpr*>(exprrhs)->getLHS();
    }

    // exprlhs and exprrhs have to be expr_atom
    if (exprlhs->getExpressionType() != expr_atom) {
      return reportSyntaxError(env.lexer,
        "Most left expression of LHS must be an atom.",
        exprlhs->getTokenPos());
    }
    if (exprrhs->getExpressionType() != expr_atom) {
      return reportSyntaxError(env.lexer,
        "Most left expression of RHS must be an atom.",
        exprrhs->getTokenPos());
    }

    // Check if atoms are equal
    if (dynamic_cast<const AtomExpr*>(exprlhs)->getName()
        != dynamic_cast<const AtomExpr*>(exprrhs)->getName()) {

      reportSyntaxError(env.lexer, "", exprlhs->getTokenPos());
      return reportSyntaxError(env.lexer,
          "Assignment of atom constructors requires same name. "
        + dynamic_cast<const AtomExpr*>(exprlhs)->getName() 
        + " != " + dynamic_cast<const AtomExpr*>(exprrhs)->getName()
        + ".", exprrhs->getTokenPos());
    }

    return thisExpr;
  }

  return nullptr;
}

Expr *BiOpExpr::eval(GCMain &gc, Environment &env) noexcept {
  switch (op) {
  case op_asg: {
      return const_cast<Expr*>(assignExpressions(gc, env, this, lhs, rhs));
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

              TokenPos mergedPos = TokenPos(newlhs->getTokenPos(), newrhs->getTokenPos());

              if (op == op_eq)
                return new AtomExpr(gc, mergedPos,
                    boolToAtom(newlhs->equals(newrhs)));

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
                case op_leq: return new AtomExpr(gc, mergedPos, boolToAtom(num0 <= num1));
                case op_geq: return new AtomExpr(gc, mergedPos, boolToAtom(num0 >= num1));
                case op_le: return new AtomExpr(gc, mergedPos, boolToAtom(num0 < num1));
                case op_gt: return new AtomExpr(gc, mergedPos, boolToAtom(num0 > num1));
                }
                return new NumExpr(gc, mergedPos, num0);
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
  const Expr *val = env.get(getName());
  if (!val) {
    return reportSyntaxError(env.lexer, "Variable " + id + " doesn't exist.",
      this->getTokenPos());
  }

  return const_cast<Expr*>(val);
}

Expr *IfExpr::eval(GCMain &gc, Environment &env) noexcept {
  Expr *resCondition = ::eval(gc, env, condition);
  if (!resCondition)
    return nullptr;

  if (resCondition->getExpressionType() != expr_atom) {
    return reportSyntaxError(env.lexer,
        "Invalid if condition. Doesn't evaluate to atom.",
        getTokenPos());
  }

  AtomExpr *cond = dynamic_cast<AtomExpr*>(resCondition);

  if (cond->getName() != "false")
    return ::eval(gc, env, exprTrue);
  else
    return ::eval(gc, env, exprFalse);
}

Expr *LetExpr::eval(GCMain &gc, Environment &env) noexcept {
  // create new scope
  Environment *scope = new Environment(gc, env.lexer, &env /* == parent */);
  // iterate through assignments and eval them
  for (BiOpExpr *expr : assignments)
    if (!expr->eval(gc, *scope)) // only one execution required (because asg)
      return nullptr;

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

  return new LambdaExpr(gc, getTokenPos(), getName(), expr->replace(gc, name, newexpr));
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
  return new IfExpr(gc, getTokenPos(),
      condition->replace(gc, name, newexpr),
      exprTrue->replace(gc, name, newexpr),
      exprFalse->replace(gc, name, newexpr));
}

Expr *LetExpr::replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept {
  bool changedAsg = false;
  bool overwritesId = false;
  std::vector<BiOpExpr*> newassignments;
  for (BiOpExpr *asg : assignments) {
    for (std::string &id : asg->getLHS().getIdentifiers()) {
      if (id == name)
        overwritesId = true;

      Expr *newasgrhs = asg->getRHS().replace(gc, name, expr);
      if (newasgrhs != &asg->getRHS()) {
        changedAsg = true;
        newassignments.push_back(new BiOpExpr(gc, op_asg,
             const_cast<Expr*>(&asg->getLHS()), newasgrhs));
      } else {
        newassignments.push_back(asg);
      }
    }
  }

  if (!overwritesId) {
    Expr *newbody = body->replace(gc, name, expr);
    if (newbody == body && !changedAsg)
      return const_cast<Expr*>(dynamic_cast<const Expr*>(this));

    return new LetExpr(gc, getTokenPos(),
        changedAsg ? newassignments : assignments, newbody);
  }

  if (changedAsg)
    return new LetExpr(gc, getTokenPos(), newassignments, body);

  return const_cast<Expr*>(dynamic_cast<const Expr*>(this));
}

// evaluate

Expr *eval(GCMain &gc, Environment &env, Expr *expr) noexcept {
  Expr *oldExpr = expr;
  while (expr && (expr = expr->eval(gc, env)) != oldExpr) {
    oldExpr = expr;
  }

  return expr;
}
