#ifndef FUNC_SYNTAX_HPP
#define FUNC_SYNTAX_HPP

/*!\file func/syntax.hpp
 * \brief A syntax parser/Syntax analysis.
 */

#include "func/global.hpp"
#include "func/lexer.hpp"
#include "func/gc.hpp"

class Expr;
class BiOpExpr;
class NumExpr;
class IdExpr;
class LambdaExpr;
class AtomExpr;
class IfExpr;
class AnyExpr;

/*!\brief Types of expressions.
 * \see Expr, Expr::getExpressionType
 */
enum ExprType : int {
  expr_biop, //!< Binary operator
  expr_num, //!< Floating-point Number
  expr_id,  //!< Identifier
  expr_lambda, //!< Lambda function
  expr_atom, //!< Atom
  expr_if, //!< If-then-else
  expr_any, //!< Any '_'
  expr_let, //!< Let statement
};

/*!\brief Environment for accessing variables.
 */
class Environment : public GCObj {
  std::map<std::string, Expr*> variables;
  Environment *parent;
public:
  Environment(GCMain &gc, Environment *parent = nullptr)
    : GCObj(gc), parent{parent}, variables() {}
  virtual ~Environment() {}

  /*!\return Returns name if in environment, nullptr if not.
   */
  bool contains(const std::string &name) const noexcept;

  /*!\return Returns associated expression (to name). nullptr if not found.
   */
  Expr *get(const std::string &name) const noexcept;

  virtual void mark(GCMain &gc) noexcept override;

  std::map<std::string, Expr*> &getVariables() noexcept
    { return variables; }

  /*!\return Returns parent of environment/scope. May be nullptr.
   */
  Environment *getParent() const noexcept { return parent; }
};

/*!\brief Main expression handle (should only be used as parent class).
 */
class Expr : public GCObj {
  ExprType type;
public:
  Expr(GCMain &gc, ExprType type) : GCObj(gc), type{type} {}
  virtual ~Expr() {}

  /*!\return Returns expression in functional-lang (the programming language to
   * parse).
   */
  virtual std::string toString() const noexcept { return std::string(); }

  ExprType getExpressionType() const noexcept { return type; }

  /*!\brief Evaluates expression one time.
   * \param gc
   * \param env Environment for accessing variables.
   * \return Returns itself if nothing evaluated, otherwise a new expression.
   */
  virtual Expr *eval(GCMain &gc, Environment &env) noexcept {
    return this;
  }

  /*!\brief Replace all identifiers equal to name with expr.
   * \param gc
   * \param name Identifer name to replace.
   * \param expr The expression to replaces identifiers.
   * \return Returns new expression, where matching identifiers are replaced.
   * Returns itself if not possible (expression can't contain any identifiers)
   * or if expression is a lambda function, where the \<id\> is equal to name.
   */
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept {
    return const_cast<Expr*>(this);
  }

  /*!\brief Checks if this expression and expr have the same structure, Every
   * expression has the same structure as '_' (it's called ANY after all).
   * \param expr The expression to compare to this.
   */
  virtual bool equals(const Expr *expr) const noexcept
    { return this == expr; }
};


/*!\brief Binary operator expression.
 */
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

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != expr_num) return false;

    const BiOpExpr *biOpExpr = dynamic_cast<const BiOpExpr*>(expr);
    if (biOpExpr->getOperator() != this->getOperator()) return false;
    if (!biOpExpr->getRHS().equals(&this->getRHS())) return false;
    if (!biOpExpr->getLHS().equals(&this->getLHS())) return false;

    return true;
  }
};

/*!\brief Floating point number expression.
 */
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

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != getExpressionType()) return false;

    return dynamic_cast<const NumExpr*>(expr)->getNumber() == getNumber();
  }
};

/*!\brief Identifier expression.
 */
class IdExpr : public Expr {
  std::string id;
public:
  IdExpr(GCMain &gc, const std::string &id) : Expr(gc, expr_id), id(id) {}
  virtual ~IdExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return id;
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != getExpressionType()) return false;

    return dynamic_cast<const IdExpr*>(expr)->getName() == getName();
  }
};

/*!\brief Lambda function expression.
 */
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

  /*\return Returns expression where getName() is replaced by expr.
   * \param gc
   * \param expr
   */
  Expr *replace(GCMain &gc, Expr *expr) const noexcept;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != getExpressionType()) return false;

    const LambdaExpr *lambdaExpr = dynamic_cast<const LambdaExpr*>(expr);
    return lambdaExpr->getName() == getName()
      && lambdaExpr->getExpression().equals(&getExpression());
  }
};

/*!\brief Atom expression.
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

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != getExpressionType()) return false;

    return dynamic_cast<const AtomExpr*>(expr)->getName() == getName();
  }
};

/*!\brief If-then-else expression.
 *
 *     if \<expr_condition\> then \<expr_true\> else \<expr_false\>
 */
class IfExpr : public Expr {
  Expr *condition, *exprTrue, *exprFalse;
public:
  IfExpr(GCMain &gc, Expr *condition, Expr *exprTrue, Expr *exprFalse)
    : Expr(gc, expr_if),
      condition{condition}, exprTrue{exprTrue}, exprFalse{exprFalse} {}
  virtual ~IfExpr() {}

  /*!\brief Condition of the if-then-else expression.
   * \return Returns abitrary expression (which SHOULD evaluate to .false or
   * .true).
   */
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

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr) const noexcept override {
    if (expr->getExpressionType() == expr_any) return true;
    if (expr->getExpressionType() != getExpressionType()) return false;

    const IfExpr *ifExpr = dynamic_cast<const IfExpr*>(expr);
    if (!ifExpr->getCondition().equals(&getCondition())) return false;
    if (!ifExpr->getTrue().equals(&getTrue())) return false;
    if (!ifExpr->getFalse().equals(&getFalse())) return false;

    return true;
  }
};

/*!\brief any expression
 *
 *     '_'
 */
class AnyExpr : public Expr {
public:
  AnyExpr(GCMain &gc) : Expr(gc, expr_any) {}
  virtual ~AnyExpr() {}

  virtual bool equals(const Expr *expr) const noexcept override {
    return true;
  }
};

/*!\brief Let expression
 */
class LetExpr : public Expr {
  std::vector<BiOpExpr*> assignments;
  Expr *body;
public:
  /*!\brief Initialize let expression.
   * \param gc
   * \param assignments All assignments of the let expression.
   * \param body Body of let expression
   */
  LetExpr(GCMain &gc, const std::vector<BiOpExpr*> &assignments, Expr *body)
    : Expr(gc, expr_let), assignments(assignments), body{body} {}
  virtual ~LetExpr() {}

  /*!\return Returns assignments done between let ... in (separated by ';').
   */
  const std::vector<BiOpExpr*> &getAssignments() const noexcept
    { return assignments; }

  /*!\return Returns body of let expression.
   */
  const Expr &getBody() const noexcept { return *body; }

  virtual void mark(GCMain &gc) noexcept {
    if (isMarked(gc))
      return;

    markSelf(gc);
    for (BiOpExpr *expr : assignments)
      expr->mark(gc);

    body->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept;
};

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg);

/*!\brief Parses primary expression(s). Also parses lambda function
 * substitutions (so also expressions, not only one primary one).
 * \param gc
 * \param lexer
 * \param env Environment to access variables.
 * \return Returns nullptr on error, otherwise primary expression(s).
 */
Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env);

/*!\return Returns nullptr on error, otherwise parsed tokens from
 * lexer.nextToken().
 *
 * \param gc
 * \param lexer
 * \param env Environment to access variables.
 * \param topLevel If top level, nullptr is returned if eol occured.
 */
Expr *parse(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel = true);

/*!\brief Parse right-hand-side
 * \param gc
 * \param lexer
 * \param lhs Already parsed Left-hand-side
 * \param prec current minimum precedence
 * \return Returns nullptr on error, otherwise parsed RHS.
 */
Expr *parseRHS(GCMain &gc, Lexer &lexer, Environment &env, Expr *lhs, int prec);

/*!\brief Executes eval function for given expr as long as different expr
 * is returned.
 * \param gc
 * \param env Environment to use
 * \param expr Expression to evaluate
 */
Expr *eval(GCMain &gc, Environment &env, Expr *expr) noexcept;

#endif /* FUNC_SYNTAX_HPP */
