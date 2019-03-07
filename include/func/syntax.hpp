#ifndef FUNC_SYNTAX_HPP
#define FUNC_SYNTAX_HPP

/*!\file func/syntax.hpp
 * \brief A syntax parser
 */

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/gc.hpp"

class Expr;
class BiOpExpr;
class NumExpr;
class IdExpr;
class LambdaExpr;

enum ExprType : int {
  expr_biop,
  expr_num,
  expr_id,
  expr_lambda,
  expr_atom,
  expr_if,
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

  virtual Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept {
    return this;
  }

  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept {
    return const_cast<Expr*>(this);
  }
};

typedef std::map<std::string, Expr*> Environment;

class BiOpExpr : public Expr {
  Operator op;
  Expr *lhs, *rhs;
public:
  BiOpExpr(GCMain &gc, Operator op, Expr *lhs,
                    Expr *rhs)
    : Expr(gc, expr_biop),
      op(op), lhs{lhs}, rhs{rhs} {}
  virtual ~BiOpExpr() {}

  //!\return Returns operator.
  Operator getOperator() const noexcept { return op; }

  //!\return Returns right-hand-side
  const Expr& getRHS() const noexcept { return *rhs; }

  //!\return Returns left-hand-side
  const Expr& getLHS() const noexcept { return *lhs; }

  virtual std::string toString() const noexcept override {
    return "(" + lhs->toString()
      + " " + std::to_string(op) + " " + rhs->toString() + ")";
  }

  //!\brief Mark self, rhs and lhs.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    lhs->mark(gc);
    rhs->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;
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

  virtual Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;
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
    return "\\" + name + " = "
      + expr->toString();
  }

  //!\brief Mark self and expr.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    expr->mark(gc);
  }

  /*\return Returns expression where getName is replcase by expr.
   * \param gc
   * \param expr
   */
  Expr *replace(GCMain &gc, Expr *expr) const noexcept;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;
};

/*!\brief '.' \<id\>
 */
class AtomExpr : public Expr {
  std::string id;
public:
  AtomExpr(GCMain &gc, const std::string &id) : Expr(gc, expr_atom), id(id) {}
  virtual ~AtomExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return "." + id;
  }
};

/*!\brief if \<expr_condition\> then \<expr_true\> else \<expr_false\>
 */
class IfExpr : public Expr {
  Expr *condition, *exprTrue, *exprFalse;
public:
  IfExpr(GCMain &gc, Expr *condition, Expr *exprTrue, Expr *exprFalse)
    : Expr(gc, expr_if),
      condition{condition}, exprTrue{exprTrue}, exprFalse{exprFalse} {}
  virtual ~IfExpr() {}

  const Expr& getCondition() const noexcept { return *condition; }
  //! Evaluated if condition evaluated not to the atom .false
  //! (but to an atom).
  const Expr& getTrue() const noexcept { return *exprTrue; }
  //! Evaluated if condition evaluated to the atom .true
  const Expr& getFalse() const noexcept { return *exprFalse; }

  virtual std::string toString() const noexcept override {
    return "if " + condition->toString() + " then "
      + exprTrue->toString() + " else " + exprFalse->toString();
  }

  //!\brief Mark self, condition, exprTrue and exprFalse.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    condition->mark(gc);
    exprTrue->mark(gc);
    exprFalse->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;
};

Expr *reportSyntaxError(GCMain &gc, Lexer &lexer, const std::string &msg);
Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env);
Expr *parse(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel = true);

/*!\brief Parse right-hand-side
 * \param lexer
 * \param lhs Already parsed Left-hand-side
 * \param prec current minimum precedence
 */
Expr *parseRHS(GCMain &gc, Lexer &lexer, Environment &env, Expr *lhs, int prec);

/*!\brief Executes eval function for given expr as long as different expr
 * is returned.
 * \param gc
 * \param env Environment to use
 * \param expr Expression to evaluate
 */
Expr *eval(GCMain &gc, std::map<std::string, Expr*> &env, Expr *expr) noexcept;

#endif /* FUNC_SYNTAX_HPP */
