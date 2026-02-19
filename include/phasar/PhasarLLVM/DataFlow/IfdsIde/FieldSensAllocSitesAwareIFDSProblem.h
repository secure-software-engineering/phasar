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
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMBasePointerAliasSet.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/TypedVector.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <type_traits>

namespace psr {

/// \file Implements field-sensitivity after the paper "Boosting the performance
/// of alias-aware IFDS analysis with CFL-based environment transformers" by Li
/// et al. <https://doi.org/10.1145/3689804>

struct StoreEvent {};
struct LoadEvent {};

struct KillEvent {};

struct GEPEvent {
  int32_t Field;
};

enum class CFLFieldStringNodeId : uint32_t {
  None = 0,
};

[[nodiscard]] inline llvm::hash_code hash_value(CFLFieldStringNodeId NId) {
  return llvm::hash_value(std::underlying_type_t<CFLFieldStringNodeId>(NId));
}

struct CFLFieldStringNode {
  CFLFieldStringNodeId Next{};
  int32_t Offset{};

  [[nodiscard]] constexpr bool
  operator==(const CFLFieldStringNode &) const noexcept = default;
};

} // namespace psr

namespace llvm {
template <> struct DenseMapInfo<psr::CFLFieldStringNode> {
  static constexpr psr::CFLFieldStringNode getEmptyKey() noexcept {
    return {psr::CFLFieldStringNodeId(UINT32_MAX), 0};
  }
  static constexpr psr::CFLFieldStringNode getTombstoneKey() noexcept {
    return {psr::CFLFieldStringNodeId(UINT32_MAX - 1), 0};
  }
  static constexpr bool isEqual(psr::CFLFieldStringNode L,
                                psr::CFLFieldStringNode R) noexcept {
    return L == R;
  }
  static llvm::hash_code getHashValue(psr::CFLFieldStringNode Nod) {
    return llvm::hash_combine(Nod.Next, Nod.Offset);
  }
};
} // namespace llvm

namespace psr {

class CFLFieldStringManager {
public:
  CFLFieldStringManager() {
    // Sentinel
    NodeCompressor.insertDummy(
        CFLFieldStringNode{CFLFieldStringNodeId::None, 0});
    Depth.push_back(0);
  }

  [[nodiscard]] CFLFieldStringNodeId intern(CFLFieldStringNode Nod) {
    auto [Id, Inserted] = NodeCompressor.insert(Nod);

    if (Inserted) {
      Depth.push_back(Depth[Nod.Next] + 1);
    }

    return Id;
  }

  [[nodiscard]] CFLFieldStringNodeId prepend(int32_t Head,
                                             CFLFieldStringNodeId Tail) {
    auto Ret = intern(CFLFieldStringNode{.Next = Tail, .Offset = Head});
    PHASAR_LOG_LEVEL(DEBUG, "[prepend]: " << Head << " :: #" << uint32_t(Tail)
                                          << " = #" << uint32_t(Ret));
    return Ret;
  }

  [[nodiscard]] CFLFieldStringNode operator[](CFLFieldStringNodeId NId) const {
    return NodeCompressor[NId];
  }

  [[nodiscard]] llvm::SmallVector<int32_t>
  getFullFieldString(CFLFieldStringNodeId NId) const;

  [[nodiscard]] CFLFieldStringNodeId
  fromFullFieldString(llvm::ArrayRef<int32_t> FieldString);

  [[nodiscard]] uint32_t depth(CFLFieldStringNodeId NId) const {
    return Depth[NId];
  }

private:
  Compressor<CFLFieldStringNode, CFLFieldStringNodeId> NodeCompressor{};
  TypedVector<CFLFieldStringNodeId, uint32_t> Depth{};
};

struct CFLFieldAccessPath {
  static constexpr int32_t TopOffset = INT32_MIN;

  CFLFieldStringNodeId Loads;
  CFLFieldStringNodeId Stores;
  llvm::SmallDenseSet<int32_t, 2> Kills;
  // Add an offset for pending GEPs; INT32_MIN is Top
  int32_t Offset = {0};
  int32_t EmptyTombstone = 0;

  [[nodiscard]] bool empty() const noexcept {
    return Loads == CFLFieldStringNodeId::None &&
           Stores == CFLFieldStringNodeId::None && Kills.empty() && Offset == 0;
  }

  [[nodiscard]] bool kills(int32_t Off) const {
    return Off != TopOffset && Kills.contains(Off);
  }

  [[nodiscard]] bool
  operator==(const CFLFieldAccessPath &Other) const noexcept {
    return EmptyTombstone == Other.EmptyTombstone && Loads == Other.Loads &&
           Stores == Other.Stores && Kills == Other.Kills;
  }

  bool operator!=(const CFLFieldAccessPath &Other) const noexcept {
    return !(*this == Other);
  }

  friend size_t hash_value(const CFLFieldAccessPath &FieldString) noexcept;

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const CFLFieldAccessPath &FieldString);

  void print(llvm::raw_ostream &OS, const CFLFieldStringManager &Mgr) const;
};

struct CFLFieldAccessPathDMI {
  static CFLFieldAccessPath getEmptyKey() {
    CFLFieldAccessPath Ret{};
    Ret.EmptyTombstone = 1;
    return Ret;
  }
  static CFLFieldAccessPath getTombstoneKey() {
    CFLFieldAccessPath Ret{};
    Ret.EmptyTombstone = 2;
    return Ret;
  }
  static auto getHashValue(const CFLFieldAccessPath &FieldString) noexcept {
    return hash_value(FieldString);
  }
  static bool isEqual(const CFLFieldAccessPath &L,
                      const CFLFieldAccessPath &R) noexcept {
    if (L.EmptyTombstone != R.EmptyTombstone) {
      return false;
    }
    if (L.EmptyTombstone) {
      return true;
    }
    return L == R;
  }
};

struct CFLFieldSensEdgeValue {
  [[clang::require_explicit_initialization]] CFLFieldStringManager *Mgr{};
  llvm::SmallDenseSet<CFLFieldAccessPath, 2, CFLFieldAccessPathDMI> Paths;

  static constexpr llvm::StringLiteral LogCategory = "CFLFieldSensEdgeValue";

  // void applyStore(uint8_t DepthKLimit);
  // void applyGepAndStore(GEPEvent Evt, uint8_t DepthKLimit);
  // void applyLoad(uint8_t DepthKLimit);
  // void applyGepAndLoad(GEPEvent Evt, uint8_t DepthKLimit);
  // void applyKill();
  // void applyGepAndKill(GEPEvent Evt);
  // void applyGep(GEPEvent Evt);
  void applyTransform(const CFLFieldAccessPath &Txn, uint8_t DepthKLimit);
  void applyTransforms(const CFLFieldSensEdgeValue &Txns, uint8_t DepthKLimit);

  bool operator==(const CFLFieldSensEdgeValue &Other) const noexcept {
    assert(Mgr == Other.Mgr);
    assert(Mgr != nullptr);
    return Paths == Other.Paths;
  }
  bool operator!=(const CFLFieldSensEdgeValue &Other) const noexcept {
    return !(*this == Other);
  }

  [[nodiscard]] friend auto hash_value(const CFLFieldSensEdgeValue EV) {
    return llvm::hash_combine_range(EV.Paths.begin(), EV.Paths.end());
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const CFLFieldSensEdgeValue &EV);

  [[nodiscard]] bool isEpsilon() const {
    return Paths.size() == 1 && Paths.begin()->empty();
  }
};

template <typename AnalysisDomainTy>
struct CFLFieldSensAnalysisDomain : AnalysisDomainTy {
  using l_t = LatticeDomain<CFLFieldSensEdgeValue>;
};

struct FieldSensAllocSitesAwareIFDSProblemConfig
    : LLVMIFDSAnalysisDomainDefault {
  llvm::unique_function<std::optional<int32_t>(n_t Curr, d_t CurrNode)> KillsAt;
  // TODO: more
};

class FieldSensAllocSitesAwareIFDSProblemBase
    : public CFLFieldSensAnalysisDomain<LLVMIFDSAnalysisDomainDefault> {
public:
  static constexpr llvm::StringLiteral LogCategory =
      "FieldSensAllocSitesAwareIFDSProblem";

  [[nodiscard]] static InitialSeeds<n_t, d_t, l_t>
  makeInitialSeeds(const InitialSeeds<n_t, d_t, BinaryDomain> &UserSeeds,
                   CFLFieldStringManager &Mgr);

  [[nodiscard]] static std::pair<const llvm::Value *, int32_t>
  getBaseAndOffset(const llvm::Value *V, const llvm::DataLayout &DL) {
    llvm::APInt Offset(64, 0);
    int32_t OffsVal = CFLFieldAccessPath::TopOffset;
    const auto *Base = V->stripAndAccumulateConstantOffsets(DL, Offset, true);

    if (llvm::isa<llvm::GEPOperator>(Base)) {
      return {Base->stripPointerCastsAndAliases(),
              CFLFieldAccessPath::TopOffset};
    }

    auto RawOffsVal = Offset.getSExtValue();
    if (RawOffsVal <= INT32_MAX && RawOffsVal >= INT32_MIN) {
      OffsVal = int32_t(RawOffsVal);
    }

    return {Base->stripPointerCastsAndAliases(), OffsVal};
  }
};

class FieldSensAllocSitesAwareIFDSProblem
    : public FieldSensAllocSitesAwareIFDSProblemBase,
      public IDETabulationProblem<
          CFLFieldSensAnalysisDomain<LLVMIFDSAnalysisDomainDefault>> {
  using Base = IDETabulationProblem<
      CFLFieldSensAnalysisDomain<LLVMIFDSAnalysisDomainDefault>>;

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

  /// Constructs an IDETabulationProblem with the usual arguments + alias
  /// information.
  ///
  /// \note It is useful to use an instance of FilteredAliasSet for the alias
  /// information to lower suprious aliases
  explicit FieldSensAllocSitesAwareIFDSProblem(
      IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem,
      FieldSensAllocSitesAwareIFDSProblemConfig Config =
          {}) noexcept(std::is_nothrow_move_constructible_v<d_t>)
      : Base(UserProblem->getProjectIRDB(), UserProblem->getEntryPoints(),
             UserProblem->getZeroValue()),
        UserProblem(UserProblem), Config(std::move(Config)) {}

  FieldSensAllocSitesAwareIFDSProblem(
      std::nullptr_t,
      FieldSensAllocSitesAwareIFDSProblemConfig Config = {}) = delete;

  // TODO: Provide a customization-point to provide gen offsets to the
  // edge-functions (generating from zero currently always generates at
  // epsilon!)

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    return makeInitialSeeds(UserProblem->initialSeeds(), Mgr);
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

  [[nodiscard]] const auto &base() const noexcept { return *UserProblem; }

private:
  IFDSTabulationProblem<LLVMIFDSAnalysisDomainDefault> *UserProblem{};
  CFLFieldStringManager Mgr{};
  FieldSensAllocSitesAwareIFDSProblemConfig Config{};

  uint8_t DepthKLimit = 5; // Original from the paper
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_FIELDSENSALLOCSITESAWAREIFDSPROBLEM_H
