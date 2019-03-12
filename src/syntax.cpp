#include "func/syntax.hpp"

std::vector<Expr*>::iterator find(std::vector<Expr*> &vec, Expr *expr) noexcept {
  for (auto it = vec.begin(); it != vec.end(); ++it)
    if (*it == expr) return it;

  return vec.end();
}

// Expr

Expr *Expr::evalWithLookup(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

  if (lastEval  && (getExpressionType() != expr_biop
          || dynamic_cast<const BiOpExpr*>(this)->getOperator() != op_asg))
    return lastEval;

  return lastEval = eval(gc, env);
}

// optimize

Expr *Expr::optimize(GCMain &gc) noexcept {
  return this;
}

Expr *BiOpExpr::optimize(GCMain &gc) noexcept {
  if (getOperator() == op_asg) {
    // because binary operator is an assignment, any optimizations
    // where equal expressions have share the same objects will cause
    // wrong behaviour (except if LHS and RHS are equal, but this is already
    // handled by the syntax analysis).
    // so only rhs is optimized

    Expr *newrhs = rhs->optimize(gc);
    if (newrhs == rhs) return this; // No changes

    // RHS changed, create new biopexpr
    return new BiOpExpr(gc, getTokenPos(), op_asg, lhs, newrhs);
  }

  // Equal expressions should share the same memory reference
  
  std::vector<Expr*> sharedPool;
  return optimize(gc, sharedPool);
}

static Expr *biopexprOptimize(GCMain &gc,
    std::vector<Expr*> &exprs, Expr *optexpr) {
  // Search for optexpr in shared pool and if found return the found expr
  for (Expr *expr : exprs)
    if (expr->equals(optexpr))
      return expr;

  // No object to share found ... add to shared pool
  exprs.push_back(optexpr);
  // Return the expression
  return optexpr;
}

static Expr *exprOptimizeList(GCMain &gc,
    std::vector<Expr*> &exprs, Expr *expr) {
  Expr *newexpr;
  switch (expr->getExpressionType()) {
  case expr_biop:
    newexpr = dynamic_cast<BiOpExpr*>(expr)->optimize(gc, exprs);
    break;
  case expr_unop:
    newexpr = dynamic_cast<UnOpExpr*>(expr)->optimize(gc, exprs);
    break;
  case expr_let:
    newexpr = dynamic_cast<LetExpr*>(expr)->optimize(gc, exprs);
    break;
  default:
    newexpr = expr->optimize(gc);
    break;
  }

  return biopexprOptimize(gc, exprs, newexpr);
}

BiOpExpr *BiOpExpr::optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept {
  if (getOperator() == op_asg)
    return dynamic_cast<BiOpExpr*>(optimize(gc));

  Expr *newlhs = exprOptimizeList(gc, exprs, lhs);
  Expr *newrhs = exprOptimizeList(gc, exprs, rhs);
 if (newrhs == rhs && newlhs == lhs) return this; // no changes

  return new BiOpExpr(gc, getTokenPos(), getOperator(), newlhs, newrhs);
}

Expr *UnOpExpr::optimize(GCMain &gc) noexcept {
  std::vector<Expr*> exprs;
  return optimize(gc, exprs);
}

UnOpExpr *UnOpExpr::optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept {
  Expr *newexpr = exprOptimizeList(gc, exprs, expr);
  if (newexpr == expr) return this;

  return new UnOpExpr(gc, getTokenPos(), op, newexpr);
}

Expr *LetExpr::optimize(GCMain &gc) noexcept {
  bool allEqual = true; // If all assignments RHS and LHS are equal
  for (BiOpExpr *asg : assignments) {
    if (!asg->getLHS().equals(&asg->getRHS()))
      allEqual = false;
  }

  if (allEqual) return body; // all assignments are equal. So just the body.

  std::vector<Expr*> exprs;
  std::vector<BiOpExpr*> newassignments;

  // Optimize assignments LHS
  bool changedAssignments = false;
  for (BiOpExpr *asg : assignments) {
    Expr *newlhs = exprOptimizeList(gc, exprs,
        const_cast<Expr*>(&asg->getLHS()));
    if (newlhs != &asg->getLHS()) {
      changedAssignments = true;
      newassignments.push_back(new BiOpExpr(gc, asg->getTokenPos(),
            op_asg, newlhs, const_cast<Expr*>(&asg->getRHS())));
    } else
      newassignments.push_back(asg);
  }

  Expr *newbody = exprOptimizeList(gc, exprs, body);
  if (!changedAssignments && newbody == body) return this; // No changes

  if (!changedAssignments) return new LetExpr(gc, getTokenPos(),
      assignments, newbody);

  if (newbody == body) return new LetExpr(gc, getTokenPos(),
      newassignments, body);

  return new LetExpr(gc, getTokenPos(), newassignments, body);
}

Expr *LetExpr::optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept {
  std::vector<BiOpExpr*> newassignments;
  bool changedAssignments = false;

  // Optimize assignments RHS
  for (BiOpExpr *asg : assignments) {
    Expr *newrhs = exprOptimizeList(gc, exprs,
        const_cast<Expr*>(&asg->getRHS()));
    if (newrhs != &asg->getRHS()) {
      changedAssignments = true;
      newassignments.push_back(new BiOpExpr(gc, asg->getTokenPos(),
            op_asg, const_cast<Expr*>(&asg->getLHS()), newrhs));
    } else
      newassignments.push_back(asg);
  }

  // No optimization in RHS. Optimize LHS and body.
  if (!changedAssignments) return optimize(gc);

  // Optimize LHS and boyd of new optimize let expression (optimized RHS)
  return (new LetExpr(gc, getTokenPos(), newassignments, body))->optimize(gc);
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
  for (std::pair<std::string, const Expr*> var : variables)
    const_cast<Expr*>(var.second)->mark(gc);

  for (Expr *expr : ctx)
    expr->mark(gc);

  if (parent) parent->mark(gc);
}

// mark

void Expr::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);
}

void UnOpExpr::mark(GCMain &gc) noexcept {
  if (isMarked(gc)) return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);

  expr->mark(gc);
}

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
    /*if (op == op_asg
        && lhs->getExpressionType() != expr_id
        && (lhs->getExpressionType() == expr_biop
          && !dynamic_cast<BiOpExpr*>(lhs)->isAtomConstructor())
        && lhs->getExpressionType() != expr_any)
      return reportSyntaxError(lexer, "Expected identifier, atom constructor or any!", lhs->getTokenPos());*/

    lexer.nextToken(); // eat op

    StackFrameObj<Expr> rhs(env, parsePrimary(gc, lexer, env));
    if (!rhs) return nullptr; // Error forwarding

    if (rhs->equals(*lhs))
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
    StackFrameObj<Expr> newrhs(env, ::eval(gc, env, const_cast<Expr*>(rhs)));
    if (!newrhs) return nullptr; // error forwarding
    if (newrhs->getExpressionType() != expr_biop
        || dynamic_cast<BiOpExpr*>(*newrhs)->getOperator() != op_fn) {
      return reportSyntaxError(*env.lexer,
          "RHS must be a substitution expression!",
          newrhs->getTokenPos());
    }

    // Cast both sides
    const BiOpExpr *bioplhs = dynamic_cast<const BiOpExpr*>(lhs);
    const BiOpExpr *bioprhs = dynamic_cast<const BiOpExpr*>(*newrhs);

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
      // save rhs
      fncase.first.insert(fncase.first.begin(),
          const_cast<Expr*>(&dynamic_cast<const BiOpExpr*>(expr)->getRHS()));
      // step down
      expr = const_cast<Expr*>(&dynamic_cast<const BiOpExpr*>(expr)->getLHS());
    }

    // because isFunctionConstructor
    const std::string &fnname = dynamic_cast<const IdExpr*>(expr)->getName();
    StackFrameObj<Expr> fnexpr(env, const_cast<Expr*>(env.currentGet(fnname)));
    if (!fnexpr) {
      fnexpr = new FunctionExpr(gc, expr->getTokenPos(), fnname, fncase);

      // replace recursive call with FunctionExpr
      dynamic_cast<FunctionExpr*>(*fnexpr)->getFunctionCases().front().second = fncase.second->replace(gc, fnname, *fnexpr);

      env.getVariables().insert(
          std::pair<std::string, Expr*>(fnname, const_cast<Expr*>(*fnexpr)));
      return thisExpr;
    } else if (fnexpr->getExpressionType() == expr_fn) {
      if(!const_cast<FunctionExpr*>(dynamic_cast<const FunctionExpr*>(*fnexpr))->addCase(fncase))
        return reportSyntaxError(*env.lexer,
            "Function argument length of \"" + fnname + "\" don't match.",
            expr->getTokenPos());

      // replace recursive call with FunctionExpr
      dynamic_cast<FunctionExpr*>(*fnexpr)->getFunctionCases().back().second = fncase.second->replace(gc, fnname, *fnexpr);

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

Expr *evalLambdaSubstitution(GCMain &gc, Environment &env,
    const TokenPos &mergedPos, Expr* thisExpr,
    Expr *lhs, Expr *rhs) noexcept {
  // special cases (built-in functions)
  if (lhs->getExpressionType() == expr_id) {
    const std::string &id = dynamic_cast<IdExpr*>(lhs)->getName();
    if (id == "error") { // print error message
      return reportSyntaxError(*env.lexer,
          rhs->toString(),
          mergedPos);
    } else if (id == "print") { // print expression and return expr
      std::cout << rhs->toString() << std::endl;
      return rhs;
    } else if (id == "to_int" || id == "round_int") { // truncate float to int
      StackFrameObj<Expr> expr(env, ::eval(gc, env, rhs));
      if (!expr) return nullptr; // error forwarding
      switch (expr->getExpressionType()) {
      case expr_int:
        return *expr;
      case expr_num:
        if (id == "to_int")
          return new IntExpr(gc, mergedPos,
              (int64_t) floor(dynamic_cast<NumExpr*>(*expr)->getNumber()));
        else if (id == "round_int")
          return new IntExpr(gc, mergedPos,
              (int64_t) round(dynamic_cast<NumExpr*>(*expr)->getNumber()));
      }
    } else if(id == "time") { // prints time spent evaluating RHS
      auto startTime = std::chrono::high_resolution_clock::now();

      StackFrameObj<Expr> expr(env, ::eval(gc, env, rhs));
      if (!expr) return nullptr;

      auto endTime = std::chrono::high_resolution_clock::now();
      auto diffTime = endTime - startTime;
      double consumedTime = diffTime.count() *
        (double)std::chrono::high_resolution_clock::period::num /
        (double)std::chrono::high_resolution_clock::period::den
        * 1000; // seconds -> milliseconds

      std::cout << "Needed "
        << consumedTime
        << " ms." << std::endl;

      return *expr;
    }
  }

  // Otherwise eval LHS.

  StackFrameObj<Expr> newlhs(env, ::eval(gc, env, lhs));
  if (!newlhs) return nullptr; // Error forwarding

  if (newlhs->getExpressionType() != expr_lambda) {
    // Evaluate rhs
    StackFrameObj<Expr> newrhs(env, ::eval(gc, env, rhs));
    if (!newrhs)
      return nullptr; // error forwarding
    else if (newrhs == rhs)
      return thisExpr;
    else
      return new BiOpExpr(gc, mergedPos, op_fn, *newlhs, *newrhs);
  }

  // Lambda calculus substitution
  return dynamic_cast<LambdaExpr*>(*newlhs)->replace(gc, rhs);
}

Expr *BiOpExpr::eval(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<BiOpExpr> thisObj(env, this);
  TokenPos mergedPos = this->getTokenPos();

  switch (op) {
  case op_asg: {
      return const_cast<Expr*>(assignExpressions(gc, env, this, lhs, rhs));
    }
  case op_land:
  case op_lor: {
       // Lazy evaluation
       StackFrameObj<Expr> newlhs(env, ::eval(gc, env, lhs));
       if (!newlhs) return nullptr; // error forwarding
       if (newlhs->getExpressionType() != expr_atom) break;

       const std::string &namelhs = dynamic_cast<const AtomExpr*>(*newlhs)->getName();
       if (op == op_land && namelhs == "false")
         return new AtomExpr(gc, mergedPos, boolToAtom(false));
       if (op == op_lor && namelhs != "false")
         return new AtomExpr(gc, mergedPos, boolToAtom(true));

       StackFrameObj<Expr> newrhs(env, ::eval(gc, env, rhs));
       if (!newrhs) return nullptr; // error forwarding
       if (newrhs->getExpressionType() != expr_atom) break;

       const std::string &namerhs = dynamic_cast<const AtomExpr*>(*newrhs)->getName();
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
              StackFrameObj<Expr> newlhs(env, ::eval(gc, env, lhs));
              if (!newlhs) return nullptr; // error forwarding

              StackFrameObj<Expr> newrhs(env, ::eval(gc, env, rhs));
              if (!newrhs) return nullptr; // error forwarding

              if (op == op_eq)
                return new AtomExpr(gc, mergedPos,
                    boolToAtom(newlhs->equals(*newrhs)));

              if (newlhs->getExpressionType() == expr_num
                  && newrhs-> getExpressionType() == expr_num) {
                return biopeval(gc, env, mergedPos, op,
                    dynamic_cast<const NumExpr*>(*newlhs),
                    dynamic_cast<const NumExpr*>(*newrhs));
              } else if (newlhs->getExpressionType() == expr_int
                  && newrhs->getExpressionType() == expr_int){
                return biopeval(gc, env, mergedPos, op,
                    dynamic_cast<const IntExpr*>(*newlhs),
                    dynamic_cast<const IntExpr*>(*newrhs));
              }

               break;
            }
  case op_fn:
    return evalLambdaSubstitution(gc, env, mergedPos,
        this, lhs, rhs);
  }

  return reportSyntaxError(*env.lexer, 
      "Invalid use of binary operator.",
      this->getTokenPos());
}

Expr *UnOpExpr::eval(GCMain &gc, Environment &env) noexcept {
  Expr *newexpr = ::eval(gc, env, expr);
  if (!newexpr) return nullptr;

  if (newexpr->getExpressionType() == expr_num)
    switch (op) {
    case op_add:
      return newexpr;
    case op_sub:
      return new NumExpr(gc, newexpr->getTokenPos(),
          -dynamic_cast<const NumExpr*>(newexpr)->getNumber());
    }
  else if (newexpr->getExpressionType() == expr_int)
    switch (op){
    case op_add:
      return newexpr;
    case op_sub:
      return new IntExpr(gc, newexpr->getTokenPos(),
          -dynamic_cast<const IntExpr*>(newexpr)->getNumber());
    }

  return reportSyntaxError(*env.lexer,
      "Invalid unary operator expression.",
      getTokenPos());
}

Expr *IdExpr::eval(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

  const Expr *val = env.get(getName());
  if (!val) {
    return reportSyntaxError(*env.lexer, "Variable " + id + " doesn't exist.",
      this->getTokenPos());
  }

  return const_cast<Expr*>(val);
}

Expr *IfExpr::eval(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

  StackFrameObj<Expr> resCondition(env, ::eval(gc, env, condition));
  if (!resCondition)
    return nullptr;

  if (resCondition->getExpressionType() != expr_atom) {
    return reportSyntaxError(*env.lexer,
        "Invalid if condition. Doesn't evaluate to atom.",
        getTokenPos());
  }

  AtomExpr *cond = dynamic_cast<AtomExpr*>(*resCondition);

  if (cond->getName() != "false")
    return ::eval(gc, env, exprTrue);
  else
    return ::eval(gc, env, exprFalse);
}

Expr *LetExpr::eval(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

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

  std::map<std::string, const Expr*> newvars;
  for (auto it = vars.begin(); it != vars.end(); ++it) {
    if (!scope->getParent()->get(it->first))
      newvars.insert(*it);
  }

  scope->getVariables() = newvars;

  return ::eval(gc, *scope, result);
}

Expr *FunctionExpr::eval(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

  StackFrameObj<Expr> lambdaFn(env);
  StackFrameObj<Expr> noMatch(env, new BiOpExpr(gc, this->getTokenPos(), op_fn,
      new IdExpr(gc, this->getTokenPos(), "error"),
      new IdExpr(gc, this->getTokenPos(), "\"No Match\"")));

  for (auto it = fncases.rbegin(); it != fncases.rend(); ++it) {
    // Work on one function case
    const std::pair<std::vector<Expr*>, Expr*> &fncase = *it;

    StackFrameObj<Expr> exprCondition(env); // if condition
    StackFrameObj<Expr> exprFnBody(env, fncase.second); // function body

    size_t xid = 0; // argument id
    for (Expr *expr : fncase.first) {
      StackFrameObj<IdExpr> argumentId(env,
        new IdExpr(gc, expr->getTokenPos(), std::string("_x") + std::to_string(xid++)));

      // let statement
      if (expr->getExpressionType() == expr_id
          || (expr->getExpressionType() == expr_biop
            && dynamic_cast<const BiOpExpr*>(expr)->isAtomConstructor())) {
        exprFnBody = new LetExpr(gc, expr->getTokenPos(),
            std::vector<BiOpExpr*>{new BiOpExpr(gc,
                fncase.second->getTokenPos(),
                op_asg, expr, *argumentId)}, *exprFnBody);
      }

      // Check if equality check needed
      if (expr->getExpressionType() == expr_any
          || expr->getExpressionType() == expr_id)
        continue;

      // For checking equality we need expr, where ids are replaced by ANY
      StackFrameObj<Expr> noidexpr(env, expr->replace(gc,  "", nullptr));
      StackFrameObj<Expr> equalityCheck(env,
          new BiOpExpr(gc, noidexpr->getTokenPos(), op_eq, *noidexpr, *argumentId));
      if (!exprCondition)
        exprCondition = equalityCheck; 
      else
        exprCondition = new BiOpExpr(gc, op_land, *exprCondition, *equalityCheck);
    }

    // exprCondition might be nullptr
    if (!exprCondition)
      lambdaFn = *exprFnBody;
    else  {
      lambdaFn = new IfExpr(gc,
          TokenPos(fncase.first.at(0)->getTokenPos(),
                   fncase.first.at(fncase.first.size() - 1)->getTokenPos()),
          *exprCondition, *exprFnBody, 
          lambdaFn ? *lambdaFn : *noMatch);
    }
  }

  //  lambdaFn is not nullptr because at least one case exists
  // Now construct lambda Function (lambdaFn is only the body)

  StackFrameObj<Expr> result(env, *lambdaFn);
  for (size_t i = fncases.at(0).first.size(); i > 0; --i) {
    result = new LambdaExpr(gc, this->getTokenPos(),
      "_x" + std::to_string(i - 1), *result); // Not type-able identifier
  }

  return result->optimize(gc);
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

Expr *eval(GCMain &gc, Environment &env, Expr *pexpr) noexcept {

  StackFrameObj<Expr> expr(env, pexpr);
  StackFrameObj<Expr> oldExpr(env, pexpr);
  while (expr && (expr = expr->evalWithLookup(gc, env)) != oldExpr) {
    oldExpr = expr;

    if (gc.getCountNewObjects() < 200)
      continue;

    env.mark(gc);
    gc.collect();
  }

  return *expr;
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

bool UnOpExpr::equals(const Expr *expr) const noexcept {
  if (expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_unop) return false;

  auto unopexpr = dynamic_cast<const UnOpExpr*>(expr);

  return op == unopexpr->getOperator()
    && expr->equals(&unopexpr->getExpression());
}
