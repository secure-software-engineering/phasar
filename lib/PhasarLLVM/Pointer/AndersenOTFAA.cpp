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
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/UnionFind.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <cstdint>
#include <optional>

using namespace psr;

// Sentinel: non-pointer argument slot (no ValueId assigned).
static constexpr ValueId NoArgId = ValueId(UINT32_MAX);

struct [[clang::internal_linkage]] AndersenOTFSolver::SolverData {
  // ---- Per-node state -------------------------------------------------

  struct NodeInfo {
    RawAliasSet<ValueId> PtsSet;
    // Assignment edges: pts(this) ⊆ pts(dst) for each dst.
    llvm::SmallVector<ValueId, 2> AssignDsts;
    llvm::SmallDenseSet<ValueId, 4> AssignDstSet; // dedup guard
    // Load constraints: dst = *this.
    llvm::SmallVector<ValueId, 1> LoadDsts;
    // Store constraints: *this = src.
    llvm::SmallVector<ValueId, 1> StoreSrcs;
    // MemCopy: memcpy(dst_ptr, this=src_ptr).
    llvm::SmallVector<ValueId, 1> MemCopyAsSrc;
    // MemCopy: memcpy(this=dst_ptr, src_ptr).
    llvm::SmallVector<ValueId, 1> MemCopyAsDst;
  };

  struct FPCallRecord {
    const llvm::CallBase *CS;
    ValueId FPId;
    llvm::SmallVector<ValueId, 4> Args;
    std::optional<ValueId> CSRetVal;
  };

  // ---- Data fields ----------------------------------------------------

  const LLVMProjectIRDB &IRDB;      // NOLINT
  const llvm::DataLayout &DL;       // NOLINT
  ValueCompressor<PAGVariable> &VC; // NOLINT

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
      : IRDB(IRDB), DL(IRDB.getModule()->getDataLayout()), VC(VC) {
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

  ValueId getOrInsert(PAGVariable Var) {
    auto [Id, Inserted] = VC.insert(Var);
    (void)Inserted;
    grow(Id);
    return Id;
  }

  ValueId getOrInsert(const llvm::Value *V) {
    return getOrInsert(PAGVariable(V));
  }

  // ---- Operand traversal ----------------------------------------------

  void forEachOpId(const llvm::Value *V, std::invocable<ValueId> auto Handler) {
    V = V->stripPointerCastsAndAliases();
    if (definitelyContainsNoPointer(V)) {
      return;
    }

    if (!llvm::isa<llvm::ConstantExpr>(V)) {
      std::invoke(Handler, getOrInsert(V));
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
          std::invoke(Handler, getOrInsert(GObj));
          continue;
        }
        if (const auto *User = llvm::dyn_cast<llvm::User>(Op)) {
          WL.push_back(User);
        }
      }
    } while (!WL.empty());
  }

  // ---- Constraint insertion -------------------------------------------

  void addPointee(ValueId Ptr, ValueId Obj) {
    auto &PtrNode = grow(Ptr);
    (void)grow(Obj);
    if (PtrNode.PtsSet.tryInsert(Obj)) {
      PropWorklist.push_back(Ptr);
    }
  }

  void addAssignEdge(ValueId Src, ValueId Dst) {
    if (Src == Dst) {
      return;
    }
    auto &SrcNode = grow(Src);
    (void)grow(Dst);
    if (SrcNode.AssignDstSet.insert(Dst).second) {
      SrcNode.AssignDsts.push_back(Dst);
      if (!SrcNode.PtsSet.empty()) {
        PropWorklist.push_back(Src);
      }
    }
  }

  void addLoad(ValueId Ptr, ValueId Dst) {
    auto &PtrNode = grow(Ptr);
    (void)grow(Dst);
    PtrNode.PtsSet.foreach ([&](ValueId Obj) { addAssignEdge(Obj, Dst); });
    PtrNode.LoadDsts.push_back(Dst);
  }

  void addStore(ValueId Ptr, ValueId Src) {
    auto &PtrNode = grow(Ptr);
    (void)grow(Src);
    PtrNode.PtsSet.foreach ([&](ValueId Obj) { addAssignEdge(Src, Obj); });
    PtrNode.StoreSrcs.push_back(Src);
  }

  void addMemCopy(ValueId SrcPtr, ValueId DstPtr) {
    auto &SrcNode = grow(SrcPtr);
    auto &DstNode = grow(DstPtr);
    SrcNode.PtsSet.foreach ([&](ValueId O1) {
      DstNode.PtsSet.foreach ([&](ValueId O2) { addAssignEdge(O1, O2); });
    });
    SrcNode.MemCopyAsSrc.push_back(DstPtr);
    DstNode.MemCopyAsDst.push_back(SrcPtr);
  }

  // ---- Propagation ----------------------------------------------------

  void onNewPointee(ValueId PtrRep, ValueId NewObj) {
    assert(Nodes.inbounds(PtrRep));
    const auto &Node = Nodes[PtrRep];

    for (ValueId Dst : Node.LoadDsts) {
      addAssignEdge(NewObj, Dst);
    }
    for (ValueId Src : Node.StoreSrcs) {
      addAssignEdge(Src, NewObj);
    }
    for (ValueId DstPtr : Node.MemCopyAsSrc) {
      if (!Nodes.inbounds(DstPtr)) {
        continue;
      }
      Nodes[DstPtr].PtsSet.foreach (
          [&](ValueId O2) { addAssignEdge(NewObj, O2); });
    }
    for (ValueId SrcPtr : Node.MemCopyAsDst) {
      if (!Nodes.inbounds(SrcPtr)) {
        continue;
      }
      Nodes[SrcPtr].PtsSet.foreach (
          [&](ValueId O1) { addAssignEdge(O1, NewObj); });
    }
  }

  void propagate() {
    while (!PropWorklist.empty()) {
      const ValueId U = PropWorklist.pop_back_val();
      if (!Nodes.inbounds(U)) {
        continue;
      }
      const auto &UNode = Nodes[U];

      for (ValueId V : UNode.AssignDsts) {
        if (!Nodes.inbounds(V) || V == U) {
          continue;
        }
        auto &VNode = Nodes[V];
        RawAliasSet<ValueId> NewPts = UNode.PtsSet;
        NewPts -= VNode.PtsSet;
        if (NewPts.empty()) {
          continue;
        }
        VNode.PtsSet |= NewPts;
        PropWorklist.push_back(V);
        NewPts.foreach ([&](ValueId NewObj) { onNewPointee(V, NewObj); });
      }
    }
  }

  // ---- IR translation -------------------------------------------------

  void initGlobals() {
    for (const auto &G : IRDB.getModule()->globals()) {
      if (definitelyContainsNoPointer(G.getValueType())) {
        continue;
      }
      const ValueId GId = getOrInsert(&G);
      addPointee(GId, GId);
    }
    propagate();
  }

  void processFunction(const llvm::Function *F) {
    for (const auto &Arg : F->args()) {
      if (!definitelyContainsNoPointer(&Arg)) {
        (void)getOrInsert(&Arg);
      }
    }
    for (const auto &I : llvm::instructions(F)) {
      processInstruction(I);
    }
  }

  void processInstruction(const llvm::Instruction &I) {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      const ValueId Id = getOrInsert(Alloca);
      addPointee(Id, Id);
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
        VC.addAlias(Cast, OpId);
        grow(OpId);
      });
      return;
    }

    // GEPs: alias result to base pointer (field-insensitive).
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
      forEachOpId(GEP->getPointerOperand(), [&](ValueId OpId) {
        VC.addAlias(GEP, OpId);
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
    const ValueId DstId = getOrInsert(L);
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
    const ValueId PhiId = getOrInsert(P);
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
    const ValueId SelId = getOrInsert(S);
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
        getOrInsert(PAGVariable::Return{R->getFunction()});
    forEachOpId(RetVal,
                [&](ValueId ValId) { addAssignEdge(ValId, RetSlotId); });
  }

  // ---- Call-graph co-refinement ---------------------------------------

  void connectCallee(const llvm::CallBase *CS, const llvm::Function *Callee,
                     llvm::ArrayRef<ValueId> Args,
                     std::optional<ValueId> CSRetVal) {
    if (Callee->isDeclaration()) {
      return;
    }

    const ValueId CalleeId = getOrInsert(Callee);
    if (!ConnectedCallees[CS].insert(CalleeId).second) {
      return;
    }

    if (Reachable.insert(Callee).second) {
      FunctionWorklist.push_back(Callee);
    }

    if (CSRetVal && !Callee->getReturnType()->isVoidTy()) {
      const ValueId RetSlotId = getOrInsert(PAGVariable::Return{Callee});
      addAssignEdge(RetSlotId, *CSRetVal);
    }

    for (const auto &[Param, ArgId] : llvm::zip(Callee->args(), Args)) {
      if (ArgId == NoArgId || definitelyContainsNoPointer(&Param)) {
        continue;
      }
      addAssignEdge(ArgId, getOrInsert(&Param));
    }

    propagate();
  }

  void handleCall(const llvm::CallBase *C) {
    if (C->isInlineAsm()) {
      return;
    }

    llvm::SmallVector<ValueId, 4> Args;
    for (const auto &Arg : C->args()) {
      if (definitelyContainsNoPointer(Arg.get())) {
        Args.push_back(NoArgId);
        continue;
      }
      ValueId ArgId = NoArgId;
      forEachOpId(Arg.get(), [&](ValueId Id) { ArgId = Id; });
      Args.push_back(ArgId);
    }

    std::optional<ValueId> CSRetVal;
    if (C->getType()->isPointerTy()) {
      CSRetVal = getOrInsert(C);
    }

    const auto *FnPtr = C->getCalledOperand()->stripPointerCastsAndAliases();

    if (const auto *Callee = llvm::dyn_cast<llvm::Function>(FnPtr)) {
      connectCallee(C, Callee, Args, CSRetVal);
      return;
    }

    // Indirect call: connect already-known targets, record for fixpoint.
    const ValueId FPId = getOrInsert(FnPtr);

    const auto ConnectKnownTargets = [&]() {
      if (!Nodes.inbounds(FPId)) {
        return;
      }
      Nodes[FPId].PtsSet.foreach ([&](ValueId ObjId) {
        if (!Nodes.inbounds(ObjId)) {
          return;
        }
        for (const auto &Var : VC.id2vars(ObjId)) {
          const auto *Fun =
              llvm::dyn_cast_or_null<llvm::Function>(Var.valueOrNull());
          if (Fun) {
            connectCallee(C, Fun, Args, CSRetVal);
          }
        }
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
      Nodes[Rec.FPId].PtsSet.foreach ([&](ValueId ObjId) {
        if (!Nodes.inbounds(ObjId)) {
          return;
        }
        for (const auto &Var : VC.id2vars(ObjId)) {
          const auto *Fun =
              llvm::dyn_cast_or_null<llvm::Function>(Var.valueOrNull());
          if (Fun) {
            connectCallee(Rec.CS, Fun, Rec.Args, Rec.CSRetVal);
          }
        }
      });
    }
  }

  // ---- Result construction --------------------------------------------

  AndersenOTFResult buildResult() {
    const size_t NumVars = VC.size();
    AndersenOTFResult Result;
    Result.NumVars = NumVars;

    // Reverse map: abstract object → set of values pointing to it.
    TypedVector<ValueId, RawAliasSet<ValueId>> Obj2Ptrs(NumVars);
    for (auto VId : iota<ValueId>(NumVars)) {
      if (!Nodes.inbounds(VId)) {
        continue;
      }
      Nodes[VId].PtsSet.foreach ([&](ValueId Obj) {
        if (size_t(Obj) < NumVars) {
          Obj2Ptrs[Obj].insert(VId);
        }
      });
    }

    Result.AliasSets.resize(NumVars);
    for (auto VId : iota<ValueId>(NumVars)) {
      if (!Nodes.inbounds(VId)) {
        continue;
      }
      Nodes[VId].PtsSet.foreach ([&](ValueId Obj) {
        if (size_t(Obj) < NumVars) {
          Result.AliasSets[VId] |= Obj2Ptrs[Obj];
        }
      });
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
