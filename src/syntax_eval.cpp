#include "func/syntax.hpp"

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
      env.getVariables().insert(
          std::pair<std::string, Expr*>(id, const_cast<Expr*>(rhs)));
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
              StackFrameObj<Expr> newlhs(env), newrhs(env);
              newlhs = ::eval(gc, env, lhs);
              if (!newlhs) return nullptr;
              newrhs = ::eval(gc, env, rhs);
              if (!newrhs) return nullptr;
             
              if (op == op_eq)
                return new AtomExpr(gc, mergedPos,
                    boolToAtom(newlhs->equals(*newrhs, false)));

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

  std::map<std::string, Expr*> newvars;
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


