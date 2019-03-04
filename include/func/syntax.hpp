#ifndef FUNC_SYNTAX_HPP
#define FUNC_SYNTAX_HPP

#include "func/global.hpp"
#include "func/lexer.hpp"

class Expr {
public:
  virtual ~Expr() {}
  virtual std::shared_ptr<Expr> eval() {}
};

class BiOpExpr : public Expr {
  char op;
  std::weak_ptr<Expr> lhs, rhs;
public:
  BiOpExpr(char op, std::weak_ptr<Expr> lhs,
                    std::weak_ptr<Expr> rhs)
    : op{op}, lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  virtual ~BiOpExpr() {}

  //!\return Returns operator.
  char getOperator() noexcept const { return op; }

  //!\return Returns right-hand-side
  const std::weak_ptr<Expr> getRHS() noexcept const { return rhs; }

  //!\return Returns left-hand-side
  const std::weak_ptr<Expr> getLHS() noexcept const { return lhs; }
};

class NumExpr : public Expr {
  double num;
public:
  NumExpr(double num)
    : num{num} {}
  virtual ~NumExpr() {}

  double getNumber() noexcept const { return num; }
};

class IdExpr : public Expr {
  std::string id;
public:
  IdExpr(const std::string &id) : id(id) {}
  virtual IdExpr() {}

  const std::string &getName() noexcept const { return id; }
};

class LambdaExpr : public Expr {
  std::weak_ptr<IdExpr> id;
  std::weak_ptr<Expr> expr;
public:
  LambdaExpr(std::weak_ptr<IdExpr> id,
             std::weak_ptr<Expr> expr)
    : id(id), expr(expr) {}
  LambdaExpr(const LambdaExpr &expr)
    : id(expr.getIdentifier()), expr(expr.getExpression()) {}
  virtual ~LambdaExpr() {}

  const std::weak_ptr<IdExpr> getIdentifier() noexcept const { return id; }
  const std::weak_ptr<Expr> getExpression() noexcept const { return expr; }

  virtual std::shared_ptr<Expr> eval() noexcept const override
    { return std::shared_ptr<Expr>(LambdaExpr(*this)); }
};

#endif /* FUNC_SYNTAX_HPP */
