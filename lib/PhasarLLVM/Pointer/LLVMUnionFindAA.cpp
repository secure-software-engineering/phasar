#include "phasar/PhasarLLVM/Pointer/LLVMUnionFindAA.h"

#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/RawAliasSet.h"
#include "phasar/Utils/ValueCompressor.h"

namespace psr {

template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
template class IndirectionSensUnionFindAA<LLVMPAGDomain>;

detail::LLVMLocalUnionFindAliasIteratorBase::
    LLVMLocalUnionFindAliasIteratorBase(
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
