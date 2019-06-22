#include "func/syntax.hpp"

std::vector<Expr*>::iterator find(std::vector<Expr*> &vec, Expr *expr) noexcept {
  for (auto it = vec.begin(); it != vec.end(); ++it)
    if (*it == expr) return it;

  return vec.end();
}

// Expr

Expr *Expr::evalWithLookup(GCMain &gc, Environment &env) noexcept {
  StackFrameObj<Expr> thisObj(env, this);

  if (lastEval  && (getExpressionType() != expr_biop
          || dynamic_cast<const BiOpExpr*>(this)->getOperator() != op_asg))
    return lastEval;

  return lastEval = eval(gc, env);
}

// FunctionExpr

FunctionExpr::FunctionExpr(GCMain &gc, const TokenPos &pos,
      const std::string &name,
      std::pair<std::vector<Expr*>, Expr*> fncase) noexcept
    : Expr(gc, expr_fn, pos), name(name), fncases{fncase} {

  calcDepth();
}

void FunctionExpr::calcDepth() noexcept {
  depth = 1;
  for (std::pair<std::vector<Expr*>, Expr*> fncase : fncases) {
    depth += fncase.second->getDepth();
    for (Expr *expr : fncase.first)
      depth += expr->getDepth();
  }
}

bool FunctionExpr::addCase(std::pair<std::vector<Expr*>, Expr*> fncase) noexcept {
  if (fncase.first.size() != fncases.at(0).first.size())
    return false;

  // Reset evaluation
  lastEval = nullptr;

  fncases.push_back(std::move(fncase));

  // Recalculate expression depth
  calcDepth();

  return true;
}

// Environment

bool Environment::contains(const std::string &name) const noexcept {
  if (variables.count(name) > 0)
    return true;

  return parent ? parent->contains(name) : false;
}

const Expr *Environment::get(const std::string &name) const noexcept {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }

  return parent ? parent->get(name) : nullptr;
}

const Expr *Environment::currentGet(const std::string &name) const noexcept {
  auto it = variables.find(name);
  if (it != variables.end()) {
    return it->second;
  }

  return nullptr;
}

void Environment::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;
  
  markSelf(gc);
  for (std::pair<std::string, const Expr*> var : variables)
    const_cast<Expr*>(var.second)->mark(gc);

  for (Expr *expr : ctx)
    expr->mark(gc);

  if (parent) parent->mark(gc);
}

// mark

void Expr::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);
}

void UnOpExpr::mark(GCMain &gc) noexcept {
  if (isMarked(gc)) return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);

  expr->mark(gc);
}

void FunctionExpr::mark(GCMain &gc) noexcept {
  if (isMarked(gc))
    return;

  markSelf(gc);
  if (lastEval) lastEval->mark(gc);

  for (const std::pair<std::vector<Expr*>, Expr*> &fncase : fncases) {
    for (Expr *expr : fncase.first) {
      expr->mark(gc); 
    }

    fncase.second->mark(gc);
  }
}

// BiOpExpr

bool BiOpExpr::isFunctionConstructor() const noexcept {
  if (getOperator() != op_fn) return false;

  if (getLHS().getExpressionType() == expr_biop)
    return dynamic_cast<const BiOpExpr&>(getLHS()).isFunctionConstructor();

  return getLHS().getExpressionType() == expr_id;
}

bool BiOpExpr::isAtomConstructor() const noexcept {
  if (getOperator() != op_fn) return false;

  if (getRHS().getExpressionType() != expr_id
      && (getRHS().getExpressionType() != expr_biop
        || !dynamic_cast<const BiOpExpr*>(rhs)->isAtomConstructor()))
      return false;

  if (getLHS().getExpressionType() == expr_atom) return true;
  if (getLHS().getExpressionType() != expr_biop) return false;

  return dynamic_cast<const BiOpExpr*>(lhs)->isAtomConstructor();
}

const AtomExpr *BiOpExpr::getAtomConstructor() const noexcept {
  if (getOperator() != op_fn) return nullptr;

  if (getLHS().getExpressionType() == expr_atom)
    return dynamic_cast<const AtomExpr*>(lhs);

  if (getLHS().getExpressionType() == expr_biop)
    return dynamic_cast<const BiOpExpr*>(lhs)->getAtomConstructor();

  return nullptr;
}

// Expressions

Expr *reportSyntaxError(Lexer &lexer, const std::string &msg, const TokenPos &pos) {
  lexer.skipNewLine = false; // reset new line skip
  lexer.reportError(msg, pos);
  return nullptr;
}

// evaluate

Expr *eval(GCMain &gc, Environment &env, Expr *pexpr) noexcept {

  StackFrameObj<Expr> expr(env, pexpr);
  StackFrameObj<Expr> oldExpr(env, pexpr);
  while (expr && (expr = expr->evalWithLookup(gc, env)) != oldExpr) {
    /* Detect endless term */
    if (expr && expr->getExpressionType() == expr_biop) {
      if (&dynamic_cast<BiOpExpr*>(*expr)->getRHS() == *oldExpr
          || &dynamic_cast<BiOpExpr*>(*expr)->getLHS() == *oldExpr)
        return reportSyntaxError(*env.lexer,
            "Endless term detected.",
            expr->getTokenPos());
    }

    oldExpr = expr;

    if (gc.getCountNewObjects() < 200)
      continue;

    env.mark(gc);
    gc.collect();
  }

  return *expr;
}

void breadthEval(GCMain &gc, Environment &env,
    StackFrameObj<Expr> &lhs, StackFrameObj<Expr> &rhs) noexcept {
  StackFrameObj<Expr> oldlhs(env, *lhs);
  StackFrameObj<Expr> oldrhs(env, *rhs);

  while (lhs && rhs
      && ((lhs = lhs->evalWithLookup(gc, env)) != oldlhs
        || (rhs = rhs->evalWithLookup(gc, env)) != oldrhs)) {

    oldlhs = lhs;
    oldrhs = rhs;

    if (gc.getCountNewObjects() < 200)
      continue;

    env.mark(gc);
    gc.collect();
  }
}
