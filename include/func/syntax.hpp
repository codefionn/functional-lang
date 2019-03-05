#ifndef FUNC_SYNTAX_HPP
#define FUNC_SYNTAX_HPP

/*!\file func/syntax.hpp
 * \brief A syntax parser
 */

#include "func/global.hpp"
#include "func/lexer.hpp"

enum ExprType : int {
  expr_biop,
  expr_num,
  expr_id,
  expr_lambda,
};

class Expr {
  ExprType type;
public:
  Expr(ExprType type) : type{type} {}
  virtual ~Expr() {}

  virtual std::string toString() const noexcept { return std::string(); }

  ExprType getExpressionType() const noexcept { return type; }
};

class BiOpExpr : public Expr {
  char op;
  std::unique_ptr<Expr> lhs, rhs;
public:
  BiOpExpr(char op, std::unique_ptr<Expr> lhs,
                    std::unique_ptr<Expr> rhs)
    : Expr(expr_biop),
      op{op}, lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  virtual ~BiOpExpr() {}

  //!\return Returns operator.
  char getOperator() const noexcept { return op; }

  //!\return Returns right-hand-side
  const Expr& getRHS() const noexcept { return *rhs; }

  //!\return Returns left-hand-side
  const Expr& getLHS() const noexcept { return *lhs; }

  virtual std::string toString() const noexcept override {
    return op + std::string("{ ")
      + lhs->toString()
      + ", " + rhs->toString() + " }";
  }
};

class NumExpr : public Expr {
  double num;
public:
  NumExpr(double num)
    : Expr(expr_num), num{num} {}
  virtual ~NumExpr() {}

  double getNumber() const noexcept { return num; }

  virtual std::string toString() const noexcept override {
    return std::to_string(num);
  }
};

class IdExpr : public Expr {
  std::string id;
public:
  IdExpr(const std::string &id) : Expr(expr_id), id(id) {}
  virtual ~IdExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return id;
  }
};

class LambdaExpr : public Expr {
  std::string name;
  std::unique_ptr<Expr> expr;
public:
  LambdaExpr(const std::string &name,
             std::unique_ptr<Expr> expr)
    : Expr(expr_lambda), name(name), expr(std::move(expr)) {}
  virtual ~LambdaExpr() {}

  const std::string &getName() const noexcept { return name; }
  const Expr& getExpression() const noexcept { return *expr; }

  virtual std::string toString() const noexcept override {
    return "\\ " + name + " = "
      + expr->toString();
  }
};

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg);
Expr *parsePrimary(Lexer &lexer);
Expr *parse(Lexer &lexer);

/*!\brief Parse right-hand-side
 * \param lexer
 * \param lhs Already parsed Left-hand-side
 * \param prec current minimum precedence
 */
Expr *parseRHS(Lexer &lexer, Expr *lhs, int prec);

#endif /* FUNC_SYNTAX_HPP */
