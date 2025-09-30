#include "phasar/PhasarLLVM/ControlFlow/Resolver/AliasBasedResolver.h"

#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"

using namespace psr;

static unsigned makeBaseResFlags(Resolver *BaseResolver,
                                 bool UseBaseResolverAsFallback,
                                 bool FilterTargetsOnBase) noexcept {
  if (!BaseResolver) {
    return 0;
  }

  return int(UseBaseResolverAsFallback) & (int(FilterTargetsOnBase) << 1);
}

static bool useBaseResolverAsFallback(
    llvm::PointerIntPair<Resolver *, 2> FallbackResolverAndFlags) noexcept {
  return FallbackResolverAndFlags.getInt() & 1;
}
static bool filterTargetsOnBase(
    llvm::PointerIntPair<Resolver *, 2> FallbackResolverAndFlags) noexcept {
  return (FallbackResolverAndFlags.getInt() >> 1) & 1;
}

AliasBasedResolver::AliasBasedResolver(const LLVMProjectIRDB *IRDB,
                                       const LLVMVFTableProvider *VTP,
                                       LLVMAliasIteratorRef AAInfo,
                                       Resolver *BaseResolver,
                                       bool UseBaseResolverAsFallback,
                                       bool FilterTargetsOnBase)
    : Resolver(IRDB, VTP), AAInfo(AAInfo),
      FallbackResolverAndFlags(BaseResolver,
                               makeBaseResFlags(BaseResolver,
                                                UseBaseResolverAsFallback,
                                                FilterTargetsOnBase)) {}

void AliasBasedResolver::resolveVirtualCall(FunctionSetTy &PossibleTargets,
                                            const llvm::CallBase *CallSite) {
  PHASAR_LOG_LEVEL(DEBUG,
                   "Call virtual function: " << llvmIRToString(CallSite));

  auto RetrievedVtableIndex = getVFTIndexAndVT(CallSite);
  if (!RetrievedVtableIndex.has_value()) {
    // An error occured
    PHASAR_LOG_LEVEL(DEBUG,
                     "Error with resolveVirtualCall : impossible to retrieve "
                     "the vtable index\n"
                         << llvmIRToString(CallSite) << "\n");
    return;
  }

  auto [VtablePtr, VtableIndex] = RetrievedVtableIndex.value();

  PHASAR_LOG_LEVEL(DEBUG, "Virtual function table entry is: " << VtableIndex);

  FunctionSetTy BaseCallees;
  if (filterTargetsOnBase(FallbackResolverAndFlags)) {
    assert(FallbackResolverAndFlags.getPointer() != nullptr &&
           "This must be ensured in the ctor");
    FallbackResolverAndFlags.getPointer()->resolveVirtualCall(BaseCallees,
                                                              CallSite);
  }

  AAInfo.forallAliasesOf(
      VtablePtr, CallSite, [&, VtableIndex = VtableIndex](const auto *P) {
        if (const auto *PGV = llvm::dyn_cast<llvm::GlobalVariable>(
                P->stripInBoundsConstantOffsets())) {
          if (PGV->hasName() &&
              PGV->getName().startswith(DIBasedTypeHierarchy::VTablePrefix) &&
              PGV->hasInitializer()) {
            if (const auto *PCS = llvm::dyn_cast<llvm::ConstantStruct>(
                    PGV->getInitializer())) {
              auto VFs = LLVMVFTable::getVFVectorFromIRVTable(*PCS);
              if (VtableIndex >= VFs.size()) {
                return;
              }
              const auto *Callee = VFs[VtableIndex];
              if (Callee == nullptr || !Callee->hasName() ||
                  Callee->getName() ==
                      DIBasedTypeHierarchy::PureVirtualCallName ||
                  !isConsistentCall(CallSite, Callee) ||
                  (!BaseCallees.empty() && !BaseCallees.count(Callee))) {
                return;
              }
              PossibleTargets.insert(Callee);
            }
          }
        }
      });

  if (PossibleTargets.empty() &&
      useBaseResolverAsFallback(FallbackResolverAndFlags)) {
    if (!BaseCallees.empty()) {
      PossibleTargets = std::move(BaseCallees);
    } else {
      FallbackResolverAndFlags.getPointer()->resolveVirtualCall(PossibleTargets,
                                                                CallSite);
    }
  }
}

void AliasBasedResolver::resolveFunctionPointer(
    FunctionSetTy &PossibleTargets, const llvm::CallBase *CallSite) {
  if (!CallSite->getCalledOperand()) {
    return;
  }

  FunctionSetTy BaseCallees;
  if (filterTargetsOnBase(FallbackResolverAndFlags)) {
    assert(FallbackResolverAndFlags.getPointer() != nullptr &&
           "This must be ensured in the ctor");
    FallbackResolverAndFlags.getPointer()->resolveFunctionPointer(BaseCallees,
                                                                  CallSite);
  }

  llvm::SmallVector<const llvm::GlobalVariable *, 2> GlobalVariableWL;
  llvm::SmallVector<const llvm::ConstantAggregate *> ConstantAggregateWL;
  llvm::SmallPtrSet<const llvm::ConstantAggregate *, 4>
      VisitedConstantAggregates;

  AAInfo.forallAliasesOf(
      CallSite->getCalledOperand(), CallSite, [&](const auto *P) {
        if (!llvm::isa<llvm::Constant>(P)) {
          return;
        }

        GlobalVariableWL.clear();
        ConstantAggregateWL.clear();

        // First check, whether the alias is directly a function -- the easy
        // case
        if (const auto *F = llvm::dyn_cast<llvm::Function>(P)) {
          if (isConsistentCall(CallSite, F)) {
            PossibleTargets.insert(F);
          }
          return;
        }

        // If it is a global, or a gep on a global, we need to check nested
        // function pointers in the global's initializer. We cannot expect the
        // alias analysis to be field-sensitive, or even indirection-sensitive
        // at this point. When we actually have alias analyses that are
        // sensitive to field offsets or pointer indirections, we should ask the
        // analysis for this information and early exit the below iteration (see
        // PT.getAnalysisProperties()).

        if (const auto *GVP = llvm::dyn_cast<llvm::GlobalVariable>(P)) {
          GlobalVariableWL.push_back(GVP);
        } else if (const auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(P)) {
          for (const auto &Op : CE->operands()) {
            if (const auto *GVOp = llvm::dyn_cast<llvm::GlobalVariable>(Op)) {
              GlobalVariableWL.push_back(GVOp);
            }
          }
        }

        if (GlobalVariableWL.empty()) {
          return;
        }

        for (const auto *GV : GlobalVariableWL) {
          if (!GV->hasInitializer()) {
            continue;
          }
          const auto *InitConst = GV->getInitializer();
          if (const auto *InitConstAggregate =
                  llvm::dyn_cast<llvm::ConstantAggregate>(InitConst)) {
            ConstantAggregateWL.push_back(InitConstAggregate);
          }
        }

        VisitedConstantAggregates.clear();

        while (!ConstantAggregateWL.empty()) {
          const auto *ConstAggregateItem = ConstantAggregateWL.pop_back_val();
          // We may have already processed the item, avoid an infinite loop
          if (!VisitedConstantAggregates.insert(ConstAggregateItem).second) {
            continue;
          }
          for (const auto &Op : ConstAggregateItem->operands()) {
            if (const auto *CE = llvm::dyn_cast<llvm::ConstantExpr>(Op)) {
              if (CE->getType()->isPointerTy() && CE->isCast()) {
                if (const auto *F =
                        llvm::dyn_cast<llvm::Function>(CE->getOperand(0));
                    F && isConsistentCall(CallSite, F) &&
                    (BaseCallees.empty() || BaseCallees.count(F))) {
                  PossibleTargets.insert(F);
                }
              }
            }

            if (const auto *F = llvm::dyn_cast<llvm::Function>(Op)) {
              if (isConsistentCall(CallSite, F) &&
                  (BaseCallees.empty() || BaseCallees.count(F))) {
                PossibleTargets.insert(F);
              }
            } else if (auto *CA = llvm::dyn_cast<llvm::ConstantAggregate>(Op)) {
              ConstantAggregateWL.push_back(CA);
            } else if (auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(Op)) {
              if (!GV->hasInitializer()) {
                continue;
              }
              if (auto *GVCA = llvm::dyn_cast<llvm::ConstantAggregate>(
                      GV->getInitializer())) {
                ConstantAggregateWL.push_back(GVCA);
              }
            }
          }
        }
      });

  if (PossibleTargets.empty() &&
      useBaseResolverAsFallback(FallbackResolverAndFlags)) {
    if (!BaseCallees.empty()) {
      PossibleTargets = std::move(BaseCallees);
    } else {
      FallbackResolverAndFlags.getPointer()->resolveFunctionPointer(
          PossibleTargets, CallSite);
    }
  }
}

std::string AliasBasedResolver::str() const { return "AliasBased"; }
