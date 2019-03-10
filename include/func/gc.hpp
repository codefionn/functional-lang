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
  std::size_t countNewObjs;

  //! All objects available.
  std::vector<GCObj*> marks;
public:
  GCMain() : markBit(true), marks() {}
  virtual ~GCMain();

  /*!\return Returns status of the mark bit.
   *
   * This is a 'hack'. Otherwise the algorithm would be force to reset the
   * mark bit after every collect. But if a the markbit can be toggled 
   * (its switched the mark state after collecting), it becomes obsolete.
   */
  bool getMarkBit() const noexcept;

  /*!\return Returns count of new objects since last collect call.
   * \see collect
   */
  std::size_t getCountNewObjects() const noexcept;

  /*!\brief Adds obj to all objects available.
   * \param obj
   */
  void add(GCObj *obj);

  /*!\brief Collects garbage.
   *
   * Use mark function of directly reachable objects (roots).
   * Resets getCountNewObjects to 0.
   */ 
  void collect();
};

#endif /* FUNC_GC_HPP */
