#include "func/lexer.hpp"

Lexer::Lexer(std::istream &input)
  : input{&input}, lineStr(), line{0}, column{0}, curchar{-2} {
}

Lexer::~Lexer() {

}

static Token identifierToken(const std::string &id) {
  if (id == "if")
    return tok_if;
  if (id == "then")
    return tok_then;
  if (id == "else")
    return tok_else;
  if (id == "let")
    return tok_let;
  if (id== "in")
    return tok_in;

  return tok_id;
}

int Lexer::nextChar() {
  curchar = input->get();

  if (curchar == '\n' || curchar == EOF) {
    ++line;
    column = 0;
    lines.push_back(lineStr);
    lineStr = "";
  } else if (curchar == ' ') {
    ++column;
    ++token_end;
    lineStr += ' ';
  } else if (curchar == '\t') {
    column += 4;
    token_end += 4;
    lineStr += "    ";
  } else {
    lineStr += (char) curchar;
    ++column;
    ++token_end;
  }
  
  return curchar;
}

int Lexer::currentChar() const noexcept {
  return curchar;
}

Token Lexer::nextToken() {
  if (curchar == -2) {
    token_end = 0;
    token_start = 0;
    nextChar();
  }

  // Skip spaces
  while (curchar == ' ' || curchar == '\r' || curchar == '\t') {
    nextChar();
  }

  token_start = column;

  switch (curchar) {
  case '+':
    nextChar(); // eat +
    curop = op_add;
    return curtok = tok_op;
  case '-':
    nextChar(); // eat -
    if (curchar == '-') // it's a comment
      break;

    curop = op_sub;
    return curtok = tok_op;
  case '*':
    nextChar(); // eat *
    curop = op_mul;
    return curtok = tok_op;
  case '/':
    nextChar(); // eat /
    curop = op_div;
    return curtok = tok_op;
  case '=':
    nextChar(); // eat =
    if (curchar == '=') {
      curop = op_eq; 
      nextChar(); // eat =
    } else
      curop = op_asg;
    return curtok = tok_op;
  case '<':
    nextChar(); // eat <
    if (curchar == '=') {
      curop = op_leq;
      nextChar(); // eat =
    } else
      curop = op_le;
    return curtok = tok_op;
  case '>':
    nextChar(); // eat >
    if (curchar == '=') {
      curop = op_geq;
      nextChar(); // eat =
    } else
      curop = op_gt;
    return curtok = tok_op;
  case '\\': /* eat \ */
    nextChar();
    return curtok = tok_lambda;
  case '.':
    nextChar(); // eat .
    return curtok = tok_atom;
  case '(':
    nextChar(); // eat (
    return curtok = tok_obrace;
  case ')':
    nextChar(); // eat )
    return curtok = tok_cbrace;
  case '^':
    nextChar(); // eat ^
    curop = op_pow;
    return curtok = tok_op;
  case '$':
    nextChar(); // eat $
    return curtok = tok_literal;
  case '_':
    nextChar(); // eat _
    return curtok = tok_any;
  case ';':
    nextChar(); // eat :
    return curtok = tok_delim;
  case '&':
    nextChar(); // eat &
    if (curchar == '&') {
      nextChar(); // eat &
      curop = op_land;
      return curtok = tok_op;
    }

    return curtok = reportError("Unknown/Unsupported character!");
  case '|':
    nextChar(); // eat |
    if (curchar == '|') {
      nextChar(); // eat |
      curop = op_lor;
      return curtok = tok_op;
    }

    return curtok = reportError("Unknown/Unsupported character!");
  }

  if (isdigit(curchar)) {
    // Number
    double num = 0.0;
    curint = 0;
    while (isdigit(curchar)) {
      curint *= 10;
      curint += curchar - '0';

      num *= 10.0;
      num += curchar - '0';

      nextChar(); // Eat digit
    }

    curnum = num;

    if (curchar == '.') {
      nextChar(); // Eat .

      std::size_t digitsAfterComma = 0;
      double numAfterComma = 0.0;
      while (isdigit(curchar)) {
        numAfterComma *= 10.0;
        numAfterComma += curchar - '0';
        nextChar(); // Eat digit
        ++digitsAfterComma; // Increase digit count after comma
      }

      if (digitsAfterComma == 0)
        return curtok = reportError("At least one digit expected after '.'.");

      while (digitsAfterComma-- > 0)
        numAfterComma /= 10.0;

      curnum = num + numAfterComma;
      return curtok = tok_num;
    } else if (isalpha(curchar))
      return curtok = reportError("Alphabetic characters are not allowed directly after numbers!");

    // it's an integer
    return curtok = tok_int;
  }

  if (isalpha(curchar)) {
    // Identifier
    curid = "";
    while (isalpha(curchar)) {
      curid += curchar;
      nextChar(); // Eat alpha.
    }

    if (isdigit(curchar))
      return curtok = reportError("Digits are not allowed directly after identifiers!");

    return curtok = identifierToken(curid);
  }

  if (curchar == '"') {
    nextChar(); // eat "
    // identifier
    curid = "\"";
    while (curchar != '"' && curchar != '\n' && curchar != EOF) {
      curid += curchar;
      nextChar(); // eat char
    }

    if (curchar != '"')
      return curtok = reportError("Expected \", not newline or eof.");

    nextChar(); // eat "

    if (isdigit(curchar))
      return curtok = reportError("Digits are not allowed after identifiers!");

    curid += "\"";

    return curtok = identifierToken(curid);
  }

  if (curchar == '-') { // Comment
    while (curchar != '\n') nextChar();
  }

  if (curchar == '\n') {
    curchar = -2; // problem of interpreted languages.
                  // we want to reach eol token before next eol was typed
    if (this->skipNewLine) {
      if (!this->skippedNewLinePrefix.empty())
        std::cout << this->skippedNewLinePrefix;

      return this->nextToken();
    }

    return curtok = tok_eol; // don't skip, return line
  }

  if (curchar == EOF)
    return curtok = tok_eof;

  return curtok = reportError("Unknown/Unsupported character!");
}

Token Lexer::currentToken() const noexcept {
  return curtok;
}

Operator Lexer::currentOperator() const noexcept {
  return curop;
}

double Lexer::currentNumber() const noexcept {
  return curnum;
}

std::int64_t Lexer::currentInteger() const noexcept {
  return curint;
}

const std::string &Lexer::currentIdentifier() const noexcept {
  return curid;
}

Token Lexer::reportError(const std::string &msg) noexcept {
  return reportError(msg, TokenPos(getTokenStartPos(), getTokenEndPos(),
        currentLine(), currentLine()));
}

Token Lexer::reportError(const std::string &msg, const TokenPos &pos) noexcept {
  // Advance to next line
  while (curchar != -2 && curchar != '\n' && curchar != EOF) nextChar();
  if (curchar != EOF) curchar = -2; // last was new line

  if (curtok == tok_eof || curchar == EOF)
    std::cerr << std::endl;

  // Print line and clear line
  for (size_t i = pos.getLineStart(); i <= pos.getLineEnd()
      && i < lines.size(); ++i) {
    std::cout << lines[i] << std::endl;
  }

  size_t start, end;
  if (pos.getStart() <= pos.getEnd()) {
    start = pos.getStart();
    end = pos.getEnd();
  } else {
    start = pos.getEnd();
    end = pos.getStart();
  }

  // Mark position
  for (size_t i = 0; i < end && end != 0; ++i) {
    if (i < start)
      std::cerr << ' ';
    else
      std::cerr << '~';
  }
  std::cerr << '^' << std::endl;

  // Print error message
  if (!msg.empty())
    std::cerr << pos.getLineStart() + 1 << ':' << pos.getStart() + 1 << ": " << msg << std::endl;
  column = 0; // Reset column

  if (!msg.empty() && curtok == tok_eof)
    std::cerr << pos.getLineStart() + 1 << ':' << pos.getStart() + 1 << ": Unexpected end of file." << std::endl;

  return tok_err;
}

Token Lexer::reportError(const std::string &msg, size_t start, size_t end) noexcept {
  return reportError(msg, TokenPos(start, end, currentLine(), currentLine()));
}

int getOperatorPrecedence(Operator op) {
  switch (op) {
  case op_asg:
      return 1;
  case op_land:
  case op_lor:
      return 2;
  case op_eq:
  case op_leq:
  case op_geq:
  case op_le:
  case op_gt:
      return 3;
  case op_add:
  case op_sub:
    return 4;
  case op_mul:
  case op_div:
    return 5;
  case op_pow:
    return 6;
  }

  return 0;
}

int Lexer::currentPrecedence() {
  switch (currentToken()) {
  case tok_op:
    return getOperatorPrecedence(currentOperator());
  }

  return 0;
}

std::string std::to_string(Operator op) noexcept {
  switch (op) {
  case op_eq:
    return "==";
  case op_leq:
    return "<=";
  case op_geq:
    return ">=";
  case op_le:
    return "<";
  case op_gt:
    return ">";
  case op_add:
    return "+";
  case op_sub:
    return "-";
  case op_mul:
    return "*";
  case op_div:
    return "/";
  case op_pow:
    return "^";
  case op_asg:
    return "=";
  case op_land:
    return "&&";
  case op_lor:
    return "||";
  }

  return ""; // invalid
}
