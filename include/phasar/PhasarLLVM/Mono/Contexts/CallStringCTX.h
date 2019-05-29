#ifndef _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_
#define _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_

#include <algorithm>
#include <deque>
#include <initializer_list>
#include <iterator>

#include <phasar/PhasarLLVM/Mono/Contexts/ContextBase.h>

namespace psr {

template <typename D, typename N, unsigned K>
class CallStringCTX {
 protected:
  std::deque<N> cs;
  static const unsigned k = K;

 public:
  CallStringCTX() {}

  CallStringCTX(std::initializer_list<N> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  virtual void addContext(const N src, const N dst, psr::MonoSet<D> &In) {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(src);
  }
  virtual void removeContext(const N src, const N dst, psr::MonoSet<D> &In) {
    if (cs.size() > 0) {
      cs.pop_back();
    }
  }

  virtual bool isEqual(const CallStringCTX &rhs) const { return cs == rhs.cs; }

  bool isDifferent(const CallStringCTX &rhs) const { return !isEqual(rhs); }

  friend bool operator==(const CallStringCTX<D, N, K> &Lhs,
                         const CallStringCTX<D, N, K> &Rhs) {
    return Lhs.isEqual(Rhs);
  }

  friend bool operator!=(const CallStringCTX<D, N, K> &Lhs,
                         const CallStringCTX<D, N, K> &Rhs) {
    return !Lhs.isEqual(Rhs);
  }

  friend bool operator<(const CallStringCTX<D, N, K> &Lhs,
                        const CallStringCTX<D, N, K> &Rhs) {
    return Lhs.cs < Rhs.cs;
  }

  void print(std::ostream &os) const {
    os << "Call string [" << cs.size() << "]: ";
    for (auto C : cs) {
      os << this->NP->NtoString(C) << " * ";
    }
  }

  std::size_t size() const { return cs.size(); }
};

}  // namespace psr

#endif