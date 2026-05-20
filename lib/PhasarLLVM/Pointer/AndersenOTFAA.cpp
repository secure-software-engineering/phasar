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
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/LibrarySummary.h"
#include "phasar/Utils/UnionFind.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <optional>

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

  // ---- Data fields ----------------------------------------------------

  const LLVMProjectIRDB &IRDB;              // NOLINT
  const llvm::DataLayout &DL;               // NOLINT
  ValueCompressor<PAGVariable> &ExternalVC; // NOLINT – caller-visible output
  ValueCompressor<AndersenVar> LocalVC{};   // internal variable+object nodes

  llvm::SmallVector<const llvm::Function *, 8> FunctionWorklist;
  llvm::DenseSet<const llvm::Function *> Reachable;
  llvm::DenseSet<const llvm::Function *> Processed;

  UnionFind<ValueId> SCCUf;
  TypedVector<ValueId, NodeInfo> Nodes;

  llvm::SmallVector<FPCallRecord> UnresolvedFPCalls;
  llvm::DenseMap<const llvm::CallBase *, llvm::SmallDenseSet<ValueId, 4>>
      ConnectedCallees;
  llvm::SmallVector<ValueId, 64> PropWorklist;

  // ---- Constructor ----------------------------------------------------

  SolverData(const LLVMProjectIRDB &IRDB,
             llvm::ArrayRef<const llvm::Function *> Entries,
             ValueCompressor<PAGVariable> &VC)
      : IRDB(IRDB), DL(IRDB.getModule()->getDataLayout()), ExternalVC(VC) {
    for (const auto *F : Entries) {
      if (Reachable.insert(F).second) {
        FunctionWorklist.push_back(F);
      }
    }
  }

  // ---- Node growth ----------------------------------------------------

  NodeInfo &grow(ValueId V) {
    const auto Idx = size_t(V);
    if (Idx >= Nodes.size()) {
      Nodes.resize(Idx + 1);
      SCCUf.grow(Idx + 1);
    }
    return Nodes[V];
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
    const auto OldRepPts = Nodes[Rep].PtsSet;
    const bool PtsGrew = Nodes[Rep].PtsSet.tryMergeWith(NRPts);
    if (PtsGrew) {
      Nodes[Rep].PendingPts |= NRPts;
      PropWorklist.push_back(Rep);
      // Fire Rep's pre-existing load/store/memcopy constraints for pointees
      // absorbed from NonRep that Rep didn't previously have.
      const auto Diff = NRPts - OldRepPts;
      Diff.foreach ([&](ValueId NewObj) { onNewPointee(Rep, NewObj); });
    }

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
          // Snapshot DstPtr's pts: addAssignEdge may resize Nodes.
          const RawAliasSet<ValueId> DstPts = Nodes[D].PtsSet;
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
          // Snapshot SrcPtr's pts: addAssignEdge may resize Nodes.
          const RawAliasSet<ValueId> SrcPts = Nodes[S].PtsSet;
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
    V = V->stripPointerCastsAndAliases();
    if (definitelyContainsNoPointer(V)) {
      return;
    }

    if (!llvm::isa<llvm::ConstantExpr>(V)) {
      const ValueId VId = getOrInsertVar(PAGVariable(V));
      // A function used as a value (e.g. stored into a function-pointer
      // variable) is an addressable abstract object: pts(F) = {F}.
      // Without this, pts(fp_alloca) never gains F and OTF call resolution
      // silently produces no callees.
      if (llvm::isa<llvm::Function>(V)) {
        addPointee(VId, VId);
      }
      std::invoke(Handler, VId);
      return;
    }

    // Walk ConstantExpr chains to find the underlying GlobalObject(s).
    llvm::SmallDenseSet<const llvm::Value *> Seen = {V};
    llvm::SmallVector<const llvm::User *> WL = {
        llvm::cast<llvm::ConstantExpr>(V)};
    do {
      const auto *Curr = WL.pop_back_val();
      for (const auto *Op : Curr->operand_values()) {
        if (definitelyContainsNoPointer(Op) || !Seen.insert(Op).second) {
          continue;
        }
        if (const auto *GObj = llvm::dyn_cast<llvm::GlobalObject>(Op)) {
          const ValueId GId = getOrInsertVar(PAGVariable(GObj));
          if (llvm::isa<llvm::Function>(GObj)) {
            addPointee(GId, GId);
          }
          std::invoke(Handler, GId);
          continue;
        }
        if (const auto *User = llvm::dyn_cast<llvm::User>(Op)) {
          WL.push_back(User);
        }
      }
    } while (!WL.empty());
  }

  // ---- Constraint insertion -------------------------------------------
  //
  // INVARIANT: every method resolves all ids through rep() first, then calls
  // grow() for all ids before accessing Nodes by reference.  Any grow() call
  // may reallocate the Nodes backing array, so no NodeInfo& must be held
  // across a grow() call or across any call that may invoke grow() (i.e.
  // addAssignEdge, addPointee, etc.).  Where the existing pts set must be
  // iterated while addAssignEdge is called inside, the pts set is first
  // copied into a local snapshot.

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
    grow(Src);
    grow(Dst); // grow before indexing Nodes[Src]
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
    grow(Dst); // grow before accessing Nodes[Ptr]
    // Snapshot pts: addAssignEdge inside the lambda may resize Nodes.
    const RawAliasSet<ValueId> ExistingPts = Nodes[Ptr].PtsSet;
    ExistingPts.foreach ([&](ValueId Obj) { addAssignEdge(Obj, Dst); });
    if (Nodes[Ptr].LoadDstSet.insert(Dst).second) {
      Nodes[Ptr].LoadDsts.push_back(Dst);
    }
  }

  void addStore(ValueId Ptr, ValueId Src) {
    Ptr = rep(Ptr);
    Src = rep(Src);
    grow(Ptr);
    grow(Src); // grow before accessing Nodes[Ptr]
    // Snapshot pts: addAssignEdge inside the lambda may resize Nodes.
    const RawAliasSet<ValueId> ExistingPts = Nodes[Ptr].PtsSet;
    ExistingPts.foreach ([&](ValueId Obj) { addAssignEdge(Src, Obj); });
    if (Nodes[Ptr].StoreSrcSet.insert(Src).second) {
      Nodes[Ptr].StoreSrcs.push_back(Src);
    }
  }

  void addMemCopy(ValueId SrcPtr, ValueId DstPtr) {
    SrcPtr = rep(SrcPtr);
    DstPtr = rep(DstPtr);
    grow(SrcPtr);
    grow(DstPtr); // grow before accessing Nodes[SrcPtr/DstPtr]
    // Snapshot both pts sets: addAssignEdge inside the lambdas may resize
    // Nodes, invalidating any reference into it.
    const RawAliasSet<ValueId> SrcPts = Nodes[SrcPtr].PtsSet;
    const RawAliasSet<ValueId> DstPts = Nodes[DstPtr].PtsSet;
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
    // Snapshot all constraint lists before any addAssignEdge call: grow()
    // inside addAssignEdge may reallocate Nodes, invalidating references.
    const auto LoadDsts = Nodes[PtrRep].LoadDsts;
    const auto StoreSrcs = Nodes[PtrRep].StoreSrcs;
    const auto MemSrcs = Nodes[PtrRep].MemCopyAsSrc;
    const auto MemDsts = Nodes[PtrRep].MemCopyAsDst;

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
      // Snapshot DstPtr's pts: addAssignEdge may resize Nodes.
      const RawAliasSet<ValueId> DstPts = Nodes[DstPtr].PtsSet;
      DstPts.foreach ([&](ValueId O2) { addAssignEdge(NewObj, O2); });
    }
    for (ValueId SrcPtr : MemDsts) {
      if (!Nodes.inbounds(SrcPtr)) {
        continue;
      }
      // Snapshot SrcPtr's pts: addAssignEdge may resize Nodes.
      const RawAliasSet<ValueId> SrcPts = Nodes[SrcPtr].PtsSet;
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

      // Drain before iterating Dsts: onNewPointee → addPointee may write
      // to Nodes[U].PendingPts while we iterate, and merge() may resize Nodes.
      RawAliasSet<ValueId> UPending = std::move(Nodes[U].PendingPts);

      for (ValueId VSnap : Dsts) {
        // Re-resolve: a prior iteration's merge() may have changed the rep.
        const ValueId V = rep(VSnap);
        if (V == U || !Nodes.inbounds(V)) {
          continue;
        }

        bool AddedAny = false;
        UPending.foreach ([&](ValueId Obj) {
          if (Nodes[V].PtsSet.tryInsert(Obj)) {
            Nodes[V].PendingPts.insert(Obj);
            onNewPointee(V, Obj);
            AddedAny = true;
          }
        });
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
             return getOrInsertVar(PAGVariable(V));
           })) {
        addStore(VarId, SrcId);
      }
    }
    propagate();
  }

  void processFunction(const llvm::Function *F) {
    for (const auto &Arg : F->args()) {
      if (!definitelyContainsNoPointer(&Arg)) {
        (void)getOrInsertVar(PAGVariable(&Arg));
      }
    }
    for (const auto &I : llvm::instructions(F)) {
      processInstruction(I);
    }
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
      if (definitelyContainsNoPointer(Cast)) {
        return;
      }
      forEachOpId(Cast->getOperand(0), [&](ValueId OpId) {
        LocalVC.addAlias(AndersenVar{PAGVariable(Cast), false}, OpId);
        grow(OpId);
      });
      return;
    }

    // GEPs: alias result to base pointer (field-insensitive).
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
      forEachOpId(GEP->getPointerOperand(), [&](ValueId OpId) {
        LocalVC.addAlias(AndersenVar{PAGVariable(GEP), false}, OpId);
        grow(OpId);
      });
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

  void connectCallee(const llvm::CallBase *CS, const llvm::Function *Callee,
                     llvm::ArrayRef<llvm::SmallVector<ValueId, 2>> Args,
                     std::optional<ValueId> CSRetVal) {
    if (Callee->isDeclaration()) {
      return;
    }

    const ValueId CalleeId = getOrInsertVar(PAGVariable(Callee));
    if (!ConnectedCallees[CS].insert(CalleeId).second) {
      return;
    }

    if (Reachable.insert(Callee).second) {
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
  }

  void handleCall(const llvm::CallBase *C) {
    if (C->isInlineAsm() || C->isDebugOrPseudoInst()) {
      return;
    }

    // Build one entry per call argument: empty inner vector = non-pointer.
    ArgList Args;
    for (const auto &Arg : C->args()) {
      llvm::SmallVector<ValueId, 2> ArgIds;
      if (!definitelyContainsNoPointer(Arg.get())) {
        forEachOpId(Arg.get(), [&](ValueId Id) { ArgIds.push_back(Id); });
      }
      Args.push_back(std::move(ArgIds));
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

    // Indirect call: connect already-known targets, record for fixpoint.
    const ValueId FPId = getOrInsertVar(PAGVariable(FnPtr));

    const auto ConnectKnownTargets = [&]() {
      if (!Nodes.inbounds(FPId)) {
        return;
      }
      // Snapshot pts(FPId): connectCallee→propagate() may grow pts(FPId).
      const RawAliasSet<ValueId> FPPts = Nodes[FPId].PtsSet;
      FPPts.foreach ([&](ValueId ObjId) {
        if (!Nodes.inbounds(ObjId)) {
          // Iteration is in sorted order
          return false;
        }
        for (const auto &Var : LocalVC.id2vars(ObjId)) {
          const auto *Fun = llvm::dyn_cast_or_null<llvm::Function>(
              Var.getBase().valueOrNull());
          if (Fun) {
            connectCallee(C, Fun, Args, CSRetVal);
          }
        }
        return true;
      });
    };

    ConnectKnownTargets();
    UnresolvedFPCalls.push_back(FPCallRecord{
        .CS = C,
        .FPId = FPId,
        .Args = {Args.begin(), Args.end()},
        .CSRetVal = CSRetVal,
    });
  }

  void checkUnresolvedFPCalls() {
    for (const auto &Rec : UnresolvedFPCalls) {
      if (!Nodes.inbounds(Rec.FPId)) {
        continue;
      }
      // Snapshot pts(FPId): connectCallee→propagate() may grow it.
      const RawAliasSet<ValueId> FPPts = Nodes[Rec.FPId].PtsSet;
      FPPts.foreach ([&](ValueId ObjId) {
        if (!Nodes.inbounds(ObjId)) {
          // Iteration is in sorted order
          return false;
        }
        for (const auto &Var : LocalVC.id2vars(ObjId)) {
          const auto *Fun = llvm::dyn_cast_or_null<llvm::Function>(
              Var.getBase().valueOrNull());
          if (Fun) {
            connectCallee(Rec.CS, Fun, Rec.Args, Rec.CSRetVal);
          }
        }
        return true;
      });
    }
  }

  // ---- Result construction --------------------------------------------

  AndersenOTFResult buildResult() {
    const size_t NumLocal = LocalVC.size();

    // Map variable local IDs → external VC IDs.
    // Object nodes are internal only and do not appear in the external result.
    TypedVector<ValueId, std::optional<ValueId>> LocalToExt(NumLocal);
    for (auto VId : iota<ValueId>(NumLocal)) {
      ValueId FirstExtId{};
      bool HasFirst = false;
      for (const auto &V : LocalVC.id2vars(VId)) {
        if (V.isObject()) {
          continue;
        }
        if (!HasFirst) {
          FirstExtId = ExternalVC.insert(V.getBase()).first;
          HasFirst = true;
          LocalToExt[VId] = FirstExtId;
        } else {
          ExternalVC.addAlias(V.getBase(), FirstExtId);
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
        if (size_t(Obj) < NumLocal) {
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
      for (auto Obj : iota<ValueId>(NumLocal)) {
        if (Obj2Reps[Obj].empty()) {
          continue;
        }
        Obj2Reps[Obj].foreach ([&](ValueId AliasRepId) {
          for (auto EId : RepToExtVIds[AliasRepId]) {
            Buf.push_back(uint32_t(EId));
          }
        });
        std::ranges::sort(Buf);
        // Buf.erase(std::ranges::unique(Buf).begin(), Buf.end());
        ObjToAliasExtVIds[Obj].insertSorted(Buf);
        Buf.clear();
      }
    }

    AndersenOTFResult Result;
    Result.NumVars = ExternalVC.size();
    Result.AliasSets.resize(Result.NumVars);

    for (auto RepId : iota<ValueId>(NumLocal)) {
      const auto &MyExtVIds = RepToExtVIds[RepId];
      if (MyExtVIds.empty()) {
        continue;
      }
      if (!Nodes.inbounds(RepId)) {
        break;
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
      for (auto ExtVId : MyExtVIds) {
        Result.AliasSets[ExtVId] |= AliasExtVIds;
      }
    }

    return Result;
  }

  // ---- Main loop ------------------------------------------------------

  AndersenOTFResult run() {
    initGlobals();

    do {
      while (!FunctionWorklist.empty()) {
        const auto *F = FunctionWorklist.pop_back_val();
        if (!Processed.insert(F).second) {
          continue;
        }
        processFunction(F);
        propagate();
      }
      checkUnresolvedFPCalls();
    } while (!FunctionWorklist.empty());

    return buildResult();
  }
};

// ---- AndersenOTFSolver --------------------------------------------------

AndersenOTFSolver::AndersenOTFSolver(
    const LLVMProjectIRDB &IRDB, llvm::ArrayRef<const llvm::Function *> Entries,
    ValueCompressor<PAGVariable> &VC) noexcept
    : IRDB(IRDB), Entries(Entries), VC(VC) {}

AndersenOTFResult AndersenOTFSolver::solve() {
  SolverData Impl{*IRDB, Entries, *VC};
  return Impl.run();
}

// ---- Factory functions --------------------------------------------------

AndersenOTFResult
psr::computeAndersenOTFRaw(const LLVMProjectIRDB &IRDB,
                           llvm::ArrayRef<const llvm::Function *> EntryPoints,
                           MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC);
  return Solver.solve();
}

LLVMUnionFindAliasIterator<AndersenOTFResult>
psr::computeAndersenOTF(const LLVMProjectIRDB &IRDB,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        MaybeUniquePtr<ValueCompressor<PAGVariable>> VC) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC);
  auto Res = Solver.solve();
  return LLVMUnionFindAliasIterator{std::move(Res), std::move(VC)};
}
