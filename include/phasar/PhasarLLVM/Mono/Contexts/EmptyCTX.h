#ifndef _PHASAR_PHASARLLVM_MONO_CONTEXTS_EMPTYCTX_H_
#define _PHASAR_PHASARLLVM_MONO_CONTEXTS_EMPTYCTX_H_

#include <ostream>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

/**
 * @brief Dummy context that does not stock anything if you do not need
 * a context.
 */
template <typename D, typename N> class EmptyCTX : ContextBase<D, N> {

public:
  EmptyCTX() : ContextBase<D, N>() {}

protected:
  virtual void addContext(const N src, const N dst,
                          psr::MonoSet<D> &In) override {}
  virtual void removeContext(const N src, const N dst,
                             psr::MonoMap<D> &In) override {}

public:
  bool isEqual(const CallString &rhs) const override { return true; }

  bool isDifferent(const CallString &rhs) const override { return false; }

  bool isLessThan(const CallString &rhs) const override { return false; }

  void print(std::ostream &os) const override {}
};

} // namespace psr

#endif