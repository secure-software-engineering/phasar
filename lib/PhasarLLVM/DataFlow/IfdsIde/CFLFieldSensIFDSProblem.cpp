#include "phasar/PhasarLLVM/DataFlow/IfdsIde/CFLFieldSensIFDSProblem.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Printer.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>

using namespace psr;
using namespace psr::cfl_fieldsens;

FieldStringManager::FieldStringManager() {
  // Sentinel
  NodeCompressor.insertDummy(
      FieldStringNode{.Next = FieldStringNodeId::None, .Offset = 0});
  Depth.push_back(0);
}

llvm::SmallVector<int32_t>
FieldStringManager::getFullFieldString(FieldStringNodeId NId) const {
  llvm::SmallVector<int32_t> Ret;
  while (NId != FieldStringNodeId::None) {
    auto Nod = NodeCompressor[NId];
    Ret.push_back(Nod.Offset);
    NId = Nod.Next;
  }
  std::ranges::reverse(Ret);
  return Ret;
}

FieldStringNodeId
FieldStringManager::fromFullFieldString(llvm::ArrayRef<int32_t> FieldString) {
  FieldStringNodeId Ret = FieldStringNodeId::None;
  for (const auto &Offset : FieldString) {
    Ret = prepend(Offset, Ret);
  }
  return Ret;
}

namespace {

using l_t = LatticeDomain<IFDSEdgeValue>;

constexpr static int32_t addOffsets(int32_t L, int32_t R) noexcept {
  if (L == AccessPath::TopOffset || R == AccessPath::TopOffset) {
    return AccessPath::TopOffset;
  }

  int32_t Sum{};
  if (llvm::AddOverflow(L, R, Sum)) {
    return AccessPath::TopOffset;
  }

  return Sum;
}

struct CFLFieldSensEdgeFunction {
  using l_t = LatticeDomain<IFDSEdgeValue>;
  [[clang::require_explicit_initialization]] IFDSEdgeValue Transform;
  [[clang::require_explicit_initialization]] uint8_t DepthKLimit{};

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    Source.onValue(fn<&IFDSEdgeValue::applyTransforms>, Transform, DepthKLimit);
    return Source;
  }

  static EdgeFunction<l_t>
  compose(EdgeFunctionRef<CFLFieldSensEdgeFunction> /*This*/,
          const EdgeFunction<l_t> & /*SecondFunction*/) {
    llvm::report_fatal_error("Use extend() instead!");
  }

  static EdgeFunction<l_t>
  join(EdgeFunctionRef<CFLFieldSensEdgeFunction> /*This*/,
       const EdgeFunction<l_t> & /*OtherFunction*/) {
    llvm::report_fatal_error("Use combine() instead!");
  }

  bool operator==(const CFLFieldSensEdgeFunction &Other) const noexcept {
    assert(DepthKLimit == Other.DepthKLimit);
    return Transform == Other.Transform;
  }

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const CFLFieldSensEdgeFunction &EF) {
    return OS << "Txn[" << EF.Transform << ']';
  }

  [[nodiscard]] static auto from(IFDSEdgeValue &&Txn, uint8_t DepthKLimit) {
    return CFLFieldSensEdgeFunction{
        .Transform = std::move(Txn),
        .DepthKLimit = DepthKLimit,
    };
  }

  [[nodiscard]] static auto from(AccessPath &&Txn, FieldStringManager &Mgr,
                                 uint8_t DepthKLimit) {
    // Avoid initializer_list as it prevents moving
    auto Ret = CFLFieldSensEdgeFunction{
        .Transform = {.Mgr = &Mgr, .Paths = {}},
        .DepthKLimit = DepthKLimit,
    };
    Ret.Transform.Paths.insert(std::move(Txn));
    return Ret;
  }

  [[nodiscard]] static auto fromEpsilon(uint8_t DepthKLimit,
                                        FieldStringManager &Mgr) {
    return CFLFieldSensEdgeFunction{
        .Transform = IFDSEdgeValue::epsilon(&Mgr),
        .DepthKLimit = DepthKLimit,
    };
  }
};

[[nodiscard]] std::string storesToString(const AccessPath &AP,
                                         const FieldStringManager &Mgr) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);

  llvm::interleave(
      Mgr.getFullFieldString(AP.Stores), ROS,
      [&ROS](auto StoreOffs) { ROS << 'S' << StoreOffs; }, ".");

  return Ret;
}

// Returns whether to retain F
[[nodiscard]] auto applyOneGepAndStore(FieldStringManager &Mgr, AccessPath &F,
                                       int32_t Field, uint8_t DepthKLimit) {
  if (Mgr.depth(F.Stores) == DepthKLimit) {
    // TODO: Optimize:
    auto Full = Mgr.getFullFieldString(F.Stores);
    Full.erase(Full.begin());
    F.Stores = Mgr.fromFullFieldString(Full);
  }
  F.Stores = Mgr.prepend(std::exchange(F.Offset, 0) + Field, F.Stores);
  return std::true_type{};
}

// Returns whether to retain F
[[nodiscard]] auto applyOneGepAndLoad(FieldStringManager &Mgr, AccessPath &F,
                                      int32_t Field, uint8_t DepthKLimit) {
  auto Offs = F.Offset + Field;
  if (F.Stores == FieldStringNodeId::None) {

    if (F.kills(Offs)) {
      return false;
    }

    F.Offset = 0;

    // TODO: Is this application of k-limiting correct here?
    // cf. Section 4.2.3 "K-Limiting" in the paper
    if (Mgr.depth(F.Loads) == DepthKLimit) {
      return true;
    }

    F.Loads = Mgr.prepend(Offs, F.Loads);
    F.Kills.clear();
    return true;
  }

  auto StoresHead = Mgr[F.Stores];

  if (StoresHead.Offset != Offs && StoresHead.Offset != AccessPath::TopOffset) {
    return false;
  }

  assert(StoresHead.Offset == Offs ||
         StoresHead.Offset == AccessPath::TopOffset);
  F.Offset = 0;
  F.Stores = StoresHead.Next;
  // llvm::errs() << "> pop_back\n";
  return true;
}

[[nodiscard]] auto applyOneGepAndKill(FieldStringManager &Mgr, AccessPath &F,
                                      int32_t Field, uint8_t /*DepthKLimit*/) {
  auto Offs = addOffsets(F.Offset, Field);
  if (Offs == AccessPath::TopOffset) {
    // We cannot kill Top
    return true;
  }

  if (F.Stores == FieldStringNodeId::None) {
    F.Kills.insert(Offs);
    PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory, "> add K" << Offs);
    return true;
  }

  auto StoresHead = Mgr[F.Stores];

  if (StoresHead.Offset == Offs) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory,
                         "> Kill " << storesToString(F, Mgr));
    return false;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory,
                       "> Retain " << storesToString(F, Mgr));

  assert(StoresHead.Offset != Offs);
  return true;
}

[[nodiscard]] auto applyOneGep(FieldStringManager &Mgr, AccessPath &F,
                               int32_t Field, uint8_t /*DepthKLimit*/) {
  if (F.Stores == FieldStringNodeId::None) {
    F.Offset = addOffsets(F.Offset, Field);
  } else {
    auto StoresHead = Mgr[F.Stores];
    F.Stores =
        Mgr.prepend(addOffsets(StoresHead.Offset, -Field), StoresHead.Next);
  }
  return std::true_type{};
}

void applyTransform(IFDSEdgeValue &EV, const AccessPath &Txn,
                    uint8_t DepthKLimit) {

  if (EV.Paths.empty() || Txn.empty()) {
    // Nothing to be done here
    return;
  }
  if (EV.isEpsilon()) {
    EV.Paths.clear();
    EV.Paths.insert(Txn);
    return;
  }

  auto Save = std::exchange(EV.Paths, {});
  EV.Paths.reserve(Save.size());

  const auto TxnOffset = Txn.Offset;
  const auto TxnLoads = EV.Mgr->getFullFieldString(Txn.Loads);
  const auto TxnStores = EV.Mgr->getFullFieldString(Txn.Stores);

  for (const auto &F : Save) {
    auto Copy = F;
    bool Retain = [&] {
      if (TxnOffset) {
        if (!applyOneGep(*EV.Mgr, Copy, TxnOffset, DepthKLimit)) {
          return false;
        }
      }

      for (auto Ld : TxnLoads) {
        if (!applyOneGepAndLoad(*EV.Mgr, Copy, Ld, DepthKLimit)) {
          return false;
        }
      }

      for (auto Kl : Txn.Kills) {
        if (!applyOneGepAndKill(*EV.Mgr, Copy, Kl, DepthKLimit)) {
          return false;
        }
      }

      for (auto St : TxnStores) {
        if (!applyOneGepAndStore(*EV.Mgr, Copy, St, DepthKLimit)) {
          return false;
        }
      }

      return true;
    }();

    if (Retain) {
      EV.Paths.insert(std::move(Copy));
    }
  }
}
} // namespace

void IFDSEdgeValue::applyTransforms(const IFDSEdgeValue &Txns,
                                    uint8_t DepthKLimit) {
  if (Mgr == nullptr) [[unlikely]] {
    llvm::report_fatal_error("Mgr is nullptr!");
  }

  if (Txns.Paths.empty()) {
    Paths.clear();
    return;
  }

  auto It = Txns.Paths.begin();
  if (Txns.Paths.size() == 1) [[likely]] {
    applyTransform(*this, *It, DepthKLimit);
    return;
  }

  // This path should be very rare, otherwise we will for sure have a
  // performance problem...

  auto End = Txns.Paths.end();
  auto Ret = *this;

  applyTransform(Ret, *It, DepthKLimit);

  for (++It; It != End; ++It) {
    if (!It->empty()) {
      auto Tmp = *this;
      applyTransform(Tmp, *It, DepthKLimit);
      Ret.Paths.insert(Tmp.Paths.begin(), Tmp.Paths.end());
    } else {
      Ret.Paths.insert(Paths.begin(), Paths.end());
    }
  }

  *this = std::move(Ret);
}

size_t psr::cfl_fieldsens::hash_value(const AccessPath &FieldString) noexcept {
  // Xor does not care about the order
  auto HCK = std::reduce(FieldString.Kills.begin(), FieldString.Kills.end(), 0,
                         std::bit_xor<>{});
  return llvm::hash_combine(FieldString.Loads, FieldString.Stores, HCK);
}

llvm::raw_ostream &
psr::cfl_fieldsens::operator<<(llvm::raw_ostream &OS,
                               const AccessPath &FieldString) {
  if (FieldString.empty()) {
    return OS << "ε";
  }

  if (FieldString.Offset) {
    if (FieldString.Offset > 0) {
      OS << '+';
    }

    OS << FieldString.Offset << '.';
  }

  if (FieldString.Loads != FieldStringNodeId::None) {
    OS << "L#" << uint32_t(FieldString.Loads) << '.';
  }

  for (auto Kl : FieldString.Kills) {
    OS << 'K' << Kl << '.';
  }

  if (FieldString.Loads != FieldStringNodeId::None) {
    OS << "S#" << uint32_t(FieldString.Loads) << '.';
  }

  return OS;
}

void AccessPath::print(llvm::raw_ostream &OS,
                       const FieldStringManager &Mgr) const {
  if (empty()) {
    OS << "ε";
    return;
  }

  if (Offset != 0) {
    if (Offset > 0) {
      OS << '+';
    }

    OS << Offset << '.';
  }

  for (auto Ld : Mgr.getFullFieldString(Loads)) {
    OS << 'L' << Ld << '.';
  }

  for (auto Kl : Kills) {
    OS << 'K' << Kl << '.';
  }

  for (auto St : Mgr.getFullFieldString(Stores)) {
    OS << 'S' << St << '.';
  }
}

llvm::raw_ostream &psr::cfl_fieldsens::operator<<(llvm::raw_ostream &OS,
                                                  const IFDSEdgeValue &EV) {
  assert(EV.Mgr != nullptr);
  if (EV.Paths.size() == 1) {
    EV.Paths.begin()->print(OS, *EV.Mgr);
    return OS;
  }

  OS << "{ ";
  llvm::interleaveComma(EV.Paths, OS, [&](const auto &FieldString) {
    FieldString.print(OS, *EV.Mgr);
  });
  return OS << " }";
}

InitialSeeds<IFDSDomain::n_t, IFDSDomain::d_t, IFDSDomain::l_t>
cfl_fieldsens::makeInitialSeeds(
    const InitialSeeds<LLVMIFDSAnalysisDomainDefault::n_t,
                       LLVMIFDSAnalysisDomainDefault::d_t, BinaryDomain>
        &UserSeeds,
    FieldStringManager &Mgr) {
  InitialSeeds<IFDSDomain::n_t, IFDSDomain::d_t,
               IFDSDomain::l_t>::GeneralizedSeeds Ret;

  for (const auto &[Inst, Facts] : UserSeeds.getSeeds()) {
    auto &SeedsAtInst = Ret[Inst];
    for (const auto &[Fact, Weight] : Facts) {
      SeedsAtInst.try_emplace(Fact, IFDSEdgeValue::epsilon(&Mgr));
    }
  }

  return {std::move(Ret)};
}

auto CFLFieldSensIFDSProblem::getStoreEdgeFunction(d_t CurrNode, d_t SuccNode,
                                                   d_t PointerOp, d_t ValueOp,
                                                   uint8_t DepthKLimit,
                                                   const llvm::DataLayout &DL)
    -> EdgeFunction<l_t> {
  auto [BasePtr, Offset] = getBaseAndOffset(PointerOp, DL);

  // TODO;: How to deal with BasePtr?

  auto [BaseBasePtr,
        BaseOffset] = [&]() -> std::pair<const llvm::Value *, int32_t> {
    if (BasePtr != SuccNode && llvm::isa<llvm::LoadInst>(BasePtr)) {
      return getBaseAndOffset(
          llvm::cast<llvm::LoadInst>(BasePtr)->getPointerOperand(), DL);
    }

    return {nullptr, INT32_MIN};
  }();
  if (CurrNode == SuccNode &&
      (BasePtr == CurrNode || BaseBasePtr == CurrNode)) {
    // Kill

    AccessPath FieldString{};
    FieldString.Kills.insert(Offset);
    return CFLFieldSensEdgeFunction::from(std::move(FieldString), Mgr,
                                          DepthKLimit);
  }

  if (ValueOp == CurrNode && CurrNode != SuccNode) {
    // Store

    AccessPath FieldString{};
    if (BasePtr != SuccNode && llvm::isa<llvm::LoadInst>(BasePtr)) {
      // This is a hack, to be more correct with field-insensitive alias
      // information

      if (BaseBasePtr == SuccNode) {
        // push before Offset, or after?
        FieldString.Stores = Mgr.prepend(BaseOffset, FieldString.Stores);
      }
    }

    FieldString.Stores = Mgr.prepend(Offset, FieldString.Stores);

    return CFLFieldSensEdgeFunction::from(std::move(FieldString), Mgr,
                                          DepthKLimit);
  }

  // unaffected by the store
  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getNormalEdgeFunction(n_t Curr, d_t CurrNode,
                                                    n_t /*Succ*/, d_t SuccNode)
    -> EdgeFunction<l_t> {
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "[getNormalEdgeFunction]:");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Curr: " << NToString(Curr));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  CurrNode: " << DToString(CurrNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  SuccNode: " << DToString(SuccNode));

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction::fromEpsilon(DepthKLimit, Mgr);
  }

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return getStoreEdgeFunction(CurrNode, SuccNode, Store->getPointerOperand(),
                                Store->getValueOperand(), DepthKLimit,
                                IRDB->getModule()->getDataLayout());
  }

  if (Curr == SuccNode) {

    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      // Load

      auto [BasePtr, Offset] = getBaseAndOffset(
          Load->getPointerOperand(), IRDB->getModule()->getDataLayout());

      // TODO;: How to deal with BasePtr?

      AccessPath FieldString{};
      FieldString.Loads = Mgr.prepend(Offset, FieldString.Loads);
      return CFLFieldSensEdgeFunction::from(std::move(FieldString), Mgr,
                                            DepthKLimit);
    }

    if (const auto *Gep = llvm::dyn_cast<llvm::GEPOperator>(Curr)) {
      auto OffsVal =
          getBaseAndOffset(Gep, IRDB->getModule()->getDataLayout()).second;

      AccessPath FieldString{};
      FieldString.Offset = OffsVal;
      return CFLFieldSensEdgeFunction::from(std::move(FieldString), Mgr,
                                            DepthKLimit);
    }
  }

  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getCallEdgeFunction(n_t CallSite, d_t SrcNode,
                                                  f_t /*DestinationFunction*/,
                                                  d_t DestNode)
    -> EdgeFunction<l_t> {
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "[getCallEdgeFunction]");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Curr: " << NToString(CallSite));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  CurrNode: " << DToString(SrcNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  SuccNode: " << DToString(DestNode));

  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction::fromEpsilon(DepthKLimit, Mgr);
  }

  // This is naturally identity
  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeFunction*/, n_t ExitStmt, d_t ExitNode,
    n_t /*RetSite*/, d_t RetNode) -> EdgeFunction<l_t> {
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "[getReturnEdgeFunction]");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Curr: " << NToString(ExitStmt));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  CurrNode: " << DToString(ExitNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  SuccNode: " << DToString(RetNode));

  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction::fromEpsilon(DepthKLimit, Mgr);
  }

  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t /*RetSite*/, d_t RetSiteNode,
    llvm::ArrayRef<f_t> /*Callees*/) -> EdgeFunction<l_t> {

  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "[getCallToRetEdgeFunction]");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Curr: " << NToString(CallSite));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  CurrNode: " << DToString(CallNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  SuccNode: " << DToString(RetSiteNode));

  if (CallNode == RetSiteNode && Config.KillsAt) {
    if (auto KillOffs = Config.KillsAt(CallSite, CallNode)) {
      // Let the summary-FF kill the fact

      // XXX: Can we somehow circumvent calling KillsAt twice? (once here, once
      // in getSummaryEdgeFunction())
      return AllTop<l_t>{};
    }
  }

  if (isZeroValue(CallNode) && !isZeroValue(RetSiteNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction::fromEpsilon(DepthKLimit, Mgr);
  }

  // This naturally identity
  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getSummaryEdgeFunction(n_t Curr, d_t CurrNode,
                                                     n_t /*Succ*/, d_t SuccNode)
    -> EdgeFunction<l_t> {

  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "[getSummaryEdgeFunction]");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory, "  Curr: " << NToString(Curr));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  CurrNode: " << DToString(CurrNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  SuccNode: " << DToString(SuccNode));

  if (CurrNode == SuccNode && Config.KillsAt) {
    if (auto KillOffs = Config.KillsAt(Curr, CurrNode)) {
      // kill
      PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                           "  > request to kill " << llvmIRToString(CurrNode)
                                                  << " with offset "
                                                  << *KillOffs);

      AccessPath FieldString{};
      FieldString.Kills.insert(*KillOffs);
      return CFLFieldSensEdgeFunction::from(std::move(FieldString), Mgr,
                                            DepthKLimit);
    }
  }

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction::fromEpsilon(DepthKLimit, Mgr);
  }

  // TODO: Is that correct? -- We may need to handle field-indirections here
  // as well
  return EdgeIdentity<l_t>{};
}

static void klimitPaths(auto &Paths, FieldStringManager &Mgr) {

  llvm::SmallDenseMap<AccessPath, llvm::SmallVector<AccessPath>, 2,
                      AccessPathDMI>
      ToInsert;
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;
    if (It->Stores != FieldStringNodeId::None) {
      AccessPath Approx = *It;
      auto StoresHead = Mgr[Approx.Stores];
      Approx.Stores = Mgr.prepend(AccessPath::TopOffset, StoresHead.Next);
      ToInsert[std::move(Approx)].push_back(*It);
      Paths.erase(It);
    }
  }
  for (auto &&[Approx, OrigPaths] : ToInsert) {
    if (OrigPaths.size() > 2) {
      Paths.insert(Approx);
    } else {
      Paths.insert(OrigPaths.begin(), OrigPaths.end());
    }
  }
}

static constexpr ptrdiff_t BreadthKLimit = 5;

auto CFLFieldSensIFDSProblem::extend(const EdgeFunction<l_t> &L,
                                     const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  auto Ret = [&]() -> EdgeFunction<l_t> {
    if (auto DfltCompose = psr::defaultComposeOrNull(L, R)) {
      return DfltCompose;
    }

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();

    if (FldSensL && FldSensR) {
      if (FldSensR->Transform.isEpsilon()) {
        return L;
      }

      if (FldSensL->Transform.Paths.empty()) {
        return L;
      }

      auto Txn = FldSensL->Transform;
      Txn.applyTransforms(FldSensR->Transform, DepthKLimit);

      if (Txn.Paths.empty()) {
        return AllTop<l_t>{};
      }

      if (Txn.Paths.size() > BreadthKLimit) {
        klimitPaths(Txn.Paths, Mgr);
      }
      return CFLFieldSensEdgeFunction::from(std::move(Txn), DepthKLimit);
    }

    llvm::report_fatal_error("[FieldSensAllocSitesAwareIFDSProblem::extend]: "
                             "Unexpected edge functions: " +
                             llvm::Twine(to_string(L)) + " EXTEND " +
                             llvm::Twine(to_string(R)));
  }();

  // if (!L.isa<EdgeIdentity<l_t>>() && !R.isa<EdgeIdentity<l_t>>()) {
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "EXTEND " << L << " X " << R << " ==> " << Ret);
  // }

  return Ret;
}

auto CFLFieldSensIFDSProblem::combine(const EdgeFunction<l_t> &L,
                                      const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  auto Ret = [&]() -> EdgeFunction<l_t> {
    if (auto Dflt = defaultJoinOrNullNoId(L, R)) {
      return Dflt;
    }

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();

    if (FldSensL) {
      if (FldSensR) {
        // A complicated way of expressing set-union of LPaths and RPaths.
        // Reason being that we don't want to unnecessarily copy the sets.
        // Rather, we like just incrementing the ref-count of L or R if somehow
        // possible.

        const auto &LPaths = FldSensL->Transform.Paths;
        const auto &RPaths = FldSensR->Transform.Paths;
        const auto LeftSz = LPaths.size();
        const auto RightSz = RPaths.size();
        const auto LeftSmaller = LeftSz < RightSz;

        if (LeftSz && RightSz) {
          const auto &Larger = LeftSmaller ? RPaths : LPaths;
          const auto &Smaller = LeftSmaller ? LPaths : RPaths;

          auto It = Smaller.begin();
          const auto End = Smaller.end();

          for (; It != End; ++It) {
            if (!Larger.contains(*It)) {
              auto Union = Larger;
              Union.insert(It, End);

              if (Union.size() > BreadthKLimit) {
                klimitPaths(Union, Mgr);
              }

              return CFLFieldSensEdgeFunction::from(
                  IFDSEdgeValue{.Mgr = &Mgr, .Paths = std::move(Union)},
                  DepthKLimit);
            }
          }
        }

        return LeftSmaller ? R : L;
      }

      if (R.isa<EdgeIdentity<l_t>>()) {
        if (FldSensL->Transform.Paths.contains(AccessPath{})) {
          return L;
        }

        auto Txn = FldSensL->Transform;
        Txn.Paths.insert(AccessPath{});
        return CFLFieldSensEdgeFunction::from(std::move(Txn), DepthKLimit);
      }
    } else if (FldSensR && L.isa<EdgeIdentity<l_t>>()) {
      if (FldSensR->Transform.Paths.contains(AccessPath{})) {
        return R;
      }

      auto Txn = FldSensR->Transform;
      Txn.Paths.insert(AccessPath{});
      return CFLFieldSensEdgeFunction::from(std::move(Txn), DepthKLimit);
    }

    llvm::errs() << "COMBINE " << L << " X " << R << " ==> AllBottom\n";

    return AllBottom<l_t>{};
  }();

  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "COMBINE " << L << " X " << R << " ==> " << Ret);

  return Ret;
}
