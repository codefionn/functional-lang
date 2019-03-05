#include "func/gc.hpp"

GCObj::GCObj(GCMain &main) noexcept : marked(!main.getMarkBit()) {
  main.add(this);
} 
void GCObj::markSelf(GCMain &main) {
  marked = main.getMarkBit();
}

bool GCObj::isMarked(GCMain &main) const noexcept {
  return marked == main.getMarkBit();
}

void GCObj::mark(GCMain &main) noexcept {
  if(isMarked(main))
   return;

  markSelf(main);
}
// GCMain

GCMain::~GCMain() {
  for (GCObj *obj : marks) {
    delete obj;
  }
}

bool GCMain::getMarkBit() const noexcept { return markBit; }

void GCMain::add(GCObj *obj) {
  for (size_t i = 0; i < marks.size(); ++i) {
    if (!marks[i]) {
      marks[i] = obj;
      return;
    }
  }

  marks.push_back(obj);
}

void GCMain::collect() {
  //size_t collected_objs = 0;
  for (size_t i = 1; i <= marks.size(); ++i) {
    if (marks[i - 1] && !marks[i-1]->isMarked(*this)) {
      // Delete
      delete marks[i - 1];
      marks[i -1 ] = nullptr; // delete reference
      --i; // Go back one position

      //++collected_objs;
    }
  }

  //std::cerr << "Collected Object Count: " << collected_objs << std::endl;

  // Flip markBit (Prevents reseting all mark bits)
  markBit = !markBit;
}
