#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  return interpret(std::cin, "> ") ? 0 : 1;
}
