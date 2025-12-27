#pragma once

#include "phasar/Utils/ByRef.h"
#include "phasar/Utils/Macros.h"
#include "phasar/Utils/Nullable.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include <cstdint>
#include <optional>
#include <variant>

namespace psr {

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

    [[nodiscard]] constexpr size_t kind() const noexcept {
      return this->index();
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

  /// The main customization point for PAG building.
  /// Implement this interface to provide several callbacks to the PAGBuilder,
  /// specifying how the PAG should be built.
  struct PBStrategy {
    constexpr PBStrategy() noexcept = default;
    virtual ~PBStrategy() = default;

    /// Called by buildPAG() for every (unique) edge that should be added to the
    /// PAG
    ///
    /// \param From The source-node.
    /// \param To The destination-node.
    /// \param E The edge-label that specifies the kind of data-flow that is
    /// denoted by this edge.
    /// \param AtInstruction Optionally holds the instruction that caused this
    /// edge to be created.
    virtual void onAddEdge(ValueId From, ValueId To, Edge E,
                           Nullable<n_t> AtInstruction) = 0;

    /// Called by buildPAG() for every (unique) node that should be added to the
    /// PAG, *excluding* nodes that have already been registered in the used
    /// ValueCompressor before buildPAG() was called.
    ///
    /// \param Variable The IR-specific variable/value for which the new node
    /// has been created.
    /// \param VId The value-id of the newly created node.
    virtual void onAddValue(ByConstRef<v_t> Variable, ValueId VId) {
      // Do things like allocating a new slot in the adjacency-list here
    }

    /// Estimates the maximum number of values created in the ValueCompressor.
    /// Must not be exact, this is just for pre-allocating buffers for
    /// optimization purposes.
    [[nodiscard]] virtual size_t
    getNumPossibleValues(const db_t &IRDB) const noexcept {
      // May use a different heuristic here...
      return IRDB.getNumInstructions();
    }

    /// Invokes a call-back with each function that may be called at the given
    /// call-site. Usually, you want to use a CallGraph or a Resolver here.
    ///
    /// \param CS The call-site.
    /// \param WithCallee The callback to be invoked for each callee.
    virtual void
    withCalleesOfCallAt(ByConstRef<n_t> CS,
                        llvm::function_ref<void(f_t)> WithCallee) const = 0;
  };

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
                        PBStrategy &Strategy) = 0;
};

} // namespace psr
