#ifndef PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONVERIFIER_H
#define PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONVERIFIER_H

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/JoinLatticeVerifier.h"

#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <type_traits>
#include <unordered_set>

namespace psr {

namespace detail {

template <typename L>
[[nodiscard]] inline bool isSqSubsetEq(const L &Lhs, const L &Rhs) {
  const auto &Join = JoinLatticeTraits<L>::join(Lhs, Rhs);
  return Join == Rhs;
}

template <typename L>
[[nodiscard]] inline bool
edgeIdentityAbsorbedByCompose(const EdgeFunction<L> &EF,
                              llvm::raw_ostream &Err) {
  const auto &Composed = EF.composeWith(EdgeIdentity<L>{});

  if (Composed != EF) {
    Err << "compose with EdgeIdentity should have no effect: compose(" << EF
        << ", Id) = " << Composed << "; expected: " << EF << '\n';
    return false;
  }
  return true;
}

template <typename L, typename ValGeneratorT>
[[nodiscard]] inline bool constantEFIsConst(const EdgeFunction<L> &EF,
                                            const ValGeneratorT &ValGenerator,
                                            llvm::raw_ostream &Err) {
  if (!EF.isConstant()) {
    return true;
  }

  bool Ret = true;
  L Base = EF.computeTarget(JoinLatticeTraits<L>::top());
  for (const auto &Val : ValGenerator) {
    const auto &Result = EF.computeTarget(Val);
    if (Result != Base) {
      Err << "EdgeFunction marked with isConstant() should always return the "
             "same value: "
          << EF << "(" << Val << ") != " << Base << '\n';
      Ret = false;
    }
  }
  return Ret;
}

template <typename L, typename ValGeneratorT>
[[nodiscard]] inline bool composeModelsFunctionComposition(
    const EdgeFunction<L> &EF, const EdgeFunction<L> &Other,
    const ValGeneratorT &ValGenerator, llvm::raw_ostream &Err) {
  const auto &Composed = EF.composeWith(Other);

  bool OtherConst = Other.isConstant();

  bool Ret = true;

  for (const auto &Val : ValGenerator) {
    const auto &Result = Composed.computeTarget(Val);

    if (!OtherConst) {
      const auto &ManualRes = Other.computeTarget(EF.computeTarget(Val));
      if (!isSqSubsetEq(ManualRes, Result)) {
        Err << "compose does not soundly approximate function composition: "
               "compose("
            << EF << ", " << Other << ")(" << Val << ") = " << Result
            << ", whereas " << Other << "(" << EF << "(" << Val
            << ")) = " << ManualRes << ", which is not approximated by "
            << Result << '\n';
        Ret = false;
      }
    } else {

      const auto &OtherRes = Other.computeTarget(Val);
      if (Result != OtherRes) {
        Err << "constant EF " << Other
            << " does not absorb composed function: compose(" << EF << ", "
            << Other << ")(" << Val << ") = " << Result
            << ", expected equality to: " << Other << "(" << Val
            << ") = " << OtherRes << '\n';
        Ret = false;
      }
    }
  }

  return Ret;
}
} // namespace detail

template <typename L, typename EFGeneratorT, typename ValGeneratorT,
          typename HasherT = std::hash<EdgeFunction<L>>,
          typename EqualT = std::equal_to<EdgeFunction<L>>,
          typename = std::enable_if_t<HasJoinLatticeTraits<L>>>
[[nodiscard]] bool
validateEdgeFunctionLattice(const EFGeneratorT &Generator,
                            const ValGeneratorT &ValGenerator,
                            llvm::raw_ostream &Err = llvm::errs(),
                            HasherT Hasher = {}, EqualT Equal = {}) {
  llvm::SmallVector<EdgeFunction<L>> WL(llvm::adl_begin(Generator),
                                        llvm::adl_end(Generator));
  llvm::SmallVector<EdgeFunction<L>> All = WL;
  llvm::SmallVector<EdgeFunction<L>> Next;

  std::unordered_set<EdgeFunction<L>, HasherT, EqualT> Seen(
      WL.begin(), WL.end(), WL.size(), std::move(Hasher), std::move(Equal));

  bool Ret = true;

  while (true) {

    for (const EdgeFunction<L> &Val : WL) {

      Ret &= detail::idempotentJoin(Val, Err);
      Ret &= detail::respectsTopBot(Val, Err);
      Ret &= detail::edgeIdentityAbsorbedByCompose(Val, Err);
      Ret &= detail::constantEFIsConst(Val, ValGenerator, Err);

      for (const EdgeFunction<L> &Other : All) {
        Ret &= detail::isPartiallyOrderedWithJoin(Val, Other, Err);
        Ret &= detail::composeModelsFunctionComposition(Val, Other,
                                                        ValGenerator, Err);

        auto &&Join = JoinLatticeTraits<EdgeFunction<L>>::join(Val, Other);

        if (Seen.insert(Join).second) {
          Next.push_back(std::forward<decltype(Join)>(Join));
        }
      }
    }

    if (!Ret || Next.empty()) {
      return Ret;
    }

    All.append(Next);
    WL = std::exchange(Next, {});
  }
}
} // namespace psr

#endif // PHASAR_DATAFLOW_IFDSIDE_EDGEFUNCTIONVERIFIER_H
