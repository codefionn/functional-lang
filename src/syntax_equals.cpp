#include "func/syntax.hpp"

// equals

bool BiOpExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_biop) return false;

  const BiOpExpr *biOpExpr = dynamic_cast<const BiOpExpr*>(expr);
  if (biOpExpr->getOperator() != this->getOperator()) return false;
  if (!biOpExpr->getRHS().equals(&this->getRHS(), exact)) return false;
  return biOpExpr->getLHS().equals(&this->getLHS(), exact);
}

bool IdExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != getExpressionType()) return false;

  return dynamic_cast<const IdExpr*>(expr)->getName() == getName();
}

bool LambdaExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_lambda) return false;

  const LambdaExpr *lambdaExpr = dynamic_cast<const LambdaExpr*>(expr);
  return lambdaExpr->getName() == getName()
    && lambdaExpr->getExpression().equals(&getExpression(), exact);
}

bool AtomExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_atom) return false;

  return dynamic_cast<const AtomExpr*>(expr)->getName() == getName();
}

bool AnyExpr::equals(const Expr *expr, bool exact) const noexcept {
  return !exact || expr->getExpressionType() == expr_any;
}

bool LetExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_let) return false;

  const LetExpr *letexpr = dynamic_cast<const LetExpr*>(expr);
  if (assignments.size() != letexpr->getAssignments().size())
    return false;

  auto thisIt = assignments.begin();
  auto exprIt = letexpr->getAssignments().begin();

  while (thisIt != assignments.end()
      && exprIt != letexpr->getAssignments().end()) {
    if (!(*thisIt)->equals(*exprIt, exact))
      return false;

    ++thisIt;
    ++exprIt;
  }

  return body->equals(&(letexpr->getBody()), exact);
}

bool IfExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_if) return false;

  const IfExpr *ifExpr = dynamic_cast<const IfExpr*>(expr);
  if (!ifExpr->getCondition().equals(condition, exact)) return false;
  if (!ifExpr->getTrue().equals(exprTrue, exact)) return false;
  if (!ifExpr->getFalse().equals(exprFalse, exact)) return false;

  return true;
}

bool NumExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (!exact && expr->getExpressionType() == expr_any) return true;

  if (expr->getExpressionType() == expr_int)
    return dynamic_cast<const IntExpr*>(expr)->getNumber() == round(getNumber());

  if (expr->getExpressionType() == expr_num)
    return dynamic_cast<const NumExpr*>(expr)->getNumber() == getNumber();

  return false;
}

bool IntExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (!exact && expr->getExpressionType() == expr_any) return true;

  if (expr->getExpressionType() == expr_num)
    return round(dynamic_cast<const NumExpr*>(expr)->getNumber()) == getNumber();

  if (expr->getExpressionType() == expr_int)
    return dynamic_cast<const IntExpr*>(expr)->getNumber() == getNumber();

  return false;
}

bool UnOpExpr::equals(const Expr *expr, bool exact) const noexcept {
  if (this == expr) return true;
  if (exact && getDepth() != expr->getDepth()) return false;

  if (!exact && expr->getExpressionType() == expr_any) return true;
  if (expr->getExpressionType() != expr_unop) return false;

  auto unopexpr = dynamic_cast<const UnOpExpr*>(expr);

  return op == unopexpr->getOperator()
    && unopexpr->getExpression().equals(expr, exact);
}
