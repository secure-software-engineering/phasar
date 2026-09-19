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

#include "phasar/DataFlow/IfdsIde/IFDSProblem.h"
#include "phasar/DataFlow/IfdsIde/IfdsIdeProblemMixin.h"
#include "phasar/Domain/BinaryDomain.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/NonNullPtr.h"
#include "phasar/Utils/SmallArraySet.h"
#include "phasar/Utils/StrongTypeDef.h"
#include "phasar/Utils/TableWrappers.h"
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include <concepts>
#include <cstdint>
#include <type_traits>

/// \file
/// Implements field-sensitivity after the paper "Boosting the performance
/// of alias-aware IFDS analysis with CFL-based environment transformers" by Li
/// et al. <https://doi.org/10.1145/3689804>

PHASAR_STRONG_TYPEDEF(psr::cfl_fieldsens, uint32_t, FieldStringNodeId,
                      None = 0);

PHASAR_STRONG_TYPEDEF(psr::cfl_fieldsens, uint32_t, KillSetId, Empty = 0);
namespace psr::cfl_fieldsens {

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
  static constexpr int32_t TopOffset = INT32_MIN;

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

  [[nodiscard]] KillSetId internKills(SmallArraySet<int32_t, 2> &&Kills) {
    return KillsCompressor.getOrInsert(std::move(Kills));
  }

  [[nodiscard]] KillSetId addKill(KillSetId KS, int32_t Offs) {
    if (Offs == TopOffset || KillsCompressor[KS].contains(Offs)) {
      return KS;
    }

    auto Kills = KillsCompressor[KS];
    Kills.insert(Offs);
    return KillsCompressor.getOrInsert(std::move(Kills));
  }

  [[nodiscard]] bool isKilledBy(KillSetId KS, int32_t Offs) const {
    if (Offs == TopOffset || KS == KillSetId::Empty) {
      return false;
    }
    if (!KillsCompressor.inbounds(KS)) [[unlikely]] {
      return false;
    }

    return KillsCompressor[KS].contains(Offs);
  }

  [[nodiscard]] const auto &kills(KillSetId KS) const {
    return KillsCompressor[KS];
  }

  void reserve(size_t ExpectedCapacity) {
    NodeCompressor.reserve(ExpectedCapacity);
    Depth.reserve(ExpectedCapacity);
  }

private:
  Compressor<FieldStringNode, FieldStringNodeId> NodeCompressor{};
  TypedVector<FieldStringNodeId, uint32_t> Depth{};
  Compressor<SmallArraySet<int32_t, 2>, KillSetId> KillsCompressor{};
};

/// A single CFL Field-Access String consisting of: gep, loads, kills, and
/// stores
struct AccessPath {
  static constexpr int32_t TopOffset = FieldStringManager::TopOffset;

  FieldStringNodeId Loads{};
  FieldStringNodeId Stores{};
  KillSetId Kills{};
  // Add an offset for pending GEPs; INT32_MIN is Top
  int32_t Offset{};

  [[nodiscard]] bool empty() const noexcept {
    return Loads == FieldStringNodeId::None &&
           Stores == FieldStringNodeId::None && Kills == KillSetId::Empty &&
           Offset == 0;
  }

  [[nodiscard]] constexpr bool
  operator==(const AccessPath &Other) const noexcept = default;

  friend constexpr size_t hash_value(const AccessPath &FieldString) noexcept {
    size_t HC = 37;
    HC = HC * 31 + size_t(FieldString.Loads);
    HC = HC * 31 + size_t(FieldString.Stores);
    HC = HC * 31 + size_t(FieldString.Kills);
    HC = HC * 31 + size_t(FieldString.Offset);
    return HC;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const AccessPath &FieldString);

  void print(llvm::raw_ostream &OS, const FieldStringManager &Mgr) const;
};

struct AccessPathDMI {
  static AccessPath getEmptyKey() {
    AccessPath Ret{};
    Ret.Loads = FieldStringNodeId(UINT32_MAX);
    return Ret;
  }
  static AccessPath getTombstoneKey() {
    AccessPath Ret{};
    Ret.Loads = FieldStringNodeId(UINT32_MAX);
    Ret.Stores = FieldStringNodeId(UINT32_MAX);
    return Ret;
  }
  static auto getHashValue(AccessPath FieldString) noexcept {
    return hash_value(FieldString);
  }
  static bool isEqual(AccessPath L, AccessPath R) noexcept { return L == R; }
};

/// An edge-value consisting of a set if CFL field access strings.
struct IFDSEdgeValue {
  using container_type = llvm::SmallDenseSet<AccessPath, 2, AccessPathDMI>;

  PSR_REQUIRE_EXPLICIT_INITIALIZATION FieldStringManager *Mgr{};
  container_type Paths;

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

  [[nodiscard]] friend auto hash_value(const IFDSEdgeValue &EV) {
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

struct CFLFieldSensEdgeFunctionImpl {
  using l_t = LatticeDomain<IFDSEdgeValue>;
  PSR_REQUIRE_EXPLICIT_INITIALIZATION IFDSEdgeValue Transform;
  PSR_REQUIRE_EXPLICIT_INITIALIZATION uint8_t DepthKLimit{};

  bool operator==(const CFLFieldSensEdgeFunctionImpl &Other) const noexcept {
    assert(DepthKLimit == Other.DepthKLimit);
    return Transform == Other.Transform;
  }

  friend auto hash_value(const CFLFieldSensEdgeFunctionImpl &EF) noexcept {
    return hash_value(EF.Transform);
  }

  [[nodiscard]] static auto from(IFDSEdgeValue &&Txn, uint8_t DepthKLimit) {
    return CFLFieldSensEdgeFunctionImpl{
        .Transform = std::move(Txn),
        .DepthKLimit = DepthKLimit,
    };
  }

  [[nodiscard]] static auto from(AccessPath Txn, FieldStringManager &Mgr,
                                 uint8_t DepthKLimit) {
    return CFLFieldSensEdgeFunctionImpl{
        .Transform = {.Mgr = &Mgr, .Paths = {Txn}},
        .DepthKLimit = DepthKLimit,
    };
  }

  [[nodiscard]] static auto fromEpsilon(uint8_t DepthKLimit,
                                        FieldStringManager &Mgr) {
    return CFLFieldSensEdgeFunctionImpl{
        .Transform = IFDSEdgeValue::epsilon(&Mgr),
        .DepthKLimit = DepthKLimit,
    };
  }
};

struct CFLFieldSensEdgeFunction {
  using l_t = LatticeDomain<IFDSEdgeValue>;
  PSR_REQUIRE_EXPLICIT_INITIALIZATION const CFLFieldSensEdgeFunctionImpl
      *Impl{};

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    assert(Impl != nullptr);
    Source.onValue(fn<&IFDSEdgeValue::applyTransforms>, Impl->Transform,
                   Impl->DepthKLimit);
    return Source;
  }

  constexpr friend bool
  operator==(CFLFieldSensEdgeFunction L,
             CFLFieldSensEdgeFunction R) noexcept = default;

  friend auto hash_value(CFLFieldSensEdgeFunction EF) noexcept {
    return llvm::hash_value(EF.Impl);
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       CFLFieldSensEdgeFunction EF);
};

class CFLFieldSensEdgeFunctions
    : public IfdsIdeProblemMixin<cfl_fieldsens::IFDSDomain> {
public:
  static constexpr llvm::StringLiteral LogCategory = "CFLFieldSensIFDSProblem";

  EdgeFunction<l_t> getStoreEdgeFunction(d_t CurrNode, d_t SuccNode,
                                         d_t PointerOp, d_t ValueOp,
                                         uint8_t DepthKLimit,
                                         const llvm::DataLayout &DL);

  EdgeFunction<l_t> getLoadEdgeFunction(d_t CurrNode, d_t PointerOp,
                                        uint8_t DepthKLimit,
                                        const llvm::DataLayout &DL);

  EdgeFunction<l_t> getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                          d_t SuccNode);

  EdgeFunction<l_t> getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                        f_t DestinationFunction, d_t DestNode);

  EdgeFunction<l_t> getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction,
                                          n_t ExitStmt, d_t ExitNode,
                                          n_t RetSite, d_t RetNode);

  EdgeFunction<l_t> getCallToRetEdgeFunction(n_t CallSite, d_t CallNode,
                                             n_t RetSite, d_t RetSiteNode,
                                             llvm::ArrayRef<f_t> Callees);

  EdgeFunction<l_t> getSummaryEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                                           d_t SuccNode);

  EdgeFunction<l_t> extend(const EdgeFunction<l_t> &L,
                           const EdgeFunction<l_t> &R);

  EdgeFunction<l_t> combine(const EdgeFunction<l_t> &L,
                            const EdgeFunction<l_t> &R);

protected:
  template <typename ProblemTy>
  CFLFieldSensEdgeFunctions(ProblemTy &Problem,
                            cfl_fieldsens::IFDSProblemConfig &&Config,
                            uint8_t DepthKLimit = 5)
      // entry-points not forwarded; getEntryPoints() overridden in
      // CFLFieldSensIFDSProblem
      : psr::IfdsIdeProblemMixin<IFDSDomain>(Problem.getProjectIRDB(), {},
                                             Problem.getZeroValue()),
        Config(std::move(Config)), DepthKLimit(DepthKLimit) {
    Mgr.reserve(Problem.getProjectIRDB()->getNumInstructions());
  }

  /// Transforms user-defined seeds from usual IFDS seeds to field-sensitive
  /// IFDS seeds
  [[nodiscard]] InitialSeeds<IFDSDomain::n_t, IFDSDomain::d_t, IFDSDomain::l_t>
  makeInitialSeeds(const InitialSeeds<LLVMIFDSAnalysisDomainDefault::n_t,
                                      LLVMIFDSAnalysisDomainDefault::d_t,
                                      BinaryDomain> &UserSeeds);

private:
  using EFConstPtr = const cfl_fieldsens::CFLFieldSensEdgeFunctionImpl *;
  using EFResultPtr = llvm::PointerIntPair<EFConstPtr, 2>;

  [[nodiscard]] EdgeFunction<l_t>
  makeEF(cfl_fieldsens::CFLFieldSensEdgeFunctionImpl &&EF);
  [[nodiscard]] EFResultPtr
  makeEFPtr(cfl_fieldsens::CFLFieldSensEdgeFunctionImpl &&EF);

  cfl_fieldsens::FieldStringManager Mgr{};
  cfl_fieldsens::IFDSProblemConfig Config{};

  UnorderedSet<cfl_fieldsens::CFLFieldSensEdgeFunctionImpl> EFInternCache{};

  llvm::DenseMap<std::pair<EFConstPtr, EFConstPtr>, EFResultPtr> ExtendCache{};
  llvm::DenseMap<std::pair<EFConstPtr, EFConstPtr>, EFResultPtr> CombineCache{};

  uint8_t DepthKLimit = 5; // Original from the paper
};

} // namespace cfl_fieldsens

/// An IFDS-Problem adaptor that makes any field-insensitive IFDS analysis
/// field-sensitive. Just wrap your IFDS problem with CFLFieldSensIFDSProblem
/// and use the IterativeIDESolver instead of the IFDSSolver.
///
/// The only thing to change in your usual IFDS problem is not to kill data-flow
/// facts when only parts of the fields should be killed. This is now handled by
/// the CFLFieldSensIFDSProblem. For that, provide a
/// problem definition with a proper killsAt()
/// implementation.
template <IFDSProblem ProblemTy>
  requires std::same_as<LLVMIFDSAnalysisDomainDefault,
                        typename ProblemTy::ProblemAnalysisDomain>
class CFLFieldSensIFDSProblem
    : public cfl_fieldsens::CFLFieldSensEdgeFunctions {
  using Base = cfl_fieldsens::CFLFieldSensEdgeFunctions;

  static decltype(cfl_fieldsens::IFDSProblemConfig::KillsAt)
  deriveKillsAt(ProblemTy *UserProblem) {
    assert(UserProblem != nullptr);
    if constexpr (requires {
                    {
                      UserProblem->killsAt()
                    } -> psr::invocable_r<std::optional<int32_t>, n_t, d_t>;
                  }) {
      return UserProblem->killsAt();
    } else if constexpr (requires {
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

  /// Constructs an IDETabulationProblem with the usual arguments, forwarded
  /// from UserProblem
  explicit CFLFieldSensIFDSProblem(
      ProblemTy *UserProblem,
      cfl_fieldsens::IFDSProblemConfig
          Config) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : Base(assertNotNull(UserProblem), std::move(Config)),
        UserProblem(UserProblem) {}

  /// Constructs an IDETabulationProblem with the usual arguments, forwarded
  /// from UserProblem and tries to automatically derive the config from
  /// additional functions specified by UserProblem

  explicit CFLFieldSensIFDSProblem(ProblemTy *UserProblem)
      : CFLFieldSensIFDSProblem(UserProblem,
                                cfl_fieldsens::IFDSProblemConfig{
                                    .KillsAt = deriveKillsAt(UserProblem),
                                }) {}

  CFLFieldSensIFDSProblem(std::nullptr_t,
                          cfl_fieldsens::IFDSProblemConfig Config) = delete;

  CFLFieldSensIFDSProblem(std::nullptr_t) = delete;

  [[nodiscard]] decltype(auto) getEntryPoints() const {
    return UserProblem->getEntryPoints();
  }

  // XXX: Perhaps we need a way to provide a customization-point to specify gen
  // offsets to the edge-functions (generating from zero currently always
  // generates at epsilon!)

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() {
    return this->makeInitialSeeds(UserProblem->initialSeeds());
  }

  [[nodiscard]] decltype(auto) getNormalFlowFunction(n_t Curr, n_t Succ) {
    return UserProblem->getNormalFlowFunction(Curr, Succ);
  }

  [[nodiscard]] decltype(auto) getCallFlowFunction(n_t CallInst,
                                                   f_t CalleeFun) {
    return UserProblem->getCallFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] decltype(auto) getSummaryFlowFunction(n_t CallInst,
                                                      f_t CalleeFun) {
    return UserProblem->getSummaryFlowFunction(CallInst, CalleeFun);
  }

  [[nodiscard]] decltype(auto) getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                                  n_t ExitInst, n_t RetSite) {
    return UserProblem->getRetFlowFunction(CallSite, CalleeFun, ExitInst,
                                           RetSite);
  }

  [[nodiscard]] decltype(auto)
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) {
    return UserProblem->getCallToRetFlowFunction(CallSite, RetSite, Callees);
  }

  /// The wrapped user-problem
  [[nodiscard]] const auto &base() const noexcept { return *UserProblem; }

private:
  NonNullPtr<ProblemTy> UserProblem;
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
