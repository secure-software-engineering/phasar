#include "phasar/PhasarLLVM/DataFlow/IfdsIde/FieldSensAllocSitesAwareIFDSProblem.h"

#include "phasar/DataFlow/IfdsIde/EdgeFunctionUtils.h"

using namespace psr;

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

  // TODO: Here, the real magic happens!
  // --> Look in the paper at pages 12-13
}

auto FieldSensAllocSitesAwareIFDSProblem::combine(const EdgeFunction<l_t> &L,
                                                  const EdgeFunction<l_t> &R)
    -> EdgeFunction<l_t> {
  if (auto DfltJoin = psr::defaultJoinOrNull(L, R)) {
    return DfltJoin;
  }

  // TODO: Join

  return AllBottom<l_t>{};
}
