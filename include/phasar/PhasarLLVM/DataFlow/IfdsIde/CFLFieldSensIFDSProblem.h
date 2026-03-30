/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H

#include "phasar/DataFlow/IfdsIde/IDETabulationProblem.h"
#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace psr::cfl_fieldsens {

/// \file
/// Implements field-sensitivity after the paper "Boosting the performance
/// of alias-aware IFDS analysis with CFL-based environment transformers" by Li
/// et al. <https://doi.org/10.1145/3689804>

// NOLINTNEXTLINE(performance-enum-size)
enum class FieldStringNodeId : uint32_t {
  None = 0,
};

[[nodiscard]] inline llvm::hash_code hash_value(FieldStringNodeId NId) {
  return llvm::hash_value(std::underlying_type_t<FieldStringNodeId>(NId));
}

struct FieldStringNode {
  FieldStringNodeId Next{};
  int32_t Offset{};

  [[nodiscard]] constexpr bool
  operator==(const FieldStringNode &) const noexcept = default;

  friend llvm::hash_code hash_value(FieldStringNode Nod) {
    return llvm::DenseMapInfo<std::pair<uint32_t, int32_t>>::getHashValue(
        {uint32_t(Nod.Next), Nod.Offset});
  }
};

} // namespace psr::cfl_fieldsens

namespace llvm {
template <> struct DenseMapInfo<psr::cfl_fieldsens::FieldStringNode> {
  using FieldStringNode = psr::cfl_fieldsens::FieldStringNode;
  using FieldStringNodeId = psr::cfl_fieldsens::FieldStringNodeId;

  static constexpr FieldStringNode getEmptyKey() noexcept {
    return {.Next = FieldStringNodeId(UINT32_MAX), .Offset = INT32_MAX};
  }
  static constexpr FieldStringNode getTombstoneKey() noexcept {
    return {.Next = FieldStringNodeId(UINT32_MAX - 1), .Offset = INT32_MAX};
  }
  static constexpr bool isEqual(FieldStringNode L, FieldStringNode R) noexcept {
    return L == R;
  }
  static auto getHashValue(FieldStringNode Nod) { return hash_value(Nod); }
};
} // namespace llvm

namespace psr {

namespace cfl_fieldsens {

/// Interns the Store- and Load field-strings
class FieldStringManager {
public:
  FieldStringManager();

  [[nodiscard]] FieldStringNodeId intern(FieldStringNode Nod) {
    auto [Id, Inserted] = NodeCompressor.insert(Nod);

    if (Inserted) {
      Depth.push_back(Depth[Nod.Next] + 1);
    }

    return Id;
  }

  [[nodiscard]] FieldStringNodeId prepend(int32_t Head,
                                          FieldStringNodeId Tail) {
    auto Ret = intern(FieldStringNode{.Next = Tail, .Offset = Head});
    PHASAR_LOG_LEVEL(DEBUG, "[prepend]: " << Head << " :: #" << uint32_t(Tail)
                                          << " = #" << uint32_t(Ret));
    return Ret;
  }

  [[nodiscard]] FieldStringNode operator[](FieldStringNodeId NId) const {
    return NodeCompressor[NId];
  }

  [[nodiscard]] llvm::SmallVector<int32_t>
  getFullFieldString(FieldStringNodeId NId) const;

  [[nodiscard]] FieldStringNodeId
  fromFullFieldString(llvm::ArrayRef<int32_t> FieldString);

  [[nodiscard]] uint32_t depth(FieldStringNodeId NId) const {
    return Depth[NId];
  }

private:
  Compressor<FieldStringNode, FieldStringNodeId> NodeCompressor{};
  TypedVector<FieldStringNodeId, uint32_t> Depth{};
};

/// A single CFL Field-Access String consisting of: gep, loads, kills, and
/// stores
struct AccessPath {
  static constexpr int32_t TopOffset = INT32_MIN;

  FieldStringNodeId Loads{};
  FieldStringNodeId Stores{};
  llvm::SmallDenseSet<int32_t, 2> Kills{};
  // Add an offset for pending GEPs; INT32_MIN is Top
  int32_t Offset = {0};
  int32_t EmptyTombstone = 0;

  [[nodiscard]] bool empty() const noexcept {
    return Loads == FieldStringNodeId::None &&
           Stores == FieldStringNodeId::None && Kills.empty() && Offset == 0;
  }

  [[nodiscard]] bool kills(int32_t Off) const {
    return Off != TopOffset && Kills.contains(Off);
  }

  [[nodiscard]] constexpr bool
  operator==(const AccessPath &Other) const noexcept {
    return EmptyTombstone == Other.EmptyTombstone && Loads == Other.Loads &&
           Stores == Other.Stores && Kills == Other.Kills;
  }

  bool operator!=(const AccessPath &Other) const noexcept {
    return !(*this == Other);
  }

  friend size_t hash_value(const AccessPath &FieldString) noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AccessPath &FieldString);

  void print(llvm::raw_ostream &OS, const FieldStringManager &Mgr) const;
};

struct AccessPathDMI {
  static AccessPath getEmptyKey() {
    AccessPath Ret{};
    Ret.EmptyTombstone = 1;
    return Ret;
  }
  static AccessPath getTombstoneKey() {
    AccessPath Ret{};
    Ret.EmptyTombstone = 2;
    return Ret;
  }
  static auto getHashValue(const AccessPath &FieldString) noexcept {
    return hash_value(FieldString);
  }
  static bool isEqual(const AccessPath &L, const AccessPath &R) noexcept {
    if (L.EmptyTombstone != R.EmptyTombstone) {
      return false;
    }
    if (L.EmptyTombstone) {
      return true;
    }
    return L == R;
  }
};

/// An edge-value consisting of a set if CFL field access strings.
struct IFDSEdgeValue {
  [[clang::require_explicit_initialization]] FieldStringManager *Mgr{};
  llvm::SmallDenseSet<AccessPath, 2, AccessPathDMI> Paths;

  static constexpr llvm::StringLiteral LogCategory = "IFDSEdgeValue";

  void applyTransforms(const IFDSEdgeValue &Txns, uint8_t DepthKLimit);

  bool operator==(const IFDSEdgeValue &Other) const noexcept {
    assert(Mgr == Other.Mgr);
    assert(Mgr != nullptr);
    return Paths == Other.Paths;
  }
  bool operator!=(const IFDSEdgeValue &Other) const noexcept {
    return !(*this == Other);
  }

  [[nodiscard]] friend auto hash_value(const IFDSEdgeValue EV) {
    return llvm::hash_combine_range(EV.Paths.begin(), EV.Paths.end());
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const IFDSEdgeValue &EV);

  [[nodiscard]] bool isEpsilon() const {
    return Paths.size() == 1 && Paths.begin()->empty();
  }

  [[nodiscard]] static IFDSEdgeValue epsilon(FieldStringManager *Mgr) {
    IFDSEdgeValue Ret{.Mgr = &assertNotNull(Mgr), .Paths = {}};
    Ret.Paths.insert({}); // Not using initializer_list to prevent copying
    return Ret;
  }

  // To be picked up via ADL by psr::join(LatticeDomain, LatticeDomain)
  [[nodiscard]] friend auto join(const IFDSEdgeValue &L,
                                 const IFDSEdgeValue &R) {
    assert(L.Mgr == R.Mgr);
    assert(L.Mgr != nullptr);
    const bool LeftSmaller = L.Paths.size() < R.Paths.size();
    auto Ret = LeftSmaller ? R : L;
    const auto &Smaller = LeftSmaller ? L : R;
    Ret.Paths.insert(Smaller.Paths.begin(), Smaller.Paths.end());
    // XXX: k-limit num-paths: This may not be necessary, as join() is only
    // called from IDE-Phase-II
    return Ret;
  }
};

struct IFDSDomain : LLVMIFDSAnalysisDomainDefault {
  using l_t = LatticeDomain<IFDSEdgeValue>;
};

/// Configures, how the CFLFieldSensIFDSProblem should handle strong updates.
struct IFDSProblemConfig : LLVMIFDSAnalysisDomainDefault {
  /// Gives the byte-offset of a kill at <Curr, CurrNode>, if any, else nullopt.
  ///
  /// Can be derived automatically, if the user-problem specifies a
  /// member-function killsAt() that returns such a function object.
  llvm::unique_function<std::optional<int32_t>(n_t Curr, d_t CurrNode)> KillsAt;
  // XXX: more
};

/// Transforms user-defined seeds from usual IFDS seeds to field-sensitive IFDS
/// seeds
[[nodiscard]] InitialSeeds<IFDSDomain::n_t, IFDSDomain::d_t, IFDSDomain::l_t>
makeInitialSeeds(const InitialSeeds<LLVMIFDSAnalysisDomainDefault::n_t,
                                    LLVMIFDSAnalysisDomainDefault::d_t,
                                    BinaryDomain> &UserSeeds,
                 FieldStringManager &Mgr);

/// Utility to strip off potential pointer-arithmetic from V and accumulating
/// the byte-offset.
[[nodiscard]] inline std::pair<const llvm::Value *, int32_t>
getBaseAndOffset(const llvm::Value *V, const llvm::DataLayout &DL) {
  llvm::APInt Offset(64, 0);
  int32_t OffsVal = AccessPath::TopOffset;
  const auto *Base = V->stripAndAccumulateConstantOffsets(DL, Offset, true);

  if (llvm::isa<llvm::GEPOperator>(Base)) {
    return {Base->stripPointerCastsAndAliases(), AccessPath::TopOffset};
  }

  auto RawOffsVal = Offset.getSExtValue();
  if (RawOffsVal <= INT32_MAX && RawOffsVal >= INT32_MIN) {
    OffsVal = int32_t(RawOffsVal);
  }

  return {Base->stripPointerCastsAndAliases(), OffsVal};
}

/// Checks whether Fact holds at Inst in a field-sensitive way
template <bool AllowDeepTaints = true, typename ResultsT>
[[nodiscard]] inline bool holdsFactAt(const ResultsT &Results,
                                      IFDSDomain::n_t Inst,
                                      IFDSDomain::d_t Fact) {
  const auto &Fields = Results.resultAt(Inst, Fact);

  if (Fields.isTop()) {
    // Was not computed by the IDE Solver
    return false;
  }

  if (const IFDSEdgeValue *FieldStrings = Fields.getValueOrNull()) {
    if constexpr (!AllowDeepTaints) {
      // whether Facts itself holds, not whether any fields of it may hold
      return FieldStrings->isEpsilon();
    }
    if (FieldStrings->Paths.empty()) {
      // has been killed entirely
      return false;
    }
  }

  return true;
}

/// Given a QueryMap of the form map<n_t, set<d_t>>, calls the Handler for all
/// inst-fact pairs that hold in a field-sensitive way and filters out all
/// others.
///
/// The Handler may opt into early exit by returning false. Returning void is
/// permitted.
template <bool AllowDeepTaints = true, typename ResultsT>
bool filterFieldSensFacts(
    const ResultsT &Results, const auto &QueryMap,
    std::invocable<IFDSDomain::n_t, IFDSDomain::d_t> auto Handler) {
  const IFDSDomain::l_t Top = psr::Top{};

  for (const auto &[Inst, FactsAtInst] : QueryMap) {
    const auto &Row = Results.row(Inst);
    for (const auto &Fact : FactsAtInst) {
      const auto &Fields = getOr(Row, Fact, Top);

      if (Fields.isTop()) {
        // Was not computed by the IDE Solver
        continue;
      }

      if (const auto *FieldStrings = Fields.getValueOrNull()) {
        if (!AllowDeepTaints && !FieldStrings->isEpsilon()) {
          // Fact does not hold itself, but fields of Fact may hold. In
          // aggressive mode, we ignore them
          continue;
        }
        if (FieldStrings->Paths.empty()) {
          // has been killed entirely
          continue;
        }
      }

      if constexpr (std::convertible_to<
                        std::invoke_result_t<decltype(Handler) &,
                                             IFDSDomain::n_t, IFDSDomain::d_t>,
                        bool>) {
        if (!std::invoke(Handler, Inst, Fact)) {
          return false;
        }
      } else {
        std::invoke(Handler, Inst, Fact);
      }
    }
  }
  return true;
}

} // namespace cfl_fieldsens

/// An IFDS-Problem adaptor that makes any field-insensitive IFDS analysis
/// field-sensitive. Just wrap your IFDS problem with
/// FieldSensAllocSitesAwareIFDSProblem and use the IterativeIDESolver instead
/// of the IFDSSolver.
///
/// The only thing to change in your usual IFDS problem is not to kill data-flow
/// facts when only parts of the fields should be killed. This is now handled by
/// the FieldSensAllocSitesAwareIFDSProblem. For that, provide a
/// FieldSensAllocSitesAwareIFDSProblemConfig with a proper KillsAt
/// implementation.
class CFLFieldSensIFDSProblem
    : public IDETabulationProblem<cfl_fieldsens::IFDSDomain> {
  using Base = IDETabulationProblem<cfl_fieldsens::IFDSDomain>;

  static decltype(cfl_fieldsens::IFDSProblemConfig::KillsAt)
  deriveKillsAt(auto *UserProblem) {
    assert(UserProblem != nullptr);
    if constexpr (requires() {
                    {
                      UserProblem->killsAt()
                    } -> psr::invocable_r<std::optional<int32_t>, n_t, d_t>;
                  }) {
      return UserProblem->killsAt();
    } else if constexpr (requires() {
                           {
                             UserProblem->killsAt()
                           } -> std::invocable<n_t, d_t>;
                         }) {
      // Intentionally leaving an unused variable, so that the compiler emits a
      // warning here
      auto KillsAtHasWrongReturnType = UserProblem->killsAt();
      return nullptr;
    } else {
      return nullptr;
    }
  }

public:
  using typename Base::container_type;
  using typename Base::d_t;
  using typename Base::db_t;
  using typename Base::f_t;
  using typename Base::FlowFunctionPtrType;
  using typename Base::i_t;
  using typename Base::l_t;
  using typename Base::n_t;
  using typename Base::ProblemAnalysisDomain;
  using typename Base::t_t;
  using typename Base::v_t;

  static constexpr llvm::StringLiteral LogCategory = "CFLFieldSensIFDSProblem";

  /// Constructs an IDETabulationProblem with the usual arguments, forwarded
  /// from UserProblem
  explicit CFLFieldSensIFDSProblem(
      IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem,
      cfl_fieldsens::IFDSProblemConfig
          Config) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : Base(assertNotNull(UserProblem).getProjectIRDB(),
             assertNotNull(UserProblem).getEntryPoints(),
             UserProblem->getZeroValue()),
        UserProblem(UserProblem), Config(std::move(Config)) {}

  /// Constructs an IDETabulationProblem with the usual arguments, forwarded
  /// from UserProblem and tries to automatically derive the config from
  /// additional functions specified by UserProblem
  explicit CFLFieldSensIFDSProblem(
      proper_subclass_of<
          IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault>> auto
          *UserProblem)
      : CFLFieldSensIFDSProblem(UserProblem,
                                cfl_fieldsens::IFDSProblemConfig{
                                    .KillsAt = deriveKillsAt(UserProblem),
                                }) {}

  CFLFieldSensIFDSProblem(std::nullptr_t,
                          cfl_fieldsens::IFDSProblemConfig Config) = delete;

  CFLFieldSensIFDSProblem(std::nullptr_t) = delete;

  // XXX: Perhaps we need a way to provide a customization-point to specify gen
  // offsets to the edge-functions (generating from zero currently always
  // generates at epsilon!)

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return cfl_fieldsens::makeInitialSeeds(UserProblem->initialSeeds(), Mgr);
  }

  [[nodiscard]] FlowFunctionPtrType getNormalFlowFunction(n_t Curr,
                                                          n_t Succ) override {
    return UserProblem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return UserProblem->getCallFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType
  getSummaryFlowFunction(n_t CallInst, f_t CalleeFun) override {
    return UserProblem->getSummaryFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] FlowFunctionPtrType getRetFlowFunction(n_t CallSite,
                                                       f_t CalleeFun,
                                                       n_t ExitInst,
                                                       n_t RetSite) override {
    return UserProblem->getRetFlowFunction(CallSite, CalleeFun, ExitInst,
                                           RetSite);
  }

  [[nodiscard]] FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override {
    return UserProblem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  EdgeFunction<l_t> getStoreEdgeFunction(d_t CurrNode, d_t SuccNode,
                                         d_t PointerOp, d_t ValueOp,
                                         uint8_t DepthKLimit,
                                         const llvm::DataLayout &DL);

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode) override;

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction,
                                        d_t DestNode) override;

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitStmt, d_t ExitNode,
                                          n_t RetSite, d_t RetNode) override;

  EdgeFunction<l_t>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                           d_t SuccNode) override;

  EdgeFunction<l_t> extend(const EdgeFunction<l_t> &L,
                           const EdgeFunction<l_t> &R) override;

  EdgeFunction<l_t> combine(const EdgeFunction<l_t> &L,
                            const EdgeFunction<l_t> &R) override;

  /// The wrapped user-problem
  [[nodiscard]] const auto &base() const noexcept { return *UserProblem; }

private:
  IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem{};
  cfl_fieldsens::FieldStringManager Mgr{};
  cfl_fieldsens::IFDSProblemConfig Config{};

  uint8_t DepthKLimit = 5; // Original from the paper
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
