#include "func/syntax.hpp"

/* only for parsing primary expression */

static bool isPrimaryToken(Token tok) {
  return tok == tok_id
      || tok == tok_num
      || tok == tok_int
      || tok == tok_obrace
      || tok == tok_lambda
      || tok == tok_atom
      || tok == tok_if
      || tok == tok_literal
      || tok == tok_any
      || tok == tok_let;
}

Expr *parsePrimary(GCMain &gc, Lexer &lexer, Environment &env, bool topLevel) {
  StackFrameObj<Expr> result(env);

  while (lexer.currentToken() == tok_eol) {
    if (!lexer.skippedNewLinePrefix.empty())
      std::cout << lexer.skippedNewLinePrefix;
    lexer.nextToken();
  }

  switch (lexer.currentToken()) {
    case tok_id: {
        result = new IdExpr(gc, lexer.getTokenPos(), lexer.currentIdentifier());

        lexer.nextToken(); // eat id
        break;
      } // end case tok_id
    case tok_num: {
        result = new NumExpr(gc, lexer.getTokenPos(),
            lexer.currentNumber());

        lexer.nextToken(); // eat num
        break;
      } // end case tok_num
    case tok_int: {
        result = new IntExpr(gc, lexer.getTokenPos(),
            lexer.currentInteger());

        lexer.nextToken(); // eat num
        break;
      } // end case tok_int
    case tok_obrace: {
        lexer.skipNewLine = true;
        lexer.nextToken(); // eat (

        result = parse(gc, lexer, env, false);
        if (!result || lexer.currentToken() != tok_cbrace) {
          return reportSyntaxError(lexer, "Expected matching closing bracket )",
              lexer.getTokenPos());
        }

        lexer.skipNewLine = false;
        lexer.nextToken(); // eat )

        break;
      } // end case tok_obrace
    case tok_lambda: {
        lexer.nextToken(); /* eat \ */
        if (lexer.currentToken() != tok_id)
          return reportSyntaxError(lexer, "Expected identifier",
              lexer.getTokenPos());

        std::string idname = lexer.currentIdentifier();
        lexer.nextToken(); // eat id

        if (lexer.currentToken() != tok_op
            && lexer.currentOperator() != op_asg)
          return reportSyntaxError(lexer, "Expected assign operator '='!",
              lexer.getTokenPos());

        lexer.nextToken(); // eat =

        StackFrameObj<Expr> expr(env, parse(gc, lexer, env, false));
        if (!expr)
          return nullptr; // Error forwarding

        result = new LambdaExpr(gc, lexer.getTokenPos(), idname, *expr);

        break;
      } // end case tok_lambda
    case tok_atom: {
        TokenPos atompos = lexer.getTokenPos();
        lexer.nextToken(); // eat .
        if (lexer.currentToken() != tok_id)
          return reportSyntaxError(lexer, "Expected identifier!",
              lexer.getTokenPos());
        atompos = TokenPos(atompos, lexer.getTokenPos());

        std::string idname = lexer.currentIdentifier();
        lexer.nextToken(); // eat id

        result = new AtomExpr(gc, atompos, idname);
        break;
      } //end case tok_atom
    case tok_if: {
        TokenPos ifpos = lexer.getTokenPos();
        lexer.skipNewLine = true;
        lexer.nextToken(); // eat if

        StackFrameObj<Expr> condition(env, parse(gc, lexer, env,false));
        if (!condition) {
          lexer.skipNewLine = false;
          return nullptr;
        }

        if (lexer.currentToken() != tok_then)
          return reportSyntaxError(lexer, "Expected keyword 'then'.",
              lexer.getTokenPos());
        lexer.nextToken(); // eat then

        StackFrameObj<Expr> exprTrue(env, parse(gc, lexer, env, false));
        if (!exprTrue) {
          lexer.skipNewLine = false;
          return nullptr;
        }

        if (lexer.currentToken() != tok_else)
          return reportSyntaxError(lexer, "Expected keyword 'else'.",
              lexer.getTokenPos());
        lexer.skipNewLine = false;
        lexer.nextToken(); // eat else

        StackFrameObj<Expr> exprFalse(env, parse(gc, lexer, env, false));
        if (!exprFalse)
          return nullptr;

        result = new IfExpr(gc, ifpos, *condition, *exprTrue, *exprFalse);

        break;
      } // end case tok_if
    case tok_literal: {
        lexer.nextToken(); // eat $

        StackFrameObj<Expr> expr(env, parse(gc, lexer, env, false));
        if (!expr) return nullptr;

        StackFrameObj<Expr> literal(env, ::eval(gc, env, *expr));
        if (!literal) return nullptr; // error forwarding

        result = literal;

        break;
      } // end case tok_literal
    case tok_any: {
         TokenPos pos = lexer.getTokenPos();
         lexer.nextToken(); // eat _

        result = new AnyExpr(gc, pos);

        break;
      } // end case tok_any
    case tok_let: {
        TokenPos letpos = lexer.getTokenPos();
        lexer.nextToken(); // eat let

        std::vector<BiOpExpr*> assignments;
        while (lexer.currentToken() != tok_in
            && lexer.currentToken() != tok_eof) { // eat let, delim, newline

          if (assignments.size() > 0 && lexer.currentToken() == tok_delim)
            lexer.nextToken(); // eat ;
          
          StackFrameObj<Expr> asg(env, parse(gc, lexer, env, false));
          if (!asg)
            return nullptr;

          if (asg->getExpressionType() != expr_biop
              || dynamic_cast<BiOpExpr*>(*asg)->getOperator() != op_asg)
            return reportSyntaxError(lexer, "Assignment expected!",
                asg->getTokenPos());

          if (lexer.currentToken() != tok_in
              && lexer.currentToken() != tok_delim
              && lexer.currentToken() != tok_eol)
            return reportSyntaxError(lexer, "Expected ';', 'in' or EOL.",
                lexer.getTokenPos());

          assignments.push_back(dynamic_cast<BiOpExpr*>(*asg));
        }

        if (assignments.size() == 0)
          return reportSyntaxError(lexer, "Assignment expected!", letpos);

        if (lexer.currentToken() != tok_in)
          return reportSyntaxError(lexer, "Keyword 'in' expected! Not EOF.",
              lexer.getTokenPos());

        lexer.nextToken(); // eat in

        StackFrameObj<Expr> body(env, parse(gc, lexer, env, false));
        if (!body)
          return nullptr;

        result = new LetExpr(gc, letpos, assignments, *body);

        break;
      } // end case tok_let
  }

  if (lexer.currentToken() == tok_err)
    return nullptr;

  if (!result)
    return reportSyntaxError(lexer, "Not a primary expression token!",
        lexer.getTokenPos());

  // Top level bool is needed because otherwise this would
  // result in a right-associative expression (we want a left associative one)
  while (topLevel && isPrimaryToken(lexer.currentToken())) {
    StackFrameObj<Expr> primaryExpr(env, parsePrimary(gc, lexer, env, false));
    if (!primaryExpr) return nullptr; // error forwarding

    result = new BiOpExpr(gc, op_fn, *result, *primaryExpr);
  }

  return *result;
}
