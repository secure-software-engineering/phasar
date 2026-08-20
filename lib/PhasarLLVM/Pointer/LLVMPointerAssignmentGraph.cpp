#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMGlobalInitCache.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerSemantics.h"
#include "phasar/PhasarLLVM/Pointer/MemSSAUtils.h"
#include "phasar/PhasarLLVM/Utils/LLVMFunctionDataFlowFacts.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Pointer/PointerAssignmentGraph.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/LibCSummary.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Operator.h"
#include "llvm/TargetParser/Triple.h"

#include <concepts>
#include <memory>

using namespace psr;
using namespace psr::pag;
using LLVMPBStrategyRef = pag::PBStrategyRef<LLVMPAGDomain>;

static_assert(detail::IsPointerWithAtleastOneFreeLowBit<PAGVariable>);

std::string psr::to_string(PAGVariable Var) {
  auto Ret = llvmIRToString(Var.get());
  if (Var.isReturnVariable()) {
    Ret += ".<ret>";
  }
  return Ret;
}

namespace {

struct PAGMappedLibrarySummary {
  const library_summary::LLVMFunctionDataFlowFacts &Facts; // NOLINT

  bool
  mapFunctionSummary(const llvm::Function *Fun,
                     std::invocable<uint32_t, library_summary::DataFlowFact,
                                    pag::Edge> auto AddEdge) const {
    const auto *LibSum = Facts.getFactsForFunctionOrNull(Fun);
    if (!LibSum) {
      return false;
    }

    const size_t NumParams = Fun->arg_size();

    for (const auto &[ParamFact, Pts] : *LibSum) {
      if (ParamFact >= NumParams ||
          !Fun->getArg(ParamFact)->getType()->isPointerTy()) {
        continue;
      }

      for (const auto &DestFact : Pts) {
        auto E = DestFact.dyn_cast<library_summary::Parameter>()
                     ? pag::Edge(pag::StorePOI{})
                     : pag::Assign{};
        std::invoke(AddEdge, ParamFact, DestFact, E);
      }
    }
    return true;
  }
};

} // namespace

struct PSR_INTERNAL_LINKAGE LLVMPAGBuilder::PAGBuildData {
  const llvm::DataLayout &DL;           // NOLINT
  ValueCompressor<v_t> &VC;             // NOLINT
  const PAGMappedLibrarySummary &MLSum; // NOLINT
  PunnedABICache PunnedABI{&DL};

  LLVMPAGBuilder::MemSSAProviderFn *MemSSAProvider = nullptr;
  llvm::MemorySSA *CurrentMemSSA = nullptr;

  llvm::DenseMap<ValueId, ValueId> TheOneLoad{};
  llvm::DenseMap<std::pair<ValueId, const llvm::Function *>, ValueId>
      TheOneGlobalLoad{};
  BitSet<ValueId> OnlyIncomingStoresAndOutgoingLoads{};
  TypedVector<ValueId, llvm::SmallDenseMap<ValueId, Edge, 2>> IncomingStores{};
  TypedVector<ValueId, llvm::SmallDenseSet<ValueId, 2>> OutgoingLoads{};
  llvm::DenseMap<const llvm::Value *, llvm::SmallDenseMap<int64_t, ValueId>>
      LocalGeps{};

  [[nodiscard]] ValueId getVariable(PAGVariable V, LLVMPBStrategyRef Strategy) {
    auto [Id, Inserted] = VC.insert(V);
    if (Inserted) {
      OnlyIncomingStoresAndOutgoingLoads.insert(Id);
      IncomingStores.emplace_back();
      OutgoingLoads.emplace_back();
      Strategy.onAddValue(V, Id);
    }

    return Id;
  }

  // Registers \p V as another name for node \p Id, i.e. makes the two the
  // same node.
  //
  // If \p V already owns a node the alias can no longer be recorded. Fall back
  // to a pair of Assign edges
  void addAliasOrEquate(LLVMPBStrategyRef Strategy, PAGVariable V, ValueId Id,
                        const llvm::Instruction *AtInstruction) {
    if (VC.addAlias(V, Id)) {
      return;
    }
    const auto Existing = VC.getOrNull(V);
    assert(Existing && "addAlias only fails when V already owns an id");
    if (*Existing == Id) {
      return;
    }
    addEdge(Strategy, Id, *Existing, Assign{}, AtInstruction);
    addEdge(Strategy, *Existing, Id, Assign{}, AtInstruction);
  }

  void addAllIncomingStores(LLVMPBStrategyRef Strategy, ValueId To,
                            llvm::SmallDenseMap<ValueId, Edge, 2> &Froms) {
    for (auto [From, E] : Froms) {
      Strategy.onAddEdge(From, To, E, nullptr);
    }
    Froms.clear();
  }
  void addAllOutgoingLoads(LLVMPBStrategyRef Strategy, ValueId From,
                           llvm::SmallDenseSet<ValueId, 2> &Tos) {
    for (auto To : Tos) {
      Strategy.onAddEdge(From, To, Load{}, nullptr);
    }
    Tos.clear();
  }

  void eraseFromSimpleStoreLoad(LLVMPBStrategyRef Strategy, ValueId VId) {
    if (OnlyIncomingStoresAndOutgoingLoads.tryErase(VId)) {
      addAllIncomingStores(Strategy, VId, IncomingStores[VId]);
      addAllOutgoingLoads(Strategy, VId, OutgoingLoads[VId]);
    }
  }

  void addEdge(LLVMPBStrategyRef Strategy, ValueId From, ValueId To, Edge E,
               const llvm::Instruction *AtInstruction) {
    if (!E.isa<Store, StorePOI>()) {
      eraseFromSimpleStoreLoad(Strategy, To);
    } else if (OnlyIncomingStoresAndOutgoingLoads.contains(To)) {
      eraseFromSimpleStoreLoad(Strategy, From);

      // We delay store edges
      IncomingStores[To].try_emplace(From, E);
      return;
    }

    if (!E.isa<Load>()) {
      eraseFromSimpleStoreLoad(Strategy, From);
    } else if (OnlyIncomingStoresAndOutgoingLoads.contains(From)) {
      eraseFromSimpleStoreLoad(Strategy, To);
      // We delay load edges
      OutgoingLoads[From].insert(To);
      return;
    }

    Strategy.onAddEdge(From, To, E, AtInstruction);
  }

  void initializeGlobals(const LLVMProjectIRDB &IRDB,
                         LLVMPBStrategyRef Strategy) {
    GlobalInitCache GCache;

    for (const auto &Glob : IRDB.getModule()->globals()) {
      if (definitelyContainsNoPointer(Glob.getValueType())) {
        continue;
      }

      if (Glob.hasInitializer()) {
        initializeGlobal(GCache, Strategy, Glob);
      }
    }
  }

  void initializeGlobal(GlobalInitCache &GCache, LLVMPBStrategyRef Strategy,
                        const llvm::GlobalVariable &Glob) {
    auto GlobObj = getVariable(&Glob, Strategy);
    auto Stores = GCache.getOrCreate(Glob.getInitializer(),
                                     [this, Strategy](const llvm::Value *V) {
                                       return getVariable(V, Strategy);
                                     });

    for (auto Src : Stores) {
      // NOTE: We don't consider this a POI for now; probably, that's fine
      addEdge(Strategy, Src, GlobObj, Store{}, nullptr);
    }
  }

  void initializeFunctions(const LLVMProjectIRDB &IRDB,
                           LLVMPBStrategyRef Strategy) {
    // XXX: Skip functions that have been proven unreachable previously

    for (const auto *Fun : IRDB.getAllFunctions()) {
      if (!Fun->isDeclaration()) {
        initializeFun(Strategy, *Fun);
      }
    }

    addDelayedEdges(Strategy);
  }

  void initializeFun(LLVMPBStrategyRef Strategy, const llvm::Function &Fun) {
    CurrentMemSSA =
        MemSSAProvider && *MemSSAProvider ? (*MemSSAProvider)(Fun) : nullptr;
    // XXX: RPO Order to profit from OTF-merged values
    for (const auto &BB : Fun) {
      propagateBB(Strategy, BB);
    }
  }

  void addDelayedEdges(LLVMPBStrategyRef Strategy) {
    OnlyIncomingStoresAndOutgoingLoads.foreach ([this, &Strategy](auto VId) {
      // Return slots always receive Assign/Return edges from handleReturn /
      // handleCallTarget, which erases them from
      // OnlyIncomingStoresAndOutgoingLoads before we get here. Only
      // address-taken variables need the safety fallback: their stored value
      // might be loaded via an alias that we have not seen yet.
      const bool NeedsStoreSafetyFallback =
          OutgoingLoads[VId].empty() &&
          llvm::any_of(VC.id2vars(VId), [](PAGVariable Var) {
            return isAddressTakenVariable(Var.valueOrNull());
          });

      for (auto [IncStore, _] : IncomingStores[VId]) {
        for (auto OutLoad : OutgoingLoads[VId]) {
          Strategy.onAddEdge(IncStore, OutLoad, Assign{}, nullptr);
        }
        if (NeedsStoreSafetyFallback) {
          // Safety-fallback: we may have missed a use of the stored value
          Strategy.onAddEdge(IncStore, VId, StorePOI{}, nullptr);
        }
      }
      if (IncomingStores[VId].empty()) {
        for (auto OutLoad : OutgoingLoads[VId]) {
          Strategy.onAddEdge(VId, OutLoad, Load{}, nullptr);
        }
      }
    });
  }

  void propagateBB(LLVMPBStrategyRef Strategy, const llvm::BasicBlock &BB) {
    const auto *Inst = &BB.front();
#if LLVM_VERSION_MAJOR <= 18
    if (Inst->isDebugOrPseudoInst()) {
      Inst = Inst->getNextNonDebugInstruction();
    }
#endif

    for (; Inst; Inst =
#if LLVM_VERSION_MAJOR <= 18
                     Inst->getNextNonDebugInstruction()
#else
                     Inst->getNextNode()
#endif
    ) {
      dispatch(Strategy, *Inst);
    }
  }

  void dispatch(LLVMPBStrategyRef Strategy, const llvm::Instruction &I) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      return (void)getVariable(Alloca, Strategy);
    }

    if (const auto Access = asMemoryAccess(I, DL)) {
      if (!Access->mayTransferPointer()) {
        return;
      }
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(&I)) {
        return handleLoad(Strategy, Load, *Access);
      }
      if (llvm::isa<llvm::StoreInst>(&I)) {
        return handleStore(Strategy, *Access);
      }
      return handleAtomicAccess(Strategy, *Access);
    }

    if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(&I)) {
      return handleCast(Strategy, Cast);
    }

    if (const auto *GEP = llvm::dyn_cast<llvm::GEPOperator>(&I)) {
      return handleGep(Strategy, GEP);
    }

    if (const auto *MemTrn = llvm::dyn_cast<llvm::MemTransferInst>(&I)) {
      return handleMemTransfer(Strategy, MemTrn);
    }

    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&I)) {
      return handleCall(Strategy, Call);
    }

    if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(&I)) {
      return handleReturn(Strategy, Ret);
    }

    if (const auto *Phi = llvm::dyn_cast<llvm::PHINode>(&I)) {
      return handlePhi(Strategy, Phi);
    }

    if (const auto *Select = llvm::dyn_cast<llvm::SelectInst>(&I)) {
      return handleSelect(Strategy, Select);
    }

    if (const auto *EV = llvm::dyn_cast<llvm::ExtractValueInst>(&I)) {
      return handleExtractValue(Strategy, EV);
    }

    if (const auto *IV = llvm::dyn_cast<llvm::InsertValueInst>(&I)) {
      return handleInsertValue(Strategy, IV);
    }
  }

  static void handleOperand(const llvm::Value *RawOp,
                            std::invocable<const llvm::Value *> auto Handler) {
    // TODO: Handle constant GEP!
    psr::forEachPointerOperand(RawOp, copyOrRef(Handler));
  }

  void handleStore(LLVMPBStrategyRef Strategy, const LLVMMemoryAccess &Access) {
    handleOperand(Access.Pointer, [&](const auto *PointerOp) {
      auto PointerObj = getVariable(PointerOp, Strategy);
      handleOperand(Access.StoredValue, [&](const auto *ValueOp) {
        auto ValueObj = getVariable(ValueOp, Strategy);
        addEdge(Strategy, ValueObj, PointerObj, StorePOI{}, Access.Instr);
      });
    });
  }

  void handleAtomicAccess(LLVMPBStrategyRef Strategy,
                          const LLVMMemoryAccess &Access) {
    auto DstObj = getVariable(Access.LoadedInto, Strategy);
    handleOperand(Access.Pointer, [&](const auto *PointerOp) {
      auto PointerObj = getVariable(PointerOp, Strategy);
      handleOperand(Access.StoredValue, [&](const auto *ValueOp) {
        auto ValueObj = getVariable(ValueOp, Strategy);
        addEdge(Strategy, ValueObj, PointerObj, StorePOI{}, Access.Instr);
      });
      addEdge(Strategy, PointerObj, DstObj, Load{}, Access.Instr);
    });
  }

  void handleLoad(LLVMPBStrategyRef Strategy, const llvm::LoadInst *Ld,
                  const LLVMMemoryAccess &Access) {
    if (CurrentMemSSA) {
      llvm::SmallPtrSet<const llvm::StoreInst *, 4> Defs;
      const bool HasLiveOnEntry = collectReachingDefs(Ld, *CurrentMemSSA, Defs);
      if (!HasLiveOnEntry) {
        if (Defs.size() == 1) {
          const auto *ValueOp = (*Defs.begin())->getValueOperand();
          if (!llvm::isa<llvm::ConstantExpr>(ValueOp) &&
              !definitelyContainsNoPointer(ValueOp)) {
            addAliasOrEquate(Strategy, Ld, getVariable(ValueOp, Strategy), Ld);
            return;
          }
        }

        auto LoadObj = getVariable(Ld, Strategy);
        bool AddedAny = false;
        for (const auto *Def : Defs) {
          handleOperand(Def->getValueOperand(), [&](const auto *ValOp) {
            Strategy.onAddEdge(getVariable(ValOp, Strategy), LoadObj, Assign{},
                               Ld);
            AddedAny = true;
          });
        }

        if (AddedAny) {
          return;
        }
        // All reaching stores have non-pointer value operands:
        // fall through to addEdge.
      }
    }

    handleOperand(Access.Pointer, [&](const auto *PointerOp) {
      auto PointerObj = getVariable(PointerOp, Strategy);

      const auto ReuseOrCreate = [&](auto &Map, auto Key) {
        auto [It, Inserted] = Map.try_emplace(Key, ValueId{});
        if (!Inserted) {
          addAliasOrEquate(Strategy, Ld, It->second, Ld);
          return;
        }
        auto LoadObj = getVariable(Ld, Strategy);
        It->second = LoadObj;
        addEdge(Strategy, PointerObj, LoadObj, Load{}, Ld);
      };

      if (llvm::isa<llvm::Argument, llvm::Instruction>(PointerOp)) {
        return ReuseOrCreate(TheOneLoad, PointerObj);
      }
      if (llvm::isa<llvm::GlobalValue>(PointerOp)) {
        return ReuseOrCreate(TheOneGlobalLoad,
                             std::pair{PointerObj, Ld->getFunction()});
      }

      auto LoadObj = getVariable(Ld, Strategy);
      addEdge(Strategy, PointerObj, LoadObj, Load{}, Ld);
    });
  }

  void handleCastImpl(LLVMPBStrategyRef Strategy, const llvm::User *Cast,
                      const llvm::Value *Op) {
    auto OperandObj = getVariable(Op, Strategy);

    if (llvm::isa<llvm::GEPOperator>(Cast)) {
      // accumulate all known offsets, since we have seen a non-constant GEP on
      // this pointer-op:
      if (const auto *LocalGeps = psr::getOrNull(this->LocalGeps, Op)) {
        for (auto [_, Gep] : *LocalGeps) {
          addEdge(Strategy, OperandObj, Gep, Assign{}, nullptr);
        }
        return;
      }
    }

    addAliasOrEquate(Strategy, Cast, OperandObj, nullptr);
  }

  void handleCast(LLVMPBStrategyRef Strategy, const llvm::User *Cast) {
    if (definitelyContainsNoPointer(Cast)) {
      return;
    }

    handleOperand(Cast->getOperand(0),
                  [&](const auto *Op) { handleCastImpl(Strategy, Cast, Op); });
  }
  void handleGep(LLVMPBStrategyRef Strategy, const llvm::GEPOperator *Gep) {
    handleOperand(Gep->getPointerOperand(), [&](const auto *PointerOp) {
      const auto *Arg = llvm::dyn_cast<llvm::Argument>(PointerOp);
      const bool IsAllocLike =
          psr::isAllocaInstOrHeapAllocaFunction(PointerOp) || Arg != nullptr;
      const bool IsAddressTaken =
          Arg ? isAddressTakenArg(Arg) : isAddressTakenVariable(PointerOp);
      if (IsAllocLike && !IsAddressTaken && Gep->hasAllConstantIndices() &&
          !Gep->hasAllZeroIndices()) {
        llvm::APInt Offset(64, 0);
        if (Gep->accumulateConstantOffset(DL, Offset)) {
          auto [It, Inserted] =
              LocalGeps[PointerOp].try_emplace(Offset.getSExtValue());
          if (!Inserted) {
            addAliasOrEquate(Strategy, Gep, It->second, nullptr);
            return;
          }

          auto GepObj = getVariable(Gep, Strategy);
          It->second = GepObj;
          // Do not add an edge! Geps should be decoupled!
          return;
        }

        // fallthrough -- non-constant GEP
      }

      handleCastImpl(Strategy, Gep, PointerOp);
    });
  }

  void handleMemTransfer(LLVMPBStrategyRef Strategy,
                         const llvm::MemTransferInst *MemTrn) {

    handleOperand(MemTrn->getDest(), [&](const auto *Dest) {
      auto DestObj = getVariable(Dest, Strategy);

      handleOperand(MemTrn->getSource(), [&](const auto *Src) {
        auto SourceObj = getVariable(Src, Strategy);
        addEdge(Strategy, SourceObj, DestObj, Copy{}, MemTrn);

        const auto &DestGeps = getOrDefault(LocalGeps, Dest);
        for (const auto &[Offset, SrcGep] : getOrDefault(LocalGeps, Src)) {
          if (const auto *DestGep = getOrNull(DestGeps, Offset)) {
            addEdge(Strategy, SrcGep, *DestGep, Copy{}, MemTrn);
          } else {
            addEdge(Strategy, SrcGep, DestObj, Copy{}, MemTrn);
          }
        }
      });
    });
  }

  void handleCallTarget(LLVMPBStrategyRef Strategy, const llvm::CallBase *Call,
                        const llvm::Function *Callee,
                        llvm::ArrayRef<llvm::SmallDenseSet<ValueId>> Args,
                        std::optional<ValueId> CSVal) {
    const bool HasLibrarySummary = MLSum.mapFunctionSummary(
        Callee, [&](uint32_t ParamIdx, library_summary::DataFlowFact Dest,
                    pag::Edge E) {
          if (ParamIdx >= Args.size()) {
            return;
          }
          const auto HandleArgs = [&](ValueId DestVal) {
            for (auto FromVal : Args[ParamIdx]) {
              addEdge(Strategy, FromVal, DestVal, E, Call);
            }
          };
          if (const auto *DestParam =
                  Dest.dyn_cast<library_summary::Parameter>()) {
            if (DestParam->Index >= Args.size()) {
              // safety fallback if library-summary is wrong
              return;
            }
            for (const auto &DestVal : Args[DestParam->Index]) {
              HandleArgs(DestVal);
            }
          } else {
            if (CSVal) {
              HandleArgs(*CSVal);
            }
          }
        });
    if (HasLibrarySummary) {
      return;
    }

    if (Callee->isDeclaration()) {
      return;
    }

    if (CSVal) {
      auto RetObj = getVariable(PAGVariable::Return{Callee}, Strategy);
      addEdge(Strategy, RetObj, *CSVal, Return{}, Call);
    }

    for (const auto &[Param, Arg] : llvm::zip(Callee->args(), Args)) {
      if (Arg.empty()) {
        continue;
      }

      // XXX: Check, whether Arg is an address-taken variable. Then we can
      // differentiate between Call and CallPOI; not relevant for now, though,
      // because we ignore call-pois
      auto ParamObj = getVariable(&Param, Strategy);
      for (auto ArgVal : Arg) {
        addEdge(Strategy, ArgVal, ParamObj,
                pag::Call{uint16_t(Param.getArgNo())}, Call);
      }
    }

    if (Callee->isVarArg()) {
      if (const auto *VaList = getVaListTagOrNull(*Callee)) {
        auto VaListObj = getVariable(VaList, Strategy);
        for (size_t I = Callee->arg_size(); I < Args.size(); ++I) {
          for (auto ArgVal : Args[I]) {
            addEdge(Strategy, ArgVal, VaListObj, StorePOI{}, Call);
          }
        }
      }
    }
  }

  void handleCall(LLVMPBStrategyRef Strategy, const llvm::CallBase *Call) {
    const auto *FnPtr = Call->getCalledOperand()->stripPointerCastsAndAliases();

    llvm::SmallVector<llvm::SmallDenseSet<ValueId>> Args;
    for (const auto &Arg : Call->args()) {
      auto &ArgVal = Args.emplace_back();
      if (definitelyContainsNoPointer(Arg) &&
          !PunnedABI.isCoercedPointer(Arg.get(), Call->getFunction())) {
        continue;
      }

      handleOperand(Arg.get(), [&](const auto *ArgOp) {
        ArgVal.insert(getVariable(ArgOp, Strategy));
      });
    }

    std::optional<ValueId> CSVal;
    if (!definitelyContainsNoPointer(Call) ||
        PunnedABI.isCoercedPointer(Call, Call->getFunction())) {
      CSVal = getVariable(Call, Strategy);
    }

    if (const auto *Callee = llvm::dyn_cast<llvm::Function>(FnPtr)) {
      return handleCallTarget(Strategy, Call, Callee, Args, CSVal);
    }

    Strategy.withCalleesOfCallAt(Call, [&](const auto *Callee) {
      handleCallTarget(Strategy, Call, Callee, Args, CSVal);
    });
  }

  void handleReturn(LLVMPBStrategyRef Strategy, const llvm::ReturnInst *Ret) {
    const auto *RetVal = Ret->getReturnValue();
    if (!RetVal || (definitelyContainsNoPointer(RetVal) &&
                    !PunnedABI.isCoercedPointer(RetVal, Ret->getFunction()))) {
      return;
    }

    auto RetObj =
        getVariable(PAGVariable::Return{Ret->getFunction()}, Strategy);
    handleOperand(RetVal, [&](const auto *ExitOp) {
      auto ExitObj = getVariable(ExitOp, Strategy);
      addEdge(Strategy, ExitObj, RetObj, Assign{}, Ret);
    });
  }

  void handlePhi(LLVMPBStrategyRef Strategy, const llvm::PHINode *Phi) {
    if (definitelyContainsNoPointer(Phi)) {
      return;
    }

    auto PhiObj = getVariable(Phi, Strategy);

    for (const auto &Inc : Phi->incoming_values()) {
      if (definitelyContainsNoPointer(Inc)) {
        continue;
      }

      // Here, we should make sure that all incoming BBs are already handled...

      handleOperand(Inc.get(), [&](const auto *IncOp) {
        auto IncObj = getVariable(IncOp, Strategy);
        addEdge(Strategy, IncObj, PhiObj, Assign{}, Phi);
      });
    }
  }

  void handleSelect(LLVMPBStrategyRef Strategy,
                    const llvm::SelectInst *Select) {
    if (definitelyContainsNoPointer(Select)) {
      return;
    }

    const auto *TrueVal = Select->getTrueValue()->stripPointerCastsAndAliases();
    const auto *FalseVal =
        Select->getFalseValue()->stripPointerCastsAndAliases();

    auto SelectObj = getVariable(Select, Strategy);
    // In the union-find, these are all merged; maybe be a bit smarter here...

    if (!definitelyContainsNoPointer(TrueVal)) {
      handleOperand(TrueVal, [&](const auto *TrueOp) {
        auto TrueObj = getVariable(TrueOp, Strategy);
        addEdge(Strategy, TrueObj, SelectObj, Assign{}, Select);
      });
    }
    if (!definitelyContainsNoPointer(FalseVal)) {
      handleOperand(FalseVal, [&](const auto *FalseOp) {
        auto FalseObj = getVariable(FalseOp, Strategy);
        addEdge(Strategy, FalseObj, SelectObj, Assign{}, Select);
      });
    }
  }

  void handleExtractValue(LLVMPBStrategyRef Strategy,
                          const llvm::ExtractValueInst *EV) {
    if (definitelyContainsNoPointer(EV)) {
      return;
    }

    auto ResultObj = getVariable(EV, Strategy);
    handleOperand(EV->getAggregateOperand(), [&](const auto *AggOp) {
      auto AggObj = getVariable(AggOp, Strategy);
      addEdge(Strategy, AggObj, ResultObj, Assign{}, EV);
    });
  }

  void handleInsertValue(LLVMPBStrategyRef Strategy,
                         const llvm::InsertValueInst *IV) {
    if (definitelyContainsNoPointer(IV)) {
      return;
    }

    auto ResultObj = getVariable(IV, Strategy);
    if (!definitelyContainsNoPointer(IV->getAggregateOperand())) {
      handleOperand(IV->getAggregateOperand(), [&](const auto *AggOp) {
        auto AggObj = getVariable(AggOp, Strategy);
        addEdge(Strategy, AggObj, ResultObj, Assign{}, IV);
      });
    }
    if (!definitelyContainsNoPointer(IV->getInsertedValueOperand())) {
      handleOperand(IV->getInsertedValueOperand(), [&](const auto *ValOp) {
        auto ValObj = getVariable(ValOp, Strategy);
        addEdge(Strategy, ValObj, ResultObj, Assign{}, IV);
      });
    }
  }
};

static const auto &getMappedLibSum(
    std::optional<library_summary::LLVMFunctionDataFlowFacts> &MLSumBuf,
    const LLVMProjectIRDB &IRDB) {
  MLSumBuf.emplace(library_summary::readFromFDFF(getLibCSummary(), IRDB));
  return *MLSumBuf;
}

void psr::LLVMPAGBuilder::buildPAG(const LLVMProjectIRDB &IRDB,
                                   ValueCompressor<v_t> &VC,
                                   LLVMPBStrategyRef Strategy) {

  std::optional<library_summary::LLVMFunctionDataFlowFacts> MLSumBuf{};
  const auto &MLSum =
      this->MLSum ? *this->MLSum : getMappedLibSum(MLSumBuf, IRDB);

  PAGBuildData BData{
      .DL = IRDB.getModule()->getDataLayout(),
      .VC = VC,
      .MLSum = {MLSum},
      .MemSSAProvider = MemSSAProvider ? &MemSSAProvider : nullptr,
  };

  const size_t NumPossibleValues = Strategy.getNumPossibleValues(IRDB);
  const size_t NumPresentValues = VC.size();

  BData.IncomingStores.reserve(NumPossibleValues);
  BData.IncomingStores.resize(NumPresentValues);

  BData.OutgoingLoads.reserve(NumPossibleValues);
  BData.OutgoingLoads.resize(NumPresentValues);

  BData.OnlyIncomingStoresAndOutgoingLoads.reserve(NumPossibleValues);

  // Strategies index their per-value tables by ValueId, so they must see every
  // id, including those a pre-populated VC already holds.
  for (const auto &[Id, Vars] : VC.id2vars().enumerate()) {
    if (!Vars.empty()) {
      Strategy.onAddValue(Vars.front(), Id);
    }
  }

  BData.initializeGlobals(IRDB, Strategy);
  BData.initializeFunctions(IRDB, Strategy);
}

LLVMPAGBuilder LLVMPAGBuilder::withBuiltinMemSSA(
    const library_summary::LLVMFunctionDataFlowFacts *MLSum) {
  struct MSSAImpl {
    llvm::TargetLibraryInfoWrapperPass TLA{};
    std::optional<MemSSABundle> Bundle{};
  };

  // Note: MemorySSA is not movable, so the bundle & impl are not movable
  // either
  MemSSAProviderFn Provider = [Impl = std::make_unique<MSSAImpl>()](
                                  const llvm::Function &F) mutable {
    // Note: Over-write the the entry if present; this is fine as long as we
    // don't use MemorySSA instances across provider-call boundaries.
    Impl->Bundle.emplace(const_cast<llvm::Function &>(F), &Impl->TLA.getTLI(F));
    return &Impl->Bundle->MSSA;
  };
  return LLVMPAGBuilder(MLSum, std::move(Provider));
}
