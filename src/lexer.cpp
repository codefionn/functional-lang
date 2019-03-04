#include "func/lexer.hpp"

Lexer::Lexer(std::istream &input)
  : input{&input}, lineStr(), line{0}, column{0}, curchar{-2} {
}

Lexer::~Lexer() {

}

int Lexer::nextChar() {
  curchar = input->get();

  if (curchar == '\n') {
    ++line;
    column = 0;
    lineStr = "";
  } else if (curchar == ' ') {
    ++column;
    lineStr += ' ';
  } else if (curchar == '\t') {
    column += 4;
    lineStr += "    ";
  } else {
    lineStr += (char) curchar;
    ++column;
  }

  return curchar;
}

int Lexer::currentChar() const noexcept {
  return curchar;
}

Token Lexer::nextToken() {
  if (curchar == -2)
    nextChar();

  // Skip spaces
  while (curchar == ' ' || curchar == '\r' || curchar == '\t') {
    nextChar();
  }

  switch (curchar) {
  case '+':
    nextChar(); // eat +
    curop = op_add;
    return curtok = tok_op;
  case '-':
    nextChar(); // eat -
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
    curop = op_asg;
    return curtok = tok_op;
  case '\\': /* eat \ */
    nextChar();
    curop = op_lambda;
    return curtok = tok_op;
  case '(':
    nextChar(); // eat (
    return curtok = tok_obrace;
  case ')':
    nextChar(); // eat )
    return curtok = tok_cbrace;
  }

  if (isdigit(curchar)) {
    // Number
    double num = 0.0;
    while (isdigit(curchar)) {
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
    }
    
    if (isalpha(curchar))
      return curtok = reportError("Alphabetic characters are not allowed directly after numbers!");

    return curtok = tok_num;
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

    return curtok = tok_id;
  }

  if (curchar == '\n') {
    curchar = -2; // problem of interpreted languages.
                  // we want to reach eol token before next eol was typed
    return curtok = tok_eol;
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

const std::string &Lexer::currentIdentifier() const noexcept {
  return curid;
}

Token Lexer::reportError(const std::string &msg) {
  // Advance to next line
  while (curchar != '\n' && curchar != EOF) {
    curchar = input->get();
    if (curchar == ' ') {
      lineStr += ' ';
    } else if (curchar == '\t') {
      lineStr += "    ";
    } else if (curchar != '\n' && curchar != EOF) {
      lineStr += (char) curchar;
    }
  }
  line++;
  // Print line and clear line
  std::cerr << lineStr << std::endl;
  lineStr = "";
  // Mark position
  for (size_t i = 0; i < column - 1 && column != 0; ++i) {
    std::cerr << ' ';
  }
  std::cerr << '^' << std::endl;
  // Print error message
  std::cerr << line << ':' << column << ": " << msg << std::endl;
  column = 0; // Reset column

  return tok_err;
}
