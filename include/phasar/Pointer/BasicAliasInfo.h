/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_POINTER_BASICALIASINFO_H
#define PHASAR_POINTER_BASICALIASINFO_H

#include "phasar/Pointer/AliasInfoTraits.h"
#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <type_traits>

namespace psr {
template <typename V> using BasicAliasHandler = llvm::function_ref<void(V)>;

#if __cplusplus >= 202002L
template <typename AI>
concept IsBasicAliasInfo =
    requires(const AI &AInfo, typename AliasInfoTraits<AI>::v_t Fact,
             typename AliasInfoTraits<AI>::n_t AtInst,
             BasicAliasHandler<typename AliasInfoTraits<AI>::v_t> WithAlias) {
  AInfo.foreachAliasOf(Fact, AtInst, WithAlias);
};
#else
namespace detail {
template <typename AI, typename = void>
struct BasicAliasInfoTest : std::false_type {};
template <typename AI>
struct BasicAliasInfoTest<
    AI,
    std::void_t<decltype(std::declval<const AI &>().foreachAliasOf(
        std::declval<typename AliasInfoTraits<AI>::v_t>(),
        std::declval<typename AliasInfoTraits<AI>::n_t>(),
        std::declval<BasicAliasHandler<typename AliasInfoTraits<AI>::v_t>>()))>>
    : std::true_type {};
} // namespace detail

template <typename AI>
PSR_CONCEPT IsBasicAliasInfo = detail::BasicAliasInfoTest<AI>::value;
#endif

template <typename V, typename N> class BasicAliasInfoRef {
public:
  using v_t = V;
  using n_t = N;
  using handler_t = BasicAliasHandler<v_t>;

  template <typename AI, typename = std::enable_if_t<IsBasicAliasInfo<AI>>>
  BasicAliasInfoRef(AI *AInfo) noexcept
      : AInfo(AInfo), ForeachAliasOf([](void *AInfo, v_t Fact, n_t AtInst,
                                        handler_t Handler) {
          static_cast<const AI *>(AInfo)->foreachAliasOf(Fact, AtInst, Handler);
        }) {}

  explicit BasicAliasInfoRef(void *AInfo,
                             void (*ForeachAliasOf)(void *, ByConstRef<v_t>,
                                                    ByConstRef<n_t>,
                                                    handler_t)) noexcept
      : AInfo(AInfo), ForeachAliasOf(ForeachAliasOf) {}

  void foreachAliasOf(v_t Fact, n_t AtInst, handler_t Handler) const {
    ForeachAliasOf(AInfo, Fact, AtInst, Handler);
  }

private:
  void *AInfo;
  void (*ForeachAliasOf)(void *, ByConstRef<v_t>, ByConstRef<n_t>, handler_t);
};

template <typename V, typename N>
struct AliasInfoTraits<BasicAliasInfoRef<V, N>> {
  using v_t = V;
  using n_t = N;
};

} // namespace psr

#endif // PHASAR_POINTER_BASICALIASINFO_H
