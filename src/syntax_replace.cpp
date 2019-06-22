#include "func/syntax.hpp"

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

