#ifndef _PHASAR_PHASARLLVM_MONO_CONTEXTS_VALUEBASEDCTX_H_
#define _PHASAR_PHASARLLVM_MONO_CONTEXTS_VALUEBASEDCTX_H_

#include <ostream>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

template <typename D, typename N> class ValueBasedCTX : ContextBase<D, N> {

public:
  ValueBasedCTX() : ContextBase<D, N>() {}

protected:
  virtual void addContext(const N src, const N dst,
                          psr::MonoSet<D> &In) override {}
  virtual void removeContext(const N src, const N dst,
                             psr::MonoMap<D> &In) override {}

public:
  bool isEqual(const CallString &rhs) const override {}

  bool isDifferent(const CallString &rhs) const override {}

  bool isLessThan(const CallString &rhs) const override {}

  void print(std::ostream &os) const override {}
};

} // namespace psr

#endif