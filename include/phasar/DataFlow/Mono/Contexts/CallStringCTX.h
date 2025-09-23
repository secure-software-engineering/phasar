#ifndef PHASAR_DATAFLOW_MONO_CONTEXTS_CALLSTRINGCTX_H
#define PHASAR_DATAFLOW_MONO_CONTEXTS_CALLSTRINGCTX_H

#include "phasar/Utils/Printer.h"

#include "llvm/ADT/Hashing.h"
#include "llvm/Support/raw_ostream.h"

#include <deque>
#include <functional>
#include <initializer_list>

namespace psr {

/// Stores a call-string context that can be used in interprocedural monotone
/// analysis to achieve (limited) context sensitivity.
/// @tparam N Type of the call-string elements.
/// @tparam K Maximal length the call string can have.
template <typename N, unsigned K> class CallStringCTX {
protected:
  std::deque<N> CallString;
  static constexpr unsigned KLimit = K;
  friend struct std::hash<psr::CallStringCTX<N, K>>;

public:
  CallStringCTX() = default;

  CallStringCTX(std::initializer_list<N> IList) : CallString(IList) {
    if (IList.size() > KLimit) {
      throw std::runtime_error(
          "initial call std::string length exceeds maximal length K");
    }
  }

  void push_back(N Stmt) { // NOLINT
    if (CallString.size() > KLimit - 1) {
      CallString.pop_front();
    }
    CallString.push_back(Stmt);
  }

  N pop_back() { // NOLINT
    if (!CallString.empty()) {
      N Stmt = CallString.back();
      CallString.pop_back();
      return Stmt;
    }
    return N{};
  }

  [[nodiscard]] bool isEqual(const CallStringCTX &Rhs) const {
    return CallString == Rhs.CallString;
  }

  [[nodiscard]] bool isDifferent(const CallStringCTX &Rhs) const {
    return !isEqual(Rhs);
  }

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
    return Lhs.CallString < Rhs.CallString;
  }

  llvm::raw_ostream &print(llvm::raw_ostream &OS) const {
    OS << "Call string: [ ";
    for (auto C : CallString) {
      OS << NToString(C);
      if (C != CallString.back()) {
        OS << " * ";
      }
    }
    return OS << " ]";
  }

  [[nodiscard]] bool empty() const { return CallString.empty(); }

  [[nodiscard]] std::size_t size() const { return CallString.size(); }
};

} // namespace psr

namespace std {

template <typename N, unsigned K> struct hash<psr::CallStringCTX<N, K>> {
  size_t operator()(const psr::CallStringCTX<N, K> &CS) const noexcept {
    auto H =
        llvm::hash_combine_range(CS.CallString.begin(), CS.CallString.end());
    return llvm::hash_combine(K, H);
  }
};

} // namespace std

#endif
