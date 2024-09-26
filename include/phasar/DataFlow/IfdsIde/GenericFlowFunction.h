/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_GENERICFLOWFUNCTION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_GENERICFLOWFUNCTION_H

#include "phasar/DataFlow/IfdsIde/FlowFunctions.h"

namespace psr {
/// Encapsulates an unmanaged pointer to a FlowFunction
template <typename D, typename Container = std::set<D>>
class GenericFlowFunctionView {
public:
  using FlowFunctionType = FlowFunction<D, Container>;
  using FlowFunctionPtrType = std::unique_ptr<FlowFunctionType>;

  using container_type = Container;
  using value_type = typename container_type::value_type;

  GenericFlowFunctionView() noexcept = default;
  GenericFlowFunctionView(FlowFunctionType *FF) noexcept : FF(FF) {}

  GenericFlowFunctionView(const GenericFlowFunctionView &) noexcept = default;
  GenericFlowFunctionView &
  operator=(const GenericFlowFunctionView &) noexcept = default;

  ~GenericFlowFunctionView() = default;

  [[nodiscard]] container_type computeTargets(D Source) const {
    assert(FF != nullptr);
    return FF->computeTargets(std::move(Source));
  }

  explicit operator bool() const noexcept { return FF; }

  [[nodiscard]] bool operator==(GenericFlowFunctionView Other) const noexcept {
    return FF == Other.FF;
  }
  [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
    return FF == nullptr;
  }
  [[nodiscard]] bool operator!=(GenericFlowFunctionView Other) const noexcept {
    return !(*this == Other);
  }
  [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept { return FF; }

private:
  FlowFunctionType *FF = nullptr;
};

/// Encapsulates a managed pointer to a FlowFunction
template <typename D, typename Container = std::set<D>>
class GenericFlowFunction {
public:
  using FlowFunctionType = FlowFunction<D, Container>;
  using FlowFunctionPtrType = typename FlowFunctionType::FlowFunctionPtrType;

  using container_type = Container;
  using value_type = typename container_type::value_type;

  GenericFlowFunction() noexcept = default;
  GenericFlowFunction(FlowFunctionPtrType FF) noexcept : FF(std::move(FF)) {}
  template <typename T, typename = std::enable_if_t<std::is_base_of_v<
                            FlowFunctionType, std::decay_t<T>>>>
  GenericFlowFunction(T &&FF)
      : FF(std::make_unique<std::decay_t<T>>(std::forward<T>(FF))) {}

  template <typename T, typename... ArgTys>
  explicit GenericFlowFunction(std::in_place_type_t<T> /*unused*/,
                               ArgTys &&...Args)
      : FF(std::make_unique<T>(std::forward<ArgTys>(Args)...)) {}

  GenericFlowFunction(GenericFlowFunction &&) noexcept = default;
  GenericFlowFunction &operator=(GenericFlowFunction &&) noexcept = default;

  GenericFlowFunction(const GenericFlowFunction &) = delete;
  GenericFlowFunction &operator=(const GenericFlowFunction &) = delete;

  ~GenericFlowFunction() = default;

  [[nodiscard]] container_type computeTargets(D Source) const {
    assert(FF != nullptr);
    return FF->computeTargets(std::move(Source));
  }

  explicit operator bool() const noexcept { return FF; }

  operator GenericFlowFunctionView<D, Container>() const noexcept {
    return FF.get();
  }

  [[nodiscard]] bool
  operator==(GenericFlowFunctionView<D, Container> Other) const noexcept {
    return FF == Other.FF;
  }
  [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
    return FF == nullptr;
  }
  [[nodiscard]] bool
  operator!=(GenericFlowFunctionView<D, Container> Other) const noexcept {
    return !(*this == Other);
  }
  [[nodiscard]] bool operator!=(std::nullptr_t) const noexcept { return FF; }

private:
  FlowFunctionPtrType FF;
};

} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_GENERICFLOWFUNCTION_H
