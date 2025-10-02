#include "phasar/PhasarLLVM/ControlFlow/Resolver/VTAResolver.h"

#include "phasar/PhasarLLVM/ControlFlow/Resolver/PrecomputedResolver.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"
#include "phasar/PhasarLLVM/ControlFlow/VTA/TypePropagator.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/MaybeUniquePtr.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/InstrTypes.h"

using namespace psr;

void VTAResolver::DefaultReachableFunctions::operator()(
    const LLVMProjectIRDB &IRDB,
    llvm::function_ref<void(const llvm::Function *)> WithFun) {
  llvm::for_each(IRDB.getAllFunctions(), WithFun);
}

static VTAResolver createWithBaseCGResolver(
    const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
    MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG, vta::AliasInfoTy AS) {
  auto ReachableFunctions =
      [BaseCG = BaseCG.get()](
          const LLVMProjectIRDB &,
          llvm::function_ref<void(const llvm::Function *)> WithFun) {
        llvm::for_each(BaseCG->getAllVertexFunctions(), WithFun);
      };
  auto BaseRes =
      std::make_unique<PrecomputedResolver>(IRDB, VTP, std::move(BaseCG));

  return VTAResolver(IRDB, VTP, AS, std::move(BaseRes), ReachableFunctions);
}

VTAResolver::VTAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP, vta::AliasInfoTy AS,
                         MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG)
    : psr::VTAResolver(
          createWithBaseCGResolver(IRDB, VTP, std::move(BaseCG), AS)) {}

VTAResolver::VTAResolver(
    const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
    vta::AliasInfoTy AS, MaybeUniquePtr<Resolver> BaseRes,
    llvm::function_ref<void(const LLVMProjectIRDB &,
                            llvm::function_ref<void(const llvm::Function *)>)>
        ReachableFunctions)
    : Resolver(IRDB, VTP), BaseResolver(std::move(BaseRes)) {
  assert(this->BaseResolver != nullptr);

  auto TAG = vta::computeTypeAssignmentGraph(
      *IRDB, *VTP, AS, *this->BaseResolver, ReachableFunctions);

  auto [SCCs, Order] = computeSCCsAndTopologicalOrder(TAG);
  auto Deps = computeSCCDependencies(TAG, SCCs);

  TA = vta::propagateTypes(TAG, SCCs, Deps, Order);

  // TAG.print(llvm::errs());
  // TA.print(llvm::errs(), TAG, SCCs);

  this->SCCs = std::move(SCCs);
  Nodes = std::move(TAG.Nodes);
}

VTAResolver::VTAResolver(const LLVMProjectIRDB *IRDB,
                         const LLVMVFTableProvider *VTP, LLVMAliasInfoRef AS,
                         MaybeUniquePtr<const LLVMBasedCallGraph> BaseCG)
    : VTAResolver(
          IRDB, VTP,
          [AS](const llvm::Value *Ptr, const llvm::Instruction *At,
               vta::AliasHandlerTy WithAlias) {
            auto ASet = AS.getAliasSet(Ptr, At);
            llvm::for_each(*ASet, WithAlias);
          },
          std::move(BaseCG)) {}

VTAResolver::VTAResolver(
    const LLVMProjectIRDB *IRDB, const LLVMVFTableProvider *VTP,
    LLVMAliasInfoRef AS, MaybeUniquePtr<Resolver> BaseRes,
    llvm::function_ref<void(const LLVMProjectIRDB &,
                            llvm::function_ref<void(const llvm::Function *)>)>
        ReachableFunctions)
    : VTAResolver(
          IRDB, VTP,
          [AS](const llvm::Value *Ptr, const llvm::Instruction *At,
               vta::AliasHandlerTy WithAlias) {
            auto ASet = AS.getAliasSet(Ptr, At);
            llvm::for_each(*ASet, WithAlias);
          },
          std::move(BaseRes), ReachableFunctions) {}

std::string VTAResolver::str() const { return "VTA"; }

void VTAResolver::resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                     const llvm::CallBase *CallSite) {

  auto RetrievedVtableIndex = getVFTIndex(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return;
  }

  auto *CalledOp = CallSite->getCalledOperand()->stripPointerCastsAndAliases();
  auto VtableIndex = RetrievedVtableIndex.value();

  auto BaseCallees = BaseResolver->resolveIndirectCall(CallSite);

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
              PossibleTargets.insert(Fun);
            }
          }
        }
      }
    }
  }

  auto TNId = Nodes.getOrNull({vta::Variable{CalledOp}});
  if (TNId) {
    auto SCC = SCCs.SCCOfNode[*TNId];
    const auto &Types = TA.TypesPerSCC[SCC];
    for (auto Ty : Types) {
      if (const auto *Fun = Ty.dyn_cast<const llvm::Function *>()) {
        if (psr::isConsistentCall(CallSite, Fun) &&
            (BaseCallees.empty() || BaseCallees.contains(Fun))) {
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
  auto BaseCallees = BaseResolver->resolveIndirectCall(CallSite);

  auto TNId = Nodes.getOrNull({vta::Variable{
      CallSite->getCalledOperand()->stripPointerCastsAndAliases()}});
  if (TNId) {
    auto SCC = SCCs.SCCOfNode[*TNId];
    const auto &Types = TA.TypesPerSCC[SCC];
    for (auto Ty : Types) {
      if (const auto *Fun = Ty.dyn_cast<const llvm::Function *>()) {
        if (psr::isConsistentCall(CallSite, Fun) &&
            (BaseCallees.empty() || BaseCallees.contains(Fun))) {
          PossibleTargets.insert(Fun);
        }
      }
    }
  }

  if (PossibleTargets.empty()) {
    PossibleTargets = std::move(BaseCallees);
  }
}
