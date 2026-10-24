#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Pointer/UnionFindAA.h"
#include "phasar/Utils/ValueCompressor.h"

#include <utility>

namespace psr {

template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
template class IndirectionSensUnionFindAA<LLVMPAGDomain>;
template class BottomupUnionFindAA<LLVMPAGDomain>;

detail::LLVMLocalRawAliasIteratorBase::LLVMLocalRawAliasIteratorBase(
    const ValueCompressor<PAGVariable> &VC) {
  RawAliasSet<ValueId> Globals;
  for (const auto &[VId, Vars] : VC.id2vars().enumerate()) {
    for (auto V : Vars) {
      if (const auto *LLVMVar = V.valueOrNull()) {
        if (const auto *Fun = psr::getFunction(LLVMVar)) {
          GlobalsOrInFun[Fun].insert(VId);
        } else {
          Globals.insert(VId);
        }
      }
    }
  }

  for (auto &[Fun, Vars] : GlobalsOrInFun) {
    Vars |= Globals;
  }
}

} // namespace psr

using namespace psr;

CallingContextSensUnionFindAAResult psr::computeCtxSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = CallingContextSensUnionFindAA<LLVMPAGDomain>{
      &CG,
      &IRDB,
  };
  return computeUnionFindAARaw(IRDB, std::move(Strategy), std::move(VC));
}

BasicUnionFindAAResult psr::computeBotCtxSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = BottomupUnionFindAA<LLVMPAGDomain>{
      ReverseCGGraph{
          &CG,
          &IRDB,
      },
  };
  return computeUnionFindAARaw(IRDB, std::move(Strategy), std::move(VC));
}

BasicUnionFindAAResult psr::computeIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = pag::PBMixin{
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
      pag::LLVMCGProvider{&CG},
  };
  return computeUnionFindAARaw(IRDB, std::move(Strategy), std::move(VC));
}

RawAAResultIntersection<CallingContextSensUnionFindAAResult,
                        BasicUnionFindAAResult>
psr::computeCtxIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = UnionFindAACombinator{
      CallingContextSensUnionFindAA<LLVMPAGDomain>{
          &CG,
          &IRDB,
      },
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
  };
  return computeUnionFindAARaw(IRDB, std::move(Strategy), std::move(VC));
}

RawAAResultIntersection<BasicUnionFindAAResult, BasicUnionFindAAResult>
psr::computeBotCtxIndSensUnionFindAARaw(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = UnionFindAACombinator{
      BottomupUnionFindAA<LLVMPAGDomain>{
          ReverseCGGraph{
              &CG,
              &IRDB,
          },
      },
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
  };
  return computeUnionFindAARaw(IRDB, std::move(Strategy), std::move(VC));
}

LLVMRawAliasIterator<CallingContextSensUnionFindAAResult>
psr::computeCtxSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = CallingContextSensUnionFindAA<LLVMPAGDomain>{
      &CG,
      &IRDB,
  };
  return computeUnionFindAA(IRDB, std::move(Strategy), std::move(VC));
}

LLVMRawAliasIterator<BasicUnionFindAAResult> psr::computeBotCtxSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = BottomupUnionFindAA<LLVMPAGDomain>{
      ReverseCGGraph{
          &CG,
          &IRDB,
      },
  };
  return computeUnionFindAA(IRDB, std::move(Strategy), std::move(VC));
}

LLVMRawAliasIterator<BasicUnionFindAAResult> psr::computeIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = pag::PBMixin{
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
      pag::LLVMCGProvider{&CG},
  };
  return computeUnionFindAA(IRDB, std::move(Strategy), std::move(VC));
}

LLVMRawAliasIterator<RawAAResultIntersection<
    CallingContextSensUnionFindAAResult, BasicUnionFindAAResult>>
psr::computeCtxIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = UnionFindAACombinator{
      CallingContextSensUnionFindAA<LLVMPAGDomain>{
          &CG,
          &IRDB,
      },
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
  };
  return computeUnionFindAA(IRDB, std::move(Strategy), std::move(VC));
}

LLVMRawAliasIterator<
    RawAAResultIntersection<BasicUnionFindAAResult, BasicUnionFindAAResult>>
psr::computeBotCtxIndSensUnionFindAA(
    const LLVMProjectIRDB &IRDB, const LLVMBasedCallGraph &CG,
    MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  auto Strategy = UnionFindAACombinator{
      BottomupUnionFindAA<LLVMPAGDomain>{
          ReverseCGGraph{
              &CG,
              &IRDB,
          },
      },
      IndirectionSensUnionFindAA<LLVMPAGDomain>{},
  };
  return computeUnionFindAA(IRDB, std::move(Strategy), std::move(VC));
}
