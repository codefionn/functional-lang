#include "func/syntax.hpp"

// Expr

Expr *Expr::evalWithLookup(GCMain &gc, Environment &env) noexcept {
  if (lastEval  && (getExpressionType() != expr_biop
          || dynamic_cast<const BiOpExpr*>(this)->getOperator() != op_asg))
    return lastEval;

  return lastEval = eval(gc, env);
}

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

const Expr *Environment::currentGet(const std::string &name) const noexcept {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }

  return nullptr;
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

// mark

void FunctionExpr::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);

  for (const std::pair<std::vector<Expr*>, Expr*> &fncase : fncases) {
    for (Expr *expr : fncase.first) {
      expr->mark(gc); 
    }

    fncase.second->mark(gc);
  }
}

// BiOpExpr

bool BiOpExpr::isFunctionConstructor() const noexcept {
  if (getOperator() != op_fn) return false;

  if (getLHS().getExpressionType() == expr_biop)
    return dynamic_cast<const BiOpExpr&>(getLHS()).isFunctionConstructor();

  return getLHS().getExpressionType() == expr_id;
}

bool BiOpExpr::isAtomConstructor() const noexcept {
  if (getOperator() != op_fn) return false;

  if (getRHS().getExpressionType() != expr_id
      && (getRHS().getExpressionType() != expr_biop
        || !dynamic_cast<const BiOpExpr*>(rhs)->isAtomConstructor()))
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

    // exceptions
    /*if (op == op_asg
        && lhs->getExpressionType() != expr_id
        && (lhs->getExpressionType() == expr_biop
          && !dynamic_cast<BiOpExpr*>(lhs)->isAtomConstructor())
        && lhs->getExpressionType() != expr_any)
      return reportSyntaxError(lexer, "Expected identifier, atom constructor or any!", lhs->getTokenPos());*/

    lexer.nextToken(); // eat op

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

    return reportSyntaxError(*env.lexer, "Variable " + id + " already exists.",
        lhs->getTokenPos());
  }

  if (lhs->getExpressionType() == expr_biop
      && dynamic_cast<const BiOpExpr*>(lhs)->isAtomConstructor()) {
    // Atom constructor (pattern matching)

    // Evaluate rhs and check for right expression type
    Expr *newrhs = ::eval(gc, env, const_cast<Expr*>(rhs));
    if (!newrhs) return nullptr; // error forwarding
    if (newrhs->getExpressionType() != expr_biop
        || dynamic_cast<BiOpExpr*>(newrhs)->getOperator() != op_fn) {
      return reportSyntaxError(*env.lexer,
          "RHS must be a substitution expression!",
          newrhs->getTokenPos());
    }

    // Cast both sides
    const BiOpExpr *bioplhs = dynamic_cast<const BiOpExpr*>(lhs);
    const BiOpExpr *bioprhs = dynamic_cast<const BiOpExpr*>(newrhs);

    // Assign all statements.
    // Iterator through lhs and rhs till no longer binary function operator.
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

      // assigned, now next expressions (iterator step)
      exprlhs = &dynamic_cast<const BiOpExpr*>(exprlhs)->getLHS();
      exprrhs = &dynamic_cast<const BiOpExpr*>(exprrhs)->getLHS();
    }

    // exprlhs and exprrhs have to be expr_atom
    if (exprlhs->getExpressionType() != expr_atom) {
      return reportSyntaxError(*env.lexer,
        "Most left expression of LHS must be an atom.",
        exprlhs->getTokenPos());
    }
    if (exprrhs->getExpressionType() != expr_atom) {
      return reportSyntaxError(*env.lexer,
        "Most left expression of RHS must be an atom.",
        exprrhs->getTokenPos());
    }

    // Check if atoms are equal
    if (dynamic_cast<const AtomExpr*>(exprlhs)->getName()
        != dynamic_cast<const AtomExpr*>(exprrhs)->getName()) {

      // Print position of atom lhs
      reportSyntaxError(*env.lexer, "", exprlhs->getTokenPos());
      // Print error with atom rhs
      return reportSyntaxError(*env.lexer,
          "Assignment of atom constructors requires same name. "
        + dynamic_cast<const AtomExpr*>(exprlhs)->getName() 
        + " != " + dynamic_cast<const AtomExpr*>(exprrhs)->getName()
        + ".", exprrhs->getTokenPos());
    }

    return thisExpr;
  }

  if (lhs->getExpressionType() == expr_biop
      && dynamic_cast<const BiOpExpr*>(lhs)->isFunctionConstructor()) {

    // function constructor

    std::pair<std::vector<Expr*>, Expr*> fncase;
    fncase.second = const_cast<Expr*>(rhs);
    
    const Expr *expr = lhs;
    while (expr->getExpressionType() == expr_biop) {
      fncase.first.insert(fncase.first.begin(),
          const_cast<Expr*>(&dynamic_cast<const BiOpExpr*>(expr)->getRHS()));
      expr = const_cast<Expr*>(&dynamic_cast<const BiOpExpr*>(expr)->getLHS());
    }

    const std::string &fnname = dynamic_cast<const IdExpr*>(expr)->getName();
    const Expr *fnexpr = env.currentGet(fnname);
    if (!fnexpr) {
      fnexpr = new FunctionExpr(gc, expr->getTokenPos(), fnname, fncase);
      env.getVariables().insert(
          std::pair<std::string, Expr*>(fnname, const_cast<Expr*>(fnexpr)));
      return thisExpr;
    } else if (fnexpr->getExpressionType() == expr_fn) {
      if(!const_cast<FunctionExpr*>(dynamic_cast<const FunctionExpr*>(fnexpr))->addCase(fncase))
        return reportSyntaxError(*env.lexer,
            "Function argument length of \"" + fnname + "\" don't match.",
            expr->getTokenPos());

      return thisExpr;
    }

    return reportSyntaxError(*env.lexer,
        "Identifier \"" + fnname + "\" already assigned to a non-function!",
        expr->getTokenPos());
  }

  return reportSyntaxError(*env.lexer,
      "Invalid assignment. Only atom constructors, functions and identifier allowed.",
      lhs->getTokenPos());
}

static Expr *biopeval(GCMain &gc, Environment &env,
    const TokenPos &mergedPos, Operator op,
    const NumExpr *lhs, const NumExpr *rhs) noexcept {
  double num0 = lhs->getNumber();
  double num1 = rhs->getNumber();
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

static Expr *biopeval(GCMain &gc, Environment &env,
    const TokenPos &mergedPos, Operator op,
    const IntExpr *lhs, const IntExpr *rhs) noexcept {
  std::int64_t num0 = lhs->getNumber();
  std::int64_t num1 = rhs->getNumber();
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
  return new IntExpr(gc, mergedPos, num0);
}

Expr *BiOpExpr::eval(GCMain &gc, Environment &env) noexcept {
  TokenPos mergedPos = this->getTokenPos();

  switch (op) {
  case op_asg: {
      return const_cast<Expr*>(assignExpressions(gc, env, this, lhs, rhs));
    }
  case op_land:
  case op_lor: {
       // Lazy evaluation
       Expr *newlhs = ::eval(gc, env, lhs);
       if (!newlhs) return nullptr; // error forwarding
       if (newlhs->getExpressionType() != expr_atom) break;

       const std::string &namelhs = dynamic_cast<const AtomExpr*>(newlhs)->getName();
       if (op == op_land && namelhs == "false")
         return new AtomExpr(gc, mergedPos, boolToAtom(false));
       if (op == op_lor && namelhs != "false")
         return new AtomExpr(gc, mergedPos, boolToAtom(true));

       Expr *newrhs = ::eval(gc, env, rhs);
       if (!newrhs) return nullptr; // error forwarding
       if (newrhs->getExpressionType() != expr_atom) break;

       const std::string &namerhs = dynamic_cast<const AtomExpr*>(newrhs)->getName();
       if (op == op_land || op == op_lor)
         return new AtomExpr(gc, mergedPos, boolToAtom(namerhs != "false"));

       break;
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
                return new AtomExpr(gc, mergedPos,
                    boolToAtom(newlhs->equals(newrhs)));

              if (newlhs->getExpressionType() == expr_num
                  && newrhs-> getExpressionType() == expr_num) {
                return biopeval(gc, env, mergedPos, op,
                    dynamic_cast<const NumExpr*>(newlhs),
                    dynamic_cast<const NumExpr*>(newrhs));
              } else if (newlhs->getExpressionType() == expr_int
                  && newrhs->getExpressionType() == expr_int){
                return biopeval(gc, env, mergedPos, op,
                    dynamic_cast<const IntExpr*>(newlhs),
                    dynamic_cast<const IntExpr*>(newrhs));
              }

               break;
            }
  case op_fn: {
              // special cases (built-in functions)
              if (lhs->getExpressionType() == expr_id) {
                const std::string &id = dynamic_cast<IdExpr*>(lhs)->getName();
                if (id == "error") { // print error message
                  return reportSyntaxError(*env.lexer,
                      rhs->toString(),
                      TokenPos(lhs->getTokenPos(), rhs->getTokenPos()));
                } else if (id == "print") { // print expression and return expr
                  std::cout << rhs->toString() << std::endl;
                  return rhs;
                } else if (id == "to_int") { // truncate float to int
                  Expr *expr = ::eval(gc, env, rhs);
                  if (!expr) return nullptr; // error forwarding
                  switch (expr->getExpressionType()) {
                  case expr_int:
                    return expr;
                  case expr_num:
                    return new IntExpr(gc, mergedPos,
                        (int64_t) floor(dynamic_cast<NumExpr*>(expr)->getNumber()));
                  }
                } else if (id == "round_int") {
                  Expr *expr = ::eval(gc, env, rhs);
                  if (!expr) return nullptr; // error forwarding
                  switch (expr->getExpressionType()) { // rounded float to int
                  case expr_int:
                    return expr;
                  case expr_num:
                    return new IntExpr(gc, mergedPos,
                        (int64_t) round(dynamic_cast<NumExpr*>(expr)->getNumber()));
                  }
                } else if(id == "time") { // prints time spent evaluating RHS
                  auto startTime = std::chrono::high_resolution_clock::now();

                  Expr *expr = ::eval(gc, env, rhs);
                  if (!expr) return nullptr;

                  auto endTime = std::chrono::high_resolution_clock::now();
                  auto diffTime = endTime - startTime;
                  double consumedTime = diffTime.count() *
                    (double)std::chrono::high_resolution_clock::period::num /
                    (double)std::chrono::high_resolution_clock::period::den
                    * 1000;

                  std::cout << "Needed "
                    << consumedTime
                    << " ms." << std::endl;

                  return expr;
                }
              }

              lhs = ::eval(gc, env, lhs);
              if (!lhs)
                return nullptr; // Error forwarding

              if (lhs->getExpressionType() != expr_lambda) {
                // Evaluate rhs
                Expr *newrhs = ::eval(gc, env, rhs);
                if (newrhs == rhs)
                  return this;
                else if (!newrhs)
                  return nullptr; // error forwarding
                else
                  return new BiOpExpr(gc, getTokenPos(),op_fn, lhs, newrhs);
              }

              return ((LambdaExpr*)lhs)->replace(gc, rhs);
            }
  }

  return reportSyntaxError(*env.lexer, 
      "Invalid use of binary operator.",
      this->getTokenPos());
}

Expr *IdExpr::eval(GCMain &gc, Environment &env) noexcept {
  const Expr *val = env.get(getName());
  if (!val) {
    return reportSyntaxError(*env.lexer, "Variable " + id + " doesn't exist.",
      this->getTokenPos());
  }

  return const_cast<Expr*>(val);
}

Expr *IfExpr::eval(GCMain &gc, Environment &env) noexcept {
  Expr *resCondition = ::eval(gc, env, condition);
  if (!resCondition)
    return nullptr;

  if (resCondition->getExpressionType() != expr_atom) {
    return reportSyntaxError(*env.lexer,
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

  // Only the main environment should be used, because with secondary
  // environments identifiers with equal names can be assigned to each other,
  // but they can be from different scopes !!!. This was necessary after having
  // problems with named functions (namely the fibonacci-function).

  // How to do this? Lambda Expression!
  // We take all identifiers from our scope environment and make them to
  // lambda sustitution expression.

  Expr *result = body;
  auto &vars = scope->getVariables();
  for (auto it = vars.begin(); it != vars.end(); ++it) {
    auto &p = *it;
    result = result->replace(gc, p.first, const_cast<Expr*>(p.second));
  }

  return ::eval(gc, *scope, result);
}

Expr *FunctionExpr::eval(GCMain &gc, Environment &env) noexcept {
  Expr *lambdaFn = nullptr;
  Expr *noMatch = new BiOpExpr(gc, this->getTokenPos(), op_fn,
      new IdExpr(gc, this->getTokenPos(), "error"),
      new IdExpr(gc, this->getTokenPos(), "\"No Match\""));

  for (auto it = fncases.rbegin(); it != fncases.rend(); ++it) {
    // Work on one function case
    const std::pair<std::vector<Expr*>, Expr*> &fncase = *it;

    Expr *exprCondition = nullptr; // if condition
    Expr *exprFnBody = fncase.second; // function body

    size_t xid = 0; // argument id
    for (Expr *expr : fncase.first) {
      IdExpr *argumentId
        = new IdExpr(gc, expr->getTokenPos(), std::string("_x") + std::to_string(xid++));

      // let statement
      if (expr->getExpressionType() == expr_id
          || (expr->getExpressionType() == expr_biop
            && dynamic_cast<const BiOpExpr*>(expr)->isAtomConstructor())) {
        exprFnBody = new LetExpr(gc, expr->getTokenPos(),
            std::vector<BiOpExpr*>{new BiOpExpr(gc,
                fncase.second->getTokenPos(),
                op_asg, expr, argumentId)}, exprFnBody);
      }

      // Check if equality check needed
      if (expr->getExpressionType() == expr_any
          || expr->getExpressionType() == expr_id)
        continue;

      // For checking equality we need expr, where ids are replaced by ANY
      Expr *noidexpr = expr->replace(gc,  "", nullptr);
      Expr *equalityCheck = new BiOpExpr(gc, noidexpr->getTokenPos(),
          op_eq, noidexpr, argumentId);
      if (!exprCondition)
        exprCondition = equalityCheck; 
      else
        exprCondition = new BiOpExpr(gc, op_land, exprCondition, equalityCheck);
    }

    // exprCondition might be nullptr
    if (!exprCondition)
      lambdaFn = exprFnBody;
    else  {
      lambdaFn = new IfExpr(gc,
          TokenPos(fncase.first.at(0)->getTokenPos(),
                   fncase.first.at(fncase.first.size() - 1)->getTokenPos()),
          exprCondition, exprFnBody, 
          lambdaFn ? lambdaFn : noMatch);
    }
  }

  //  lambdaFn is not nullptr because at least one case exists
  // Now construct lambda Function (lambdaFn is only the body)

  Expr *result = lambdaFn;
  for (size_t i = fncases.at(0).first.size(); i > 0; --i) {
    result = new LambdaExpr(gc, this->getTokenPos(),
      "_x" + std::to_string(i - 1), result); // Not type-able identifier
  }

  return result;
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
  return new BiOpExpr(gc, this->getTokenPos(), op,
      lhs->replace(gc, name, newexpr),
      rhs->replace(gc, name, newexpr));
}

Expr *IdExpr::replace(GCMain &gc, const std::string &name, Expr *newexpr) const noexcept {
  if (name.empty())
    return new AnyExpr(gc, getTokenPos());

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
        newassignments.push_back(new BiOpExpr(gc, asg->getTokenPos(),
              op_asg,
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
  while (expr && (expr = expr->evalWithLookup(gc, env)) != oldExpr) {
    oldExpr = expr;
  }

  return expr;
}

// equals

bool NumExpr::equals(const Expr *expr) const noexcept {
  if (expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_int
      && expr->getExpressionType() != expr_num) return false;

  if (expr->getExpressionType() == expr_int)
    return dynamic_cast<const IntExpr*>(expr)->getNumber() == round(getNumber());

  return dynamic_cast<const NumExpr*>(expr)->getNumber() == getNumber();
}

bool IntExpr::equals(const Expr *expr) const noexcept {
  if (expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_int
      && expr->getExpressionType() != expr_num) return false;

  if (expr->getExpressionType() == expr_num)
    return round(dynamic_cast<const NumExpr*>(expr)->getNumber()) == getNumber();

  return dynamic_cast<const IntExpr*>(expr)->getNumber() == getNumber();
}
