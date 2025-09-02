#include "phasar/PhasarLLVM/ControlFlow/Resolver/VTAResolver.h"

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypePropagator.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/InstrTypes.h"

using namespace psr;

VTAResolver::VTAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP,
                         const LLVMBasedCallGraph *BaseCG, vta::AliasInfoTy AS)
    : Resolver(IRDB, VTP), BaseCG(BaseCG) {
  assert(BaseCG != nullptr);

  auto TAG =
      vta::computeTypeAssignmentGraph(*IRDB->getModule(), *BaseCG, AS, *VTP);

  SCCs = computeSCCs(TAG);
  auto Deps = computeSCCDependencies(TAG, SCCs);
  auto Order = computeSCCOrder(SCCs, Deps);
  TA = vta::propagateTypes(TAG, SCCs, Deps, Order);

  // TAG.print(llvm::errs());
  // TA.print(llvm::errs(), TAG, SCCs);

  Nodes = std::move(TAG.Nodes);
}

std::string VTAResolver::str() const { return "VTA"; }

void VTAResolver::resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                     const llvm::CallBase *CallSite) {

  // llvm::errs() << "[resolveVirtualCall] At " << llvmIRToString(CallSite)
  //              << '\n';

  // TODO: Use getVFTIndexAndVT(), once #785 is merged
  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return;
  }

  auto *VT = CallSite->getCalledOperand()->stripPointerCastsAndAliases();
  auto VtableIndex = RetrievedVtableIndex.value();

  auto BaseCalleesVec = BaseCG->getCalleesOfCallAt(CallSite);
  llvm::SmallDenseSet<const llvm::Function *> BaseCallees(
      BaseCalleesVec.begin(), BaseCalleesVec.end());

  auto ReceiverIdx = CallSite->hasStructRetAttr();
  if (CallSite->arg_size() > ReceiverIdx) {
    const auto *Receiver = CallSite->getArgOperand(ReceiverIdx);
    if (auto ReceiverNod = Nodes.getOrNull({vta::Variable{Receiver}})) {
      auto SCC = SCCs.SCCOfNode[*ReceiverNod];
      const auto *ReceiverType = getReceiverType(CallSite);

      const auto &Types = TA.TypesPerSCC[SCC];
      for (auto Ty : Types) {
        if (const auto *DITy = Ty.dyn_cast<const llvm::DIType *>()) {
          if (const auto *Fun = getNonPureVirtualVFTEntry(
                  DITy, VtableIndex, CallSite, ReceiverType)) {
            if (psr::isConsistentCall(CallSite, Fun) &&
                (BaseCallees.empty() || BaseCallees.contains(Fun))) {
              // llvm::errs() << "  Add possible target " << Fun->getName()
              //              << " through vtable lookup at index " <<
              //              VtableIndex
              //              << " on type " << llvmTypeToString(DITy) << '\n';
              PossibleTargets.insert(Fun);
            }
          }
        }
      }
    }
  }

  auto TNId = Nodes.getOrNull({vta::Variable{VT}});
  if (TNId) {
    auto SCC = SCCs.SCCOfNode[*TNId];
    const auto &Types = TA.TypesPerSCC[SCC];
    for (auto Ty : Types) {
      if (const auto *Fun = Ty.dyn_cast<const llvm::Function *>()) {
        if (psr::isConsistentCall(CallSite, Fun) &&
            (BaseCallees.empty() || BaseCallees.contains(Fun))) {
          // llvm::errs() << "  Add possible target " << Fun->getName()
          //              << " through direct function pointer\n";
          PossibleTargets.insert(Fun);
        }
      }
    }
  }

  if (PossibleTargets.empty()) {
    PossibleTargets = std::move(BaseCallees);
  }
}

void VTAResolver::resolveFunctionPointer(FunctionSetTy &PossibleTargets,
                                         const llvm::CallBase *CallSite) {
  // llvm::errs() << "[resolveFunctionPointer] At " << llvmIRToString(CallSite)
  //              << '\n';

  auto BaseCalleesVec = BaseCG->getCalleesOfCallAt(CallSite);
  llvm::SmallDenseSet<const llvm::Function *> BaseCallees(
      BaseCalleesVec.begin(), BaseCalleesVec.end());

  auto TNId = Nodes.getOrNull({vta::Variable{
      CallSite->getCalledOperand()->stripPointerCastsAndAliases()}});
  if (TNId) {
    auto SCC = SCCs.SCCOfNode[*TNId];
    const auto &Types = TA.TypesPerSCC[SCC];
    for (auto Ty : Types) {
      if (const auto *Fun = Ty.dyn_cast<const llvm::Function *>()) {
        if (psr::isConsistentCall(CallSite, Fun) &&
            (BaseCallees.empty() || BaseCallees.contains(Fun))) {
          // llvm::errs() << "  Add possible target " << Fun->getName()
          //              << " through direct function pointer\n";
          PossibleTargets.insert(Fun);
        }
      }
    }
  }

  if (PossibleTargets.empty()) {
    PossibleTargets = std::move(BaseCallees);
  }
}
