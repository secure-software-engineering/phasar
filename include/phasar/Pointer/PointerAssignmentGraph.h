#pragma once

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

namespace psr {

namespace pag {
struct Assign {};
struct Gep {
  std::optional<int16_t> Offset;
};
struct Call {
  uint16_t ArgNo;
};
struct Return {};
struct Load {};
struct Store {};
struct StorePOI {};
struct Copy {};

struct Edge : public std::variant<Assign, Gep, Call, Return, Load, Store,
                                  StorePOI, Copy> {
  using Base =
      std::variant<Assign, Gep, Call, Return, Load, Store, StorePOI, Copy>;
  using std::variant<Assign, Gep, Call, Return, Load, Store, StorePOI,
                     Copy>::variant;

  template <typename T> static constexpr size_t kindOf() noexcept {
    return psr::variant_idx<Base, T>;
  }

  [[nodiscard]] constexpr size_t kind() const noexcept { return this->index(); }

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

  template <typename HandlerFn>
  constexpr decltype(auto) apply(HandlerFn &&Handler) const {
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

template <typename T>
concept PBStrategy =
    requires(T &Strategy, const T &CStrategy, ValueId From, ValueId To,
             const typename T::n_t &Inst, pag::Edge E) {
      typename T::v_t;
      typename T::db_t;
      typename T::f_t;
      typename T::n_t;

      { T() } noexcept;
      Strategy.onAddEdge(From, To, E, Nullable<typename T::n_t>{});
      CStrategy.withCalleesOfCallAt(Inst, [](typename T::f_t Callee) {});
    };

template <typename T>
concept CanOnAddValue = requires(T &Strategy, const T::v_t &Var) {
  Strategy.onAddValue(Var, ValueId{});
};

template <typename T>
concept CanGetNumPossibleValues =
    requires(const T &CStrategy, const T::db_t &IRDB) {
      {
        CStrategy.getNumPossibleValues(IRDB)
      } noexcept -> std::convertible_to<size_t>;
    };

template <PBStrategy FirstT, PBStrategy SecondT>
  requires(std::same_as<typename FirstT::n_t, typename SecondT::n_t> &&
           std::same_as<typename FirstT::v_t, typename SecondT::v_t> &&
           std::same_as<typename FirstT::f_t, typename SecondT::f_t> &&
           std::same_as<typename FirstT::db_t, typename SecondT::db_t>)
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
    if constexpr (CanGetNumPossibleValues<SecondT>) {
      return Second.getNumPossibleValues(IRDB);
    }
    if constexpr (CanGetNumPossibleValues<FirstT>) {
      return First.getNumPossibleValues(IRDB);
    }
  }

  constexpr void
  withCalleesOfCallAt(ByConstRef<n_t> CS,
                      llvm::function_ref<void(f_t)> WithCallee) const {
    Second.withCalleesOfCallAt(CS, WithCallee);
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
  static void getNumPossibleValuesThunk(const void *Ctx,
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
