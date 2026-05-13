#include "phasar/PhasarLLVM/Utils/UsedGlobals.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/FunctionId.h"
#include "phasar/Utils/SCCGeneric.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

using namespace psr;

static bool isEffectivelyConstant(const llvm::GlobalVariable *Glob) {
  auto Name = Glob->getName();
  if (Name.starts_with("_ZTV") || Name.starts_with("_ZTI")) {
    return true;
  }

  for (const auto &Use : Glob->uses()) {
    const auto *User = Use.getUser();
    if (llvm::isa<llvm::LoadInst>(User)) {
      continue;
    }

    if (const auto *MemTrn = llvm::dyn_cast<llvm::MemTransferInst>(User)) {
      if (MemTrn->getRawSource() != Use.get()) {
        return false;
      }

      continue;
    }

    if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(User)) {
      if (Use.get() == Call->getCalledOperand()) {
        continue;
      }

      if (const auto *DestFun = Call->getCalledFunction()) {
        if (DestFun->onlyReadsMemory() ||
            DestFun->onlyAccessesInaccessibleMemory()) {
          // llvm::errs() << "[NOTE]: At " << *Call
          //              << ": Readonly or Inaccessiblememonly\n";
          continue;
        }
      }

      auto Idx = Use.getOperandNo();

      const bool IsNocaptureParam =
#if LLVM_VERSION_MAJOR <= 20
          Call->paramHasAttr(Idx, llvm::Attribute::NoCapture);
#else
          [&] {
            auto Captures = Call->getCaptureInfo(Idx);
            auto CComp =
                Captures.getOtherComponents() | Captures.getRetComponents();
            return !(llvm::capturesAnyProvenance(CComp) ||
                     (llvm::capturesAddress(CComp) &&
                      !llvm::capturesAddressIsNullOnly(CComp)));
          }();
#endif

      bool IsReadonlyParam =
          IsNocaptureParam &&
          (Call->paramHasAttr(Idx, llvm::Attribute::ReadOnly) ||
           Call->paramHasAttr(Idx, llvm::Attribute::ReadNone));

      // llvm::errs() << "[NOTE]: At " << *Call << ": Param " << Idx
      //              << " is readonly: " << IsReadonlyParam << '\n';

      if (!IsReadonlyParam) {
        return false;
      }

      continue;
    }

    // llvm::errs() << "Other: " << *Use << ": Idx=" << Use.getOperandNo()
    //              << "; User=" << *Use.getUser() << '\n';
    return false;
  }
  return true;
}

static llvm::SmallDenseSet<const llvm::GlobalVariable *>
computeEffectivelyConstGlobals(const llvm::Module &Mod) {
  llvm::SmallDenseSet<const llvm::GlobalVariable *> Ret;

  for (const auto &Glob : Mod.globals()) {
    if (Glob.isConstant()) {
      continue;
    }

    if (isEffectivelyConstant(&Glob)) {
      Ret.insert(&Glob);
    }
  }

  return Ret;
}

static void initializeFun(auto &Globs, const llvm::Function *Fun,
                          const auto &EffectivelyConstGlobals) {
  for (const auto &Inst : llvm::instructions(Fun)) {
    if (Inst.isDebugOrPseudoInst()) {
      continue;
    }

    for (const auto *Op : Inst.operand_values()) {
      if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(
              Op->stripInBoundsConstantOffsets())) {

        // TODO: ispointerty must be done by the taint config!
        if (Glob->isConstant() || EffectivelyConstGlobals.contains(Glob) ||
            !Glob->getValueType()->isPointerTy()) {
          continue;
        }

        Globs.insert(Glob);
      }
    }
  }
}

static void
initialize(UsedGlobalsHolder<const llvm::GlobalVariable *> &Ret,
           const Compressor<const llvm::Function *, FunctionId> &Functions,
           const SCCHolder<FunctionId> &SCCs,
           const auto &EffectivelyConstGlobals) {
  for (auto FunId : psr::iota<FunctionId>(Functions.size())) {
    const auto *Fun = Functions[FunId];
    auto &InitialGlobs = Ret.InitialGlobsPerSCC[SCCs.SCCOfNode[FunId]];
    auto &Globs = Ret.GlobsPerSCC[SCCs.SCCOfNode[FunId]];

    initializeFun(InitialGlobs, Fun, EffectivelyConstGlobals);
    Globs = InitialGlobs;
  }
}

static void
propagateGlobals(UsedGlobalsHolder<const llvm::GlobalVariable *> &Ret,
                 const SCCDependencyGraph<FunctionId> &Callers) {
  std::deque<SCCId<FunctionId>> WL;
  BitSet<SCCId<FunctionId>> Seen(Callers.ChildrenOfSCC.size());

  for (auto Leaf : Callers.SCCRoots) {
    WL.push_back(Leaf);
    Seen.insert(Leaf);
  }

  while (!WL.empty()) {
    auto CurrSCC = WL.front();
    WL.pop_front();

    const auto &Globs = Ret.GlobsPerSCC[CurrSCC];

    for (auto Caller : Callers.ChildrenOfSCC[CurrSCC]) {
      auto &CallerGlobs = Ret.GlobsPerSCC[Caller];
      bool Inserted = false;
      for (const auto *G : Globs) {
        Inserted |= CallerGlobs.insert(G).second;
      }

      if (Seen.tryInsert(Caller) || Inserted) {
        WL.push_back(Caller);
      }
    }
  }
}

UsedGlobalsHolder<const llvm::GlobalVariable *> psr::computeUsedGlobals(
    const llvm::Module &Mod,
    const Compressor<const llvm::Function *, FunctionId> &Functions,
    const SCCHolder<FunctionId> &SCCs,
    const SCCDependencyGraph<FunctionId> &Callers) {
  UsedGlobalsHolder<const llvm::GlobalVariable *> Ret;
  Ret.InitialGlobsPerSCC.resize(SCCs.size());
  Ret.GlobsPerSCC.resize(SCCs.size());

  auto EffectivelyConstGlobals = computeEffectivelyConstGlobals(Mod);

  initialize(Ret, Functions, SCCs, EffectivelyConstGlobals);
  propagateGlobals(Ret, Callers);

  return Ret;
}
