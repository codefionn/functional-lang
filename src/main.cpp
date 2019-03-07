#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  if (vargsc == 2) {
    std::ifstream input;
    input.open(vargs[1]);

    if (!input) {
      std::cerr << "Failed opening file \"" << vargs[1] << "\"." << std::endl;
      return 1;
    }

    return interpret(input, "") ? 0 : 1;
  };

  return interpret(std::cin, "> ") ? 0 : 1;
}
