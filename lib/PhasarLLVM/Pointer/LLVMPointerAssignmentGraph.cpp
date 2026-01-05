#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/BitSet.h"
#include "phasar/Utils/MapUtils.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Operator.h"

#include <concepts>

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

struct GlobalCache {
  const llvm::DataLayout &DL; // NOLINT
  // Due to the recursion in getOrCreateGCacheEntry, we need pointer stability
  std::unordered_map<const llvm::Constant *, llvm::SmallVector<ValueId, 1>>
      Cache{};

  [[nodiscard]] llvm::ArrayRef<ValueId> getOrCreateGCacheEntry(
      LLVMPBStrategyRef Strategy, const llvm::Constant *Const,
      std::invocable<const llvm::Value *, LLVMPBStrategyRef> auto GetVariable) {
    if (definitelyContainsNoPointer(Const)) {
      return {};
    }

    auto [It, Inserted] = Cache.try_emplace(Const);
    if (!Inserted) {
      return It->second;
    }

    auto &Vec = It->second;

    // We do not care about null here
    if (llvm::isa<llvm::ConstantPointerNull>(Const)) {
      return {};
    }

    if (const auto *CGep = llvm::dyn_cast<llvm::GEPOperator>(Const)) {
      // TODO: Properly handle constant GEPs
      return getOrCreateGCacheEntry(
          Strategy, llvm::cast<llvm::Constant>(CGep->getPointerOperand()),
          GetVariable);
    }

    if (Const->getType()->isPointerTy()) {
      Vec.push_back(GetVariable(Const, Strategy));

      return Vec;
    }

    if (!llvm::isa<llvm::ConstantAggregate>(Const)) {
      return {};
    }

    // TODO: Get rid of the recursion

    if (const auto *Arr = llvm::dyn_cast<llvm::ConstantAggregate>(Const)) {
      if (Arr->getType()->isArrayTy() &&
          definitelyContainsNoPointer(Arr->getType()->getArrayElementType())) {
        return {};
      }

      size_t ArrayLen = Arr->getNumOperands();
      for (size_t I = 0; I < ArrayLen; ++I) {
        auto *Elem = llvm::cast<llvm::Constant>(
            Arr->getAggregateElement(I)->stripPointerCastsAndAliases());
        auto ElemVars = getOrCreateGCacheEntry(Strategy, Elem, GetVariable);
        Vec.append(ElemVars.begin(), ElemVars.end());
      }
      return Vec;
    }

    // TODO: more

    return Vec;
  }
};
} // namespace

struct [[clang::internal_linkage]] LLVMPAGBuilder::PAGBuildData {
  const llvm::DataLayout &DL; // NOLINT
  ValueCompressor<v_t> &VC;   // NOLINT

  llvm::DenseMap<ValueId, ValueId> TheOneLoad{};
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
    // llvm::errs() << "[addEdge]: " << int(From) << "->" << int(To)
    //              << " :: " << to_string(E) << '\n';
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
    GlobalCache GCache{IRDB.getModule()->getDataLayout()};

    for (const auto &Glob : IRDB.getModule()->globals()) {
      if (definitelyContainsNoPointer(Glob.getValueType())) {
        continue;
      }

      if (Glob.hasInitializer()) {
        initializeGlobal(GCache, Strategy, Glob);
      }
    }
  }

  void initializeGlobal(GlobalCache &GCache, LLVMPBStrategyRef Strategy,
                        const llvm::GlobalVariable &Glob) {
    auto GlobObj = getVariable(&Glob, Strategy);
    auto Stores = GCache.getOrCreateGCacheEntry(
        Strategy, Glob.getInitializer(),
        [this](const llvm::Value *V, LLVMPBStrategyRef Strategy) {
          return getVariable(V, Strategy);
        });

    for (auto Src : Stores) {
      // NOTE: We don't consider this a POI for now; probably, that's fine
      addEdge(Strategy, Src, GlobObj, Store{}, nullptr);
    }
  }

  void initializeFunctions(const LLVMProjectIRDB &IRDB,
                           LLVMPBStrategyRef Strategy) {
    // TODO: Use LibrarySummary here
    // TODO: Skip functions that have been proven unreachable previously

    for (const auto *Fun : IRDB.getAllFunctions()) {
      if (!Fun->isDeclaration()) {
        initializeFun(Strategy, *Fun);
      }
    }

    addDelayedEdges(Strategy);
  }

  void initializeFun(LLVMPBStrategyRef Strategy, const llvm::Function &Fun) {
    // TODO: RPO Order to profit from OTF-merged values
    for (const auto &BB : Fun) {
      propagateBB(Strategy, BB);
    }
  }

  void addDelayedEdges(LLVMPBStrategyRef Strategy) {
    OnlyIncomingStoresAndOutgoingLoads.foreach ([this, &Strategy](auto VId) {
      const bool NeedsStoreSafetyFallback =
          OutgoingLoads[VId].empty() &&
          llvm::any_of(VC.id2vars(VId), [](PAGVariable Var) {
            return Var.isReturnVariable() ||
                   isAddressTakenVariable(Var.valueOrNull());
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
    if (Inst->isDebugOrPseudoInst()) {
      Inst = Inst->getNextNonDebugInstruction();
    }

    for (; Inst; Inst = Inst->getNextNonDebugInstruction()) {
      dispatch(Strategy, *Inst);
    }
  }

  void dispatch(LLVMPBStrategyRef Strategy, const llvm::Instruction &I) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      return (void)getVariable(Alloca, Strategy);
    }

    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
      return handleStore(Strategy, Store);
    }

    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(&I)) {
      return handleLoad(Strategy, Load);
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
  }

  static void handleOperand(const llvm::Value *RawOp,
                            std::invocable<const llvm::Value *> auto Handler) {
    RawOp = RawOp->stripPointerCastsAndAliases();
    const auto *RawOpCExpr = llvm::dyn_cast<llvm::ConstantExpr>(RawOp);
    if (!RawOpCExpr) [[likely]] {
      // fast-path:
      return (void)std::invoke(Handler, RawOp);
    }

    llvm::SmallDenseSet<const llvm::Value *> Seen = {RawOp};
    llvm::SmallVector<const llvm::User *> WL = {RawOpCExpr};
    do {
      const auto *Curr = WL.pop_back_val();
      for (const auto *Op : Curr->operand_values()) {
        if (definitelyContainsNoPointer(Op) || !Seen.insert(Op).second) {
          continue;
        }

        if (const auto *GObj = llvm::dyn_cast<llvm::GlobalObject>(Op)) {
          std::invoke(Handler, GObj);
          continue;
        }

        // TODO: Handle constant GEP!

        if (const auto *OpUser = llvm::dyn_cast<llvm::User>(Op)) {
          WL.push_back(OpUser);
          continue;
        }
      }

    } while (!WL.empty());
  }

  void handleStore(LLVMPBStrategyRef Strategy, const llvm::StoreInst *Store) {

    if (definitelyContainsNoPointer(Store->getValueOperand())) {
      return;
    }

    handleOperand(Store->getPointerOperand(), [&](const auto *PointerOp) {
      auto PointerObj = getVariable(PointerOp, Strategy);
      handleOperand(Store->getValueOperand(), [&](const auto *ValueOp) {
        auto ValueObj = getVariable(ValueOp, Strategy);
        addEdge(Strategy, ValueObj, PointerObj, StorePOI{}, Store);
      });
    });
  }

  void handleLoad(LLVMPBStrategyRef Strategy, const llvm::LoadInst *Ld) {
    if (definitelyContainsNoPointer(Ld)) {
      return;
    }

    handleOperand(Ld->getPointerOperand(), [&](const auto *PointerOp) {
      auto PointerObj = getVariable(PointerOp, Strategy);
      if (llvm::isa<llvm::Argument, llvm::Instruction>(PointerOp)) {
        auto [It, Inserted] = TheOneLoad.try_emplace(PointerObj, ValueId{});

        if (!Inserted) {
          VC.addAlias(Ld, It->second);
          return;
        }

        auto LoadObj = getVariable(Ld, Strategy);
        It->second = LoadObj;
        addEdge(Strategy, PointerObj, LoadObj, Load{}, Ld);
        return;
      }

      auto LoadObj = getVariable(Ld, Strategy);
      addEdge(Strategy, PointerObj, LoadObj, Load{}, Ld);
    });
  }

  void handleCastImpl(LLVMPBStrategyRef Strategy, const llvm::User *Cast,
                      const llvm::Value *Op) {
    auto OperandObj = getVariable(Op, Strategy);

    if (llvm::isa<llvm::GEPOperator>(Cast)) {
      if (const auto *LocalGeps = psr::getOrNull(this->LocalGeps, Cast)) {
        for (auto [_, Gep] : *LocalGeps) {
          addEdge(Strategy, OperandObj, Gep, Assign{}, nullptr);
        }
        return;
      }
    }

    VC.addAlias(Cast, OperandObj);
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
      if (psr::isAllocaInstOrHeapAllocaFunction(PointerOp) &&
          !isAddressTakenVariable(PointerOp) && Gep->hasAllConstantIndices() &&
          !Gep->hasAllZeroIndices()) {
        llvm::APInt Offset(64, 0);
        if (Gep->accumulateConstantOffset(DL, Offset)) {
          auto [It, Inserted] =
              LocalGeps[PointerOp].try_emplace(Offset.getSExtValue());
          if (!Inserted) {
            VC.addAlias(Gep, It->second);
            return;
          }

          auto GepObj = getVariable(Gep, Strategy);
          It->second = GepObj;
          // Do not add an edge! Geps should be decoupled!
          return;
        }
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
    // TODO: Handle library summaries here!
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

    // TODO: Varargs
  }

  void handleCall(LLVMPBStrategyRef Strategy, const llvm::CallBase *Call) {
    const auto *FnPtr = Call->getCalledOperand()->stripPointerCastsAndAliases();

    llvm::SmallVector<llvm::SmallDenseSet<ValueId>> Args;
    for (const auto &Arg : Call->args()) {
      auto &ArgVal = Args.emplace_back();
      if (definitelyContainsNoPointer(Arg)) {
        continue;
      }

      handleOperand(Arg.get(), [&](const auto *ArgOp) {
        ArgVal.insert(getVariable(ArgOp, Strategy));
      });
    }
    std::optional<ValueId> CSVal;
    if (Call->getType()->isPointerTy()) {
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
    if (!RetVal || definitelyContainsNoPointer(RetVal)) {
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
        auto TrueObj = getVariable(FalseOp, Strategy);
        addEdge(Strategy, TrueObj, SelectObj, Assign{}, Select);
      });
    }
  }
};

void psr::LLVMPAGBuilder::buildPAG(const LLVMProjectIRDB &IRDB,
                                   ValueCompressor<v_t> &VC,
                                   LLVMPBStrategyRef Strategy) {
  PAGBuildData BData{IRDB.getModule()->getDataLayout(), VC};

  const size_t NumPossibleValues = Strategy.getNumPossibleValues(IRDB);
  const size_t NumPresentValues = VC.size();

  BData.IncomingStores.reserve(NumPossibleValues);
  BData.IncomingStores.resize(NumPresentValues);

  BData.OutgoingLoads.reserve(NumPossibleValues);
  BData.OutgoingLoads.resize(NumPresentValues);

  BData.OnlyIncomingStoresAndOutgoingLoads.reserve(NumPossibleValues);

  BData.initializeGlobals(IRDB, Strategy);
  BData.initializeFunctions(IRDB, Strategy);
}
