#ifndef _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_
#define _PHASAR_PHASARLLVM_MONO_CALLSTRINGCTX_H_

#include <deque>
#include <functional>
#include <initializer_list>

#include "boost/functional/hash.hpp"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename N, unsigned K> class CallStringCTX {
protected:
  std::deque<N> cs;
  static const unsigned k = K;
  friend struct std::hash<psr::CallStringCTX<N, K>>;

public:
  CallStringCTX() {}

  CallStringCTX(std::initializer_list<N> ilist) : cs(ilist) {
    if (ilist.size() > k) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  void push_back(N n) {
    if (cs.size() > k - 1) {
      cs.pop_front();
    }
    cs.push_back(n);
  }

  N pop_back() {
    if (cs.size() > 0) {
      N n = cs.back();
      cs.pop_back();
      return n;
    }
    return N{};
  }

  bool isEqual(const CallStringCTX &rhs) const { return cs == rhs.cs; }

  bool isDifferent(const CallStringCTX &rhs) const { return !isEqual(rhs); }

  friend bool operator==(const CallStringCTX<N, K> &Lhs,
                         const CallStringCTX<N, K> &Rhs) {
    return Lhs.isEqual(Rhs);
  }

  friend bool operator!=(const CallStringCTX<N, K> &Lhs,
                         const CallStringCTX<N, K> &Rhs) {
    return !Lhs.isEqual(Rhs);
  }

  friend bool operator<(const CallStringCTX<N, K> &Lhs,
                        const CallStringCTX<N, K> &Rhs) {
    return Lhs.cs < Rhs.cs;
  }

  void print(std::ostream &os) const {
    os << "Call string: [ ";
    for (auto C : cs) {
      os << llvmIRToString(C);
      if (C != cs.back()) {
        os << " * ";
      }
    }
    os << " ]";
  }

  friend std::ostream &operator<<(std::ostream &os,
                                  const CallStringCTX<N, K> &c) {
    c.print(os);
    return os;
  }

  bool empty() const { return cs.empty(); }

  std::size_t size() const { return cs.size(); }
};

} // namespace psr

namespace std {

template <typename N, unsigned K> struct hash<psr::CallStringCTX<N, K>> {
  size_t operator()(const psr::CallStringCTX<N, K> &CS) const noexcept {
    boost::hash<std::deque<N>> hash_deque;
    std::hash<unsigned> hash_unsigned;
    size_t u = hash_unsigned(K);
    size_t h = hash_deque(CS.cs);
    return u ^ (h << 1);
  }
};

} // namespace std

#endif