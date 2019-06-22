#include "func/syntax.hpp"

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
    if (expr->equals(optexpr, true))
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
  case expr_lambda:
    newexpr = dynamic_cast<LambdaExpr*>(expr)->optimize(gc, exprs);
    break;
  case expr_if:
    newexpr = dynamic_cast<IfExpr*>(expr)->optimize(gc, exprs);
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
    if (!asg->getLHS().equals(&asg->getRHS(), true))
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

Expr *LambdaExpr::optimize(GCMain &gc) noexcept {
  std::vector<Expr*> exprs;
  return optimize(gc, exprs);
}

LambdaExpr *LambdaExpr::optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept {
  Expr *newexpr = exprOptimizeList(gc, exprs, expr);
  if (newexpr == expr) return this; // no changes

  return new LambdaExpr(gc, getTokenPos(), name, newexpr);
}

Expr *IfExpr::optimize(GCMain &gc) noexcept {
  std::vector<Expr*> exprs;
  return optimize(gc, exprs);
}

Expr *IfExpr::optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept {
  if (condition->getExpressionType() == expr_atom) {
    if (dynamic_cast<AtomExpr*>(condition)->getName() != "false")
      return exprOptimizeList(gc, exprs, exprTrue);
    else // false
      return exprOptimizeList(gc, exprs, exprFalse);
  }

  Expr *newcondition = exprOptimizeList(gc, exprs, condition);
  Expr *newTrue = exprOptimizeList(gc, exprs, exprTrue);
  Expr *newFalse = exprOptimizeList(gc, exprs, exprFalse);
  if (newcondition == condition
      && newTrue == exprTrue && newFalse == exprFalse)
    return this; // no changes

  return new IfExpr(gc, getTokenPos(), newcondition, newTrue, newFalse);
}


