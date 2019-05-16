#ifndef _PHASAR_PHASARLLVM_MONO_CONTEXTBASE_H_
#define _PHASAR_PHASARLLVM_MONO_CONTEXTBASE_H_

#include <map>
#include <phasar/Config/ContainerConfiguration.h>
#include <stack>
#include <utility>

namespace psr {

template <typename D, typename N> class ContextBase {
protected:
  // management structures here
  std::map<std::pair<psr::MonoSet<D>, N>, psr::MonoSet<D>> contextmap;

  std::stack<std::pair<psr::MonoSet<D>, N>> tempstack;
  bool switchSmartContext = true;

public:
  ContextBase() : contextmap(), tempstack() {}

  void setSmartContext(bool smart) : switchSmartContext(smart) {}

  bool isUnknown(psr::MonoSet<D> &In, const N src) {
    if (contextmap.find(std::make_pair<psr::MonoSet<D>, N>(In, src)) ==
        contextmap.end()) {
      return true;
    } else {
      return false;
    }
  }

  psr::MonoSet<D> getResult(psr::MonoSet<D> &In, const N src) {

    if (contextmap.find(std::make_pair<psr::MonoSet<D>, N>(In, src)) ==
        contextmap.end()) {
      return psr::MonoSet<D>();
    } else {
      return psr::MonoSet<D>(
          contextmap.find(std::make_pair<psr::MonoSet<D>, N>(In, src))->second);
    }
  }

  void enterFunction(const N src, const N dst, psr::MonoSet<D> &In) {
    if (switchSmartContext) {
      const std::pair<psr::MonoSet<D>, N> tmp(In, src);
      contextmap[tmp];
      tempstack.push(tmp);
    }
    addContext(src, dest, In);
  }

  void exitFunction(const N src, const N dst, psr::MonoSet<D> &In) {
    if (switchSmartContext) {
      contextmap[tempstack.pop()] = In;
    }
    removeContext(src, dst, In);
  }

protected:
  virtual void addContext(const N src, const N dst, psr::MonoSet<D> &In) = 0;
  virtual void removeContext(const N src, const N dst, psr::MonoMap<D> &In) = 0;

public:
  virtual bool isEqual(const ContextBase &rhs) const = 0;
  virtual bool isDifferent(const ContextBase &rhs) const = 0;
  virtual bool isLessThan(const ContextBase &rhs) const = 0;
  virtual void print(std::ostream &os) const = 0;

  friend bool operator==(const ContextBase &lhs, const ContextBase &rhs) {
    return lhs.isEqual(rhs);
  }
  friend bool operator!=(const ContextBase &lhs, const ContextBase &rhs) {
    return lhs.isDifferent(rhs);
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