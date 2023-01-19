/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/ArrayRef.h"

#include <type_traits>

namespace psr {
template <typename L> struct EdgeIdentity {
  using l_t = L;

  [[nodiscard]] l_t computeTarget(l_t Source) const
      noexcept(std::is_nothrow_move_constructible_v<l_t>) {
    static_assert(std::is_trivially_copyable_v<EdgeIdentity>);
    static_assert(IsEdgeFunction<EdgeIdentity>);
    return Source;
  }

  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<EdgeIdentity> /*This*/,
          const EdgeFunction<l_t> &SecondFunction) {
    return SecondFunction;
  }
  [[nodiscard]] static EdgeFunction<l_t>
  join(EdgeFunctionRef<EdgeIdentity> This,
       const EdgeFunction<l_t> &OtherFunction);
};

template <typename L> struct AllBottom {
  using l_t = L;
  using JLattice = JoinLatticeTraits<L>;

  [[no_unique_address]] std::conditional_t<HasJoinLatticeTraits<l_t>, EmptyType,
                                           l_t>
      BottomValue;

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> /*Source*/) const
      noexcept(noexcept(JLattice::bottom())) {
    static_assert(std::is_trivially_copyable_v<AllBottom>);
    static_assert(IsEdgeFunction<AllBottom>);
    if constexpr (HasJoinLatticeTraits<l_t>) {
      return JLattice::bottom();
    } else {
      return BottomValue;
    }
  }

  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<AllBottom> This,
          const EdgeFunction<l_t> &SecondFunction) {
    return SecondFunction.isConstant() ? SecondFunction : This;
  }

  [[nodiscard]] static EdgeFunction<l_t>
  join(EdgeFunctionRef<AllBottom> This,
       const EdgeFunction<l_t> & /*OtherFunction*/) {
    return This;
  }

  [[nodiscard]] constexpr bool isConstant() const noexcept { return true; }
};

template <typename L> struct AllTop {
  using l_t = L;
  using JLattice = JoinLatticeTraits<L>;

  [[no_unique_address]] std::conditional_t<HasJoinLatticeTraits<l_t>, EmptyType,
                                           l_t>
      TopValue;

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> /*Source*/) const
      noexcept(noexcept(JLattice::top())) {
    static_assert(std::is_trivially_copyable_v<AllTop>);
    static_assert(IsEdgeFunction<AllTop>);
    if constexpr (HasJoinLatticeTraits<l_t>) {
      return JLattice::top();
    } else {
      return TopValue;
    }
  }

  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<AllTop> This,
          const EdgeFunction<l_t> &SecondFunction) {
    return llvm::isa<EdgeIdentity<l_t>>(SecondFunction) ? This : SecondFunction;
  }

  [[nodiscard]] static EdgeFunction<l_t>
  join(EdgeFunctionRef<AllTop> /*This*/,
       const EdgeFunction<l_t> &OtherFunction) {
    return OtherFunction;
  }

  [[nodiscard]] constexpr bool isConstant() const noexcept { return true; }
};

template <typename L, typename ConcreteEF>
EdgeFunction<L>
defaultComposeOrNull(EdgeFunctionRef<ConcreteEF> This,
                     const EdgeFunction<L> &SecondFunction) noexcept {
  if (llvm::isa<EdgeIdentity<L>>(SecondFunction)) {
    return This;
  }
  if (SecondFunction.isConstant()) {
    return SecondFunction;
  }
  return nullptr;
}

template <typename L> struct ConstantEdgeFunction {
  using l_t = L;
  using JLattice = JoinLatticeTraits<L>;
  using value_type = typename NonTopBotValue<l_t>::type;

  [[nodiscard]] l_t computeTarget(ByConstRef<l_t> /*Source*/) const
      noexcept(std::is_nothrow_constructible_v<l_t, const value_type &>) {
    static_assert(IsEdgeFunction<ConstantEdgeFunction>);
    return Value;
  }

  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<ConstantEdgeFunction> This,
          const EdgeFunction<l_t> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }

    auto ConstVal = SecondFunction.computeTarget(This->Value);
    if constexpr (!EdgeFunctionBase::IsSOOCandidate<ConstantEdgeFunction>) {
      if (ConstVal == This->Value) {
        return This;
      }
    }

    if (JLattice::bottom() == ConstVal) {
      return AllBottom<l_t>{};
    }

    if (JLattice::top() == ConstVal) {
      /// TODO: Can this ever happen?
      return AllTop<l_t>{};
    }

    return ConstantEdgeFunction{
        NonTopBotValue<l_t>::unwrap(std::move(ConstVal))};
  }

  [[nodiscard]] static EdgeFunction<l_t>
  join(EdgeFunctionRef<ConstantEdgeFunction> This,
       const EdgeFunction<l_t> &OtherFunction);

  [[nodiscard]] friend llvm::raw_ostream &
  operator<<(llvm::raw_ostream &OS, ByConstRef<ConstantEdgeFunction> Id) {
    OS << "ConstantEF";
    if constexpr (is_llvm_printable_v<value_type>) {
      OS << '[' << Id.Value << ']';
    }
    return OS;
  }

  [[nodiscard]] friend bool
  operator==(ByConstRef<ConstantEdgeFunction> LHS,
             ByConstRef<ConstantEdgeFunction> RHS) noexcept {
    return LHS.Value == RHS.Value;
  }

  [[nodiscard]] constexpr bool isConstant() const noexcept { return true; }

  // -- constant data member

  value_type Value{};
};

template <typename L> struct EdgeFunctionComposer {
  using l_t = L;

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    static_assert(IsEdgeFunction<EdgeFunctionComposer>);
    return Second.computeTarget(First.computeTarget(std::move(Source)));
  }

  /**
   * Function composition is implemented as an explicit composition, i.e.
   *     (secondFunction * G) * F = EFC(F, EFC(G , otherFunction))
   *
   * However, it is advised to immediately reduce the resulting edge function
   * by providing an own implementation of this function.
   */
  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<EdgeFunctionComposer> This,
          const EdgeFunction<l_t> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }
    return This->First.composeWith(This->Second.composeWith(SecondFunction));
  }

  /// This join function is to be provided by derived classes

  // static EdgeFunction<l_t> join(EdgeFunctionRef<EdgeFunctionComposer> This,
  //                               const EdgeFunction<l_t> &OtherFunction);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const EdgeFunctionComposer &EF) {
    OS << "EFComposer[" << EF.First << ", " << EF.Second << ']';
    return OS;
  }

  [[nodiscard]] friend bool
  operator==(const EdgeFunctionComposer &LHS,
             const EdgeFunctionComposer &RHS) noexcept {
    return LHS.First == RHS.First && LHS.Second == RHS.Second;
  }

  // -- data members

  EdgeFunction<l_t> First{};
  EdgeFunction<l_t> Second{};
};

template <typename L, uint8_t N, typename JLattice> struct JoinEdgeFunction {
  using l_t = L;

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    static_assert(IsEdgeFunction<JoinEdgeFunction>);
    auto Val = Seed;
    for (const auto &EF : OtherEF) {
      Val = JLattice::join(std::move(Val), EF.computeTarget(Source));
      if (Val == JLattice::bottom()) {
        return Val;
      }
    }
    return Val;
  }

  [[nodiscard]] static EdgeFunction<l_t>
  compose(EdgeFunctionRef<JoinEdgeFunction> This,
          const EdgeFunction<l_t> &SecondFunction) {
    if (auto Default = defaultComposeOrNull(This, SecondFunction)) {
      return Default;
    }

    return EdgeFunctionComposer<l_t>{This, SecondFunction};
  }

  [[nodiscard]] static EdgeFunction<l_t>
  join(EdgeFunctionRef<JoinEdgeFunction> This,
       const EdgeFunction<l_t> &OtherFunction);

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const JoinEdgeFunction &EF) {
    OS << "JoinEF<" << N << ">[ Seed: " << EF.Seed << "; EF: { ";
    size_t Idx = 0;
    for (const auto &EF : EF.OtherEF) {
      if (Idx) {
        OS << ", ";
      }
      OS << '#' << Idx << ": " << EF;
      ++Idx;
    }
    return OS << ']';
  }

  [[nodiscard]] friend bool operator==(const JoinEdgeFunction &LHS,
                                       const JoinEdgeFunction &RHS) noexcept {
    if (LHS.Seed != RHS.Seed) {
      return false;
    }

    return llvm::equal(LHS.OtherEF, RHS.OtherEF);
  }

  [[nodiscard]] static EdgeFunction<l_t> create(EdgeFunction<l_t> LHS,
                                                EdgeFunction<l_t> RHS) {

    auto GetEfArrayAndSeed = [](const EdgeFunction<l_t> &EF)
        -> std::pair<llvm::ArrayRef<EdgeFunction<l_t>>, l_t> {
      if (const auto *Join = llvm::dyn_cast<JoinEdgeFunction>(EF)) {
        return {Join->OtherEF, Join->Seed};
      }
      return {llvm::makeArrayRef(EF), JLattice::top()};
    };

    auto [LVec, LSeed] = getEFArray(LHS);
    auto [RVec, RSeed] = getEFArray(RHS);

    auto JoinSeed = JLattice::join(std::move(LSeed), std::move(RSeed));
    if (JoinSeed == JLattice::bottom()) {
      return AllBottom<l_t>{};
    }

    llvm::SmallVector<EdgeFunction<l_t>, N> JoinVec;

    std::set_union(LVec.begin(), LVec.end(), RVec->begin(), RVec->end(),
                   std::back_inserter(JoinVec));
    if (JoinVec.size() > N) {
      return AllBottom<l_t>{};
    }

    return JoinEdgeFunction{std::move(JoinSeed), std::move(JoinVec)};
  }

  // --- data members
  l_t Seed{};
  llvm::SmallVector<EdgeFunction<l_t>, N> OtherEF{};
};

//
// --- nontrivial join impls
//

/// Default implementation of EdgeFunction::join. Considers AllBottom, AllTop
/// and EdgeIdentity for OtherFunction as well as This==OtherFunction.
/// Joining with EdgeIdentity will overapproximate to (AllBottom if N==0, else
/// JoinEdgeFunction).
template <typename L, typename JLattice = JoinLatticeTraits<L>, uint8_t N = 0,
          typename ConcreteEF>
EdgeFunction<L> defaultJoinOrNull(EdgeFunctionRef<ConcreteEF> This,
                                  const EdgeFunction<L> &OtherFunction) {
  if (llvm::isa<AllBottom<L>>(OtherFunction)) {
    return OtherFunction;
  }
  if (llvm::isa<AllTop<L>>(OtherFunction) || OtherFunction == This) {
    return This;
  }
  if (llvm::isa<EdgeIdentity<L>>(OtherFunction)) {
    if constexpr (N > 0) {
      return JoinEdgeFunction<L, N, JLattice>::create(This, OtherFunction);
    } else {
      return AllBottom<L>{};
    }
  }
  return nullptr;
}

template <typename L>
EdgeFunction<L> EdgeIdentity<L>::join(EdgeFunctionRef<EdgeIdentity> This,
                                      const EdgeFunction<L> &OtherFunction) {
  if (llvm::isa<EdgeIdentity<L>>(OtherFunction) ||
      llvm::isa<AllTop<L>>(OtherFunction)) {
    return This;
  }
  if (llvm::isa<AllBottom<L>>(OtherFunction)) {
    return OtherFunction;
  }

  // do not know how to join; hence ask other function to decide on this
  return OtherFunction.joinWith(This);
}

template <typename L>
EdgeFunction<L>
ConstantEdgeFunction<L>::join(EdgeFunctionRef<ConstantEdgeFunction> This,
                              const EdgeFunction<l_t> &OtherFunction) {
  if (auto Default = defaultJoinOrNull<l_t, JLattice>(This, OtherFunction)) {
    return Default;
  }
  if (!OtherFunction.isConstant()) {
    // do not know how to join; hence ask other function to decide on this
    return OtherFunction.joinWith(This);
  }

  auto OtherVal = OtherFunction.computeTarget(JLattice::top());
  auto JoinedVal = JLattice::join(This->Value, OtherVal);

  if (JLattice::bottom() == JoinedVal) {
    return AllBottom<l_t>{};
  }
  if constexpr (!EdgeFunctionBase::IsSOOCandidate<ConstantEdgeFunction>) {
    if (JoinedVal == OtherVal) {
      return OtherFunction;
    }
    if (JoinedVal == This->Value) {
      return This;
    }
  }
  return ConstantEdgeFunction{
      NonTopBotValue<l_t>::unwrap(std::move(JoinedVal))};
}

template <typename L, uint8_t N, typename JLattice>
EdgeFunction<L>
JoinEdgeFunction<L, N, JLattice>::join(EdgeFunctionRef<JoinEdgeFunction> This,
                                       const EdgeFunction<l_t> &OtherFunction) {
  if (auto Default = defaultJoinOrNull<L, JLattice, N>(This, OtherFunction)) {
    return Default;
  }

  return create(This, OtherFunction);
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H
