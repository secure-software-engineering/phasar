#include "phasar/PhasarLLVM/DataFlow/IfdsIde/CFLFieldSensIFDSProblem.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Domain/LatticeDomain.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Lazy.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"
#include "phasar/Utils/Printer.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
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
#include <type_traits>
#include <utility>

using namespace psr;
using namespace psr::cfl_fieldsens;

FieldStringManager::FieldStringManager() {
  // Sentinel
  NodeCompressor.insertDummy(
      FieldStringNode{.Next = FieldStringNodeId::None, .Offset = 0});
  Depth.push_back(0);
  // Empty kill-set has index 0
  KillsCompressor.getOrInsert({});
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
    // XXX: Optimize:
    auto Full = Mgr.getFullFieldString(F.Stores);
    Full.erase(Full.begin());
    F.Stores = Mgr.fromFullFieldString(Full);
  }
  F.Stores = Mgr.prepend(Field, F.Stores);
  return std::true_type{};
}

// Returns whether to retain F
[[nodiscard]] auto applyOneGepAndLoad(FieldStringManager &Mgr, AccessPath &F,
                                      int32_t Field, uint8_t DepthKLimit) {
  if (F.Stores == FieldStringNodeId::None) {
    auto Offs = F.Offset + Field;

    if (Mgr.isKilledBy(F.Kills, Offs)) {
      return false;
    }

    F.Offset = 0;

    // cf. Section 4.2.3 "K-Limiting" in the paper
    if (Mgr.depth(F.Loads) == DepthKLimit) {
      return true;
    }

    F.Loads = Mgr.prepend(Offs, F.Loads);
    F.Kills = KillSetId::Empty;
    return true;
  }

  auto StoresHead = Mgr[F.Stores];

  if (StoresHead.Offset != Field &&
      StoresHead.Offset != AccessPath::TopOffset) {
    return false;
  }

  assert(StoresHead.Offset == Field ||
         StoresHead.Offset == AccessPath::TopOffset);
  F.Stores = StoresHead.Next;
  return true;
}

[[nodiscard]] auto applyOneGepAndKill(FieldStringManager &Mgr, AccessPath &F,
                                      int32_t Field, uint8_t /*DepthKLimit*/) {
  if (Field == AccessPath::TopOffset) {
    // We cannot kill Top
    return true;
  }

  if (F.Stores == FieldStringNodeId::None) {
    auto Offs = addOffsets(F.Offset, Field);
    if (Offs == AccessPath::TopOffset) {
      // We cannot kill Top
      return true;
    }

    F.Kills = Mgr.addKill(F.Kills, Offs);
    PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory, "> add K" << Offs);
    return true;
  }

  auto StoresHead = Mgr[F.Stores];

  if (StoresHead.Offset == Field) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory,
                         "> Kill " << storesToString(F, Mgr));
    return false;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, IFDSEdgeValue::LogCategory,
                       "> Retain " << storesToString(F, Mgr));

  assert(StoresHead.Offset != Field);
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

static void applyTransformImpl(const IFDSEdgeValue::container_type &EV,
                               IFDSEdgeValue::container_type &Into,
                               FieldStringManager &Mgr, const AccessPath &Txn,
                               uint8_t DepthKLimit) {
  const auto TxnOffset = Txn.Offset;
  const auto TxnLoads = Mgr.getFullFieldString(Txn.Loads);
  const auto TxnStores = Mgr.getFullFieldString(Txn.Stores);
  const auto Kills = Mgr.kills(Txn.Kills); // safety copy

  for (const auto &F : EV) {
    auto Copy = F;
    bool Retain = [&] {
      if (TxnOffset) {
        if (!applyOneGep(Mgr, Copy, TxnOffset, DepthKLimit)) {
          return false;
        }
      }

      for (auto Ld : TxnLoads) {
        if (!applyOneGepAndLoad(Mgr, Copy, Ld, DepthKLimit)) {
          return false;
        }
      }

      for (auto Kl : Kills) {
        if (!applyOneGepAndKill(Mgr, Copy, Kl, DepthKLimit)) {
          return false;
        }
      }

      for (auto St : TxnStores) {
        if (!applyOneGepAndStore(Mgr, Copy, St, DepthKLimit)) {
          return false;
        }
      }

      return true;
    }();

    if (Retain) {
      Into.insert(Copy);
    }
  }
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

  applyTransformImpl(Save, EV.Paths, *EV.Mgr, Txn, DepthKLimit);
}

void applyTransformInto(const IFDSEdgeValue &EV, IFDSEdgeValue &Into,
                        const AccessPath &Txn, uint8_t DepthKLimit) {
  assert(&EV != &Into);
  assert(EV.Mgr == Into.Mgr);
  if (EV.Paths.empty() || Txn.empty()) {
    // Nothing to be done here
    return;
  }
  if (EV.isEpsilon()) {
    Into.Paths.insert(Txn);
    return;
  }

  applyTransformImpl(EV.Paths, Into.Paths, *EV.Mgr, Txn, DepthKLimit);
}

static auto &printOffset(llvm::raw_ostream &OS, int32_t Offset,
                         bool WithSign = false) {

  if (WithSign && (Offset > 0 || Offset == AccessPath::TopOffset)) {
    OS << '+';
  }
  if (Offset == AccessPath::TopOffset) {
    OS << 'T';
  } else {
    OS << Offset;
  }
  return OS;
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
      applyTransformInto(*this, Ret, *It, DepthKLimit);
    } else {
      Ret.Paths.insert(Paths.begin(), Paths.end());
    }
  }

  *this = std::move(Ret);
}

llvm::raw_ostream &
psr::cfl_fieldsens::operator<<(llvm::raw_ostream &OS,
                               const AccessPath &FieldString) {
  if (FieldString.empty()) {
    return OS << "ε";
  }

  if (FieldString.Offset) {
    printOffset(OS, FieldString.Offset, true) << '.';
  }

  if (FieldString.Loads != FieldStringNodeId::None) {
    OS << "L#" << uint32_t(FieldString.Loads) << '.';
  }

  if (FieldString.Kills != KillSetId::Empty) {
    OS << "K#" << uint32_t(FieldString.Kills) << '.';
  }

  if (FieldString.Stores != FieldStringNodeId::None) {
    OS << "S#" << uint32_t(FieldString.Stores) << '.';
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
    printOffset(OS, Offset, true) << '.';
  }

  for (auto Ld : Mgr.getFullFieldString(Loads)) {
    printOffset(OS << 'L', Ld) << '.';
  }

  for (auto Kl : Mgr.kills(Kills)) {
    printOffset(OS << 'K', Kl) << '.';
  }

  for (auto St : Mgr.getFullFieldString(Stores)) {
    printOffset(OS << 'S', St) << '.';
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

llvm::raw_ostream &cfl_fieldsens::operator<<(llvm::raw_ostream &OS,
                                             CFLFieldSensEdgeFunction EF) {
  return OS << "Txn[" << EF.Impl->Transform << ']';
}

EdgeFunction<l_t> CFLFieldSensIFDSProblem::makeEF(
    cfl_fieldsens::CFLFieldSensEdgeFunctionImpl &&EF) {
  auto It = EFInternCache.insert(std::move(EF));
  return CFLFieldSensEdgeFunction{&*It.first};
}
auto CFLFieldSensIFDSProblem::makeEFPtr(
    cfl_fieldsens::CFLFieldSensEdgeFunctionImpl &&EF) -> EFResultPtr {
  auto It = EFInternCache.insert(std::move(EF));
  return EFResultPtr{&*It.first};
}

auto CFLFieldSensIFDSProblem::getStoreEdgeFunction(d_t CurrNode, d_t SuccNode,
                                                   d_t PointerOp, d_t ValueOp,
                                                   uint8_t DepthKLimit,
                                                   const llvm::DataLayout &DL)
    -> EdgeFunction<l_t> {
  auto [BasePtr, Offset] = getBaseAndOffset(PointerOp, DL);

  // Trace the pointer chain from BasePtr toward SuccNode. DerefOffsets[0] is
  // the outermost GEP offset (closest to SuccNode). The Stores chain is built
  // with the outermost offset as HEAD so applyOneGepAndLoad matches in
  // traversal order (outermost first, matching the actual memory access
  // sequence).
  llvm::SmallVector<int32_t> DerefOffsets;
  const bool FoundSuccNode = walkLoadChainTo(
      BasePtr, SuccNode, DL, DepthKLimit, [&](int64_t ByteOffset) {
        DerefOffsets.push_back(ByteOffset != INT64_MIN ? int32_t(ByteOffset)
                                                       : AccessPath::TopOffset);
      });

  if (CurrNode == SuccNode && FoundSuccNode) {
    // Kill
    AccessPath FieldString{};
    FieldString.Kills = Mgr.addKill(FieldString.Kills, Offset);
    return makeEF(
        CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
  }

  // Also match when ValueOp is a zero-offset GEP of CurrNode (e.g. the -O0
  // arraydecay pattern where `%arraydecay = gep arr, 0, 0` is stored but the
  // tainted fact is `arr` itself).
  const auto *ValueBase = ValueOp->stripPointerCastsAndAliases();
  const bool IsValueCurrNode = ValueOp == CurrNode || ValueBase == CurrNode;

  if (IsValueCurrNode && CurrNode != SuccNode && FoundSuccNode) {
    // Store: prepend innermost first so the outermost becomes the HEAD.
    AccessPath FieldString{};
    FieldString.Stores = Mgr.prepend(Offset, FieldString.Stores);
    for (int32_t DerefOffset : llvm::reverse(DerefOffsets)) {
      FieldString.Stores = Mgr.prepend(DerefOffset, FieldString.Stores);
    }
    return makeEF(
        CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
  }

  // unaffected by the store
  return EdgeIdentity<l_t>{};
}

auto CFLFieldSensIFDSProblem::getLoadEdgeFunction(d_t CurrNode, d_t PointerOp,
                                                  uint8_t DepthKLimit,
                                                  const llvm::DataLayout &DL)
    -> EdgeFunction<l_t> {

  const auto *ZeroOffsBase = PointerOp->stripPointerCastsAndAliases();
  if (CurrNode == PointerOp || CurrNode == ZeroOffsBase) {
    // Note: Offsets handled in GEP below
    AccessPath FieldString{};
    FieldString.Loads = Mgr.prepend(/*Offset*/ 0, FieldString.Loads);
    return makeEF(
        CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
  }

  // In case CurrNode!=PointerOp, we filter llvm values for allocation sites.
  // GEPs are no alloc-sites!
  // => we must handle the offsetting here in the load, without relying it being
  //    handled in the GEP-EF

  auto [BasePtr, Offset] = getBaseAndOffset(ZeroOffsBase, DL);

  AccessPath FieldString{};
  FieldString.Loads = Mgr.prepend(Offset, FieldString.Stores);
  return makeEF(
      CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
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

    return makeEF(CFLFieldSensEdgeFunctionImpl::fromEpsilon(DepthKLimit, Mgr));
  }

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    return getStoreEdgeFunction(CurrNode, SuccNode, Store->getPointerOperand(),
                                Store->getValueOperand(), DepthKLimit,
                                IRDB->getModule()->getDataLayout());
  }

  if (Curr == SuccNode) {

    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      return getLoadEdgeFunction(CurrNode, Load->getPointerOperand(),
                                 DepthKLimit,
                                 IRDB->getModule()->getDataLayout());
    }

    if (const auto *Gep = llvm::dyn_cast<llvm::GEPOperator>(Curr)) {
      auto OffsVal =
          getBaseAndOffset(Gep, IRDB->getModule()->getDataLayout()).second;

      AccessPath FieldString{};
      FieldString.Offset = OffsVal;
      return makeEF(
          CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
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

    return makeEF(CFLFieldSensEdgeFunctionImpl::fromEpsilon(DepthKLimit, Mgr));
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

    return makeEF(CFLFieldSensEdgeFunctionImpl::fromEpsilon(DepthKLimit, Mgr));
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

    return makeEF(CFLFieldSensEdgeFunctionImpl::fromEpsilon(DepthKLimit, Mgr));
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
      FieldString.Kills = Mgr.addKill(FieldString.Kills, *KillOffs);
      return makeEF(
          CFLFieldSensEdgeFunctionImpl::from(FieldString, Mgr, DepthKLimit));
    }
  }

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    // Gen from zero

    return makeEF(CFLFieldSensEdgeFunctionImpl::fromEpsilon(DepthKLimit, Mgr));
  }

  // TODO: Is that correct? -- We may need to handle field-indirections here
  // as well
  return EdgeIdentity<l_t>{};
}

static void klimitPaths(auto &Paths, FieldStringManager &Mgr) {
  llvm::SmallDenseMap<AccessPath, llvm::SmallVector<AccessPath>, 2,
                      AccessPathDMI>
      ToInsert;
  ToInsert.reserve(Paths.size()); // retained across .clear() calls below

  // Merge stores
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;
    if (It->Stores != FieldStringNodeId::None) {
      AccessPath Approx = *It;
      auto StoresHead = Mgr[Approx.Stores];
      Approx.Stores = Mgr.prepend(AccessPath::TopOffset, StoresHead.Next);
      ToInsert[Approx].push_back(*It);
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

  // Merge geps
  ToInsert.clear();
  for (const AccessPath &AP : Paths) {
    auto NoOffs = AP;
    NoOffs.Offset = AccessPath::TopOffset;
    ToInsert[NoOffs].push_back(AP);
  }
  Paths.clear();
  for (auto &&[Approx, OrigPaths] : ToInsert) {
    if (OrigPaths.size() > 2) {
      Paths.insert(Approx);
    } else {
      Paths.insert(OrigPaths.begin(), OrigPaths.end());
    }
  }

  // Merge loads
  ToInsert.clear();
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;
    if (It->Loads != FieldStringNodeId::None) {
      AccessPath Approx = *It;
      auto LoadsHead = Mgr[Approx.Loads];
      Approx.Loads = Mgr.prepend(AccessPath::TopOffset, LoadsHead.Next);
      ToInsert[Approx].push_back(*It);
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

  // Merge Kills
  ToInsert.clear();
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;

    AccessPath Approx = *It;
    Approx.Kills = {};
    ToInsert[Approx].push_back(*It);
    Paths.erase(It);
  }
  for (auto &&[Approx, OrigPaths] : ToInsert) {
    if (OrigPaths.size() > 2) {
      auto KillSet = Mgr.kills(OrigPaths.front().Kills);
      for (const auto &AP : llvm::drop_begin(OrigPaths)) {
        KillSet.intersectWith(Mgr.kills(AP.Kills));
      }

      auto ApproxMut = Approx;
      ApproxMut.Kills = Mgr.internKills(std::move(KillSet));
      Paths.insert(std::move(ApproxMut));
    } else {
      Paths.insert(OrigPaths.begin(), OrigPaths.end());
    }
  }
}

static constexpr ptrdiff_t BreadthKLimit = 5;
static constexpr ptrdiff_t WidenKLimit = 128;

static constexpr unsigned AllEFPtrId = 0;
static constexpr unsigned AllBottomId = 1;
static constexpr unsigned AllTopId = 2;

[[nodiscard]] static llvm::PointerIntPair<const CFLFieldSensEdgeFunctionImpl *,
                                          2>
allBotPtr() noexcept {
  return {nullptr, AllBottomId};
}

[[nodiscard]] static llvm::PointerIntPair<const CFLFieldSensEdgeFunctionImpl *,
                                          2>
allTopPtr() noexcept {
  return {nullptr, AllTopId};
}

[[nodiscard]] static EdgeFunction<l_t>
getResultEF(llvm::PointerIntPair<const CFLFieldSensEdgeFunctionImpl *, 2>
                Ptr) noexcept {
  PAMM_GET_INSTANCE;
  switch (Ptr.getInt()) {
  [[likely]] case AllEFPtrId:
    INC_COUNTER("getResultEF Ptr", 1, Full);
    assert(Ptr.getPointer() != nullptr);
    assert(Ptr.getPointer() == Ptr.getOpaqueValue() &&
           "Zero-tag does not pollute the alignment bits");
    return CFLFieldSensEdgeFunction{
        static_cast<const CFLFieldSensEdgeFunctionImpl *>(
            Ptr.getOpaqueValue())};
  case AllBottomId:
    INC_COUNTER("getResultEF Bot", 1, Full);
    return AllBottom<l_t>{};
  case AllTopId:
    INC_COUNTER("getResultEF Top", 1, Full);
    return AllTop<l_t>{};
  default:
    llvm_unreachable("All valid tags should be handled explicitly");
  }
}

void CFLFieldSensIFDSProblem::regCounters() noexcept {
  PAMM_GET_INSTANCE;

  REG_COUNTER("ExtendCache Refs", 0, Full);
  REG_COUNTER("ExtendCache Misses", 0, Full);

  REG_COUNTER("CombineCache Refs", 0, Full);
  REG_COUNTER("CombineCache Misses", 0, Full);
  REG_COUNTER("Combine CallsTotal", 0, Full);
  REG_COUNTER("Combine LIdentity", 0, Full);
  REG_COUNTER("Combine LIdentitySlow", 0, Full);
  REG_COUNTER("Combine RIdentity", 0, Full);
  REG_COUNTER("Combine RIdentitySlow", 0, Full);

  REG_COUNTER("getResultEF Top", 0, Full);
  REG_COUNTER("getResultEF Bot", 0, Full);
  REG_COUNTER("getResultEF Ptr", 0, Full);
}

auto CFLFieldSensIFDSProblem::extend(const EdgeFunction<l_t> &L,
                                     const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  auto Ret = [&]() -> EdgeFunction<l_t> {
    if (auto DfltCompose = psr::defaultComposeOrNull(L, R)) {
      return DfltCompose;
    }

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();

    if (!FldSensL || !FldSensR) {
      llvm::report_fatal_error("[CFLFieldSensIFDSProblem::extend]: "
                               "Unexpected edge functions: " +
                               llvm::Twine(to_string(L)) + " EXTEND " +
                               llvm::Twine(to_string(R)));
    }

    if (FldSensR->Impl->Transform.isEpsilon()) {
      return L;
    }

    PAMM_GET_INSTANCE;

    INC_COUNTER("ExtendCache Refs", 1, Full);

    auto [It, Inserted] = ExtendCache.try_emplace(
        std::pair{FldSensL->Impl, FldSensR->Impl}, lazy{[&]() -> EFResultPtr {
          INC_COUNTER("ExtendCache Misses", 1, Full);

          auto Txn = FldSensL->Impl->Transform;
          Txn.applyTransforms(FldSensR->Impl->Transform, DepthKLimit);

          if (Txn.Paths.empty()) {
            return allTopPtr();
          }

          if (Txn.Paths.size() > BreadthKLimit) {
            klimitPaths(Txn.Paths, Mgr);
            if (Txn.Paths.size() > WidenKLimit) {
              return allBotPtr();
            }
          }

          return makeEFPtr(
              CFLFieldSensEdgeFunctionImpl::from(std::move(Txn), DepthKLimit));
        }});

    return getResultEF(It->second);
  }();

  if (!L.isa<EdgeIdentity<l_t>>() && !R.isa<EdgeIdentity<l_t>>()) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "EXTEND " << L << " X " << R << " ==> " << Ret);
  }

  return Ret;
}

auto CFLFieldSensIFDSProblem::combine(const EdgeFunction<l_t> &L,
                                      const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  if (auto Dflt = defaultJoinOrNullNoId(L, R)) {
    return Dflt;
  }
  auto Ret = [&]() -> EdgeFunction<l_t> {
    PAMM_GET_INSTANCE;
    INC_COUNTER("Combine CallsTotal", 1, Full);

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();
    if (FldSensL) {
      if (FldSensR) {

        INC_COUNTER("CombineCache Refs", 1, Full);
        auto [CacheIt, CacheInserted] = CombineCache.try_emplace(
            psr::minmaxVal(FldSensL->Impl, FldSensR->Impl),
            lazy{[this, FldSensL{*FldSensL},
                  FldSensR{*FldSensR}]() -> EFResultPtr {
              PAMM_GET_INSTANCE;
              INC_COUNTER("CombineCache Misses", 1, Full);

              // A complicated way of expressing set-union of LPaths and RPaths.
              // Reason being that we don't want to unnecessarily copy the sets.
              // Rather, we like just incrementing the ref-count of L or R if
              // somehow possible.

              const auto &RPaths = FldSensR.Impl->Transform.Paths;
              const auto &LPaths = FldSensL.Impl->Transform.Paths;
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

                    // NOTE: No k-limit in combine()!!! Otherwise, we may loose
                    // monotonicity of the lattice!

                    if (Union.size() > WidenKLimit) {
                      return allBotPtr();
                    }

                    return makeEFPtr(CFLFieldSensEdgeFunctionImpl::from(
                        IFDSEdgeValue{.Mgr = &Mgr, .Paths = std::move(Union)},
                        DepthKLimit));
                  }
                }
              }

              return EFResultPtr{LeftSmaller ? FldSensR.Impl : FldSensL.Impl};
            }});

        return getResultEF(CacheIt->second);
      }

      if (R.isa<EdgeIdentity<l_t>>()) {
        INC_COUNTER("Combine RIdentity", 1, Full);
        if (FldSensL->Impl->Transform.Paths.contains(AccessPath{})) {
          return L;
        }

        INC_COUNTER("Combine RIdentitySlow", 1, Full);
        auto Txn = FldSensL->Impl->Transform;
        Txn.Paths.insert(AccessPath{});
        return makeEF(
            CFLFieldSensEdgeFunctionImpl::from(std::move(Txn), DepthKLimit));
      }
    } else if (FldSensR && L.isa<EdgeIdentity<l_t>>()) {
      INC_COUNTER("Combine LIdentity", 1, Full);
      if (FldSensR->Impl->Transform.Paths.contains(AccessPath{})) {
        return R;
      }

      INC_COUNTER("Combine LIdentitySlow", 1, Full);
      auto Txn = FldSensR->Impl->Transform;
      Txn.Paths.insert(AccessPath{});
      return makeEF(
          CFLFieldSensEdgeFunctionImpl::from(std::move(Txn), DepthKLimit));
    }

    llvm::errs() << "COMBINE " << L << " X " << R << " ==> AllBottom\n";

    return AllBottom<l_t>{};
  }();

  if (L != R) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                         "COMBINE " << L << " X " << R << " ==> " << Ret
                                    << "; Ret==L: " << (Ret == L)
                                    << "; Ret==R: " << (Ret == R));
  }

  return Ret;
}
