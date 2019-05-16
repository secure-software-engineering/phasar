#ifndef _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_
#define _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iterator>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

template <typename D, typename N, unsigned K>
class CallStringCTX : public ContextBase<D, N> {

protected:
  std::deque<N> cs;
  static const unsigned k = K;

public:
  CallStringCTX() : ContextBase<D, N>() {}
  CallStringCTX(std::initializer_list<N> ilist)
      : ContextBase<N, D, CallString<N, D, K>>(np, dp), cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

protected:
  virtual void addContext(const N src, const N dst,
                          psr::MonoSet<D> &In) override {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(src);
  }
  virtual void removeContext(const N src, const N dst,
                             psr::MonoMap<D> &In) override {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }

public:
  bool isEqual(const CallString &rhs) const override {
    return cs == rhs.cs || (cs.size() == 0) || (rhs.cs.size() == 0);
  }

  bool isDifferent(const CallString &rhs) const override {
    return !isEqual(rhs);
  }

  bool isLessThan(const CallString &rhs) const override {
    // Base : lhs.cs < rhs.cs
    // Addition : (lhs.cs.size() != 0) && (rhs.cs.size() != 0)
    // Enable that every empty call-string context match every context
    // That allows an output of a retFlow with an empty callString context
    // to be join with every analysis results at the arrival node.
    return cs < rhs.cs && (cs.size() != 0) && (rhs.cs.size() != 0);
  }

  void print(std::ostream &os) const override {
    os << "Call string [" << cs.size() << "]: ";
    for (auto C : cs) {
      os << this->NP->NtoString(C) << " * ";
    }
  }

  std::size_t size() const { return cs.size(); }
  std::deque<Node_t> getInternalCS() const { return cs; }
};

} // namespace psr

#endif