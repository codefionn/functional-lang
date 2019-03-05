#ifndef FUNC_SYNTAX_HPP
#define FUNC_SYNTAX_HPP

/*!\file func/syntax.hpp
 * \brief A syntax parser
 */

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/gc.hpp"

enum ExprType : int {
  expr_biop,
  expr_num,
  expr_id,
  expr_lambda,
};

/*!\brief Main expression handle (should only be used as parent class).
 */
class Expr : public GCObj {
  ExprType type;
public:
  Expr(GCMain &gc, ExprType type) : GCObj(gc), type{type} {}
  virtual ~Expr() {}

  virtual std::string toString() const noexcept { return std::string(); }

  ExprType getExpressionType() const noexcept { return type; }
};

class BiOpExpr : public Expr {
  char op;
  Expr *lhs, *rhs;
public:
  BiOpExpr(GCMain &gc, char op, Expr *lhs,
                    Expr *rhs)
    : Expr(gc, expr_biop),
      op{op}, lhs{lhs}, rhs{rhs} {}
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

  //!\brief Mark self, rhs and lhs.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    lhs->mark(gc);
    rhs->mark(gc);
  }
};

class NumExpr : public Expr {
  double num;
public:
  NumExpr(GCMain &gc, double num)
    : Expr(gc, expr_num), num{num} {}
  virtual ~NumExpr() {}

  double getNumber() const noexcept { return num; }

  virtual std::string toString() const noexcept override {
    return std::to_string(num);
  }
};

class IdExpr : public Expr {
  std::string id;
public:
  IdExpr(GCMain &gc, const std::string &id) : Expr(gc, expr_id), id(id) {}
  virtual ~IdExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return id;
  }
};

class LambdaExpr : public Expr {
  std::string name;
  Expr* expr;
public:
  LambdaExpr(GCMain &gc, const std::string &name,
             Expr* expr)
    : Expr(gc, expr_lambda), name(name), expr(std::move(expr)) {}
  virtual ~LambdaExpr() {}

  const std::string &getName() const noexcept { return name; }
  const Expr& getExpression() const noexcept { return *expr; }

  virtual std::string toString() const noexcept override {
    return "\\ " + name + " = "
      + expr->toString();
  }

  //!\brief Mark self and expr.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    expr->mark(gc);
  }
};

Expr *reportSyntaxError(GCMain &gc, Lexer &lexer, const std::string &msg);
Expr *parsePrimary(GCMain &gc, Lexer &lexer);
Expr *parse(GCMain &gc, Lexer &lexer);

/*!\brief Parse right-hand-side
 * \param lexer
 * \param lhs Already parsed Left-hand-side
 * \param prec current minimum precedence
 */
Expr *parseRHS(GCMain &gc, Lexer &lexer, Expr *lhs, int prec);

#endif /* FUNC_SYNTAX_HPP */
