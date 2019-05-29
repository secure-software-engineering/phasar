#ifndef _PHASAR_PHASARLLVM_MONO_CONTEXTBASE_H_
#define _PHASAR_PHASARLLVM_MONO_CONTEXTBASE_H_

#include <map>
#include <phasar/Config/ContainerConfiguration.h>
#include <stack>
#include <utility>

namespace psr {

template <typename D, typename N> class ContextBase {
protected:

public:
  virtual ~ContextBase() = default;

  bool isKnown(psr::MonoSet<D> &In, const N src) {
  }

  psr::MonoSet<D> getResult(psr::MonoSet<D> &In, const N src) {
  }

  void enterFunction(const N src, const N dst, psr::MonoSet<D> &In) {
  }

  void exitFunction(const N src, const N dst, psr::MonoSet<D> &In) {
  }

  bool isEqual(const ContextBase &rhs) const { return false; };
  virtual bool isLessThan(const ContextBase &rhs) const { return false; }
  virtual void print(std::ostream &os) const = 0;

  friend bool operator==(const ContextBase &lhs, const ContextBase &rhs) {
    return lhs.isEqual(rhs);
  }
  friend bool operator!=(const ContextBase &lhs, const ContextBase &rhs) {
    return !lhs.isEqual(rhs);
  }
  friend bool operator<(const ContextBase &lhs, const ContextBase &rhs) {
    return lhs.isLessThan(rhs);
  }
  friend std::ostream &operator<<(std::ostream &os, const ContextBase &c) {
    c.print(os);
    return os;
  }
};

} // namespace psr

#endif