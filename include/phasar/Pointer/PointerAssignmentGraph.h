#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/Support/Compiler.h"

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace psr {

namespace pag {
/// Direct pointer assignment: `To = From`.
struct Assign {};
/// GEP (field-access) edge. \p Offset holds the byte offset when statically
/// known; \c std::nullopt for non-constant GEPs.
struct Gep {
  std::optional<int16_t> Offset;
};
/// Passing an argument at a call site. \p ArgNo is the 0-based parameter index.
struct Call {
  uint16_t ArgNo;
};
/// Return value flows from callee to caller call-site.
struct Return {};
/// Load of a pointer: `To = *From`.
struct Load {};
/// Store of a non-aliased pointer: `*To = From`.
struct Store {};
/// Store of a potentially aliased pointer: `*To = From`.
struct StorePOI {};
/// Block-copy of aggregate memory (e.g., memcpy/memmove): all pointer fields
/// inside the source are assumed to flow to the corresponding fields of the
/// destination.
struct Copy {};

/// Tagged union of all PAG edge kinds, see above.
struct Edge : public std::variant<Assign, Gep, Call, Return, Load, Store,
                                  StorePOI, Copy> {
  using Base =
      std::variant<Assign, Gep, Call, Return, Load, Store, StorePOI, Copy>;
  using std::variant<Assign, Gep, Call, Return, Load, Store, StorePOI,
                     Copy>::variant;

  enum class EdgeKind : uint32_t {};

  template <typename T> static constexpr EdgeKind kindOf() noexcept {
    return EdgeKind(psr::variant_idx<Base, T>);
  }

  [[nodiscard]] constexpr EdgeKind kind() const noexcept {
    return EdgeKind(this->index());
  }

  template <typename T, typename... Ts>
  [[nodiscard]] constexpr bool isa() const noexcept {
    return std::holds_alternative<T>(*this) ||
           (... || std::holds_alternative<Ts>(*this));
  }

  template <typename T>
  [[nodiscard]] constexpr std::optional<T> dyn_cast() const noexcept {
    if (const auto *Ptr = std::get_if<T>(this)) {
      return *Ptr;
    }

    return std::nullopt;
  }

  template <typename T> [[nodiscard]] constexpr T cast() const noexcept {
    assert(isa<T>() && "Invalid cast!");
    const auto *Ptr = std::get_if<T>(this);
    return *Ptr;
  }

  template <typename HandlerFn>
  LLVM_ATTRIBUTE_ALWAYS_INLINE constexpr decltype(auto)
  apply(HandlerFn &&Handler) const {
    return std::visit(PSR_FWD(Handler), *this);
  }

  [[nodiscard]] friend constexpr llvm::StringRef to_string(Edge E) noexcept {
    return E.apply(psr::Overloaded{
        [](Assign) { return "Assign"; },
        // TODO: Print offset, if present
        [](Gep) { return "Gep"; },
        // TODO: Print ArgNo
        [](Call) { return "Call"; },
        [](Return) { return "Return"; },
        [](Load) { return "Load"; },
        [](Store) { return "Store"; },
        [](StorePOI) { return "StorePOI"; },
        [](Copy) { return "Copy"; },
    });
  }
};

template <typename T, typename N>
concept CanOnAddEdgeN =
    requires(T &Strategy, ValueId From, ValueId To, pag::Edge E) {
      Strategy.onAddEdge(From, To, E, Nullable<N>{});
    };

template <typename T>
concept CanOnAddEdge = CanOnAddEdgeN<T, typename T::n_t>;

template <typename T>
concept CanWithCalleesOfCallAt =
    requires(const T &CStrategy, const typename T::n_t &Inst) {
      CStrategy.withCalleesOfCallAt(Inst, [](typename T::f_t Callee) {});
    };

/// Minimal requirements for a PAG-builder strategy: analysis-domain type
/// members plus \c withCalleesOfCallAt.  Does not require \c onAddEdge; use
/// \c PBStrategy for the full requirement set.
template <typename T>
concept PBStrategyBase =
    CanWithCalleesOfCallAt<T> &&
    requires(T &Strategy, const T &CStrategy, ValueId From, ValueId To,
             const typename T::n_t &Inst, pag::Edge E) {
      typename T::v_t;
      typename T::db_t;
      typename T::f_t;
      typename T::n_t;
    };

template <typename T>
concept PBStrategy = CanOnAddEdge<T> && PBStrategyBase<T>;

template <typename T>
concept CanOnAddValue = requires(T &Strategy, const T::v_t &Var) {
  Strategy.onAddValue(Var, ValueId{});
};

/// A PAG-strategy that can provide a capacity hint for the
/// number of PAG nodes (used to pre-allocate internal buffers).
template <typename T>
concept CanGetNumPossibleValues =
    requires(const T &CStrategy, const T::db_t &IRDB) {
      {
        CStrategy.getNumPossibleValues(IRDB)
      } noexcept -> std::convertible_to<size_t>;
    };

template <typename T>
concept CanConsumeAAResults =
    requires(T Strategy) { std::move(Strategy).consumeAAResults(size_t(0)); };

/// Fans out every PAG-builder callback to two independent strategies.
/// \p FirstT must be a full \c PBStrategy; \p SecondT only needs to satisfy
/// \c CanOnAddEdge (it borrows the \c withCalleesOfCallAt and analysis-domain
/// types from \p FirstT).
///
/// Useful for intersecting alias analyses.
template <PBStrategy FirstT, CanOnAddEdgeN<typename FirstT::n_t> SecondT>
// requires(std::same_as<typename FirstT::n_t, typename SecondT::n_t> &&
//          std::same_as<typename FirstT::v_t, typename SecondT::v_t> &&
//          std::same_as<typename FirstT::f_t, typename SecondT::f_t> &&
//          std::same_as<typename FirstT::db_t, typename SecondT::db_t>)
struct PBStrategyCombinator {
  using n_t = typename FirstT::n_t;
  using v_t = typename FirstT::v_t;
  using f_t = typename FirstT::f_t;
  using db_t = typename FirstT::db_t;

  [[no_unique_address]] FirstT First;
  [[no_unique_address]] SecondT Second;

  constexpr void onAddEdge(ValueId From, ValueId To, Edge E,
                           Nullable<n_t> AtInstruction) {
    First.onAddEdge(From, To, E, AtInstruction);
    Second.onAddEdge(From, To, E, AtInstruction);
  }

  constexpr void onAddValue(ByConstRef<v_t> Variable, ValueId VId)
    requires(CanOnAddValue<FirstT> || CanOnAddValue<SecondT>)
  {
    if constexpr (CanOnAddValue<FirstT>) {
      First.onAddValue(Variable, VId);
    }
    if constexpr (CanOnAddValue<SecondT>) {
      Second.onAddValue(Variable, VId);
    }
  }

  [[nodiscard]] constexpr size_t
  getNumPossibleValues(const db_t &IRDB) const noexcept
    requires(CanGetNumPossibleValues<FirstT> ||
             CanGetNumPossibleValues<SecondT>)
  {
    const auto Retrieve = [&IRDB]<typename S>(const S &Strategy) {
      if constexpr (CanGetNumPossibleValues<S>) {
        return Strategy.getNumPossibleValues(IRDB);
      }
      return 0;
    };

    return std::max(Retrieve(First), Retrieve(Second));
  }

  constexpr void
  withCalleesOfCallAt(ByConstRef<n_t> CS,
                      llvm::function_ref<void(f_t)> WithCallee) const {
    // We assume, both are on the same call-graph.
    if constexpr (CanWithCalleesOfCallAt<SecondT>) {
      Second.withCalleesOfCallAt(CS, WithCallee);
    } else {
      First.withCalleesOfCallAt(CS, WithCallee);
    }
  }

  constexpr auto consumeAAResults(size_t NumVars) &&
      requires(CanConsumeAAResults<FirstT> && !CanConsumeAAResults<SecondT>) {
        return std::move(First).consumeAAResults(NumVars);
      }
};

/// Upgrades a \c CanOnAddEdge type to a full \c PBStrategy by mixing in the
/// analysis-domain types and \c withCalleesOfCallAt from a separate
/// \c PBStrategyBase.  Typical use: wrap an edge-listener (e.g., a union-find
/// accumulator) that does not own a call-graph with a call-graph provider.
///
/// \tparam FirstT  The edge-listener. Must satisfy \c CanOnAddEdge.
/// \tparam SecondT The call-graph / domain provider. Must satisfy
///                 \c PBStrategyBase.
template <CanOnAddEdge FirstT, PBStrategyBase SecondT>
struct PBMixin : public FirstT {
  using n_t = typename SecondT::n_t;
  using v_t = typename SecondT::v_t;
  using f_t = typename SecondT::f_t;
  using db_t = typename SecondT::db_t;

  [[no_unique_address]] SecondT Second;

  [[nodiscard]] constexpr size_t
  getNumPossibleValues(const db_t &IRDB) const noexcept
    requires(!CanGetNumPossibleValues<FirstT> &&
             CanGetNumPossibleValues<SecondT>)
  {
    return Second.getNumPossibleValues(IRDB);
  }

  constexpr void
  withCalleesOfCallAt(ByConstRef<n_t> CS,
                      llvm::function_ref<void(f_t)> WithCallee) const {
    Second.withCalleesOfCallAt(CS, WithCallee);
  }
};

/// Debug-only PAG-builder strategy that emits the PAG as a Graphviz DOT
/// digraph to \c llvm::errs().  The opening \c "digraph PAG {" is printed in
/// the constructor and the closing brace in the destructor (move-construction
/// transfers ownership, preventing a double-close).
struct TracingPBStrategy {
  bool Engaged = true;

  TracingPBStrategy() { llvm::errs() << "digraph PAG {\n"; }
  ~TracingPBStrategy() {
    if (Engaged) {
      llvm::errs() << "}\n";
    }
  }

  constexpr TracingPBStrategy(TracingPBStrategy &&Other) noexcept
      : Engaged(std::exchange(Other.Engaged, false)) {}
  constexpr TracingPBStrategy &operator=(TracingPBStrategy &&Other) noexcept {
    Engaged = std::exchange(Other.Engaged, false);
    return *this;
  }

  void onAddEdge(ValueId From, ValueId To, Edge E,
                 const auto & /*AtInstruction*/) const {
    llvm::errs() << "  " << uint32_t(From) << "->" << uint32_t(To)
                 << "[label=\"";
    llvm::errs().write_escaped(to_string(E)) << "\"];\n";
  }
};

/// The main customization point for PAG building.
/// Implement this interface to provide several callbacks to the PAGBuilder,
/// specifying how the PAG should be built.
template <typename AnalysisDomainT> class PBStrategyRef final {
public:
  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  /// Creates a type-erased PBStrategyRef from a non-null pointer to a concrete
  /// Strategy-object.
  template <PBStrategy ConcreteStrategyT>
    requires(!same_as_decay<PBStrategyRef, ConcreteStrategyT>)
  constexpr PBStrategyRef(ConcreteStrategyT *Strategy) noexcept
      : VT(&VTableFor<ConcreteStrategyT>), Ctx(Strategy) {
    assert(Strategy != nullptr);
  }

  /// Called by buildPAG() for every (unique) edge that should be added to the
  /// PAG
  ///
  /// \param From The source-node.
  /// \param To The destination-node.
  /// \param E The edge-label that specifies the kind of data-flow that is
  /// denoted by this edge.
  /// \param AtInstruction Optionally holds the instruction that caused this
  /// edge to be created.
  void onAddEdge(ValueId From, ValueId To, pag::Edge E,
                 Nullable<n_t> AtInstruction) {
    VT->OnAddEdge(Ctx, From, To, E, AtInstruction);
  }

  /// Called by buildPAG() for every (unique) node that should be added to the
  /// PAG, *excluding* nodes that have already been registered in the used
  /// ValueCompressor before buildPAG() was called.
  ///
  /// \param Variable The IR-specific variable/value for which the new node
  /// has been created.
  /// \param VId The value-id of the newly created node.
  void onAddValue(ByConstRef<v_t> Variable, ValueId VId) {
    VT->OnAddValue(Ctx, Variable, VId);
  }

  /// Estimates the maximum number of values created in the ValueCompressor.
  /// Must not be exact, this is just for pre-allocating buffers for
  /// optimization purposes.
  [[nodiscard]] size_t getNumPossibleValues(const db_t &IRDB) const noexcept {
    return VT->GetNumPossibleValues(Ctx, IRDB);
  }

  /// Invokes a call-back with each function that may be called at the given
  /// call-site. Usually, you want to use a CallGraph or a Resolver to implement
  /// this function.
  ///
  /// \param CS The call-site.
  /// \param WithCallee The callback to be invoked for each callee.
  void withCalleesOfCallAt(ByConstRef<n_t> CS,
                           llvm::function_ref<void(f_t)> WithCallee) const {
    VT->WithCalleesOfCallAt(Ctx, CS, WithCallee);
  }

private:
  struct VTable {
    void (*OnAddEdge)(void *Ctx, ValueId From, ValueId To, pag::Edge E,
                      Nullable<n_t> AtInstruction);
    void (*OnAddValue)(void *Ctx, ByConstRef<v_t> Variable, ValueId VId);
    size_t (*GetNumPossibleValues)(const void *Ctx, const db_t &IRDB) noexcept;
    void (*WithCalleesOfCallAt)(const void *Ctx, ByConstRef<n_t> CS,
                                llvm::function_ref<void(f_t)> WithCallee);
  };

  template <typename ConcreteStrategyT>
  static void onAddEdgeThunk(void *Ctx, ValueId From, ValueId To, pag::Edge E,
                             Nullable<n_t> AtInstruction) {
    // Do things like allocating a new slot in the adjacency-list here
    static_cast<ConcreteStrategyT *>(Ctx)->onAddEdge(From, To, E,
                                                     AtInstruction);
  }

  template <typename ConcreteStrategyT>
  static void onAddValueThunk(void *Ctx, ByConstRef<v_t> Variable,
                              ValueId VId) {
    if constexpr (CanOnAddValue<ConcreteStrategyT>) {
      static_cast<ConcreteStrategyT *>(Ctx)->onAddValue(Variable, VId);
    }
  }

  template <typename ConcreteStrategyT>
  static size_t getNumPossibleValuesThunk(const void *Ctx,
                                          const db_t &IRDB) noexcept {
    if constexpr (CanGetNumPossibleValues<ConcreteStrategyT>) {
      return static_cast<const ConcreteStrategyT *>(Ctx)->getNumPossibleValues(
          IRDB);
    } else {
      // Fallback-heuristic
      return IRDB.getNumInstructions();
    }
  }

  template <typename ConcreteStrategyT>
  static void
  withCalleesOfCallAtThunk(const void *Ctx, ByConstRef<n_t> CS,
                           llvm::function_ref<void(f_t)> WithCallee) {

    static_cast<const ConcreteStrategyT *>(Ctx)->withCalleesOfCallAt(
        CS, WithCallee);
  }

  template <typename ConcreteStrategyT>
  static constexpr VTable VTableFor = {
      &onAddEdgeThunk<ConcreteStrategyT>,
      &onAddValueThunk<ConcreteStrategyT>,
      &getNumPossibleValuesThunk<ConcreteStrategyT>,
      &withCalleesOfCallAtThunk<ConcreteStrategyT>,
  };

  const VTable *VT{};
  void *Ctx{};
};
} // namespace pag

/// A utility-class that can be used to build a pointer-assignment graph (PAG).
///
/// This class does not enforce a specifiy graph-layout -- it does not even
/// require that you explicitly store a graph in memory. Instead, you get
/// notified about every node and edge that should be created in the PAG.
///
/// \tparam AnalysisDomainT The analysis domain to use.
template <typename AnalysisDomainT> class PAGBuilder {
public:
  using n_t = typename AnalysisDomainT::n_t;
  using v_t = typename AnalysisDomainT::v_t;
  using f_t = typename AnalysisDomainT::f_t;
  using db_t = typename AnalysisDomainT::db_t;

  constexpr PAGBuilder() noexcept = default;
  virtual ~PAGBuilder() = default;

  /// Iterates the passed IRDB to build a PAG, based on the given Strategy.
  ///
  /// \param IRDB The IR program to analyze.
  /// \param VC The value-compressor to use for assigning unique sequential
  /// integer-ids to each PAG-node. This VC may be pre-populated. You can expect
  /// that buildPAG() will respect the pre-population in that case. Otherwise,
  /// ids will be assigned in visitation order (which is not stable and should
  /// not be relied on!).
  /// \param Strategy The customization-point for this function. See the docs of
  /// PBStrategy for more information.
  virtual void buildPAG(const db_t &IRDB, ValueCompressor<v_t> &VC,
                        pag::PBStrategyRef<AnalysisDomainT> Strategy) = 0;
};

} // namespace psr
