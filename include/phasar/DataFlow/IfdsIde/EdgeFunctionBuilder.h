/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/DataFlow/IfdsIde/DefaultEdgeFunctionSingletonCache.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/FunctionExtras.h"

#include <type_traits>
#include <utility>

namespace psr {

template <typename L> class EdgeFunctionFactory {
public:
  EdgeFunctionFactory() noexcept = default;

  EdgeFunctionFactory(const EdgeFunctionFactory &) = delete;
  EdgeFunctionFactory &operator=(const EdgeFunctionFactory &) = delete;

  EdgeFunctionFactory(EdgeFunctionFactory &&) noexcept = default;
  EdgeFunctionFactory &operator=(EdgeFunctionFactory &&) noexcept = default;

  ~EdgeFunctionFactory() {
    for (auto [Deleter, Cache] : EFCaches) {
      assert(Deleter);
      assert(Cache);
      (*Deleter)(Cache);
    }
    EFCaches.clear();
  }

  template <typename ConcreteEF, typename... ArgTys>
  [[nodiscard]] EdgeFunction<L> create(ArgTys &&...Args) {
    if constexpr (is_llvm_hashable_v<ConcreteEF>) {
      return CachedEdgeFunction<ConcreteEF>{{std::forward<ArgTys>(Args)...},
                                            getEFCache<ConcreteEF>()};
    } else {
      return EdgeFunction<L>(std::in_place_type<ConcreteEF>,
                             std::forward<ArgTys>(Args)...);
    }
  }

private:
  template <typename ConcreteEF>
  static constexpr auto DeleterFor = [](const void *EFCache) {
    delete static_cast<const EdgeFunctionSingletonCache<ConcreteEF> *>(EFCache);
  };

  template <typename ConcreteEF>
  [[nodiscard]] EdgeFunctionSingletonCache<ConcreteEF> *getEFCache() {
    auto [It, Inserted] =
        EFCaches.try_emplace(&DeleterFor<ConcreteEF>, nullptr);
    if (Inserted) {
      It->second = new DefaultEdgeFunctionSingletonCache<ConcreteEF>();
    }

    return static_cast<DefaultEdgeFunctionSingletonCache<ConcreteEF>>(
        It->second);
  }

  llvm::SmallDenseMap<void (**)(const void *), void *> EFCaches{};
};

template <typename L> class EdgeFunctionBuilder {
public:
  template <typename T,
            typename = std::enable_if_t<IsEdgeFunction<std::decay_t<T>>>>
  EdgeFunctionBuilder(T &&EF)
      : Builder([EF{std::forward<T>(EF)}](EdgeFunctionFactory<L> &) mutable {
          return std::move(EF);
        }) {}

  template <typename Fun, typename = std::enable_if_t<std::is_invocable_v<
                              Fun, EdgeFunctionFactory<L> &>>>
  EdgeFunctionBuilder(Fun F) : Builder(std::move(F)) {}

  auto operator()(EdgeFunctionFactory<L> &Factory) && {
    return std::move(Builder)(Factory);
  }

private:
  llvm::unique_function<EdgeFunction<L>(EdgeFunctionFactory<L> &)> Builder;
};
} // namespace psr