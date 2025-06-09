#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FieldSensAllocSitesAwareIFDSProblem.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunction.h"
#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"
#include "phasar/Utils/Fn.h"
#include "phasar/Utils/JoinLattice.h"
#include "phasar/Utils/Union.h"

#include "llvm/ADT/Hashing.h"
#include "llvm/Support/ErrorHandling.h"

#include <functional>
#include <numeric>
#include <utility>

using namespace psr;

namespace {

using l_t = LatticeDomain<CFLFieldSensEdgeValue>;

struct FieldSensEdgeFunctionComposer : EdgeFunctionComposer<l_t> {

  static EdgeFunction<l_t>
  join(EdgeFunctionRef<FieldSensEdgeFunctionComposer> This,
       const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Use combine() instead!");
  }
};

struct StoreEdgeFunction {
  using l_t = LatticeDomain<CFLFieldSensEdgeValue>;

  [[nodiscard]] l_t computeTarget(l_t Source) const {
    Source.onValue(fn<&CFLFieldSensEdgeValue::applyStore>);
    return Source;
  }

  static EdgeFunction<l_t> compose(EdgeFunctionRef<StoreEdgeFunction> This,
                                   const EdgeFunction<l_t> &SecondFunction) {
    llvm::report_fatal_error("Use extend() instead!");
  }

  static EdgeFunction<l_t> join(EdgeFunctionRef<StoreEdgeFunction> This,
                                const EdgeFunction<l_t> &OtherFunction) {
    llvm::report_fatal_error("Use combine() instead!");
  }
};
} // namespace

void CFLFieldSensEdgeValue::applyStore() {
  for (auto &F : Paths) {
    // TODO: K-limiting!
    F.Stores.push_back(std::exchange(F.Offset, 0));
  }

  // TODO: What if Paths is empty? Or can't that happen?
}

void CFLFieldSensEdgeValue::applyLoad() {
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;
    auto &F = *It;

    auto Offs = std::exchange(F.Offset, 0);
    if (F.Stores.empty()) {
      if (F.kills(Offs)) {
        Paths.erase(It);
      } else {
        // TODO: K-limiting!
        F.Loads.push_back(Offs);
        F.Kills.clear();
      }
      continue;
    }

    if (F.Stores.back() != Offs) {
      Paths.erase(It);
      continue;
    }

    assert(F.Stores.back() == Offs);
    F.Stores.pop_back();
  }
}
void CFLFieldSensEdgeValue::applyKill() {
  for (auto IIt = Paths.begin(), End = Paths.end(); IIt != End;) {
    auto It = IIt++;
    auto &F = *It;

    if (F.Stores.empty()) {
      F.Kills.insert(F.Offset);
      continue;
    }

    if (F.Stores.back() == F.Offset) {
      Paths.erase(It);
      continue;
    }

    assert(F.Stores.back() != F.Offset);
    // fallthrough
  }
}
void CFLFieldSensEdgeValue::applyGep(GEPEvent Evt) {
  for (auto &F : Paths) {
    F.Offset += Evt.Field;
    // TODO: k-limiting
  }
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

auto FieldSensAllocSitesAwareIFDSProblem::initialSeeds()
    -> InitialSeeds<n_t, d_t, l_t> {
  auto UserSeeds = UserProblem->initialSeeds();
  InitialSeeds<n_t, d_t, l_t>::GeneralizedSeeds Ret;

  for (const auto &[Inst, Facts] : UserSeeds.getSeeds()) {
    auto &SeedsAtInst = Ret[Inst];
    for (const auto &[Fact, Weight] : Facts) {
      SeedsAtInst[Fact] = {};
    }
  }

  return {std::move(Ret)};
}

auto FieldSensAllocSitesAwareIFDSProblem::getNormalEdgeFunction(
    n_t Curr, d_t CurrNode, n_t Succ, d_t SuccNode) -> EdgeFunction<l_t> {
  // TODO: Store, Load, Gep

  return nullptr;
}

auto FieldSensAllocSitesAwareIFDSProblem::getCallEdgeFunction(
    n_t CallSite, d_t SrcNode, f_t DestinationFunction, d_t DestNode)
    -> EdgeFunction<l_t> {
  // This is naturally identity
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getReturnEdgeFunction(
    n_t CallSite, f_t CalleeFunction, n_t ExitStmt, d_t ExitNode, n_t RetSite,
    d_t RetNode) -> EdgeFunction<l_t> {
  // TODO: Need to map the fields to the ret-site

  return nullptr;
}

auto FieldSensAllocSitesAwareIFDSProblem::getCallToRetEdgeFunction(
    n_t CallSite, d_t CallNode, n_t RetSite, d_t RetSiteNode,
    llvm::ArrayRef<f_t> Callees) -> EdgeFunction<l_t> {
  // This naturally identity
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::getSummaryEdgeFunction(
    n_t Curr, d_t CurrNode, n_t Succ, d_t SuccNode) -> EdgeFunction<l_t> {
  // TODO: Is that correct? -- We may need to handle field-indirections here as
  // well
  return EdgeIdentity<l_t>{};
}

auto FieldSensAllocSitesAwareIFDSProblem::extend(const EdgeFunction<l_t> &L,
                                                 const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  if (auto DfltCompose = psr::defaultComposeOrNull(L, R)) {
    return DfltCompose;
  }

  if (R.isa<AllBottom<l_t>>()) {
    return R;
  }

  if (L.isConstant()) {
    auto FieldStringSet = R.computeTarget(L.computeTarget(bottomElement()));

    if (FieldStringSet.isBottom()) {
      return AllBottom<l_t>{};
    }
    if (FieldStringSet.isTop()) {
      llvm::errs() << "WARNING: We should never produce TOP!";
      return AllTop<l_t>{};
    }

    return ConstantEdgeFunction<l_t>{
        NonTopBotValue<l_t>::unwrap(std::move(FieldStringSet)),
    };
  }

  return FieldSensEdgeFunctionComposer{{L, R}};
}

auto FieldSensAllocSitesAwareIFDSProblem::combine(const EdgeFunction<l_t> &L,
                                                  const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  if (L.isa<EdgeIdentity<l_t>>()) {
    // TODO
  }

  if (auto DfltJoin = psr::defaultJoinOrNull(L, R)) {
    return DfltJoin;
  }

  if (L.isConstant() && R.isConstant()) {
    auto LSet = L.computeTarget(bottomElement());
    auto RSet = R.computeTarget(bottomElement());

    if (LSet.isBottom() || RSet.isBottom()) {
      return AllBottom<l_t>{};
    }

    assert(!LSet.isTop() && !RSet.isTop());

    bool LeftSmaller =
        LSet.assertGetValue().Paths.size() < RSet.assertGetValue().Paths.size();

    bool Changed = false;
    auto Union = setUnion(std::move(LSet.assertGetValue().Paths),
                          std::move(RSet.assertGetValue().Paths), &Changed);

    if (Changed) {
      return ConstantEdgeFunction<l_t>{{std::move(Union)}};
    }

    return LeftSmaller ? L : R;
  }

  // TODO: Join

  return AllBottom<l_t>{};
}
