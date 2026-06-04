/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/Pointer/AndersenOTFAA.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Pointer/LLVMGlobalInitCache.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/PhasarLLVM/Pointer/MemSSAUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/DIBasedTypeHierarchy.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMVFTable.h"
#include "phasar/PhasarLLVM/Utils/LLVMFunctionDataFlowFacts.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/PhasarLLVM/Utils/VirtualCallUtils.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/LibCSummary.h"
#include "phasar/Utils/LibrarySummary.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/UnionFind.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <cassert>
#include <optional>
#include <utility>

using namespace psr;

namespace {
/// File-local wrapper: extends PAGVariable with a variable/object flag.
/// Variable nodes (IsObject=false) represent SSA pointer values.
/// Object nodes (IsObject=true) represent abstract memory cells.
class AndersenVar {
public:
  AndersenVar() noexcept = default;
  AndersenVar(PAGVariable Base, bool IsObject) : Base(Base, IsObject) {}

  [[nodiscard]] PAGVariable getBase() const noexcept {
    return Base.getPointer();
  }
  [[nodiscard]] bool isObject() const noexcept { return Base.getInt(); }

  friend bool operator==(AndersenVar A, AndersenVar B) noexcept {
    return A.Base == B.Base;
  }

  friend auto hash_value(AndersenVar V) noexcept {
    return llvm::hash_value(V.Base.getOpaqueValue());
  }

private:
  llvm::PointerIntPair<PAGVariable, 1, bool> Base{};
};
} // namespace

namespace llvm {
template <> struct DenseMapInfo<AndersenVar> {
  static AndersenVar getEmptyKey() noexcept {
    return {DenseMapInfo<psr::PAGVariable>::getEmptyKey(), false};
  }
  static AndersenVar getTombstoneKey() noexcept {
    return {DenseMapInfo<psr::PAGVariable>::getTombstoneKey(), false};
  }
  static unsigned getHashValue(AndersenVar V) noexcept { return hash_value(V); }
  static bool isEqual(AndersenVar A, AndersenVar B) noexcept { return A == B; }
};
} // namespace llvm

struct [[clang::internal_linkage]] AndersenOTFSolver::SolverData {
  // ---- Per-node state -------------------------------------------------

  struct NodeInfo {
    RawAliasSet<ValueId> PtsSet;
    RawAliasSet<ValueId> PendingPts;
    // Assignment edges: pts(this) ⊆ pts(dst) for each dst.
    llvm::SmallVector<ValueId, 2> AssignDsts;
    llvm::SmallDenseSet<ValueId, 4> AssignDstSet; // dedup guard
    // Load constraints: dst = *this.
    llvm::SmallVector<ValueId, 1> LoadDsts;
    llvm::SmallDenseSet<ValueId, 2> LoadDstSet; // dedup guard
    // Store constraints: *this = src.
    llvm::SmallVector<ValueId, 1> StoreSrcs;
    llvm::SmallDenseSet<ValueId, 2> StoreSrcSet; // dedup guard
    // MemCopy: memcpy(dst_ptr, this=src_ptr).
    llvm::SmallVector<ValueId, 1> MemCopyAsSrc;
    llvm::SmallDenseSet<ValueId, 2> MemCopyAsSrcSet; // dedup guard
    // MemCopy: memcpy(this=dst_ptr, src_ptr).
    llvm::SmallVector<ValueId, 1> MemCopyAsDst;
    llvm::SmallDenseSet<ValueId, 2> MemCopyAsDstSet; // dedup guard
  };

  // One set of ValueIds per call argument; empty means non-pointer.
  using ArgList = llvm::SmallVector<llvm::SmallVector<ValueId, 2>>;

  struct FPCallRecord {
    const llvm::CallBase *CS;
    ValueId FPId;
    ArgList Args;
    std::optional<ValueId> CSRetVal;
  };

  struct VCallRecord {
    const llvm::CallBase *CS;
    ValueId VtablePtrId;
    uint64_t VtableIndex;
    ArgList Args;
    std::optional<ValueId> CSRetVal;
  };

  struct StructVCallRecord {
    const llvm::CallBase *CS;
    ValueId BaseId; // pts(BaseId) = struct objects
    ValueId FPId;   // pts(FPId) = fn objects (field-insensitive fallback)
    llvm::SmallVector<uint64_t, 3> Indices; // all GEP indices
    llvm::Type *GEPElemTy; // GEP source element type (for type check)
    ArgList Args;
    std::optional<ValueId> CSRetVal;
  };

  // ---- Data fields ----------------------------------------------------

  const LLVMProjectIRDB &IRDB;              // NOLINT
  const llvm::DataLayout &DL;               // NOLINT
  ValueCompressor<PAGVariable> &ExternalVC; // NOLINT – caller-visible output
  ValueCompressor<AndersenVar> LocalVC{};   // internal variable+object nodes
  Soundness SoundnessFlag;
  library_summary::LLVMFunctionDataFlowFacts LibFacts;

  llvm::TargetLibraryInfoWrapperPass TLA{};
  std::optional<MemSSABundle> MSSABundle{};
  llvm::MemorySSA *CurrentMemSSA = nullptr;

  llvm::SmallVector<const llvm::Function *, 8> FunctionWorklist;
  llvm::DenseSet<const llvm::Function *> Queued; // ever pushed to worklist
  llvm::DenseSet<const llvm::Function *> Processed;

  UnionFind<ValueId> SCCUf;
  TypedVector<ValueId, NodeInfo> Nodes;

  llvm::SmallVector<FPCallRecord> UnresolvedFPCalls;
  llvm::SmallVector<VCallRecord> UnresolvedVCalls;
  llvm::SmallVector<StructVCallRecord> UnresolvedStructVCalls;
  llvm::DenseMap<const llvm::CallBase *, llvm::SmallDenseSet<ValueId, 4>>
      ConnectedCallees;
  CallGraphBuilder<const llvm::Instruction *, const llvm::Function *> CGBuilder;
  llvm::SmallVector<ValueId, 64> PropWorklist;

  // ---- Constructor ----------------------------------------------------

  SolverData(const LLVMProjectIRDB &IRDB,
             llvm::ArrayRef<const llvm::Function *> Entries,
             ValueCompressor<PAGVariable> &VC, Soundness S)
      : IRDB(IRDB), DL(IRDB.getModule()->getDataLayout()), ExternalVC(VC),
        SoundnessFlag(S), LibFacts(library_summary::readFromFDFF(
                              getLibCSummary(), [&IRDB](llvm::StringRef Name) {
                                return IRDB.getFunction(Name);
                              })) {

    CGBuilder.reserve(IRDB.getNumFunctions());
    for (const auto *F : Entries) {
      if (Queued.insert(F).second) {
        FunctionWorklist.push_back(F);

        // entry functions may be missed in the CG, if they are never called
        // explicitly in the code
        std::ignore = CGBuilder.addFunctionVertex(F);
      }
    }
  }

  // ---- Node growth ----------------------------------------------------

  void grow(ValueId V) {
    const auto Idx = size_t(V);
    if (Idx >= Nodes.size()) {
      Nodes.resize(Idx + 1);
      SCCUf.grow(Idx + 1);
    }
  }

  ValueId getOrInsertVar(PAGVariable Var) {
    auto [Id, _] = LocalVC.insert(AndersenVar{Var, false});
    grow(Id);
    return Id;
  }

  ValueId getOrInsertObj(PAGVariable Var) {
    auto [Id, _] = LocalVC.insert(AndersenVar{Var, true});
    grow(Id);
    return Id;
  }

  // pts(VarId) for global objects: functions self-point (the address IS
  // the abstract object); global variables point to their object node.
  void addGlobalPointee(const llvm::GlobalObject *GO, ValueId VarId) {
    if (llvm::isa<llvm::Function>(GO)) {
      addPointee(VarId, VarId);
    } else if (const auto *GVar = llvm::dyn_cast<llvm::GlobalVariable>(GO)) {
      addPointee(VarId, getOrInsertObj(PAGVariable(GVar)));
    }
  }

  [[nodiscard]] ValueId rep(ValueId V) const { return SCCUf.find(V); }

  // Merges the SCCs containing A and B.  Returns the new representative.
  // Folds all pts/edges/constraints from the non-rep into the rep, then
  // clears the non-rep's NodeInfo.  All NonRep data is snapshotted before any
  // addAssignEdge call to avoid reference invalidation via grow().
  ValueId merge(ValueId A, ValueId B) {
    A = rep(A);
    B = rep(B);
    if (A == B) {
      return A;
    }
    const ValueId Rep = SCCUf.join(A, B);
    const ValueId NonRep = (Rep == A) ? B : A;

    // Snapshot all NonRep data before any addAssignEdge / grow calls that
    // may reallocate Nodes and invalidate references.
    auto NRAssignDsts = std::move(Nodes[NonRep].AssignDsts);
    Nodes[NonRep].AssignDstSet.clear();
    const RawAliasSet<ValueId> NRPts = Nodes[NonRep].PtsSet;
    auto NRLoadDsts = std::move(Nodes[NonRep].LoadDsts);
    auto NRStoreSrcs = std::move(Nodes[NonRep].StoreSrcs);
    auto NRMemCopyAsSrc = std::move(Nodes[NonRep].MemCopyAsSrc);
    auto NRMemCopyAsDst = std::move(Nodes[NonRep].MemCopyAsDst);

    // Re-register NonRep's assign edges under Rep.
    for (ValueId Dst : NRAssignDsts) {
      const ValueId DstRep = rep(Dst);
      if (DstRep != Rep) {
        addAssignEdge(Rep, DstRep);
      }
    }

    // Merge pts sets.
    Nodes[Rep].PtsSet.mergeWithDiff(
        NRPts, [&](ValueId NewObj) { onNewPointee(Rep, NewObj); },
        Nodes[Rep].PendingPts);

    // Snapshot Rep's pts (after merge) for retroactive constraint firing.
    const auto RepPts = Nodes[Rep].PtsSet;

    // Transfer NonRep's load constraints and retroactively fire them for
    // Rep's existing pts members.
    for (ValueId D : NRLoadDsts) {
      if (Nodes[Rep].LoadDstSet.insert(D).second) {
        Nodes[Rep].LoadDsts.push_back(D);
        RepPts.foreach ([&](ValueId Obj) { addAssignEdge(Obj, D); });
      }
    }

    // Transfer NonRep's store constraints with retroactive firing.
    for (ValueId S : NRStoreSrcs) {
      if (Nodes[Rep].StoreSrcSet.insert(S).second) {
        Nodes[Rep].StoreSrcs.push_back(S);
        RepPts.foreach ([&](ValueId Obj) { addAssignEdge(S, Obj); });
      }
    }

    // Transfer NonRep's memcpy-as-src constraints with retroactive firing.
    for (ValueId D : NRMemCopyAsSrc) {
      if (Nodes[Rep].MemCopyAsSrcSet.insert(D).second) {
        Nodes[Rep].MemCopyAsSrc.push_back(D);
        if (Nodes.inbounds(D)) {
          const auto &DstPts = Nodes[D].PtsSet;
          RepPts.foreach ([&](ValueId O1) {
            DstPts.foreach ([&](ValueId O2) { addAssignEdge(O1, O2); });
          });
        }
      }
    }

    // Transfer NonRep's memcpy-as-dst constraints with retroactive firing.
    for (ValueId S : NRMemCopyAsDst) {
      if (Nodes[Rep].MemCopyAsDstSet.insert(S).second) {
        Nodes[Rep].MemCopyAsDst.push_back(S);
        if (Nodes.inbounds(S)) {
          const auto &SrcPts = Nodes[S].PtsSet;
          SrcPts.foreach ([&](ValueId O1) {
            RepPts.foreach ([&](ValueId O2) { addAssignEdge(O1, O2); });
          });
        }
      }
    }

    Nodes[NonRep] = NodeInfo{};
    return Rep;
  }

  // ---- Operand traversal ----------------------------------------------

  void forEachOpId(const llvm::Value *V, std::invocable<ValueId> auto Handler) {
    const llvm::Value *Stripped = V->stripPointerCastsAndAliases();
    if (definitelyContainsNoPointer(Stripped)) {
      return;
    }
    psr::forEachPointerOperand(
        Stripped, [this, Handler = copyOrRef(Handler)](const llvm::Value *Op) {
          const ValueId VId = getOrInsertVar(PAGVariable(Op));
          if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(Op)) {
            addGlobalPointee(GO, VId);
          }
          std::invoke(Handler, VId);
        });
  }

  // ---- Constraint insertion -------------------------------------------
  //
  // INVARIANT: every method resolves all ids through rep() first, then calls
  // grow() for all ids before accessing Nodes by reference.  Any grow() call
  // may reallocate the Nodes backing array, so no NodeInfo& must be held
  // across a grow() call.  addAssignEdge does not call grow(), so references
  // into Nodes remain valid across it.

  void addPointee(ValueId Ptr, ValueId Obj) {
    Ptr = rep(Ptr);
    Obj = rep(Obj);
    grow(Ptr);
    grow(Obj); // grow before indexing Nodes[Ptr]
    if (Nodes[Ptr].PtsSet.tryInsert(Obj)) {
      Nodes[Ptr].PendingPts.insert(Obj);
      PropWorklist.push_back(Ptr);
    }
  }

  void addAssignEdge(ValueId Src, ValueId Dst) {
    Src = rep(Src);
    Dst = rep(Dst);
    if (Src == Dst) {
      return;
    }

    if (!Nodes.inbounds(Src) || !Nodes.inbounds(Dst)) [[unlikely]] {
      llvm::report_fatal_error(
          "Connecting nodes which are not allocated yet. Node allocation "
          "should happen through getOrInsertVar or getOrInsertObj");
    }

    if (Nodes[Src].AssignDstSet.insert(Dst).second) {
      Nodes[Src].AssignDsts.push_back(Dst);
      if (!Nodes[Src].PtsSet.empty()) {
        // New edge: Dst has never seen Src's pts history, so mark all of
        // Src's current pts as pending (not just the incremental delta).
        Nodes[Src].PendingPts |= Nodes[Src].PtsSet;
        PropWorklist.push_back(Src);
      }
    }
  }

  void addLoad(ValueId Ptr, ValueId Dst) {
    Ptr = rep(Ptr);
    Dst = rep(Dst);
    grow(Ptr);
    grow(Dst);
    const auto &ExistingPts = Nodes[Ptr].PtsSet;
    ExistingPts.foreach ([&](ValueId Obj) { addAssignEdge(Obj, Dst); });
    if (Nodes[Ptr].LoadDstSet.insert(Dst).second) {
      Nodes[Ptr].LoadDsts.push_back(Dst);
    }
  }

  void addStore(ValueId Ptr, ValueId Src) {
    Ptr = rep(Ptr);
    Src = rep(Src);
    grow(Ptr);
    grow(Src);
    const auto &ExistingPts = Nodes[Ptr].PtsSet;
    ExistingPts.foreach ([&](ValueId Obj) { addAssignEdge(Src, Obj); });
    if (Nodes[Ptr].StoreSrcSet.insert(Src).second) {
      Nodes[Ptr].StoreSrcs.push_back(Src);
    }
  }

  void addMemCopy(ValueId SrcPtr, ValueId DstPtr) {
    SrcPtr = rep(SrcPtr);
    DstPtr = rep(DstPtr);
    grow(SrcPtr);
    grow(DstPtr);
    const auto &SrcPts = Nodes[SrcPtr].PtsSet;
    const auto &DstPts = Nodes[DstPtr].PtsSet;
    SrcPts.foreach ([&](ValueId O1) {
      DstPts.foreach ([&](ValueId O2) { addAssignEdge(O1, O2); });
    });
    if (Nodes[SrcPtr].MemCopyAsSrcSet.insert(DstPtr).second) {
      Nodes[SrcPtr].MemCopyAsSrc.push_back(DstPtr);
    }
    if (Nodes[DstPtr].MemCopyAsDstSet.insert(SrcPtr).second) {
      Nodes[DstPtr].MemCopyAsDst.push_back(SrcPtr);
    }
  }

  // ---- Propagation ----------------------------------------------------

  void onNewPointee(ValueId PtrRep, ValueId NewObj) {
    assert(Nodes.inbounds(PtrRep));
    const auto &LoadDsts = Nodes[PtrRep].LoadDsts;
    const auto &StoreSrcs = Nodes[PtrRep].StoreSrcs;
    const auto &MemSrcs = Nodes[PtrRep].MemCopyAsSrc;
    const auto &MemDsts = Nodes[PtrRep].MemCopyAsDst;

    for (ValueId Dst : LoadDsts) {
      addAssignEdge(NewObj, Dst);
    }
    for (ValueId Src : StoreSrcs) {
      addAssignEdge(Src, NewObj);
    }
    for (ValueId DstPtr : MemSrcs) {
      if (!Nodes.inbounds(DstPtr)) {
        continue;
      }
      const auto &DstPts = Nodes[DstPtr].PtsSet;
      DstPts.foreach ([&](ValueId O2) { addAssignEdge(NewObj, O2); });
    }
    for (ValueId SrcPtr : MemDsts) {
      if (!Nodes.inbounds(SrcPtr)) {
        continue;
      }
      const auto &SrcPts = Nodes[SrcPtr].PtsSet;
      SrcPts.foreach ([&](ValueId O1) { addAssignEdge(O1, NewObj); });
    }
  }

  void propagate() {
    while (!PropWorklist.empty()) {
      ValueId U = rep(PropWorklist.pop_back_val());
      if (!Nodes.inbounds(U) || Nodes[U].PendingPts.empty()) {
        continue;
      }

      // Snapshot resolved successors: merge() can modify Nodes[U].AssignDsts.
      llvm::SmallVector<ValueId, 4> Dsts;
      for (ValueId V : Nodes[U].AssignDsts) {
        Dsts.push_back(rep(V));
      }

      // Drain before iterating Dsts: addAssignEdge inside onNewPointee/merge()
      // may write to Nodes[U].PendingPts while we iterate.
      RawAliasSet<ValueId> UPending = std::exchange(Nodes[U].PendingPts, {});

      for (ValueId VSnap : Dsts) {
        // Re-resolve: a prior iteration's merge() may have changed the rep.
        const ValueId V = rep(VSnap);
        if (V == U || !Nodes.inbounds(V)) {
          continue;
        }

        const bool AddedAny = Nodes[V].PtsSet.mergeWithDiff(
            UPending, [this, V](ValueId Obj) { onNewPointee(V, Obj); },
            Nodes[V].PendingPts);

        if (!AddedAny) {
          // LCD: V has all of U's pending wave, so V.PtsSet ⊇ U.PtsSet.
          if (Nodes[V].AssignDstSet.contains(U)) {
            U = merge(U, V);
          }
          continue;
        }
        PropWorklist.push_back(V);
      }
    }
  }

  // ---- IR translation -------------------------------------------------

  void initGlobals() {
    GlobalInitCache GCache;
    for (const auto &G : IRDB.getModule()->globals()) {
      if (definitelyContainsNoPointer(G.getValueType())) {
        continue;
      }
      const ValueId VarId = getOrInsertVar(PAGVariable(&G));
      const ValueId ObjId = getOrInsertObj(PAGVariable(&G));
      addPointee(VarId, ObjId);
      if (!G.hasInitializer()) {
        continue;
      }
      for (ValueId SrcId :
           GCache.getOrCreate(G.getInitializer(), [&](const llvm::Value *V) {
             const ValueId VId = getOrInsertVar(PAGVariable(V));
             if (const auto *GO = llvm::dyn_cast<llvm::GlobalObject>(V)) {
               addGlobalPointee(GO, VId);
             }
             return VId;
           })) {
        addStore(VarId, SrcId);
      }
    }
    propagate();
  }

  void processFunction(const llvm::Function *F) {
    MSSABundle.emplace(const_cast<llvm::Function &>(*F), &TLA.getTLI(*F));
    CurrentMemSSA = &MSSABundle->MSSA;
    for (const auto &Arg : F->args()) {
      if (!definitelyContainsNoPointer(&Arg)) {
        (void)getOrInsertVar(PAGVariable(&Arg));
      }
    }
    for (const auto &I : llvm::instructions(F)) {
      processInstruction(I);
    }
  }

  void addPtrAlias(const llvm::Value *V, const llvm::Value *Src) {
    forEachOpId(Src, [&](ValueId OpId) {
      LocalVC.addAlias(AndersenVar{PAGVariable(V), false}, OpId);
      grow(OpId);
    });
  }

  void processInstruction(const llvm::Instruction &I) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      const ValueId VarId = getOrInsertVar(PAGVariable(Alloca));
      const ValueId ObjId = getOrInsertObj(PAGVariable(Alloca));
      addPointee(VarId, ObjId);
      return;
    }
    if (const auto *S = llvm::dyn_cast<llvm::StoreInst>(&I)) {
      handleStore(S);
      return;
    }
    if (const auto *L = llvm::dyn_cast<llvm::LoadInst>(&I)) {
      handleLoad(L);
      return;
    }
    if (const auto *M = llvm::dyn_cast<llvm::MemTransferInst>(&I)) {
      handleMemTransfer(M);
      return;
    }
    if (const auto *C = llvm::dyn_cast<llvm::CallBase>(&I)) {
      handleCall(C);
      return;
    }
    if (const auto *R = llvm::dyn_cast<llvm::ReturnInst>(&I)) {
      handleReturn(R);
      return;
    }
    if (const auto *P = llvm::dyn_cast<llvm::PHINode>(&I)) {
      handlePhi(P);
      return;
    }
    if (const auto *S = llvm::dyn_cast<llvm::SelectInst>(&I)) {
      handleSelect(S);
      return;
    }

    // Casts: alias result to stripped operand (field-insensitive).
    if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(&I)) {
      if (!definitelyContainsNoPointer(Cast)) {
        addPtrAlias(Cast, Cast->getOperand(0));
      }
      return;
    }

    // GEPs: alias result to base pointer (field-insensitive).
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
      addPtrAlias(GEP, GEP->getPointerOperand());
    }
  }

  void handleStore(const llvm::StoreInst *S) {
    if (definitelyContainsNoPointer(S->getValueOperand())) {
      return;
    }
    forEachOpId(S->getPointerOperand(), [&](ValueId PtrId) {
      forEachOpId(S->getValueOperand(),
                  [&](ValueId ValId) { addStore(PtrId, ValId); });
    });
  }

  void handleLoad(const llvm::LoadInst *L) {
    if (definitelyContainsNoPointer(L)) {
      return;
    }
    if (CurrentMemSSA) {
      llvm::SmallPtrSet<const llvm::StoreInst *, 4> Defs;
      const bool HasLiveOnEntry = collectReachingDefs(L, *CurrentMemSSA, Defs);
      if (!HasLiveOnEntry) {
        if (Defs.size() == 1) {
          const auto *ValueOp = (*Defs.begin())->getValueOperand();
          if (!llvm::isa<llvm::ConstantExpr>(ValueOp) &&
              !definitelyContainsNoPointer(ValueOp)) {
            addPtrAlias(L, ValueOp);
            return;
          }
          // Non-pointer or ConstantExpr store value: fall through to addLoad.
        } else {
          const ValueId DstId = getOrInsertVar(PAGVariable(L));
          bool AnyEdge = false;
          for (const auto *Def : Defs) {
            forEachOpId(Def->getValueOperand(), [&](ValueId SrcId) {
              addAssignEdge(SrcId, DstId);
              AnyEdge = true;
            });
          }
          if (AnyEdge) {
            return;
          }
          // All reaching stores have non-pointer value operands:
          // fall through to addLoad.
        }
      }
    }
    const ValueId DstId = getOrInsertVar(PAGVariable(L));
    forEachOpId(L->getPointerOperand(),
                [&](ValueId PtrId) { addLoad(PtrId, DstId); });
  }

  void handleMemTransfer(const llvm::MemTransferInst *M) {
    forEachOpId(M->getDest(), [&](ValueId DstPtr) {
      forEachOpId(M->getSource(),
                  [&](ValueId SrcPtr) { addMemCopy(SrcPtr, DstPtr); });
    });
  }

  void handlePhi(const llvm::PHINode *P) {
    if (definitelyContainsNoPointer(P)) {
      return;
    }
    const ValueId PhiId = getOrInsertVar(PAGVariable(P));
    for (const auto &Inc : P->incoming_values()) {
      if (definitelyContainsNoPointer(Inc.get())) {
        continue;
      }
      forEachOpId(Inc.get(),
                  [&](ValueId IncId) { addAssignEdge(IncId, PhiId); });
    }
  }

  void handleSelect(const llvm::SelectInst *S) {
    if (definitelyContainsNoPointer(S)) {
      return;
    }
    const ValueId SelId = getOrInsertVar(PAGVariable(S));
    const auto *TV = S->getTrueValue();
    const auto *FV = S->getFalseValue();
    if (!definitelyContainsNoPointer(TV)) {
      forEachOpId(TV, [&](ValueId Id) { addAssignEdge(Id, SelId); });
    }
    if (!definitelyContainsNoPointer(FV)) {
      forEachOpId(FV, [&](ValueId Id) { addAssignEdge(Id, SelId); });
    }
  }

  void handleReturn(const llvm::ReturnInst *R) {
    const auto *RetVal = R->getReturnValue();
    if (!RetVal || definitelyContainsNoPointer(RetVal)) {
      return;
    }
    const ValueId RetSlotId =
        getOrInsertVar(PAGVariable::Return{R->getFunction()});
    forEachOpId(RetVal,
                [&](ValueId ValId) { addAssignEdge(ValId, RetSlotId); });
  }

  // ---- Call-graph co-refinement ---------------------------------------

  // For each argument, add every function in pts(ArgId) to the worklist
  // as an entry point.  Used when a callee is a declaration and we want to
  // treat fn-ptr arguments as reachable callbacks (Soundy / Sound mode).
  void
  addFnPtrArgsAsEntries(llvm::ArrayRef<llvm::SmallVector<ValueId, 2>> Args) {
    for (const auto &ArgIds : Args) {
      for (ValueId ArgId : ArgIds) {
        ArgId = rep(ArgId);
        if (!Nodes.inbounds(ArgId)) {
          continue;
        }
        Nodes[ArgId].PtsSet.foreach ([&](ValueId ObjId) {
          if (!Nodes.inbounds(ObjId)) {
            return false;
          }
          for (const auto &Var : LocalVC.id2vars(ObjId)) {
            const auto *Fun = llvm::dyn_cast_or_null<llvm::Function>(
                Var.getBase().valueOrNull());
            if (Fun && !Fun->isDeclaration() && Queued.insert(Fun).second) {
              FunctionWorklist.push_back(Fun);
              std::ignore = CGBuilder.addFunctionVertex(Fun);
            }
          }
          return true;
        });
      }
    }
  }

  void applyLibrarySummary(
      const library_summary::LLVMFunctionDataFlowFacts::ParameterMappingTy
          &LibSum,
      const llvm::Function *Fun,
      llvm::ArrayRef<llvm::SmallVector<ValueId, 2>> Args,
      std::optional<ValueId> CSRetVal) {
    const size_t NumParams = Fun->arg_size();
    for (const auto &[ParamIdx, Dests] : LibSum) {
      if (ParamIdx >= NumParams || ParamIdx >= Args.size() ||
          !Fun->getArg(ParamIdx)->getType()->isPointerTy()) {
        continue;
      }
      for (const auto &DestFact : Dests) {
        if (const auto *DestParam =
                DestFact.dyn_cast<library_summary::Parameter>()) {
          if (DestParam->Index >= Args.size()) {
            continue;
          }
          for (ValueId DstId : Args[DestParam->Index]) {
            for (ValueId SrcId : Args[ParamIdx]) {
              addStore(DstId, SrcId);
            }
          }
        } else {
          if (!CSRetVal) {
            continue;
          }
          for (ValueId SrcId : Args[ParamIdx]) {
            addAssignEdge(SrcId, *CSRetVal);
          }
        }
      }
    }
  }

  bool connectCallee(const llvm::CallBase *CS, const llvm::Function *Callee,
                     llvm::ArrayRef<llvm::SmallVector<ValueId, 2>> Args,
                     std::optional<ValueId> CSRetVal) {
    const ValueId CalleeId = getOrInsertVar(PAGVariable(Callee));
    if (!ConnectedCallees[CS].insert(CalleeId).second) {
      return false;
    }
    CGBuilder.addCallEdge(CS, Callee);

    if (Callee->isDeclaration()) {
      if (const auto *LibSum = LibFacts.getFactsForFunctionOrNull(Callee)) {
        applyLibrarySummary(*LibSum, Callee, Args, CSRetVal);
        return false;
      }
      if (SoundnessFlag != Soundness::Unsound) {
        addFnPtrArgsAsEntries(Args);
      }
      return false;
    }

    if (Queued.insert(Callee).second) {
      FunctionWorklist.push_back(Callee);
    }

    if (CSRetVal && !Callee->getReturnType()->isVoidTy()) {
      const ValueId RetSlotId = getOrInsertVar(PAGVariable::Return{Callee});
      addAssignEdge(RetSlotId, *CSRetVal);
    }

    for (const auto &[Param, ArgIds] : llvm::zip(Callee->args(), Args)) {
      if (ArgIds.empty() || definitelyContainsNoPointer(&Param)) {
        continue;
      }
      const ValueId ParamId = getOrInsertVar(PAGVariable(&Param));
      for (ValueId ArgId : ArgIds) {
        addAssignEdge(ArgId, ParamId);
      }
    }

    propagate();
    return true;
  }

  bool resolveVtableCall(const llvm::CallBase *CS, ValueId VtablePtrId,
                         uint64_t VtableIndex, const ArgList &Args,
                         std::optional<ValueId> CSRetVal) {
    VtablePtrId = rep(VtablePtrId);
    if (!Nodes.inbounds(VtablePtrId)) {
      llvm::report_fatal_error("Invalid Vtable Id #" +
                               llvm::Twine(uint32_t(VtablePtrId)));
    }
    bool NewEdge = false;
    // Snapshot: connectCallee→propagate() may grow pts(VtablePtrId).
    const RawAliasSet<ValueId> VPPts = Nodes[VtablePtrId].PtsSet;
    VPPts.foreach ([&](ValueId ObjId) {
      if (!Nodes.inbounds(ObjId)) {
        return false;
      }
      for (const auto &Var : LocalVC.id2vars(ObjId)) {
        const auto *GV = llvm::dyn_cast_or_null<llvm::GlobalVariable>(
            Var.getBase().valueOrNull());
        if (!GV || !GV->hasName() ||
            !GV->getName().starts_with(DIBasedTypeHierarchy::VTablePrefix) ||
            !GV->hasInitializer()) {
          continue;
        }
        const auto *VTStruct =
            llvm::dyn_cast<llvm::ConstantStruct>(GV->getInitializer());
        if (!VTStruct) {
          continue;
        }
        auto VFs = LLVMVFTable::getVFVectorFromIRVTable(*VTStruct);
        if (VtableIndex >= VFs.size()) {
          continue;
        }
        const auto *Callee = VFs[VtableIndex];
        if (!Callee || !isConsistentCall(CS, Callee)) {
          continue;
        }
        NewEdge |= connectCallee(CS, Callee, Args, CSRetVal);
      }
      return true;
    });
    return NewEdge;
  }

  bool resolveStructVCall(const StructVCallRecord &Rec) {
    const ValueId BaseId = rep(Rec.BaseId);
    if (!Nodes.inbounds(BaseId)) {
      llvm::report_fatal_error("Invalid BaseId in resolveStructVCall");
    }
    bool NewEdge = false;
    bool NeedFPFallback = false;
    // Snapshot: connectCallee->propagate() may grow pts(BaseId).
    const RawAliasSet<ValueId> BasePts = Nodes[BaseId].PtsSet;
    BasePts.foreach ([&](ValueId ObjId) {
      if (!Nodes.inbounds(ObjId)) {
        return false;
      }
      for (const auto &Var : LocalVC.id2vars(ObjId)) {
        // Resolve GlobalAlias to the underlying GlobalVariable.
        const llvm::Value *Val = Var.getBase().valueOrNull();
        if (const auto *GA = llvm::dyn_cast_or_null<llvm::GlobalAlias>(Val)) {
          Val = GA->getAliaseeObject();
        }
        const auto *GV = llvm::dyn_cast_or_null<llvm::GlobalVariable>(Val);
        if (!GV || !GV->isConstant() || !GV->hasInitializer()) {
          NeedFPFallback = true;
          continue;
        }
        // Type check: GV must be of GEPElemTy or [N x GEPElemTy].
        // Field-insensitive aliasing can put wrong-type objects in pts.
        llvm::Type *const GVTy = GV->getValueType();
        if (GVTy != Rec.GEPElemTy) {
          const auto *ArrTy = llvm::dyn_cast<llvm::ArrayType>(GVTy);
          if (!ArrTy || ArrTy->getElementType() != Rec.GEPElemTy) {
            NeedFPFallback = true;
            continue;
          }
        }
        const auto *Callee =
            walkConstInitPath(GV->getInitializer(), Rec.Indices);
        if (!Callee || !isConsistentCall(Rec.CS, Callee)) {
          continue;
        }
        NewEdge |= connectCallee(Rec.CS, Callee, Rec.Args, Rec.CSRetVal);
      }
      return true;
    });
    if (NeedFPFallback) {
      NewEdge |= resolveFPCall(Rec.CS, Rec.FPId, Rec.Args, Rec.CSRetVal);
    }
    return NewEdge;
  }

  bool resolveFPCall(const llvm::CallBase *CS, ValueId FPId,
                     const ArgList &Args, std::optional<ValueId> CSRetVal) {
    FPId = rep(FPId);
    if (!Nodes.inbounds(FPId)) {
      llvm::report_fatal_error("Invalid FPId");
    }
    bool NewEdge = false;
    // Snapshot pts(FPId): connectCallee→propagate() may grow pts(FPId).
    const RawAliasSet<ValueId> FPPts = Nodes[FPId].PtsSet;
    FPPts.foreach ([&](ValueId ObjId) {
      if (!Nodes.inbounds(ObjId)) {
        // Iteration is in sorted order
        return false;
      }
      for (const auto &Var : LocalVC.id2vars(ObjId)) {
        const auto *Fun =
            llvm::dyn_cast_or_null<llvm::Function>(Var.getBase().valueOrNull());
        if (Fun && isConsistentCall(CS, Fun)) {
          NewEdge |= connectCallee(CS, Fun, Args, CSRetVal);
        }
      }
      return true;
    });
    return NewEdge;
  }

  void handleCall(const llvm::CallBase *C) {
    if (C->isInlineAsm() || C->isDebugOrPseudoInst()) {
      return;
    }

    // Build one entry per call argument: empty inner vector = non-pointer.
    ArgList Args;
    for (const auto &Arg : C->args()) {
      auto &ArgIds = Args.emplace_back();
      if (!definitelyContainsNoPointer(Arg.get())) {
        forEachOpId(Arg.get(), [&](ValueId Id) { ArgIds.push_back(Id); });
      }
    }

    std::optional<ValueId> CSRetVal;
    if (C->getType()->isPointerTy()) {
      const ValueId VarId = getOrInsertVar(PAGVariable(C));
      CSRetVal = VarId;
      const auto *DirectCallee = llvm::dyn_cast<llvm::Function>(
          C->getCalledOperand()->stripPointerCastsAndAliases());
      if (DirectCallee &&
          psr::isHeapAllocatingFunction(DirectCallee->getName())) {
        const ValueId ObjId = getOrInsertObj(PAGVariable(C));
        addPointee(VarId, ObjId);
      }
    }

    const auto *FnPtr = C->getCalledOperand()->stripPointerCastsAndAliases();

    if (const auto *Callee = llvm::dyn_cast<llvm::Function>(FnPtr)) {
      connectCallee(C, Callee, Args, CSRetVal);
      return;
    }

    // Virtual call: read the concrete vtable at the specific slot index.
    if (auto VCallInfo = getVFTIndexAndVT(C)) {
      auto [VtablePtr, VtableIndex] = *VCallInfo;
      const ValueId VtablePtrId = getOrInsertVar(PAGVariable(VtablePtr));
      resolveVtableCall(C, VtablePtrId, VtableIndex, Args, CSRetVal);
      UnresolvedVCalls.push_back(VCallRecord{
          .CS = C,
          .VtablePtrId = VtablePtrId,
          .VtableIndex = VtableIndex,
          .Args = std::move(Args),
          .CSRetVal = CSRetVal,
      });
      return;
    }

    // Struct-field vtable call: call(load(GEP(base, const_indices...)))
    // with a typed (>=3-operand) GEP. Resolve via global initializer for
    // const globals; fall back to FP resolution for non-const objects.
    if (auto SVInfo = getStructVCallInfo(C)) {
      auto &[BasePtr, Indices, GEPElemTy] = *SVInfo;
      const auto *Load = llvm::cast<llvm::LoadInst>(C->getCalledOperand());
      const ValueId BaseId = getOrInsertVar(PAGVariable(BasePtr));
      const ValueId FPId = getOrInsertVar(PAGVariable(Load));
      StructVCallRecord Rec{
          .CS = C,
          .BaseId = BaseId,
          .FPId = FPId,
          .Indices = std::move(Indices),
          .GEPElemTy = GEPElemTy,
          .Args = std::move(Args),
          .CSRetVal = CSRetVal,
      };
      resolveStructVCall(Rec);
      UnresolvedStructVCalls.push_back(std::move(Rec));
      return;
    }

    // Indirect call: connect already-known targets, record for fixpoint.
    const ValueId FPId = getOrInsertVar(PAGVariable(FnPtr));
    resolveFPCall(C, FPId, Args, CSRetVal);
    UnresolvedFPCalls.push_back(FPCallRecord{
        .CS = C,
        .FPId = FPId,
        .Args = std::move(Args),
        .CSRetVal = CSRetVal,
    });
  }

  bool checkUnresolvedFPCalls() {
    bool NewEdge = false;
    for (const auto &Rec : UnresolvedFPCalls) {
      NewEdge |= resolveFPCall(Rec.CS, Rec.FPId, Rec.Args, Rec.CSRetVal);
    }
    return NewEdge;
  }

  bool checkUnresolvedVCalls() {
    bool NewEdge = false;
    for (const auto &Rec : UnresolvedVCalls) {
      NewEdge |= resolveVtableCall(Rec.CS, Rec.VtablePtrId, Rec.VtableIndex,
                                   Rec.Args, Rec.CSRetVal);
    }
    return NewEdge;
  }

  bool checkUnresolvedStructVCalls() {
    bool NewEdge = false;
    for (const auto &Rec : UnresolvedStructVCalls) {
      NewEdge |= resolveStructVCall(Rec);
    }
    return NewEdge;
  }

  // ---- Result construction --------------------------------------------

  AndersenOTFResult buildResult() {
    const size_t NumLocal = LocalVC.size();

    // Map variable local IDs → external VC IDs.
    // Object nodes are internal only and do not appear in the external result.
    TypedVector<ValueId, std::optional<ValueId>> LocalToExt(NumLocal);
    for (auto VId : iota<ValueId>(NumLocal)) {
      std::optional<ValueId> FirstExtId;
      for (const auto &V : LocalVC.id2vars(VId)) {
        if (V.isObject()) {
          continue;
        }
        if (!FirstExtId) {
          FirstExtId = ExternalVC.insert(V.getBase()).first;
          LocalToExt[VId] = FirstExtId;
        } else {
          ExternalVC.addAlias(V.getBase(), *FirstExtId);
        }
      }
    }

    // Build rep → bitset of external IDs for all vars in that SCC.
    TypedVector<ValueId, llvm::SmallVector<ValueId>> RepToExtVIds(NumLocal);
    for (auto VId : iota<ValueId>(NumLocal)) {
      if (!LocalToExt[VId]) {
        continue;
      }
      const ValueId RepId = rep(VId);
      if (!Nodes.inbounds(RepId)) {
        continue;
      }
      RepToExtVIds[RepId].push_back(*LocalToExt[VId]);
    }

    // Reverse map: abstract object → bitset of representatives pointing to it.
    // Only representatives with at least one external variable are inserted.
    TypedVector<ValueId, RawAliasSet<ValueId>> Obj2Reps(NumLocal);
    for (auto RepId : iota<ValueId>(NumLocal)) {
      if (RepToExtVIds[RepId].empty()) {
        continue;
      }
      if (!Nodes.inbounds(RepId)) {
        continue;
      }
      Nodes[RepId].PtsSet.foreach ([&](ValueId Obj) {
        if (Obj2Reps.inbounds(Obj)) {
          Obj2Reps[Obj].insert(RepId);
          return true;
        }
        // Iteration is in sorted order
        return false;
      });
    }

    // Precompute per-object alias set: for each abstract object, the union of
    // all external IDs of every representative that points to it.  Built once
    // here via sort+insertSorted so the main loop below can use fast |=.
    TypedVector<ValueId, RawAliasSet<ValueId>> ObjToAliasExtVIds(NumLocal);
    {
      llvm::SmallVector<uint32_t, 64> Buf;
      for (const auto &[Obj, Reps] : Obj2Reps.enumerate()) {
        if (Reps.empty()) {
          continue;
        }
        Reps.foreach ([&](ValueId AliasRepId) {
          for (auto EId : RepToExtVIds[AliasRepId]) {
            Buf.push_back(uint32_t(EId));
          }
        });
        std::ranges::sort(Buf);
        ObjToAliasExtVIds[Obj].insertSorted(Buf);
        Buf.clear();
      }
    }

    AndersenOTFResult Result{};
    Result.AliasSets.resize(ExternalVC.size());

    for (const auto &[RepId, ExtVIds] : RepToExtVIds.enumerate()) {
      if (ExtVIds.empty()) {
        continue;
      }
      if (!Nodes.inbounds(RepId)) {
        break; // iota is monotone; all subsequent IDs exceed Nodes.size()
      }

      // Union the pre-built per-object alias sets for all pointees.
      RawAliasSet<ValueId> AliasExtVIds;
      Nodes[RepId].PtsSet.foreach ([&](ValueId Obj) {
        if (size_t(Obj) >= NumLocal) {
          // Iteration is in sorted order
          return false;
        }
        AliasExtVIds |= ObjToAliasExtVIds[Obj];
        return true;
      });

      // Broadcast to every external ID mapped to this representative.
      for (auto ExtVId : ExtVIds) {
        Result.AliasSets[ExtVId] |= AliasExtVIds;
      }
    }

    Result.CG = CGBuilder.consumeCallGraph();
    return Result;
  }

  // ---- Main loop ------------------------------------------------------

  AndersenOTFResult run() {
    initGlobals();

    bool Changed{};
    do {
      while (!FunctionWorklist.empty()) {
        const auto *F = FunctionWorklist.pop_back_val();
        if (!Processed.insert(F).second) {
          continue;
        }
        processFunction(F);
        // Drain pending pts for functions that make no pointer-relevant
        // calls (connectCallee would otherwise be the only propagate site).
        propagate();
      }
      Changed = checkUnresolvedFPCalls();
      Changed |= checkUnresolvedVCalls();
      Changed |= checkUnresolvedStructVCalls();
    } while (!FunctionWorklist.empty() || Changed);

    return buildResult();
  }
};

// ---- AndersenOTFSolver --------------------------------------------------

AndersenOTFSolver::AndersenOTFSolver(
    const LLVMProjectIRDB &IRDB, llvm::ArrayRef<const llvm::Function *> Entries,
    ValueCompressor<PAGVariable> &VC, Soundness S) noexcept
    : IRDB(IRDB), Entries(Entries), VC(VC), S(S) {}

AndersenOTFResult AndersenOTFSolver::solve() {
  SolverData Impl{*IRDB, Entries, *VC, S};
  return Impl.run();
}

// ---- Factory functions --------------------------------------------------

AndersenOTFResult
psr::computeAndersenOTFRaw(const LLVMProjectIRDB &IRDB,
                           llvm::ArrayRef<const llvm::Function *> EntryPoints,
                           MaybeUniquePtr<ValueCompressor<PAGVariable>> VC,
                           Soundness S) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC, S);
  return Solver.solve();
}

LLVMUnionFindAliasIterator<AndersenOTFResult>
psr::computeAndersenOTF(const LLVMProjectIRDB &IRDB,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        MaybeUniquePtr<ValueCompressor<PAGVariable>> VC,
                        Soundness S) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC, S);
  auto Res = Solver.solve();
  return LLVMUnionFindAliasIterator{std::move(Res), std::move(VC)};
}
