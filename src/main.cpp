#include "func/func.hpp"

int main(int vargsc, char * vargs[]) {
  std::vector<std::string> lines;

  GCMain gc;
  Environment *env = new Environment(gc);
  if (vargsc == 2) {
    std::ifstream input;
    input.open(vargs[1]);

    if (!input) {
      std::cerr << "Failed opening file \"" << vargs[1] << "\"." << std::endl;
      return 1;
    }

    interpret(input, gc, lines, env);
  };

  return interpret(std::cin, gc, lines, env, true) ? 0 : 1;
}
