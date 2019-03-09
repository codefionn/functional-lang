#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  GCMain gc;
  Environment *env = new Environment(gc);
  if (vargsc == 2) {
    std::ifstream input;
    input.open(vargs[1]);

    if (!input) {
      std::cerr << "Failed opening file \"" << vargs[1] << "\"." << std::endl;
      return 1;
    }

    interpret(input, gc, env);
  };

  return interpret(std::cin, gc, env, true) ? 0 : 1;
}
