#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FieldSensAllocSitesAwareIFDSProblem.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Union.h"

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <cstring>
#include <functional>
#include <numeric>
#include <type_traits>
#include <utility>

using namespace psr;

namespace {

using l_t = LatticeDomain<CFLFieldSensEdgeValue>;

constexpr static int32_t addOffsets(int32_t L, int32_t R) noexcept {
  if (L == CFLFieldAccessPath::TopOffset ||
      R == CFLFieldAccessPath::TopOffset) {
    return CFLFieldAccessPath::TopOffset;
  }

  int32_t Sum{};
  if (llvm::AddOverflow(L, R, Sum)) {
    return CFLFieldAccessPath::TopOffset;
  }

  return Sum;
}

struct CFLFieldSensEdgeFunction {
  using l_t = LatticeDomain<CFLFieldSensEdgeValue>;
  CFLFieldSensEdgeValue Transform{};
  uint8_t DepthKLimit{};

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    Source.onValue(fn<&CFLFieldSensEdgeValue::applyTransforms>, Transform,
                   DepthKLimit);
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
};

[[nodiscard]] std::string storesToString(const CFLFieldAccessPath &AP) {
  std::string Ret;
  llvm::raw_string_ostream ROS(Ret);

  llvm::interleave(
      AP.Stores, ROS, [&ROS](auto StoreOffs) { ROS << 'S' << StoreOffs; }, ".");

  return Ret;
}

// Returns whether to retain F
[[nodiscard]] auto applyOneGepAndStore(CFLFieldAccessPath &F, GEPEvent Evt,
                                       uint8_t DepthKLimit) {
  if (F.Stores.size() == DepthKLimit) {
    // TODO: Optimize:
    F.Stores.erase(F.Stores.begin());
  }
  F.Stores.push_back(std::exchange(F.Offset, 0) + Evt.Field);
  return std::true_type{};
}

// Returns whether to retain F
[[nodiscard]] auto applyOneGepAndLoad(CFLFieldAccessPath &F, GEPEvent Evt,
                                      uint8_t DepthKLimit) {
  auto Offs = F.Offset + Evt.Field;
  if (F.Stores.empty()) {

    if (F.kills(Offs)) {
      return false;
    }

    F.Offset = 0;

    // TODO: Is this application of k-limiting correct here?
    // cf. Section 4.2.3 "K-Limiting" in the paper
    if (F.Loads.size() == DepthKLimit) {
      return true;
    }

    F.Loads.push_back(Offs);
    F.Kills.clear();
    return true;
  }

  if (F.Stores.back() != Offs) {
    return false;
  }

  assert(F.Stores.back() == Offs);
  F.Offset = 0;
  F.Stores.pop_back();
  // llvm::errs() << "> pop_back\n";
  return true;
}

[[nodiscard]] auto applyOneGepAndKill(CFLFieldAccessPath &F, GEPEvent Evt,
                                      uint8_t /*DepthKLimit*/) {
  auto Offs = F.Offset + Evt.Field;

  if (F.Stores.empty()) {
    F.Kills.insert(Offs);
    PHASAR_LOG_LEVEL_CAT(DEBUG, CFLFieldSensEdgeValue::LogCategory,
                         "> add K" << Offs);
    return true;
  }

  if (F.Stores.back() == Offs) {
    PHASAR_LOG_LEVEL_CAT(DEBUG, CFLFieldSensEdgeValue::LogCategory,
                         "> Kill " << storesToString(F));
    return false;
  }

  PHASAR_LOG_LEVEL_CAT(DEBUG, CFLFieldSensEdgeValue::LogCategory,
                       "> Retain " << storesToString(F));

  assert(F.Stores.back() != Offs);
  return true;
}

[[nodiscard]] auto applyOneGep(CFLFieldAccessPath &F, GEPEvent Evt,
                               uint8_t /*DepthKLimit*/) {
  if (F.Stores.empty()) {
    F.Offset = addOffsets(F.Offset, Evt.Field);
  } else {
    F.Stores.back() = addOffsets(F.Stores.back(), -Evt.Field);
  }
  return std::true_type{};
}

} // namespace

void CFLFieldSensEdgeValue::applyGepAndStore(GEPEvent Evt,
                                             uint8_t DepthKLimit) {
  auto Save = std::exchange(Paths, {});
  Paths.reserve(Save.size());

  for (auto F : Save) {
    // TODO: Check, whether we can safely exchange Offset with 0 here!

    if (F.Stores.size() == DepthKLimit) {
      // TODO: Optimize:
      F.Stores.erase(F.Stores.begin());
    }
    F.Stores.push_back(std::exchange(F.Offset, 0) + Evt.Field);
    Paths.insert(std::move(F));
  }

  // TODO: What if Paths is empty? Or can't that happen?
  // --> Does not happen, as long as the fact is not killed in all paths
}

void CFLFieldSensEdgeValue::applyGepAndLoad(GEPEvent Evt, uint8_t DepthKLimit) {
  llvm::errs() << "[applyGepAndLoad]: " << *this << " + " << Evt.Field << "\n";

  auto Save = std::exchange(Paths, {});

  for (const auto &F : Save) {
    auto Offs = F.Offset + Evt.Field;
    if (F.Stores.empty()) {

      if (F.kills(Offs)) {
        continue;
      }
      auto FF = F;
      FF.Offset = 0;

      // TODO: Is this application of k-limiting correct here?
      // cf. Section 4.2.3 "K-Limiting" in the paper
      if (F.Loads.size() == DepthKLimit) {
        Paths.insert(std::move(FF));
        continue;
      }

      FF.Loads.push_back(Offs);
      FF.Kills.clear();
      Paths.insert(std::move(FF));

      continue;
    }

    if (F.Stores.back() != Offs) {
      continue;
    }

    assert(F.Stores.back() == Offs);
    auto FF = F;
    FF.Offset = 0;
    FF.Stores.pop_back();
    Paths.insert(std::move(FF));
    llvm::errs() << "> pop_back\n";
  }

  llvm::errs() << "=> " << *this << '\n';
}

void CFLFieldSensEdgeValue::applyGepAndKill(GEPEvent Evt) {
  llvm::errs() << "[applyGepAndKill]: " << *this << " + " << Evt.Field << "\n";

  auto Save = std::exchange(Paths, {});

  for (const auto &F : Save) {
    auto Offs = F.Offset + Evt.Field;

    if (F.Stores.empty()) {
      auto FF = F;
      FF.Kills.insert(Offs);
      Paths.insert(std::move(FF));
      llvm::errs() << "> add K" << Offs << '\n';
      continue;
    }

    if (F.Stores.back() == Offs) {
      llvm::errs() << "> Kill ";
      llvm::interleave(
          F.Stores, llvm::errs(),
          [](auto StoreOffs) { llvm::errs() << 'S' << StoreOffs; }, ".");
      llvm::errs() << '\n';
      continue;
    }

    llvm::errs() << "> Retain ";
    llvm::interleave(
        F.Stores, llvm::errs(),
        [](auto StoreOffs) { llvm::errs() << 'S' << StoreOffs; }, ".");
    llvm::errs() << '\n';

    assert(F.Stores.back() != Offs);
    Paths.insert(F);
  }
}

void CFLFieldSensEdgeValue::applyGep(GEPEvent Evt) {
  auto Save = std::exchange(Paths, {});
  Paths.reserve(Save.size());

  for (auto F : Save) {
    if (F.Stores.empty()) {
      F.Offset = addOffsets(F.Offset, Evt.Field);
    } else {
      F.Stores.back() = addOffsets(F.Stores.back(), -Evt.Field);
    }
    Paths.insert(std::move(F));
  }
}

void CFLFieldSensEdgeValue::applyStore(uint8_t DepthKLimit) {
  applyGepAndStore(GEPEvent{0}, DepthKLimit);
}
void CFLFieldSensEdgeValue::applyLoad(uint8_t DepthKLimit) {
  applyGepAndLoad(GEPEvent{0}, DepthKLimit);
}
void CFLFieldSensEdgeValue::applyKill() { //
  applyGepAndKill(GEPEvent{0});
}

void CFLFieldSensEdgeValue::applyTransform(const CFLFieldAccessPath &Txn,
                                           uint8_t DepthKLimit) {
  if (Paths.empty() || Txn.empty()) {
    // Nothing to be done here
    return;
  }
  if (Paths.size() == 1 && Paths.begin()->empty()) {
    Paths.clear();
    Paths.insert(Txn);
    return;
  }
  // llvm::errs() << "[applyTransform]: " << *this << " X " << Txn << '\n';

  auto Save = std::exchange(Paths, {});
  Paths.reserve(Save.size());

  const auto TxnOffset = Txn.Offset;

  for (const auto &F : Save) {
    auto Copy = F;
    bool Retain = [&] {
      if (TxnOffset) {
        if (!applyOneGep(Copy, GEPEvent{TxnOffset}, DepthKLimit)) {
          return false;
        }
      }
      for (auto Ld : Txn.Loads) {
        if (!applyOneGepAndLoad(Copy, GEPEvent{Ld}, DepthKLimit)) {
          return false;
        }
      }

      for (auto Kl : Txn.Kills) {
        if (!applyOneGepAndKill(Copy, GEPEvent{Kl}, DepthKLimit)) {
          return false;
        }
      }

      for (auto St : Txn.Stores) {
        if (!applyOneGepAndStore(Copy, GEPEvent{St}, DepthKLimit)) {
          return false;
        }
      }

      return true;
    }();

    if (Retain) {
      Paths.insert(std::move(Copy));
    }
  }

  // llvm::errs() << "[applyTransform]: > result: " << *this << '\n';

  // // TODO: Optimize!

  // if (Txn.Offset) {
  //   applyGep(GEPEvent{Txn.Offset});
  // }

  // for (auto Ld : Txn.Loads) {
  //   applyGepAndLoad(GEPEvent{Ld}, DepthKLimit);
  // }
  // for (auto Kl : Txn.Kills) {
  //   applyGepAndKill(GEPEvent{Kl});
  // }
  // for (auto St : Txn.Stores) {
  //   applyGepAndStore(GEPEvent{St}, DepthKLimit);
  // }
}

void CFLFieldSensEdgeValue::applyTransforms(const CFLFieldSensEdgeValue &Txns,
                                            uint8_t DepthKLimit) {
  if (Txns.Paths.empty()) {
    Paths.clear();
    return;
  }

  auto It = Txns.Paths.begin();
  if (Txns.Paths.size() == 1) [[likely]] {
    applyTransform(*It, DepthKLimit);
    return;
  }

  // This path should be very rare, otherwise we will for sure have a
  // performance problem...

  auto End = Txns.Paths.end();
  auto Ret = *this;

  Ret.applyTransform(*It, DepthKLimit);

  for (++It; It != End; ++It) {
    if (!It->empty()) {
      auto Tmp = *this;
      Tmp.applyTransform(*It, DepthKLimit);
      Ret.Paths.insert(Tmp.Paths.begin(), Tmp.Paths.end());
    } else {
      Ret.Paths.insert(Paths.begin(), Paths.end());
    }
  }

  *this = std::move(Ret);
}

size_t psr::hash_value(const CFLFieldAccessPath &FieldString) noexcept {
  auto HCL = llvm::hash_combine_range(FieldString.Loads.begin(),
                                      FieldString.Loads.end());
  auto HCS = llvm::hash_combine_range(FieldString.Stores.begin(),
                                      FieldString.Stores.end());
  // Xor does not care about the order
  auto HCK = std::accumulate(FieldString.Kills.begin(), FieldString.Kills.end(),
                             0, std::bit_xor<>{});
  return llvm::hash_combine(HCL, HCS, HCK);
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const CFLFieldAccessPath &FieldString) {
  if (FieldString.empty()) {
    return OS << "ε";
  }

  if (FieldString.Offset) {
    if (FieldString.Offset > 0) {
      OS << '+';
    }

    OS << FieldString.Offset << '.';
  }

  for (auto Ld : FieldString.Loads) {
    OS << 'L' << Ld << '.';
  }

  for (auto Kl : FieldString.Kills) {
    OS << 'K' << Kl << '.';
  }

  for (auto St : FieldString.Stores) {
    OS << 'S' << St << '.';
  }

  return OS;
}

llvm::raw_ostream &psr::operator<<(llvm::raw_ostream &OS,
                                   const CFLFieldSensEdgeValue &EV) {
  if (EV.Paths.size() == 1) {
    return OS << *EV.Paths.begin();
  }

  OS << "{ ";
  llvm::interleaveComma(EV.Paths, OS);
  return OS << " }";
}

auto FieldSensAllocSitesAwareIFDSProblem::initialSeeds()
    -> InitialSeeds<n_t, d_t, l_t> {
  auto UserSeeds = UserProblem->initialSeeds();
  InitialSeeds<n_t, d_t, l_t>::GeneralizedSeeds Ret;

  for (const auto &[Inst, Facts] : UserSeeds.getSeeds()) {
    auto &SeedsAtInst = Ret[Inst];
    for (const auto &[Fact, Weight] : Facts) {
      SeedsAtInst[Fact] = CFLFieldSensEdgeValue{{CFLFieldAccessPath{}}};
    }
  }

  return {std::move(Ret)};
}

auto FieldSensAllocSitesAwareIFDSProblem::getNormalEdgeFunction(
    n_t Curr, d_t CurrNode, n_t /*Succ*/, d_t SuccNode) -> EdgeFunction<l_t> {
  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction{{{CFLFieldAccessPath{}}}, DepthKLimit};
  }

  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    const auto *PointerOp = Store->getPointerOperand();

    // TODO;: How to deal with BasePtr?

    const auto &DL = IRDB->getModule()->getDataLayout();
    auto [BasePtr, Offset] = getBaseAndOffset(PointerOp, DL);

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

      CFLFieldAccessPath FieldString{};
      FieldString.Kills.insert(Offset);
      return CFLFieldSensEdgeFunction{{{std::move(FieldString)}}, DepthKLimit};
    }

    const auto *ValueOp = Store->getValueOperand();
    if (ValueOp == CurrNode && CurrNode != SuccNode) {
      // Store

      CFLFieldAccessPath FieldString{};
      if (BasePtr != SuccNode && llvm::isa<llvm::LoadInst>(BasePtr)) {
        // This is a hack, to be more correct wih field-insensitive alias
        // information
        auto [BaseBasePtr, BaseOffset] = getBaseAndOffset(
            llvm::cast<llvm::LoadInst>(BasePtr)->getPointerOperand(), DL);
        if (BaseBasePtr == SuccNode) {
          // push before Offset, or after?
          FieldString.Stores.push_back(BaseOffset);
        }
      }

      FieldString.Stores.push_back(Offset);

      return CFLFieldSensEdgeFunction{{{std::move(FieldString)}}, DepthKLimit};
    }

    // unaffected by the store
    return EdgeIdentity<l_t>{};
  }

  if (Curr == SuccNode) {

    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
      // Load

      auto [BasePtr, Offset] = getBaseAndOffset(
          Load->getPointerOperand(), IRDB->getModule()->getDataLayout());

      int32_t LoadOffs = 0;

      // if (BasePtr == CurrNode && Load->getPointerOperand() != CurrNode) {
      // This is a hack, but we do sth similar in the IDEExtendedTaintAnalysis
      // (see forEachAliasOf() Lines 144 ff)
      LoadOffs = Offset;
      // }

      // TODO;: How to deal with BasePtr?

      CFLFieldAccessPath FieldString{};
      FieldString.Loads.push_back(LoadOffs);
      // llvm::errs() << "Handle load: " << llvmIRToString(Load) << '\n';
      // llvm::errs() << "> CurrNode: " << llvmIRToString(CurrNode) << '\n';
      return CFLFieldSensEdgeFunction{{{std::move(FieldString)}}, DepthKLimit};
    }

    if (const auto *Gep = llvm::dyn_cast<llvm::GEPOperator>(Curr)) {
      auto OffsVal =
          getBaseAndOffset(Gep, IRDB->getModule()->getDataLayout()).second;

      CFLFieldAccessPath FieldString{};
      FieldString.Offset = OffsVal;
      return CFLFieldSensEdgeFunction{{{std::move(FieldString)}}, DepthKLimit};
    }
  }

  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getCallEdgeFunction(
    n_t /*CallSite*/, d_t SrcNode, f_t /*DestinationFunction*/, d_t DestNode)
    -> EdgeFunction<l_t> {
  if (isZeroValue(SrcNode) && !isZeroValue(DestNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction{{{CFLFieldAccessPath{}}}, DepthKLimit};
  }

  // This is naturally identity
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getReturnEdgeFunction(
    n_t /*CallSite*/, f_t /*CalleeFunction*/, n_t /*ExitStmt*/, d_t ExitNode,
    n_t /*RetSite*/, d_t RetNode) -> EdgeFunction<l_t> {
  if (isZeroValue(ExitNode) && !isZeroValue(RetNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction{{{CFLFieldAccessPath{}}}, DepthKLimit};
  }

  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t /*RetSite*/, d_t RetSiteNode,
    llvm::ArrayRef<f_t> /*Callees*/) -> EdgeFunction<l_t> {

  if (CallNode == RetSiteNode && Config.KillsAt) {
    if (auto KillOffs = Config.KillsAt(CallSite, CallNode)) {
      // Let the summary-FF kill the fact

      // XXX: Can we somehow circumvent calling KillsAt twice?
      return AllTop<l_t>{};
    }
  }

  if (isZeroValue(CallNode) && !isZeroValue(RetSiteNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction{{{CFLFieldAccessPath{}}}, DepthKLimit};
  }

  // This naturally identity
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getSummaryEdgeFunction(
    n_t Curr, d_t CurrNode, n_t /*Succ*/, d_t SuccNode) -> EdgeFunction<l_t> {

  PHASAR_LOG_LEVEL_CAT(
      DEBUG, LogCategory,
      "[getSummaryEdgeFunction]: Curr: " << llvmIRToString(Curr) << ":");
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  > CurrNode: " << llvmIRToString(CurrNode));
  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "  > SuccNode: " << llvmIRToString(SuccNode));

  if (CurrNode == SuccNode && Config.KillsAt) {
    if (auto KillOffs = Config.KillsAt(Curr, CurrNode)) {
      // kill
      PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                           "  > request to kill " << llvmIRToString(CurrNode)
                                                  << " with offset "
                                                  << *KillOffs);

      CFLFieldAccessPath FieldString{};
      FieldString.Kills.insert(*KillOffs);
      return CFLFieldSensEdgeFunction{{{std::move(FieldString)}}, DepthKLimit};
    }
  }

  if (isZeroValue(CurrNode) && !isZeroValue(SuccNode)) {
    // Gen from zero

    return CFLFieldSensEdgeFunction{{{CFLFieldAccessPath{}}}, DepthKLimit};
  }

  // TODO: Is that correct? -- We may need to handle field-indirections here
  // as well
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::extend(const EdgeFunction<l_t> &L,
                                                 const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  auto Ret = [&]() -> EdgeFunction<l_t> {
    if (auto DfltCompose = psr::defaultComposeOrNull(L, R)) {
      return DfltCompose;
    }

    if (R.isa<AllBottom<l_t>>()) {
      return R;
    }

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();

    if (FldSensL && FldSensR) {

      // TODO: Be smarter with copying the transforms:
      auto Txn = FldSensL->Transform;
      Txn.applyTransforms(FldSensR->Transform, DepthKLimit);
      // TODO: k-limit the number of paths!
      return CFLFieldSensEdgeFunction{std::move(Txn), DepthKLimit};
    }

    llvm::report_fatal_error("[FieldSensAllocSitesAwareIFDSProblem::extend]: "
                             "Unexpected edge functions: " +
                             llvm::Twine(to_string(L)) + " EXTEND " +
                             llvm::Twine(to_string(R)));
  }();

  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "EXTEND " << L << " X " << R << " ==> " << Ret);

  return Ret;
}

auto FieldSensAllocSitesAwareIFDSProblem::combine(const EdgeFunction<l_t> &L,
                                                  const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  auto Ret = [&]() -> EdgeFunction<l_t> {
    if (llvm::isa<AllBottom<l_t>>(R) || llvm::isa<AllTop<l_t>>(L)) {
      return R;
    }
    if (llvm::isa<AllTop<l_t>>(R) || llvm::isa<AllBottom<l_t>>(L)) {
      return L;
    }

    if (llvm::isa<EdgeIdentity<l_t>>(L) && llvm::isa<EdgeIdentity<l_t>>(R)) {
      return L;
    }

    const auto *FldSensL = L.dyn_cast<CFLFieldSensEdgeFunction>();
    const auto *FldSensR = R.dyn_cast<CFLFieldSensEdgeFunction>();

    if (FldSensL) {
      if (FldSensR) {

        const bool LeftSmaller =
            FldSensL->Transform.Paths.size() < FldSensR->Transform.Paths.size();

        bool Changed = false;
        auto Union = setUnion(FldSensL->Transform.Paths,
                              FldSensR->Transform.Paths, &Changed);

        if (Changed) {
          // TODO: k-limit the number of paths!
          return CFLFieldSensEdgeFunction{{std::move(Union)}, DepthKLimit};
        }

        return LeftSmaller ? R : L;
      }

      if (R.isa<EdgeIdentity<l_t>>()) {
        if (FldSensL->Transform.Paths.contains(CFLFieldAccessPath{})) {
          return L;
        }

        auto Txn = FldSensL->Transform;
        Txn.Paths.insert(CFLFieldAccessPath{});
        return CFLFieldSensEdgeFunction{std::move(Txn), DepthKLimit};
      }
    } else if (FldSensR && L.isa<EdgeIdentity<l_t>>()) {
      if (FldSensR->Transform.Paths.contains(CFLFieldAccessPath{})) {
        return R;
      }

      auto Txn = FldSensR->Transform;
      Txn.Paths.insert(CFLFieldAccessPath{});
      return CFLFieldSensEdgeFunction{std::move(Txn), DepthKLimit};
    }

    return AllBottom<l_t>{};
  }();

  PHASAR_LOG_LEVEL_CAT(DEBUG, LogCategory,
                       "COMBINE " << L << " X " << R << " ==> " << Ret);

  return Ret;
}

// static std::pair<const llvm::Value *, llvm::SmallVector<int32_t, 10>>
// createAccessPath(const llvm::Value *Pointer, const llvm::DataLayout &DL) {

//   std::pair<const llvm::Value *, llvm::SmallVector<int32_t, 10>> Ret;
//   auto &[BasePtr, Offsets] = Ret;

//   BasePtr = Pointer;
//   Offsets.push_back(0);

//   // Note: llvm::Constant includes llvm::GlobalValue
//   if (llvm::isa<llvm::Constant>(Pointer) ||
//       llvm::isa<llvm::AllocaInst>(Pointer) ||
//       llvm::isa<llvm::Argument>(Pointer) ||
//       llvm::isa<llvm::CallBase>(Pointer)) {
//     // Globals, argument, function calls and allocas define themselves
//     return Ret;
//   }

//   while (true) {
//     // TODO: Should we look into the cache within this loop?
//     // TODO: Handle constant GEPs

//     if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(BasePtr)) {
//       Offsets.push_back(0);
//       BasePtr = Load->getPointerOperand()->stripPointerCasts();
//     } else if (const auto *Gep =
//     llvm::dyn_cast<llvm::GEPOperator>(BasePtr))
//     {

//       auto GepOffs = detail::AbstractMemoryLocationImpl::computeOffset(DL,
//       Gep); if (GepOffs.has_value() && *GepOffs >= INT32_MIN &&
//           *GepOffs <= INT32_MAX) {
//         Offsets.back() = addOffsets(Offsets.back(), int32_t(*GepOffs));
//       } else {
//         Offsets.back() = CFLFieldAccessPath::TopOffset;
//       }
//       BasePtr = Gep->getPointerOperand()->stripPointerCasts();
//     } else {
//       // TODO aggregate instructions, e.g. insertvalue, extractvalue, ...
//       break;
//     }
//   }

//   // NOTE: Do not reverse the offsets as we do in
//   // AbstractMemoryLocationFactoryBase::createImpl().
//   // For the CFL formulation, we need the offsets in inverse order anyway!

//   return Ret;
// }

// auto FieldSensAllocSitesAwareIFDSProblem::getAccessPath(
//     const llvm::Value *Pointer) -> const CachedAccessPath * {
//   auto &Ret = MemLocCache[Pointer];
//   if (Ret) {
//     return Ret;
//   }

//   auto [BasePtr, Offsets] =
//       createAccessPath(Pointer, IRDB->getModule()->getDataLayout());
//   assert(Offsets.size() < UINT32_MAX);

//   using OffsetType = CachedAccessPath::OffsetType;

//   auto NumBytes =
//       CachedAccessPath::totalSizeToAlloc<OffsetType>(Offsets.size());
//   auto *RawMem = MemLocAlloc.allocate(NumBytes);
//   auto *AP = new (RawMem) CachedAccessPath(BasePtr, Offsets.size());
//   memcpy(AP->getTrailingObjects<OffsetType>(), Offsets.data(),
//          Offsets.size() * sizeof(OffsetType));

//   Ret = AP;
//   return AP;
// }
