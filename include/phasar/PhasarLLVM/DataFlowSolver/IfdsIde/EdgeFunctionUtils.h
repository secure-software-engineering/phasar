/******************************************************************************
 * Copyright (c) 2022 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H_
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/JoinLattice.h"
#include "phasar/PhasarLLVM/Utils/ByRef.h"
#include <memory>

namespace psr {
template <typename L>
class EdgeIdentity : public EdgeFunction<L>,
                     public std::enable_shared_from_this<EdgeIdentity<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &EI) = delete;
  EdgeIdentity &operator=(const EdgeIdentity &EI) = delete;
  ~EdgeIdentity() override = default;

  L computeTarget(L Source) override { return Source; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    return SecondFunction;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    return this == Other.get();
  }

  static ByConstRef<EdgeFunctionPtrType> getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeFunctionPtrType Instance(new EdgeIdentity<L>());
    return Instance;
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug = false*/) const override {
    OS << "EdgeIdentity";
  }
};

template <typename L, typename = std::enable_if_t<HasJoinLatticeTraits<L>>>
class ConstantEdgeFunction
    : public EdgeFunction<L>,
      public std::enable_shared_from_this<ConstantEdgeFunction<L>> {
public:
  using StoredType = typename NonTopBotValue<L>::type;
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  explicit ConstantEdgeFunction(StoredType Value) noexcept(
      std::is_nothrow_move_constructible_v<StoredType>)
      : Value(std::move(Value)) {}

  L computeTarget(L /*Source*/) override { return Value; }

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override;
  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override;

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto *OtherConst = dynamic_cast<ConstantEdgeFunction *>(Other.get())) {
      return OtherConst->Value == Value;
    }
    return false;
  }

  [[nodiscard]] bool isConstant() const noexcept override { return true; }

  void print(llvm::raw_ostream &OS,
             [[maybe_unused]] bool IsForDebug = false) const override {
    if constexpr (is_llvm_printable_v<L>) {
      OS << "ConstantEF(" << Value << ')';
    } else {
      OS << "ConstantEF";
    }
  }

protected:
  StoredType Value;
};

namespace detail {
template <typename L, typename = void>
class AllBottomBase : public EdgeFunction<L> {
  const L BottomElement;

public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  AllBottomBase(L BottomElement) noexcept(
      std::is_nothrow_move_constructible_v<L>)
      : BottomElement(std::move(BottomElement)) {}

  L computeTarget(L /*Source*/) override { return BottomElement; }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto *AB = dynamic_cast<AllBottomBase<L> *>(Other.get())) {
      return (AB->BottomElement == BottomElement);
    }
    return false;
  }
};

template <typename L>
class AllBottomBase<L, std::enable_if_t<HasJoinLatticeTraits<L>>>
    : public EdgeFunction<L> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  L computeTarget(L /*Source*/) override {
    return JoinLatticeTraits<L>::bottom();
  }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    return !!dynamic_cast<AllBottomBase<L> *>(Other.get());
  }
};

template <typename L, typename = void>
class AllTopBase : public EdgeFunction<L> {
  const L TopElement;

public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  AllTopBase(L TopElement) noexcept(std::is_nothrow_move_constructible_v<L>)
      : TopElement(std::move(TopElement)) {}

  L computeTarget(L /*Source*/) override { return TopElement; }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    if (auto *AB = dynamic_cast<AllTopBase<L> *>(Other.get())) {
      return (AB->TopElement == TopElement);
    }
    return false;
  }
};

template <typename L>
class AllTopBase<L, std::enable_if_t<HasJoinLatticeTraits<L>>>
    : public EdgeFunction<L> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  L computeTarget(L /*Source*/) override {
    return JoinLatticeTraits<L>::bottom();
  }

  [[nodiscard]] bool equal_to // NOLINT - would break too many client analyses
      (EdgeFunctionPtrType Other) const override {
    return !!dynamic_cast<AllTopBase<L> *>(Other.get());
  }
};

} // namespace detail

template <typename L>
class AllBottom : public detail::AllBottomBase<L>,
                  public std::enable_shared_from_this<AllBottom<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  using detail::AllBottomBase<L>::AllBottomBase;

  ~AllBottom() override = default;

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    return SecondFunction->isConstant() ? SecondFunction
                                        : this->shared_from_this();
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType /*OtherFunction*/) override {
    return this->shared_from_this();
  }

  void print(llvm::raw_ostream &OS,
             bool /*IsForDebug = false*/) const override {
    OS << "AllBottom";
  }

  [[nodiscard]] bool isConstant() const noexcept override { return true; }

  template <typename LL = L,
            typename = std::enable_if_t<HasJoinLatticeTraits<LL>>>
  static ByConstRef<EdgeFunctionPtrType> getInstance() {
    static EdgeFunctionPtrType AllBotFn = std::make_shared<AllBottom>();
    return AllBotFn;
  }
};

template <typename L>
class AllTop : public detail::AllTopBase<L>,
               public std::enable_shared_from_this<AllTop<L>> {
public:
  using typename EdgeFunction<L>::EdgeFunctionPtrType;

  using detail::AllTopBase<L>::AllTopBase;

  ~AllTop() override = default;

  EdgeFunctionPtrType composeWith(EdgeFunctionPtrType SecondFunction) override {
    if (SecondFunction == EdgeIdentity<L>::getInstance()) {
      return this->shared_from_this();
    }
    return SecondFunction;
  }

  EdgeFunctionPtrType joinWith(EdgeFunctionPtrType OtherFunction) override {
    return OtherFunction;
  }

  void print(llvm::raw_ostream &OS,
             [[maybe_unused]] bool IsForDebug = false) const override {
    OS << "AllTop";
  }

  [[nodiscard]] bool isConstant() const noexcept override { return true; }
};

template <typename L>
auto EdgeIdentity<L>::joinWith(EdgeFunctionPtrType OtherFunction)
    -> EdgeFunctionPtrType {
  if ((OtherFunction.get() == this) ||
      OtherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AB = dynamic_cast<AllBottom<L> *>(OtherFunction.get())) {
    return OtherFunction;
  }
  if (auto *AT = dynamic_cast<AllTop<L> *>(OtherFunction.get())) {
    return this->shared_from_this();
  }
  // do not know how to join; hence ask other function to decide on this
  return OtherFunction->joinWith(this->shared_from_this());
}

template <typename L, typename Enable>
auto ConstantEdgeFunction<L, Enable>::joinWith(
    EdgeFunctionPtrType OtherFunction) -> EdgeFunctionPtrType {
  if (this == OtherFunction.get() || equal_to(OtherFunction)) {
    return OtherFunction;
  }

  if (!OtherFunction->isConstant()) {
    return AllBottom<L>::getInstance();
  }

  auto OtherVal = OtherFunction->computeTarget(JoinLatticeTraits<L>::top());
  auto JoinedVal = JoinLatticeTraits<L>::join(Value, OtherVal);

  if (JoinLatticeTraits<L>::bottom() == JoinedVal) {
    return AllBottom<L>::getInstance();
  }

  if (JoinLatticeTraits<L>::top() == JoinedVal) [[unlikely]] {
    // XXX: Can this ever happen?
    return std::make_shared<AllTop<L>>();
  }

  if (JoinedVal == OtherVal) {
    return OtherFunction;
  }
  if (JoinedVal == Value) {
    return this->shared_from_this();
  }

  return std::make_shared<ConstantEdgeFunction<L>>(
      NonTopBotValue<L>::unwrap(std::move(JoinedVal)));
}

template <typename L, typename Enable>
auto ConstantEdgeFunction<L, Enable>::composeWith(
    EdgeFunctionPtrType SecondFunction) -> EdgeFunctionPtrType {
  if (SecondFunction == EdgeIdentity<L>::getInstance()) {
    return this->shared_from_this();
  }

  if (SecondFunction->isConstant()) {
    return SecondFunction;
  }

  auto NextVal = SecondFunction->computeTarget(Value);
  if (NextVal == Value) {
    return this->shared_from_this();
  }

  if (NextVal == JoinLatticeTraits<L>::bottom()) {
    return AllBottom<L>::getInstance();
  }
  if (NextVal == JoinLatticeTraits<L>::top()) [[unlikely]] {
    // XXX: Can this ever happen?
    return std::make_shared<AllTop<L>>();
  }

  return std::make_shared<ConstantEdgeFunction<L>>(
      NonTopBotValue<L>::unwrap(std::move(NextVal)));
}

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_EDGEFUNCTIONUTILS_H_
