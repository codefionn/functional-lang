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
class IntExpr;
class IdExpr;
class LambdaExpr;
class AtomExpr;
class IfExpr;
class AnyExpr;
class LetExpr;

/*!\brief Types of expressions.
 * \see Expr, Expr::getExpressionType
 */
enum ExprType : int {
  expr_biop, //!< Binary operator
  expr_unop, //!< Unary left-operator
  expr_num, //!< Floating-point Number
  expr_int, //!< Integer number
  expr_id,  //!< Identifier
  expr_lambda, //!< Lambda function
  expr_atom, //!< Atom
  expr_if, //!< If-then-else
  expr_any, //!< Any '_'
  expr_let, //!< Let statement
  expr_fn, //!< Intern statement for named functions
};

/*!\brief Environment for accessing variables.
 */
class Environment : public GCObj {
  std::map<std::string, Expr*> variables;
  Environment *parent;
public:
  Lexer *lexer;
  std::vector<Expr*> ctx; //!< Context to store e.g. stack variables

  Environment(GCMain &gc, Lexer *lexer = nullptr, Environment *parent = nullptr)
    : GCObj(gc), lexer{lexer}, parent{parent}, variables() {}
  virtual ~Environment() {}

  /*!\return Returns name if in environment, nullptr if not.
   */
  bool contains(const std::string &name) const noexcept;

  /*!\return Returns associated expression (to name). nullptr if not found.
   */
  const Expr *get(const std::string &name) const noexcept;

  /*!\return Returns associated expression (to name), nullptr if not found.
   * But only looks in current scope/environment, not in parent.
   */
  const Expr *currentGet(const std::string &name) const noexcept;

  virtual void mark(GCMain &gc) noexcept override;

  std::map<std::string, Expr*> &getVariables() noexcept
    { return variables; }

  /*!\return Returns parent of environment/scope. May be nullptr.
   */
  Environment *getParent() const noexcept { return parent; }
};

std::vector<Expr*>::iterator find(std::vector<Expr*> &vec, Expr *expr) noexcept;

/*!\brief Class for adding Expr's to stack frame of Environment.
 *
 * Should only be created in stack not heap. The class adds an expression
 * to stack frame on initialization or assignment and removes the expression
 * from stack frame on deconstruction or if new expression assigned.
 */
template<typename T>
class StackFrameObj {
  Environment &env;
  T* expr;
public:
  StackFrameObj(Environment &env, T *expr = nullptr) noexcept
      : env{env}, expr{expr} {

    if (this->expr) env.ctx.push_back(dynamic_cast<Expr*>(expr));
  }
  
  ~StackFrameObj() {
    if (this->expr) {
      auto it = find(env.ctx, dynamic_cast<Expr*>(this->expr));
      if (it != env.ctx.end()) env.ctx.erase(it);
    }
  }
  
  /*\return Returns contained expression.
   */
  T* operator ->() const noexcept {
    return expr;
  }
  
  /*!\return Returns contained expression.
   */
  T* operator *() const noexcept {
    return expr;
  }
  
  /*!\return Returns **this == expr.
   * \param expr
   */
  bool operator ==(T *expr) const noexcept {
    return this->expr == expr;
  }

  /*!\return Returns **this == *obj.
   * \param
   */
  bool operator ==(StackFrameObj<T> &obj) const noexcept {
    return this->expr == *obj;
  }

  /*!\return Returns **this != expr.
   * \param expr
   */
  bool operator !=(T *expr) const noexcept {
    return this->expr != expr;
  }

  /*!\return Returns **this == *obj.
   * \param obj
   */
  bool operator !=(StackFrameObj<T> &obj) const noexcept {
    return this->expr != *obj;
  }
  
  /*!\return Assigns expression to expr. Removes old expression from stack
   * frame and adds expr to stack frame.
   * \param expr
   */
  StackFrameObj<T>& operator=(T *expr) noexcept {
    if (expr) env.ctx.push_back(dynamic_cast<Expr*>(expr));

    if (this->expr) {
      auto it = find(env.ctx, dynamic_cast<Expr*>(this->expr));
      if (it != env.ctx.end()) env.ctx.erase(it);
    }
  
    this->expr = expr; return *this;
  }
 
  /*!\brief Assigns **this = *obj. Removes old expression from stack frame
   * and adds *obj to stack frame.
   * \return Returns this.
   * \param obj
   */
  StackFrameObj<T>& operator=(StackFrameObj<T> &obj) noexcept {
    *this = *obj;
    return *this;
  }
 
  /*!\return Returns **this != nullptr.
   */
  operator bool() const {
    return expr != nullptr;
  }
};

/*!\brief Main expression handle (should only be used as parent class).
 * \see StackFrameObj
 *
 * To use Expr as local heap variable, use a StackFrameObj.
 */
class Expr : public GCObj {
  TokenPos pos;
  ExprType type;
protected:
  std::size_t depth;
  Expr *lastEval = nullptr;
public:
  Expr(GCMain &gc, ExprType type, const TokenPos &pos)
      : GCObj(gc), type{type}, pos(pos) {}

  virtual ~Expr() {}

  /*\return Returns a pre-calculated depth of the expresssion.
   */
  std::size_t getDepth() const noexcept { return depth; }

  /*!\return Returns true, if has last evaluation (not nullptr), otherwise
   * false.
   */
  bool hasLastEval() const noexcept { return lastEval != nullptr; }

  /*!\return Returns position of token in code.
   */
  const TokenPos &getTokenPos() const noexcept { return pos; }

  /*!\return Returns expression in functional-lang (the programming language to
   * parse).
   */
  virtual std::string toString() const noexcept { return std::string(); }

  /*!\return Returns the type of expression.
   * \see ExprType
   */
  ExprType getExpressionType() const noexcept { return type; }

  /*!\brief Evaluates expression one time.
   * \param gc
   * \param env Environment for accessing variables.
   * \return Returns itself if nothing evaluated, otherwise a new expression.
   */
  virtual Expr *eval(GCMain &gc, Environment &env) noexcept {
    return this;
  }

  /*!\return Returns Last evaluation if already computed (except assignments).
   * If no last evaluation, evaluate. To reset evaluation "lastEval = nullptr".
   * \param gc
   * \param env
   */
  Expr *evalWithLookup(GCMain &gc, Environment &env) noexcept;

  /*!\brief Replace all identifiers equal to name with expr.
   * \param gc
   * \param name Identifer name to replace. If name is empty, every
   * identifier will be replaced by AnyExpr(expr_any).
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
   * \param exact If true any must equal any and nothing else. Otherwise
   * any is equal to anyting else.
   */
  virtual bool equals(const Expr *expr, bool exact = false) const noexcept
    { return this == expr; }

  /*!\return Returns all identifiers used in expression.
   */
  virtual std::vector<std::string> getIdentifiers() const noexcept {
    return std::vector<std::string>();
  }

  virtual void mark(GCMain &gc) noexcept override;

  /*!\return Returns an optimized version of this expression. If nothing was
   * optimized, returns itself.
   */
  virtual Expr *optimize(GCMain &gc) noexcept;
};

/*!\brief Binary operator expression.
 */
class BiOpExpr : public Expr {
  Operator op;
  Expr *lhs, *rhs;
public:
  BiOpExpr(GCMain &gc, Operator op, Expr *lhs, Expr *rhs)
    : Expr(gc, expr_biop, TokenPos(lhs->getTokenPos(), rhs->getTokenPos())),
      op(op), lhs{lhs}, rhs{rhs} {
    depth = 1 + lhs->getDepth() + rhs->getDepth();
  }

  BiOpExpr(GCMain &gc, const TokenPos &pos, Operator op, Expr *lhs, Expr *rhs)
    : Expr(gc, expr_biop, pos),
      op(op), lhs{lhs}, rhs{rhs} {
    depth = 1 + lhs->getDepth() + rhs->getDepth();   
  }

  virtual ~BiOpExpr() {}

  //!\return Returns operator.
  Operator getOperator() const noexcept { return op; }

  //!\return Returns right-hand-side
  const Expr& getRHS() const noexcept { return *rhs; }

  //!\return Returns left-hand-side
  const Expr& getLHS() const noexcept { return *lhs; }

  virtual std::string toString() const noexcept override {
    if (op == op_fn)
      return lhs->toString() + " " + rhs->toString();

    return "(" + lhs->toString()
      + " " + std::to_string(op) + " " + rhs->toString() + ")";
  }

  //!\brief Mark self, rhs and lhs.
  virtual void mark(GCMain &gc) noexcept override {
    if (isMarked(gc))
      return;

    markSelf(gc);
    if (lastEval) lastEval->mark(gc);
    lhs->mark(gc);
    rhs->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  /*!\return Returns true if all expressions are binary operator functions with
   * identifiers, except the expression the most left has to be an atom. Like:
   * \<atom\> (\<Id\> | '_')+
   */
  bool isAtomConstructor() const noexcept;

  bool isFunctionConstructor() const noexcept;

  /*!\returns \<atom\> of atom constructor RHS don't have to be \<Id\> or '_'.
   * \see isAtomConstructor
   */
  const AtomExpr *getAtomConstructor() const noexcept;

  virtual std::vector<std::string> getIdentifiers() const noexcept override {
    std::vector<std::string> result = getLHS().getIdentifiers();
    for (std::string &id : getRHS().getIdentifiers())
      result.push_back(id);

    return result;
  }

  virtual Expr *optimize(GCMain &gc) noexcept override;

  /*!\return Returns optimized binary operator expressions.
   * \param exprs Unique expressions to share memory references with.
   */
  BiOpExpr *optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept;
};

class UnOpExpr : public Expr {
  Operator op;
  Expr *expr;
public:
  UnOpExpr(GCMain &gc, const TokenPos &pos, Operator op, Expr *expr)
    : Expr(gc, expr_unop, pos), op{op}, expr{expr} {
    
    depth = 1 + expr->getDepth();
  }

  Operator getOperator() const noexcept { return op; }
  const Expr &getExpression() const noexcept { return *expr; }

  virtual void mark(GCMain &gc) noexcept override;

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual Expr *optimize(GCMain &gc) noexcept override;
  virtual UnOpExpr *optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept;
};

const Expr *assignExpressions(GCMain &gc, Environment &env,
    const Expr *thisExpr,
    const Expr *lhs, const Expr *rhs) noexcept;

/*!\brief Floating point number expression.
 */
class NumExpr : public Expr {
  double num;
public:
  NumExpr(GCMain &gc, const TokenPos &pos, double num)
      : Expr(gc, expr_num, pos), num{num} {
    depth = 1;
  }

  virtual ~NumExpr() {}

  double getNumber() const noexcept { return num; }

  virtual std::string toString() const noexcept override {
    return std::to_string(num);
  }

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;
};

/*!\brief Integer number expression.
 */
class IntExpr : public Expr {
  std::int64_t num;
public:
  IntExpr(GCMain &gc, const TokenPos &pos, std::int64_t num)
      : Expr(gc, expr_int, pos), num{num} {
    depth = 1;
  }

  virtual ~IntExpr() {}

  std::int64_t getNumber() const noexcept { return num; }

  virtual std::string toString() const noexcept override {
    return std::to_string(num);
  }

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;
};


/*!\brief Identifier expression.
 */
class IdExpr : public Expr {
  std::string id;
public:
  IdExpr(GCMain &gc, const TokenPos &pos, const std::string &id)
      : Expr(gc, expr_id, pos), id(id) {
    depth = 1;
  }

  virtual ~IdExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return id;
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual std::vector<std::string> getIdentifiers() const noexcept override {
    std::vector<std::string> result;
    result.push_back(id);
    return result;
  }
};

/*!\brief Lambda function expression.
 */
class LambdaExpr : public Expr {
  std::string name;
  Expr* expr;
public:
  LambdaExpr(GCMain &gc, const TokenPos &pos, const std::string &name,
             Expr* expr)
      : Expr(gc, expr_lambda, TokenPos(pos, expr->getTokenPos())),
        name(name), expr(std::move(expr)) {
    depth = 1 + expr->getDepth();  
  }

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
    if (lastEval) lastEval->mark(gc);
    expr->mark(gc);
  }

  /*\return Returns expression where getName() is replaced by expr.
   * \param gc
   * \param expr
   */
  Expr *replace(GCMain &gc, Expr *expr) const noexcept;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual std::vector<std::string> getIdentifiers() const noexcept override {
    std::vector<std::string> result = expr->getIdentifiers();
    result.push_back(name);
    return result;
  }

  virtual Expr *optimize(GCMain &gc) noexcept override;
  virtual LambdaExpr *optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept;
};

/*!\brief Atom expression.
 */
class AtomExpr : public Expr {
  std::string id;
public:
  AtomExpr(GCMain &gc, const TokenPos &pos, const std::string &id)
      : Expr(gc, expr_atom, pos), id(id) {
    depth = 1;
  }

  virtual ~AtomExpr() {}

  const std::string &getName() const noexcept { return id; }

  virtual std::string toString() const noexcept override {
    return "." + id;
  }

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;
};

/*!\brief If-then-else expression.
 *
 *     if \<expr_condition\> then \<expr_true\> else \<expr_false\>
 */
class IfExpr : public Expr {
  Expr *condition, *exprTrue, *exprFalse;
public:
  IfExpr(GCMain &gc, const TokenPos &pos, Expr *condition, Expr *exprTrue, Expr *exprFalse)
    : Expr(gc, expr_if, TokenPos(pos, exprFalse->getTokenPos())),
      condition{condition}, exprTrue{exprTrue}, exprFalse{exprFalse} {
    depth = 1 + condition->getDepth()
      + exprTrue->getDepth() + exprFalse->getDepth();
  }

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
    if (lastEval) lastEval->mark(gc);
    condition->mark(gc);
    exprTrue->mark(gc);
    exprFalse->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept override;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual std::vector<std::string> getIdentifiers() const noexcept override {
    std::vector<std::string> result = condition->getIdentifiers();
    for (std::string &id : exprTrue->getIdentifiers())
      result.push_back(id);
    for (std::string &id : exprFalse->getIdentifiers())
      result.push_back(id);

    return result;
  }

  virtual Expr *optimize(GCMain &gc) noexcept override;
  virtual Expr *optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept;
};

/*!\brief any expression
 *
 *     '_'
 */
class AnyExpr : public Expr {
public:
  AnyExpr(GCMain &gc, const TokenPos &pos)
      : Expr(gc, expr_any, pos) {
    depth = 1;
  }
  virtual ~AnyExpr() {}

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual std::string toString() const noexcept override {
    return std::string("_");
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
  LetExpr(GCMain &gc, const TokenPos &pos, const std::vector<BiOpExpr*> &assignments, Expr *body)
    : Expr(gc, expr_let, TokenPos(pos, body->getTokenPos())),
      assignments(assignments), body{body} {
    depth = 1 + body->getDepth();
    for (BiOpExpr *expr : assignments) depth += expr->getDepth();
  }

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
    if(lastEval) lastEval->mark(gc);
    for (BiOpExpr *expr : assignments)
      expr->mark(gc);

    body->mark(gc);
  }

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept;
  virtual Expr *replace(GCMain &gc, const std::string &name, Expr *expr) const noexcept override;

  virtual std::vector<std::string> getIdentifiers() const noexcept override {
    std::vector<std::string> result = body->getIdentifiers();
    for (BiOpExpr *expr : assignments)
      for (std::string &id : expr->getIdentifiers())
        result.push_back(id);

    return result;
  }

  virtual bool equals(const Expr *expr, bool exact = false) const noexcept override;

  virtual std::string toString() const noexcept override {
    std::string result = "let ";
    auto &asgs = const_cast<std::vector<BiOpExpr*>&>(assignments);
    for (auto it = asgs.begin(); it != assignments.end(); ++it) {
      if (it != assignments.begin())
        result += "; ";

      result += (*it)->toString();
    }

    return result + " in " + body->toString();
  }

  virtual Expr *optimize(GCMain &gc) noexcept override;
  virtual Expr *optimize(GCMain &gc, std::vector<Expr*> &exprs) noexcept; 
};

/*!\brief Represents a named function.
 */
class FunctionExpr : public Expr {
  std::string name;
  std::vector<std::pair<std::vector<Expr*>, Expr*>>  fncases;
public:
  FunctionExpr(GCMain &gc, const TokenPos &pos,
      const std::string &name,
      std::pair<std::vector<Expr*>, Expr*> fncase) noexcept;

  virtual ~FunctionExpr() {}

  void calcDepth() noexcept;

  /*!\brief Adds function evaluation case to function.
   * \param fncase 
   * \return Returns
   *
   *     fncase.first.size() == getFunctionCases().front().first.size()
   */
  bool addCase(std::pair<std::vector<Expr*>, Expr*> fncase) noexcept;

  /*!\return Returns name of function.
   */
  const std::string &getName() const noexcept { return name; }

  std::vector<std::pair<std::vector<Expr*>, Expr*>> &getFunctionCases()
    noexcept { return fncases; }
  const std::vector<std::pair<std::vector<Expr*>, Expr*>> &getFunctionCases()
    const noexcept { return fncases; }

  virtual void mark(GCMain &gc) noexcept override;

  virtual Expr *eval(GCMain &gc, Environment &env) noexcept;

  virtual std::string toString() const noexcept override {
    return getName();
  }
};

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg,
    const TokenPos &pos);

/*!\brief Parses primary expression(s). Also parses lambda function
 * substitutions (so also expressions, not only one primary one).
 * \param gc
 * \param lexer
 * \param env Environment to access variables.
 * \param topLevel Is the primary a top level primary expression (typically
 * true).
 * \return Returns nullptr on error, otherwise primary expression(s).
 */
Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel = true);

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

/*!\brief Executes eval function for given expressions as long as differenct
 * expr returned. Breadth first execution.
 * \param gc
 * \param  env
 * \param lhs
 * \param rhs
 */
void breadthEval(GCMain &gc, Environment &env,
    StackFrameObj<Expr> &lhs, StackFrameObj<Expr> &rhs) noexcept;

#endif /* FUNC_SYNTAX_HPP */
