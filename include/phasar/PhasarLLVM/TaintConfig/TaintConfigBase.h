/******************************************************************************
 * Copyright (c) 2023 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGBASE_H
#define PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGBASE_H

#include "phasar/Utils/Nullable.h"

#include "llvm/ADT/FunctionExtras.h"

#include <map>
#include <set>
#include <type_traits>
#include <utility>

namespace psr {

enum class TaintCategory { Source, Sink, Sanitizer, None };

[[nodiscard]] llvm::StringRef to_string(TaintCategory Cat) noexcept;

template <typename Derived, typename AnalysisDomainTy> class TaintConfigBase {
public:
  using n_t = typename AnalysisDomainTy::n_t;
  using v_t = typename AnalysisDomainTy::v_t;
  using f_t = typename AnalysisDomainTy::f_t;

  using TaintDescriptionCallBackTy = llvm::unique_function<std::set<v_t>(n_t)>;

  void registerSourceCallBack(TaintDescriptionCallBackTy CB) noexcept {
    SourceCallBack = std::move(CB);
  }
  void registerSinkCallBack(TaintDescriptionCallBackTy CB) noexcept {
    SinkCallBack = std::move(CB);
  }
  void registerSanitizerCallBack(TaintDescriptionCallBackTy CB) noexcept {
    SanitizerCallBack = std::move(CB);
  }

  [[nodiscard]] const TaintDescriptionCallBackTy &
  getRegisteredSourceCallBack() const noexcept {
    return SourceCallBack;
  }
  [[nodiscard]] const TaintDescriptionCallBackTy &
  getRegisteredSinkCallBack() const noexcept {
    return SinkCallBack;
  }
  [[nodiscard]] const TaintDescriptionCallBackTy &
  getRegisteredSanitizerCallBack() const noexcept {
    return SanitizerCallBack;
  }

  [[nodiscard]] bool isSource(v_t Val) const {
    return self().isSourceImpl(std::move(Val));
  }
  [[nodiscard]] bool isSink(v_t Val) const {
    return self().isSinkImpl(std::move(Val));
  }
  [[nodiscard]] bool isSanitizer(v_t Val) const {
    return self().isSanitizerImpl(std::move(Val));
  }

  /// \brief Calls Handler for all operands of Inst (maybe including Inst
  /// itself) that are generated unconditionally as tainted.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
  template <typename HandlerFn>
  void forAllGeneratedValuesAt(n_t Inst, Nullable<f_t> Callee,
                               HandlerFn &&Handler) const {
    self().forAllGeneratedValuesAtImpl(std::move(Inst), std::move(Callee),
                                       std::forward<HandlerFn>(Handler));
  }

  /// \brief Calls Handler for all operands of Inst that may generate a leak
  /// when they are tainted.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
  template <typename HandlerFn>
  void forAllLeakCandidatesAt(n_t Inst, Nullable<f_t> Callee,
                              HandlerFn &&Handler) const {
    self().forAllLeakCandidatesAtImpl(std::move(Inst), std::move(Callee),
                                      std::forward<HandlerFn>(Handler));
  }

  /// \brief Calls Handler for all operands of Inst that become sanitized after
  /// the instruction is completed.
  ///
  /// If Inst is a function-call, the Callee function should be specified
  /// explicitly.
  template <typename HandlerFn>
  void forAllSanitizedValuesAt(n_t Inst, Nullable<f_t> Callee,
                               HandlerFn &&Handler) const {
    self().forAllSanitizedValuesAtImpl(std::move(Inst), std::move(Callee),
                                       std::forward<HandlerFn>(Handler));
  }

  [[nodiscard]] bool generatesValuesAt(n_t Inst, Nullable<f_t> Callee) const {
    return self().generatesValuesAtImpl(std::move(Inst), std::move(Callee));
  }
  [[nodiscard]] bool mayLeakValuesAt(n_t Inst, Nullable<f_t> Callee) const {
    return self().mayLeakValuesAtImpl(std::move(Inst), std::move(Callee));
  }
  [[nodiscard]] bool sanitizesValuesAt(n_t Inst, Nullable<f_t> Callee) const {
    return self().sanitizesValuesAtImpl(std::move(Inst), std::move(Callee));
  }

  [[nodiscard]] TaintCategory getCategory(v_t Val) const {
    return self().getCategoryImpl(std::move(Val));
  }

  [[nodiscard]] std::map<n_t, std::set<v_t>> makeInitialSeeds() const {
    return self().makeInitialSeedsImpl();
  }

private:
  [[nodiscard]] Derived &self() noexcept {
    static_assert(std::is_base_of_v<TaintConfigBase, Derived>,
                  "Invalid CRTP instantiation!");
    return *static_cast<Derived *>(this);
  }
  [[nodiscard]] const Derived &self() const noexcept {
    static_assert(std::is_base_of_v<TaintConfigBase, Derived>,
                  "Invalid CRTP instantiation!");
    return *static_cast<const Derived *>(this);
  }

  // --- data members

  TaintDescriptionCallBackTy SourceCallBack{};
  TaintDescriptionCallBackTy SinkCallBack{};
  TaintDescriptionCallBackTy SanitizerCallBack{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_TAINTCONFIG_TAINTCONFIGBASE_H
