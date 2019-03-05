#ifndef FUNC_GC_HPP
#define FUNC_GC_HPP

/*!\file func/gc.hpp
 * \brief Tracing garbage collection.
 */

#include "func/global.hpp"

class GCObj;
class GCMain;

/*!\brief Garbage collection object (objects to collect).
 */
class GCObj {
  bool marked = false; //!< Toggled if marked
protected:
  //! Marks itself
  virtual void markSelf(GCMain &main);
public:
  //!\brief Initialize as not marked
  GCObj(GCMain &main) noexcept;

  virtual ~GCObj() {}

  //!\return Returns true if marked, false if not.
  virtual bool isMarked(GCMain &main) const noexcept;

  //!\brief If not marked mark itself and children.
  virtual void mark(GCMain &main) noexcept;
};

/*!\brief Implements a tracing garbage collector.
 */
class GCMain {
  bool markBit; //!< Toggled after every collect() 
  std::vector<GCObj*> marks;
public:
  GCMain() : markBit(true), marks() {}
  virtual ~GCMain();

  //!\return Returns status of the mark bit.
  bool getMarkBit() const noexcept;

  void add(GCObj *obj);

  /*!\brief Collects garbage.
   *
   * Please use mark function of directly reachable objects.
   */ 
  void collect();
};

#endif /* FUNC_GC_HPP */
