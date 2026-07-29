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
#include "phasar/Pointer/CallingContextConstructor.h"
#include "phasar/Utils/Compressor.h"
#include "phasar/Utils/IotaIterator.h"
#include "phasar/Utils/LibCSummary.h"
#include "phasar/Utils/LibrarySummary.h"
#include "phasar/Utils/Soundness.h"
#include "phasar/Utils/TypedVector.h"
#include "phasar/Utils/UnionFind.h"
#include "phasar/Utils/Utilities.h"
#include "phasar/Utils/ValueCompressor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
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
#include "llvm/Support/GlobPattern.h"

#include <algorithm>
#include <cassert>
#include <memory>
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

/// A PAG node of a context-sensitively analyzed function: the plain node
/// identity plus the calling context it belongs to.
struct ContextualVar {
  AndersenVar Var;
  CallingContextId Ctx{};

  friend bool operator==(ContextualVar A, ContextualVar B) noexcept {
    return A.Var == B.Var && A.Ctx == B.Ctx;
  }

  friend auto hash_value(ContextualVar V) noexcept {
    return llvm::hash_combine(hash_value(V.Var), uint32_t(V.Ctx));
  }
};

/// An allocation-site object, qualified by the context it was allocated in.
struct ObjectKey {
  const llvm::Value *Val = nullptr;
  CallingContextId Ctx{};

  friend bool operator==(ObjectKey A, ObjectKey B) noexcept {
    return A.Val == B.Val && A.Ctx == B.Ctx;
  }

  friend auto hash_value(ObjectKey K) noexcept {
    return llvm::hash_combine(K.Val, uint32_t(K.Ctx));
  }
};

/// Key for FnPtrFieldWrites: an allocation-site object + constant GEP
/// index sequence.
struct FieldWriteKey {
  ObjectKey Obj;
  llvm::SmallVector<uint64_t, 3> Indices;

  friend bool operator==(const FieldWriteKey &A,
                         const FieldWriteKey &B) noexcept {
    return A.Obj == B.Obj && A.Indices == B.Indices;
  }
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

template <> struct DenseMapInfo<ContextualVar> {
  static ContextualVar getEmptyKey() noexcept {
    return {DenseMapInfo<AndersenVar>::getEmptyKey(), {}};
  }
  static ContextualVar getTombstoneKey() noexcept {
    return {DenseMapInfo<AndersenVar>::getTombstoneKey(), {}};
  }
  static unsigned getHashValue(ContextualVar V) noexcept {
    return hash_value(V);
  }
  static bool isEqual(ContextualVar A, ContextualVar B) noexcept {
    return A == B;
  }
};

template <> struct DenseMapInfo<ObjectKey> {
  static ObjectKey getEmptyKey() noexcept {
    return {DenseMapInfo<const Value *>::getEmptyKey(), {}};
  }
  static ObjectKey getTombstoneKey() noexcept {
    return {DenseMapInfo<const Value *>::getTombstoneKey(), {}};
  }
  static unsigned getHashValue(ObjectKey K) noexcept { return hash_value(K); }
  static bool isEqual(ObjectKey A, ObjectKey B) noexcept { return A == B; }
};

template <> struct DenseMapInfo<FieldWriteKey> {
  static FieldWriteKey getEmptyKey() noexcept {
    return {DenseMapInfo<ObjectKey>::getEmptyKey(), {}};
  }
  static FieldWriteKey getTombstoneKey() noexcept {
    return {DenseMapInfo<ObjectKey>::getTombstoneKey(), {}};
  }
  static unsigned getHashValue(const FieldWriteKey &K) noexcept {
    auto H1 = llvm::hash_combine_range(K.Indices.begin(), K.Indices.end());
    return llvm::hash_combine(H1, hash_value(K.Obj));
  }
  static bool isEqual(const FieldWriteKey &A, const FieldWriteKey &B) noexcept {
    return A == B;
  }
};
} // namespace llvm

namespace {
/// The k-limited call-string used as calling context; k is fixed at 1.
using CallCtx = CallingContext<const llvm::CallBase *, 1>;

/// A function analyzed under one particular calling context.
using FuncCtx = std::pair<const llvm::Function *, CallingContextId>;

/// Interning table for the PAG nodes of context-sensitively analyzed
/// functions.  Node ids are *not* allocated here but carved out of the
/// solver's shared ValueId space, so points-to sets, assignment edges and the
/// union-find stay single-typed and never learn about contexts.
///
/// Stays empty -- and costs nothing -- while context-sensitivity is off.
class ContextualNodeTable {
public:
  /// Id of \p CV, allocating a fresh one via \p AllocId on first use.
  ValueId getOrInsert(ContextualVar CV, auto &&AllocId) {
    auto [It, Inserted] = Var2Id.try_emplace(CV, ValueId{});
    if (Inserted) {
      It->second = AllocId();
      recordVar(It->second, CV);
    }
    return It->second;
  }

  /// Registers \p CV as an additional name for the existing node \p Id.
  void addAlias(ContextualVar CV, ValueId Id) {
    if (Var2Id.try_emplace(CV, Id).second) {
      recordVar(Id, CV);
    }
  }

  /// All context-qualified names of node \p Id; empty for plain nodes.
  [[nodiscard]] llvm::ArrayRef<ContextualVar> vars(ValueId Id) const noexcept {
    return Id2Vars.inbounds(Id) ? llvm::ArrayRef<ContextualVar>(Id2Vars[Id])
                                : llvm::ArrayRef<ContextualVar>{};
  }

private:
  void recordVar(ValueId Id, ContextualVar CV) {
    if (!Id2Vars.inbounds(Id)) {
      Id2Vars.resize(size_t(Id) + 1);
    }
    Id2Vars[Id].push_back(CV);
  }

  llvm::DenseMap<ContextualVar, ValueId> Var2Id;
  TypedVector<ValueId, llvm::SmallVector<ContextualVar, 1>> Id2Vars;
};
} // namespace

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
    CallingContextId Ctx; // context of the calling function
  };

  struct VCallRecord {
    const llvm::CallBase *CS;
    ValueId VtablePtrId;
    uint64_t VtableIndex;
    ArgList Args;
    std::optional<ValueId> CSRetVal;
    CallingContextId Ctx;
  };

  struct StructVCallRecord {
    const llvm::CallBase *CS;
    ValueId BaseId; // pts(BaseId) = struct objects
    ValueId FPId;   // pts(FPId) = fn objects (field-insensitive fallback)
    llvm::SmallVector<uint64_t, 3> Indices; // all GEP indices
    llvm::Type *GEPElemTy; // GEP source element type (for type check)
    ArgList Args;
    std::optional<ValueId> CSRetVal;
    CallingContextId Ctx;
  };

  struct QualifyingFieldWriteRecord {
    ValueId PtrId{}; // pts(PtrId) = candidate base objects
    llvm::SmallVector<uint64_t, 3> Indices{}; // meaningful iff Qualifying
    llvm::Type *GEPElemTy = nullptr;          // meaningful iff Qualifying
    const llvm::Function *Callee = nullptr;   // meaningful iff Qualifying
  };

  struct CopyForwardFieldWriteRecord {
    ValueId PtrId{};    // pts(PtrId) = candidate base obj (src)
    ValueId DstPtrId{}; // meaningful iff CopyForward
    std::optional<uint64_t> CopyLength{}; // meaningful iff CopyForward
  };

  // ---- Data fields ----------------------------------------------------

  const LLVMProjectIRDB &IRDB;              // NOLINT
  const llvm::DataLayout &DL;               // NOLINT
  ValueCompressor<PAGVariable> &ExternalVC; // NOLINT – caller-visible output
  ValueCompressor<AndersenVar> LocalVC{};   // internal variable+object nodes
  Soundness SoundnessFlag;
  library_summary::LLVMFunctionDataFlowFacts LibFacts;

  llvm::TargetLibraryInfoWrapperPass TLA{};
  // Per-function MemSSA cache: shared between processFunction() (which needs
  // the MemorySSA of whichever function is currently being translated) and
  // the allocation-wrapper classifier (which needs the MemorySSA of an
  // arbitrary callee at classification time). Building at most once per
  // function avoids redundant dominator-tree/AA construction for functions
  // that are both classified and later processed.
  llvm::DenseMap<const llvm::Function *, std::unique_ptr<MemSSABundle>>
      MemSSACache;
  llvm::MemorySSA *CurrentMemSSA = nullptr;

  llvm::SmallVector<FuncCtx, 8> FunctionWorklist;
  llvm::DenseSet<FuncCtx> Queued; // ever pushed to worklist
  llvm::DenseSet<FuncCtx> Processed;

  UnionFind<ValueId> SCCUf;
  TypedVector<ValueId, NodeInfo> Nodes;

  llvm::SmallVector<FPCallRecord> UnresolvedFPCalls;
  llvm::SmallVector<VCallRecord> UnresolvedVCalls;
  llvm::SmallVector<StructVCallRecord> UnresolvedStructVCalls;

  // Observed fn-ptr field writes for heap/stack dispatch tables.
  struct FieldWriteInfo {
    llvm::SmallVector<const llvm::Function *, 2> Callees;
    llvm::Type *ElemTy = nullptr;
  };
  llvm::DenseMap<FieldWriteKey, FieldWriteInfo> FnPtrFieldWrites;
  // Reverse index: all Indices tracked for a given object, so a memcpy can
  // enumerate "every known field" of its source object (see CopyForward).
  llvm::DenseMap<ObjectKey,
                 llvm::SmallVector<llvm::SmallVector<uint64_t, 3>, 2>>
      FieldsByObject;
  // Objects with an untrusted write; FnPtrFieldWrites is ignored for these.
  llvm::DenseSet<ObjectKey> ImpureObjects;
  llvm::SmallVector<ValueId> UnresolvedPoisenFieldWrites;
  llvm::SmallVector<QualifyingFieldWriteRecord> UnresolvedQualFieldWrites;
  llvm::SmallVector<CopyForwardFieldWriteRecord> UnresolvedCopyFieldWrites;
  // Per (call-site, caller-context): the (callee node, callee context) pairs
  // already wired up.  One call site reached from several contexts must bind
  // its actuals once per context, hence the context in both key and value.
  llvm::DenseMap<std::pair<const llvm::CallBase *, CallingContextId>,
                 llvm::SmallDenseSet<uint64_t, 4>>
      ConnectedCallees;
  CallGraphBuilder<const llvm::Instruction *, const llvm::Function *> CGBuilder;
  llvm::SmallVector<ValueId, 64> PropWorklist;

  // ---- Context-sensitivity --------------------------------------------

  ContextSensitivityOptions CSOpts;
  llvm::SmallVector<llvm::GlobPattern, 0> AllowPatterns;
  llvm::SmallVector<llvm::GlobPattern, 0> DenyPatterns;
  Compressor<CallCtx, CallingContextId> Contexts;
  ContextualNodeTable CtxNodes;
  llvm::DenseMap<const llvm::Function *, bool> SelectedCache;
  llvm::DenseMap<const llvm::Function *, unsigned> CallSiteCounts;
  // Contexts already instantiated per selected function; see calleeContext().
  llvm::DenseMap<const llvm::Function *,
                 llvm::SmallDenseSet<CallingContextId, 4>>
      ContextsPerFn;
  size_t NumContextualNodes = 0;
  bool BudgetExhausted = false;
  // The function/context currently being translated by processFunction().
  const llvm::Function *CurFunc = nullptr;
  CallingContextId CurCtx = CallingContextId::None;

  // ---- Constructor ----------------------------------------------------

  SolverData(const LLVMProjectIRDB &IRDB,
             llvm::ArrayRef<const llvm::Function *> Entries,
             ValueCompressor<PAGVariable> &VC, Soundness S,
             ContextSensitivityOptions CSOpts)
      : IRDB(IRDB), DL(IRDB.getModule()->getDataLayout()), ExternalVC(VC),
        SoundnessFlag(S),
        LibFacts(library_summary::readFromFDFF(
            getLibCSummary(),
            [&IRDB](llvm::StringRef Name) { return IRDB.getFunction(Name); })),
        CSOpts(std::move(CSOpts)) {

    // Id 0 == CallingContextId::None is the root (context-insensitive) context.
    std::ignore = Contexts.getOrInsert(CallCtx{});
    AllowPatterns = compileGlobs(this->CSOpts.AllowList);
    DenyPatterns = compileGlobs(this->CSOpts.DenyList);

    CGBuilder.reserve(IRDB.getNumFunctions());
    for (const auto *F : Entries) {
      if (Queued.insert({F, CallingContextId::None}).second) {
        FunctionWorklist.emplace_back(F, CallingContextId::None);

        // entry functions may be missed in the CG, if they are never called
        // explicitly in the code
        std::ignore = CGBuilder.addFunctionVertex(F);
      }
      // Entry-function args have no caller to propagate pts through.
      // Create an abstract object for each pointer arg so that loads through
      // them produce non-empty pts sets and aliases are reported correctly.
      for (const auto &Arg : F->args()) {
        if (definitelyContainsNoPointer(&Arg)) {
          continue;
        }
        const ValueId VarId = getOrInsertVar(PAGVariable(&Arg));
        const ValueId ObjId = getOrInsertObj(PAGVariable(&Arg));
        addPointee(VarId, ObjId);
      }
    }
  }

  static llvm::SmallVector<llvm::GlobPattern, 0>
  compileGlobs(llvm::ArrayRef<std::string> Patterns) {
    llvm::SmallVector<llvm::GlobPattern, 0> Ret;
    Ret.reserve(Patterns.size());
    for (const auto &Pat : Patterns) {
      if (auto Glob = llvm::GlobPattern::create(Pat)) {
        Ret.push_back(std::move(*Glob));
      } else {
        llvm::consumeError(Glob.takeError());
      }
    }
    return Ret;
  }

  // ---- Node growth ----------------------------------------------------

  void grow(ValueId V) {
    const auto Idx = size_t(V);
    if (Idx >= Nodes.size()) {
      Nodes.resize(Idx + 1);
      SCCUf.grow(Idx + 1);
    }
  }

  // The context a node of \p Var belongs to while translating CurFunc.
  // Globals, constants and values of other functions always stay in the root
  // context.
  [[nodiscard]] CallingContextId contextOf(PAGVariable Var) const noexcept {
    if (CurCtx == CallingContextId::None) {
      return CallingContextId::None;
    }
    return Var.getFunction() == CurFunc ? CurCtx : CallingContextId::None;
  }

  ValueId getOrInsertNode(AndersenVar Var, CallingContextId Ctx) {
    const ValueId Id =
        Ctx == CallingContextId::None
            ? LocalVC.insert(Var).first
            : CtxNodes.getOrInsert({.Var = Var, .Ctx = Ctx}, [this] {
                ++NumContextualNodes;
                return LocalVC.addDummy();
              });
    grow(Id);
    return Id;
  }

  ValueId getOrInsertVar(PAGVariable Var) {
    return getOrInsertNode(AndersenVar{Var, false}, contextOf(Var));
  }
  ValueId getOrInsertVar(PAGVariable Var, CallingContextId Ctx) {
    return getOrInsertNode(AndersenVar{Var, false}, Ctx);
  }

  ValueId getOrInsertObj(PAGVariable Var) {
    return getOrInsertNode(AndersenVar{Var, true}, contextOf(Var));
  }
  ValueId getOrInsertObj(PAGVariable Var, CallingContextId Ctx) {
    return getOrInsertNode(AndersenVar{Var, true}, Ctx);
  }

  // Visits every (base var, context) behind node \p Id.  A single id can carry
  // both plain and context-qualified names, since addAlias() merges GEP/cast
  // results into their base pointer's node.
  void forEachVar(ValueId Id, std::invocable<ContextualVar> auto Fn) const {
    for (const auto &Var : LocalVC.id2vars(Id)) {
      std::invoke(Fn, ContextualVar{.Var = Var, .Ctx = {}});
    }
    for (const auto &CVar : CtxNodes.vars(Id)) {
      std::invoke(Fn, CVar);
    }
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

  MemSSABundle &getOrCreateMemSSA(const llvm::Function *F) {
    auto &Bundle = MemSSACache[F];
    if (!Bundle) {
      Bundle = std::make_unique<MemSSABundle>(const_cast<llvm::Function &>(*F),
                                              &TLA.getTLI(*F));
    }
    return *Bundle;
  }

  // Translates F's body once per calling context: nodes created here are
  // qualified by Ctx (see contextOf).
  void processFunction(const llvm::Function *F, CallingContextId Ctx) {
    CurrentMemSSA = &getOrCreateMemSSA(F).MSSA;
    CurFunc = F;
    CurCtx = Ctx;
    for (const auto &Arg : F->args()) {
      if (!definitelyContainsNoPointer(&Arg)) {
        (void)getOrInsertVar(PAGVariable(&Arg));
      }
    }
    for (const auto &I : llvm::instructions(F)) {
      processInstruction(I);
    }
    CurFunc = nullptr;
    CurCtx = CallingContextId::None;
  }

  void addPtrAlias(const llvm::Value *V, const llvm::Value *Src) {
    const AndersenVar Var{PAGVariable(V), false};
    // The context is loop-invariant, so branch on it before iterating.
    if (const auto Ctx = contextOf(PAGVariable(V));
        Ctx != CallingContextId::None) {
      forEachOpId(Src, [&](ValueId OpId) {
        CtxNodes.addAlias({.Var = Var, .Ctx = Ctx}, OpId);
        grow(OpId);
      });
      return;
    }
    forEachOpId(Src, [&](ValueId OpId) {
      LocalVC.addAlias(Var, OpId);
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
    recordFieldWrite(S);
    forEachOpId(S->getPointerOperand(), [&](ValueId PtrId) {
      forEachOpId(S->getValueOperand(),
                  [&](ValueId ValId) { addStore(PtrId, ValId); });
    });
  }

  // Populates FnPtrFieldWrites for a `*GEP(base, const-indices) = Function`
  // write (qualifying); any other pointer store poisons every object it may
  // target (disqualifying), since it could be clobbering a tracked field
  // through a shape resolveStructVCall can't statically verify.
  void recordFieldWrite(const llvm::StoreInst *S) {
    if (auto Info = getConstGEPFieldAccess(S->getPointerOperand())) {
      auto &[BasePtr, Indices, GEPElemTy] = *Info;
      if (const auto *StoredFn = llvm::dyn_cast<llvm::Function>(
              S->getValueOperand()->stripPointerCastsAndAliases())) {
        const ValueId BaseId = getOrInsertVar(PAGVariable(BasePtr));
        QualifyingFieldWriteRecord Rec{
            .PtrId = BaseId,
            .Indices = std::move(Indices),
            .GEPElemTy = GEPElemTy,
            .Callee = StoredFn,
        };
        resolveFieldWrite(Rec);
        UnresolvedQualFieldWrites.push_back(std::move(Rec));
        return;
      }
    }
    forEachOpId(S->getPointerOperand(), [&](ValueId PtrId) {
      resolveFieldWrite(PtrId);
      UnresolvedPoisenFieldWrites.push_back(PtrId);
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
    // Bypasses handleStore/recordFieldWrite: may propagate the source
    // object's known field writes to the destination object, or poison it
    // if that can't be proven safe (see resolveFieldWrite, CopyForward).
    std::optional<uint64_t> CopyLength;
    if (const auto *Len = llvm::dyn_cast<llvm::ConstantInt>(M->getLength())) {
      CopyLength = Len->getZExtValue();
    }
    forEachOpId(M->getDest(), [&](ValueId DstPtr) {
      forEachOpId(M->getSource(), [&](ValueId SrcPtr) {
        addMemCopy(SrcPtr, DstPtr);
        CopyForwardFieldWriteRecord Rec{
            .PtrId = SrcPtr,
            .DstPtrId = DstPtr,
            .CopyLength = CopyLength,
        };
        resolveCopyForward(Rec);
        UnresolvedCopyFieldWrites.push_back(Rec);
      });
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

  // ---- Allocation-wrapper classification -------------------------------
  //
  // Recognizes functions whose returned pointer, on every return path,
  // provably traces back to a fresh heap allocation (directly, or through
  // another such wrapper).  Calls to a classified wrapper are then treated
  // like direct calls to malloc(): each call SITE gets its own fresh
  // abstract object instead of merging through the wrapper's single,
  // context-insensitively-shared internal allocation site.

  enum class WrapperState : uint8_t { InProgress, IsWrapper, NotWrapper };
  llvm::DenseMap<const llvm::Function *, WrapperState> WrapperCache;

  // Does V have any use that is not part of a "return-preserving" chain
  // (pointer casts, PHI merges, a local scratch-alloca spill/reload, or the
  // standard null-check idiom) and the eventual ReturnInst itself?  Called
  // on the freshly allocated pointer (or a call to another classified
  // wrapper); if it returns true, giving each call SITE its own fresh
  // object would silently disconnect that escaping use from the object it
  // actually observes/mutates.
  bool hasEscapingUse(const llvm::Value *V,
                      llvm::SmallPtrSetImpl<const llvm::Value *> &Visited) {
    if (!Visited.insert(V).second) {
      return false;
    }
    for (const llvm::Use &U : V->uses()) {
      const auto *Usr = U.getUser();
      if (llvm::isa<llvm::ReturnInst>(Usr)) {
        continue;
      }
      if (const auto *Cmp = llvm::dyn_cast<llvm::ICmpInst>(Usr)) {
        // Allow the standard OOM null-check idiom: compare against a null
        // pointer constant only.
        const unsigned OtherIdx = U.getOperandNo() == 0 ? 1 : 0;
        if (llvm::isa<llvm::ConstantPointerNull>(Cmp->getOperand(OtherIdx))) {
          continue;
        }
        return true;
      }
      if (const auto *II = llvm::dyn_cast<llvm::IntrinsicInst>(Usr)) {
        if (llvm::isLifetimeIntrinsic(II->getIntrinsicID()) ||
            llvm::isa<llvm::DbgInfoIntrinsic>(II)) {
          continue;
        }
        return true;
      }
      if (llvm::isa<llvm::CastInst>(Usr) || llvm::isa<llvm::PHINode>(Usr)) {
        if (hasEscapingUse(Usr, Visited)) {
          return true;
        }
        continue;
      }
      if (const auto *St = llvm::dyn_cast<llvm::StoreInst>(Usr)) {
        if (St->getPointerOperand() == V) {
          // Something is written INTO V: only benign if V is itself a
          // local scratch alloca (writes to it can't leak the allocated
          // object's identity elsewhere).
          if (llvm::isa<llvm::AllocaInst>(V)) {
            continue;
          }
          return true;
        }
        // V is the stored value: only benign if spilled to a local scratch
        // alloca, and only once we also check everything later reloaded
        // from it (else a reload-then-leak before the final return, e.g.
        // passing the reloaded pointer to another function, would go
        // unnoticed).
        if (llvm::isa<llvm::AllocaInst>(St->getPointerOperand())) {
          if (hasEscapingUse(St->getPointerOperand(), Visited)) {
            return true;
          }
          continue;
        }
        return true;
      }
      if (llvm::isa<llvm::LoadInst>(Usr)) {
        // Only reached when V is a local scratch alloca (see above); the
        // loaded value must stay within the same safe-use closure.
        if (hasEscapingUse(Usr, Visited)) {
          return true;
        }
        continue;
      }
      return true; // call argument, GEP, or any other unrecognized use
    }
    return false;
  }

  // Does V, after stripping pointer casts, provably denote a freshly
  // allocated object (a direct call to a heap allocator or to another
  // classified wrapper, possibly reached through a load with exactly one
  // reaching store, or a PHI merge of such values) whose identity is not
  // observed or mutated anywhere except along the path to the return?
  // MSSA must be the MemorySSA of the function containing V.  Visited
  // guards against self-/mutually-referencing PHIs and load/store cycles
  // introduced by loops (e.g. a loop-carried pointer that is unchanged
  // around the back edge produces a self-referencing PHI); a revisit
  // conservatively means "not provably fresh" rather than recursing
  // forever.
  bool traceIsFreshAlloc(const llvm::Value *V, llvm::MemorySSA &MSSA,
                         llvm::SmallPtrSetImpl<const llvm::Value *> &Visited) {
    V = V->stripPointerCasts();
    if (!Visited.insert(V).second) {
      return false;
    }
    if (const auto *CB = llvm::dyn_cast<llvm::CallBase>(V)) {
      const auto *Callee = llvm::dyn_cast_or_null<llvm::Function>(
          CB->getCalledOperand()->stripPointerCastsAndAliases());
      if (!Callee ||
          !(psr::isHeapAllocatingFunction(Callee) || isAllocWrapper(Callee))) {
        return false;
      }
      llvm::SmallPtrSet<const llvm::Value *, 8> EscVisited;
      return !hasEscapingUse(CB, EscVisited);
    }
    if (const auto *L = llvm::dyn_cast<llvm::LoadInst>(V)) {
      llvm::SmallPtrSet<const llvm::StoreInst *, 4> Defs;
      const bool HasLiveOnEntry = collectReachingDefs(L, MSSA, Defs);
      if (HasLiveOnEntry || Defs.size() != 1) {
        // Ambiguous (multiple reaching stores), or the value may come from
        // outside the function (parameter/global-backed memory): not
        // provably fresh.
        return false;
      }
      return traceIsFreshAlloc((*Defs.begin())->getValueOperand(), MSSA,
                               Visited);
    }
    if (const auto *P = llvm::dyn_cast<llvm::PHINode>(V)) {
      return llvm::all_of(P->incoming_values(), [&](const llvm::Use &Op) {
        return traceIsFreshAlloc(Op.get(), MSSA, Visited);
      });
    }
    return false; // argument, global, GEP, unresolved call, ...
  }

  bool computeIsAllocWrapper(const llvm::Function *F) {
    if (definitelyContainsNoPointer(F->getReturnType())) {
      return false;
    }
    llvm::MemorySSA &MSSA = getOrCreateMemSSA(F).MSSA;
    bool SawQualifyingReturn = false;
    for (const auto &BB : *F) {
      // ReturnInst is always a terminator; only check block terminators
      // instead of scanning every instruction.
      const auto *R = llvm::dyn_cast<llvm::ReturnInst>(BB.getTerminator());
      if (!R) {
        continue;
      }
      const auto *RetVal = R->getReturnValue();
      llvm::SmallPtrSet<const llvm::Value *, 8> Visited;
      if (!RetVal || definitelyContainsNoPointer(RetVal) ||
          !traceIsFreshAlloc(RetVal, MSSA, Visited)) {
        return false;
      }
      SawQualifyingReturn = true;
    }
    return SawQualifyingReturn;
  }

  bool isAllocWrapper(const llvm::Function *F) {
    if (F->isDeclaration()) {
      return false; // real allocators are handled via isHeapAllocatingFunction
    }
    auto [It, Inserted] = WrapperCache.try_emplace(F, WrapperState::InProgress);
    if (!Inserted) {
      // InProgress means F is on the current recursion stack (a cycle):
      // conservatively not a wrapper.
      return It->second == WrapperState::IsWrapper;
    }
    const bool Result = computeIsAllocWrapper(F);
    WrapperCache[F] =
        Result ? WrapperState::IsWrapper : WrapperState::NotWrapper;
    return Result;
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
          forEachVar(ObjId, [&](ContextualVar CVar) {
            const auto *Fun = llvm::dyn_cast_or_null<llvm::Function>(
                CVar.Var.getBase().valueOrNull());
            // Callbacks have no known caller, so they run in the root context.
            if (Fun && !Fun->isDeclaration() &&
                Queued.insert({Fun, CallingContextId::None}).second) {
              FunctionWorklist.emplace_back(Fun, CallingContextId::None);
              std::ignore = CGBuilder.addFunctionVertex(Fun);
            }
          });
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

  // ---- Selection ------------------------------------------------------

  static bool matchesAny(llvm::ArrayRef<llvm::GlobPattern> Patterns,
                         llvm::StringRef Name) {
    return llvm::any_of(Patterns,
                        [&](const auto &Glob) { return Glob.match(Name); });
  }

  // Sticky once tripped, so selection can't oscillate mid-solve.
  bool budgetExhausted() {
    BudgetExhausted =
        BudgetExhausted || NumContextualNodes >= CSOpts.MaxContextualNodes;
    return BudgetExhausted;
  }

  // Whether Fun's PAG nodes are cloned per calling context.  Decided on first
  // query and cached: selection must never change once nodes have been wired,
  // since this solver has no way to retract an edge or shrink a pts-set.
  [[nodiscard]] bool isSelected(const llvm::Function *Fun) {
    if (CSOpts.isOff() || Fun->isDeclaration()) {
      return false;
    }
    auto [It, Inserted] = SelectedCache.try_emplace(Fun, false);
    if (Inserted) {
      It->second = computeIsSelected(Fun);
    }
    return It->second;
  }

  bool computeIsSelected(const llvm::Function *Fun) {
    const llvm::StringRef Name = Fun->getName();
    if (matchesAny(DenyPatterns, Name)) {
      return false;
    }
    if (matchesAny(AllowPatterns, Name)) {
      return true;
    }
    switch (CSOpts.SelectionMode) {
    case ContextSensitivityOptions::Mode::All:
      return !budgetExhausted();
    case ContextSensitivityOptions::Mode::Dynamic: {
      // Cheapest tests first; only paramsEscapeOrDispatch scans the body.
      if (budgetExhausted() || !hasMultipleCallSites(Fun)) {
        return false;
      }
      const size_t Size = Fun->getInstructionCount();
      if (Size > CSOpts.MaxContextualFunctionSize) {
        return false;
      }
      if (paramsEscapeOrDispatch(Fun)) {
        return true;
      }
      // Nothing a caller passed in ever leaves again, so merging the callers
      // can only make the formals spuriously alias *within* the body. That
      // is real but far weaker and far more common, so only pay for it where a
      // clone is nearly free.
      return Size <= CSOpts.MaxLocalMergeFunctionSize &&
             hasMultiplePointerParams(Fun);
    }
    default:
      return false;
    }
  }

  // Once every caller is merged into one set of formals, several pointer
  // parameters become mutually may-alias inside the body even though no
  // single caller ever passed aliasing arguments -- the spec-mesa end(p, q)
  // case, which nothing escapes, so paramsEscapeOrDispatch misses it.
  [[nodiscard]] static bool
  hasMultiplePointerParams(const llvm::Function *Fun) {
    unsigned NumPtrParams = 0;
    for (const auto &Param : Fun->args()) {
      if (!definitelyContainsNoPointer(&Param) && ++NumPtrParams == 2) {
        return true;
      }
    }
    return false;
  }

  // Cheap over-approximation: several direct call sites, or address-taken.
  [[nodiscard]] bool hasMultipleCallSites(const llvm::Function *Fun) {
    auto [It, Inserted] = CallSiteCounts.try_emplace(Fun, 0);
    if (Inserted) {
      for (const auto *User : Fun->users()) {
        const auto *CS = llvm::dyn_cast<llvm::CallBase>(User);
        if (!CS || CS->getCalledOperand() != Fun) {
          It->second = 2; // address-taken: reachable from unknown call sites
          break;
        }
        if (++It->second >= 2) {
          break;
        }
      }
    }
    return It->second >= 2;
  }

  // Scratch state for one function's worth of isParamDerived queries.
  // OnPath breaks cycles within a single query; NotDerived memoizes completed
  // negative answers across queries, which is what keeps the whole scan
  // linear in the function size.
  struct ParamDerivedCache {
    llvm::SmallPtrSet<const llvm::Value *, 16> OnPath;
    llvm::SmallPtrSet<const llvm::Value *, 32> NotDerived;
  };

  // Whether Val is derived from one of Fun's parameters.  Walks def-use
  // chains backwards; on reaching a local alloca it continues through the
  // values stored into it, which covers parameters spilled to the stack.
  //
  // May under-approximate through loop-carried cycles (a negative answer
  // reached via a cut back-edge is still memoized).  This only gates
  // selection, so a missed one costs precision, never soundness.
  bool isParamDerived(const llvm::Value *Val, const llvm::Function *Fun,
                      ParamDerivedCache &Cache) {
    Val = Val->stripPointerCastsAndAliases();
    if (Cache.NotDerived.contains(Val) || !Cache.OnPath.insert(Val).second) {
      return false;
    }
    const bool Derived = computeIsParamDerived(Val, Fun, Cache);
    Cache.OnPath.erase(Val);
    if (!Derived) {
      Cache.NotDerived.insert(Val);
    }
    return Derived;
  }

  bool computeIsParamDerived(const llvm::Value *Val, const llvm::Function *Fun,
                             ParamDerivedCache &Cache) {
    if (const auto *Arg = llvm::dyn_cast<llvm::Argument>(Val)) {
      return Arg->getParent() == Fun;
    }
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(Val)) {
      return llvm::any_of(Alloca->users(), [&](const llvm::User *User) {
        const auto *Store = llvm::dyn_cast<llvm::StoreInst>(User);
        return Store && Store->getPointerOperand() == Alloca &&
               isParamDerived(Store->getValueOperand(), Fun, Cache);
      });
    }
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Val)) {
      return isParamDerived(Load->getPointerOperand(), Fun, Cache);
    }
    if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Val)) {
      return isParamDerived(GEP->getPointerOperand(), Fun, Cache);
    }
    if (llvm::isa<llvm::PHINode, llvm::SelectInst>(Val)) {
      const auto *Inst = llvm::cast<llvm::Instruction>(Val);
      return llvm::any_of(Inst->operand_values(), [&](const llvm::Value *Op) {
        return !Op->getType()->isVoidTy() && isParamDerived(Op, Fun, Cache);
      });
    }
    return false;
  }

  // The generalizing signal: does anything a caller passed in *leave* Fun
  // again?  If not, merging Fun's callers cannot pollute anything outside its
  // body.  These are the syntactic counterparts of the precision-loss
  // patterns Zipper identifies (Li, Tan, Xue, OOPSLA 2018):
  //
  //   1. a param-derived value is returned      -> pollutes callers
  //   2. it is stored somewhere callers can see -> pollutes the heap
  //   3. it is the callee of an indirect call   -> pollutes the call graph
  //
  // One pass with one shared cache, so the whole test is linear in |Fun|.
  //
  // Deciding this syntactically, before anything is wired, is what makes the
  // choice usable: observing the same imprecision from the solver's own state
  // would come too late, because the merged facts have already propagated out
  // through the shared formals and cannot be taken back.
  bool paramsEscapeOrDispatch(const llvm::Function *Fun) {
    ParamDerivedCache Cache;
    for (const auto &Inst : llvm::instructions(Fun)) {
      if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(&Inst)) {
        const auto *Val = Ret->getReturnValue();
        if (Val && !definitelyContainsNoPointer(Val) &&
            isParamDerived(Val, Fun, Cache)) {
          return true;
        }
        continue;
      }
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(&Inst)) {
        const auto *Val = Store->getValueOperand();
        if (definitelyContainsNoPointer(Val) ||
            !isParamDerived(Val, Fun, Cache)) {
          continue;
        }
        // A store into a local alloca stays inside Fun; only stores through a
        // global or a param-derived pointer are visible to the caller.
        const auto *Ptr =
            Store->getPointerOperand()->stripPointerCastsAndAliases();
        if (llvm::isa<llvm::GlobalValue>(Ptr) ||
            isParamDerived(Ptr, Fun, Cache)) {
          return true;
        }
        continue;
      }
      const auto *CS = llvm::dyn_cast<llvm::CallBase>(&Inst);
      if (!CS || CS->isInlineAsm() || CS->isDebugOrPseudoInst()) {
        continue;
      }
      const auto *Callee =
          CS->getCalledOperand()->stripPointerCastsAndAliases();
      if (!llvm::isa<llvm::Function>(Callee) &&
          isParamDerived(Callee, Fun, Cache)) {
        return true;
      }
    }
    return false;
  }

  // Pushes CS onto CallerCtx; with k = 1 the result depends on CS alone.
  CallingContextId pushContext(CallingContextId CallerCtx,
                               const llvm::CallBase *CS) {
    return Contexts.getOrInsert(Contexts[CallerCtx].withPrefix(CS));
  }

  // The context the body of Callee runs in for this call.  Once Callee has
  // been cloned MaxContextsPerFunction times, further call sites fall back to
  // the shared root context -- sound, just as imprecise as before, and it
  // keeps one heavily-called function from consuming the whole node budget.
  CallingContextId calleeContext(const llvm::Function *Callee,
                                 CallingContextId CallerCtx,
                                 const llvm::CallBase *CS) {
    if (!isSelected(Callee)) {
      return CallingContextId::None;
    }
    const CallingContextId Ctx = pushContext(CallerCtx, CS);
    auto &Seen = ContextsPerFn[Callee];
    if (Seen.contains(Ctx)) {
      return Ctx;
    }
    if (Seen.size() >= CSOpts.MaxContextsPerFunction) {
      return CallingContextId::None;
    }
    Seen.insert(Ctx);
    return Ctx;
  }

  bool connectCallee(const llvm::CallBase *CS, const llvm::Function *Callee,
                     llvm::ArrayRef<llvm::SmallVector<ValueId, 2>> Args,
                     std::optional<ValueId> CSRetVal,
                     CallingContextId CallerCtx) {
    const ValueId CalleeId =
        getOrInsertVar(PAGVariable(Callee), CallingContextId::None);
    const CallingContextId CalleeCtx = calleeContext(Callee, CallerCtx, CS);
    const uint64_t Connection =
        uint64_t(uint32_t(CalleeId)) << 32 | uint32_t(CalleeCtx);
    if (!ConnectedCallees[{CS, CallerCtx}].insert(Connection).second) {
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

    if (Queued.insert({Callee, CalleeCtx}).second) {
      FunctionWorklist.emplace_back(Callee, CalleeCtx);
    }

    if (CSRetVal && !Callee->getReturnType()->isVoidTy()) {
      if (isAllocWrapper(Callee)) {
        // Give this call SITE its own fresh object instead of merging
        // through Callee's shared internal allocation site, which would
        // spuriously alias every call to this wrapper.
        const ValueId ObjId = getOrInsertObj(PAGVariable(CS), CallerCtx);
        addPointee(*CSRetVal, ObjId);
      } else {
        const ValueId RetSlotId =
            getOrInsertVar(PAGVariable::Return{Callee}, CalleeCtx);
        addAssignEdge(RetSlotId, *CSRetVal);
      }
    }

    for (const auto &[Param, ArgIds] : llvm::zip(Callee->args(), Args)) {
      if (ArgIds.empty() || definitelyContainsNoPointer(&Param)) {
        continue;
      }
      const ValueId ParamId = getOrInsertVar(PAGVariable(&Param), CalleeCtx);
      for (ValueId ArgId : ArgIds) {
        addAssignEdge(ArgId, ParamId);
      }
    }

    propagate();
    return true;
  }

  bool resolveVtableCall(const llvm::CallBase *CS, ValueId VtablePtrId,
                         uint64_t VtableIndex, const ArgList &Args,
                         std::optional<ValueId> CSRetVal,
                         CallingContextId CallerCtx) {
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
      forEachVar(ObjId, [&](ContextualVar CVar) {
        const auto *GV = llvm::dyn_cast_or_null<llvm::GlobalVariable>(
            CVar.Var.getBase().valueOrNull());
        if (!GV || !GV->hasName() ||
            !GV->getName().starts_with(DIBasedTypeHierarchy::VTablePrefix) ||
            !GV->hasInitializer()) {
          return;
        }
        const auto *VTStruct =
            llvm::dyn_cast<llvm::ConstantStruct>(GV->getInitializer());
        if (!VTStruct) {
          return;
        }
        auto VFs = LLVMVFTable::getVFVectorFromIRVTable(*VTStruct);
        if (VtableIndex >= VFs.size()) {
          return;
        }
        const auto *Callee = VFs[VtableIndex];
        if (!Callee || !isConsistentCall(CS, Callee)) {
          return;
        }
        NewEdge |= connectCallee(CS, Callee, Args, CSRetVal, CallerCtx);
      });
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
      forEachVar(ObjId, [&](ContextualVar CVar) {
        // Resolve GlobalAlias to the underlying GlobalVariable.
        const llvm::Value *Val = CVar.Var.getBase().valueOrNull();
        if (const auto *GA = llvm::dyn_cast_or_null<llvm::GlobalAlias>(Val)) {
          Val = GA->getAliaseeObject();
        }
        const auto *GV = llvm::dyn_cast_or_null<llvm::GlobalVariable>(Val);
        if (!GV || !GV->isConstant() || !GV->hasInitializer()) {
          // Not a usable const global: try the dynamically observed
          // field-write table for heap/stack dispatch-table objects.
          const ObjectKey Obj{.Val = Val, .Ctx = CVar.Ctx};
          if (Val && !ImpureObjects.contains(Obj)) {
            auto It = FnPtrFieldWrites.find(
                FieldWriteKey{.Obj = Obj, .Indices = Rec.Indices});
            if (It != FnPtrFieldWrites.end() &&
                It->second.ElemTy == Rec.GEPElemTy) {
              for (const auto *Callee : It->second.Callees) {
                if (isConsistentCall(Rec.CS, Callee)) {
                  NewEdge |= connectCallee(Rec.CS, Callee, Rec.Args,
                                           Rec.CSRetVal, Rec.Ctx);
                }
              }
              return;
            }
          }
          NeedFPFallback = true;
          return;
        }
        // Type check: GV must be of GEPElemTy or [N x GEPElemTy].
        // Field-insensitive aliasing can put wrong-type objects in pts.
        llvm::Type *const GVTy = GV->getValueType();
        if (GVTy != Rec.GEPElemTy) {
          const auto *ArrTy = llvm::dyn_cast<llvm::ArrayType>(GVTy);
          if (!ArrTy || ArrTy->getElementType() != Rec.GEPElemTy) {
            NeedFPFallback = true;
            return;
          }
        }
        const auto *Callee =
            walkConstInitPath(GV->getInitializer(), Rec.Indices);
        if (!Callee || !isConsistentCall(Rec.CS, Callee)) {
          return;
        }
        NewEdge |=
            connectCallee(Rec.CS, Callee, Rec.Args, Rec.CSRetVal, Rec.Ctx);
      });
      return true;
    });
    if (NeedFPFallback) {
      NewEdge |=
          resolveFPCall(Rec.CS, Rec.FPId, Rec.Args, Rec.CSRetVal, Rec.Ctx);
    }
    return NewEdge;
  }

  [[nodiscard]] bool poisonObject(ObjectKey AllocObj) {
    return ImpureObjects.insert(AllocObj).second;
  }

  // Merges one field's known writes (Src, e.g. a single-callee write, or an
  // entry copied from another object) into AllocVal's own entry for
  // Indices. A differently-typed pre-existing entry for the same slot means
  // type punning: poison the whole object instead of trusting either write.
  bool mergeFieldWriteInfo(ObjectKey AllocObj,
                           const llvm::SmallVector<uint64_t, 3> &Indices,
                           const FieldWriteInfo &Src) {
    if (ImpureObjects.contains(AllocObj)) {
      return false;
    }
    auto [It, Inserted] = FnPtrFieldWrites.try_emplace(
        FieldWriteKey{.Obj = AllocObj, .Indices = Indices});
    if (Inserted) {
      It->second.ElemTy = Src.ElemTy;
      FieldsByObject[AllocObj].push_back(Indices);
    } else if (It->second.ElemTy != Src.ElemTy) {
      return poisonObject(AllocObj);
    }
    bool Changed = Inserted;
    for (const auto *Callee : Src.Callees) {
      if (!llvm::is_contained(It->second.Callees, Callee)) {
        It->second.Callees.push_back(Callee);
        Changed = true;
      }
    }
    return Changed;
  }

  // Propagates a memcpy's source object's known field writes onto its
  // destination object(s), when provably safe: the source object isn't
  // impure, the copy length is a known constant covering every copied
  // field, and the destination doesn't already have unrelated entries that
  // the memcpy could silently overwrite with unverified bytes. Otherwise
  // poisons the destination, exactly like any other unverifiable write.
  bool resolveCopyForward(const CopyForwardFieldWriteRecord &Rec) {
    const ValueId SrcPtrId = rep(Rec.PtrId);
    const ValueId DstPtrId = rep(Rec.DstPtrId);
    if (!Nodes.inbounds(SrcPtrId) || !Nodes.inbounds(DstPtrId)) {
      return false;
    }
    bool Changed = false;
    const auto &SrcPts = Nodes[SrcPtrId].PtsSet;
    const auto &DstPts = Nodes[DstPtrId].PtsSet;
    DstPts.foreach ([&](ValueId DstObjId) {
      if (!Nodes.inbounds(DstObjId)) {
        return false;
      }
      forEachVar(DstObjId, [&](ContextualVar DstVar) {
        const llvm::Value *DstVal = DstVar.Var.getBase().valueOrNull();
        const ObjectKey DstObj{.Val = DstVal, .Ctx = DstVar.Ctx};
        if (!DstVal || ImpureObjects.contains(DstObj)) {
          return;
        }
        const auto DstFieldsIt = FieldsByObject.find(DstObj);
        const bool DstHasEntries =
            DstFieldsIt != FieldsByObject.end() && !DstFieldsIt->second.empty();
        bool Poison = !Rec.CopyLength;
        // Copy FieldWriteInfo by value: mergeFieldWriteInfo() below mutates
        // FnPtrFieldWrites (possibly rehashing it), so pointers/references
        // into that map can't be held across the merge loop.
        llvm::SmallVector<
            std::pair<llvm::SmallVector<uint64_t, 3>, FieldWriteInfo>, 4>
            ToMerge;
        if (!Poison) {
          SrcPts.foreach ([&](ValueId SrcObjId) {
            if (!Nodes.inbounds(SrcObjId)) {
              return false;
            }
            forEachVar(SrcObjId, [&](ContextualVar SrcVar) {
              const llvm::Value *SrcVal = SrcVar.Var.getBase().valueOrNull();
              if (!SrcVal) {
                return;
              }
              const ObjectKey SrcObj{.Val = SrcVal, .Ctx = SrcVar.Ctx};
              if (ImpureObjects.contains(SrcObj)) {
                Poison = true;
                return;
              }
              if (SrcObj != DstObj && DstHasEntries) {
                // A genuinely external source could clobber DstVal's own
                // separately-tracked fields with bytes we know nothing
                // about. A self-copy (field-insensitively-aliased src/dst,
                // as with a same-struct `ctx->A = ctx->B` pattern) is safe:
                // merging an object's own known fields into itself is a
                // no-op.
                Poison = true;
                return;
              }
              const auto SrcFieldsIt = FieldsByObject.find(SrcObj);
              if (SrcFieldsIt == FieldsByObject.end()) {
                return;
              }
              for (const auto &Indices : SrcFieldsIt->second) {
                const auto FWIt = FnPtrFieldWrites.find(
                    FieldWriteKey{.Obj = SrcObj, .Indices = Indices});
                assert(FWIt != FnPtrFieldWrites.end());
                if (*Rec.CopyLength <
                    DL.getTypeAllocSize(FWIt->second.ElemTy).getFixedValue()) {
                  Poison = true;
                  continue;
                }
                ToMerge.emplace_back(Indices, FWIt->second);
              }
            });
            return true;
          });
        }
        if (Poison) {
          Changed |= poisonObject(DstObj);
          return;
        }
        for (const auto &[Indices, Info] : ToMerge) {
          Changed |= mergeFieldWriteInfo(DstObj, Indices, Info);
        }
      });
      return true;
    });
    return Changed;
  }

  // Updates FnPtrFieldWrites/ImpureObjects for every object in pts(PtrId).
  // Returns whether anything changed (grew), so callers can drive the
  // outer fixpoint like the other checkUnresolvedX functions do. Unlike
  // resolveStructVCall/resolveFPCall, no snapshot is needed: these loop
  // bodies never call connectCallee/grow(), so pts sets can't be
  // invalidated mid-iteration.
  bool resolveFieldWrite(const QualifyingFieldWriteRecord &Rec) {
    const ValueId PtrId = rep(Rec.PtrId);
    if (!Nodes.inbounds(PtrId)) {
      return false;
    }
    bool Changed = false;
    const auto &Pts = Nodes[PtrId].PtsSet;
    Pts.foreach ([&](ValueId ObjId) {
      if (!Nodes.inbounds(ObjId)) {
        return false;
      }
      forEachVar(ObjId, [&](ContextualVar CVar) {
        const llvm::Value *AllocVal = CVar.Var.getBase().valueOrNull();
        if (!AllocVal) {
          return;
        }

        FieldWriteInfo Info;
        Info.ElemTy = Rec.GEPElemTy;
        Info.Callees.push_back(Rec.Callee);
        Changed |= mergeFieldWriteInfo({.Val = AllocVal, .Ctx = CVar.Ctx},
                                       Rec.Indices, Info);
      });
      return true;
    });
    return Changed;
  }

  // Poisen all
  bool resolveFieldWrite(ValueId PtrId) {
    if (!Nodes.inbounds(PtrId)) {
      return false;
    }
    bool Changed = false;
    const auto &Pts = Nodes[PtrId].PtsSet;
    Pts.foreach ([&](ValueId ObjId) {
      if (!Nodes.inbounds(ObjId)) {
        return false;
      }
      forEachVar(ObjId, [&](ContextualVar CVar) {
        const llvm::Value *AllocVal = CVar.Var.getBase().valueOrNull();
        if (!AllocVal) {
          return;
        }

        Changed |= poisonObject({.Val = AllocVal, .Ctx = CVar.Ctx});
      });
      return true;
    });
    return Changed;
  }

  bool checkUnresolvedFieldWrites() {
    bool Changed = false;

    for (const auto &Rec : UnresolvedCopyFieldWrites) {
      Changed |= resolveCopyForward(Rec);
    }
    for (const auto &Rec : UnresolvedQualFieldWrites) {
      Changed |= resolveFieldWrite(Rec);
    }
    for (const auto &Rec : UnresolvedPoisenFieldWrites) {
      Changed |= resolveFieldWrite(Rec);
    }
    return Changed;
  }

  bool resolveFPCall(const llvm::CallBase *CS, ValueId FPId,
                     const ArgList &Args, std::optional<ValueId> CSRetVal,
                     CallingContextId CallerCtx) {
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
      forEachVar(ObjId, [&](ContextualVar CVar) {
        const auto *Fun = llvm::dyn_cast_or_null<llvm::Function>(
            CVar.Var.getBase().valueOrNull());
        if (Fun && isConsistentCall(CS, Fun)) {
          NewEdge |= connectCallee(CS, Fun, Args, CSRetVal, CallerCtx);
        }
      });
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
      connectCallee(C, Callee, Args, CSRetVal, CurCtx);
      return;
    }

    // Virtual call: read the concrete vtable at the specific slot index.
    if (auto VCallInfo = getVFTIndexAndVT(C)) {
      auto [VtablePtr, VtableIndex] = *VCallInfo;
      const ValueId VtablePtrId = getOrInsertVar(PAGVariable(VtablePtr));
      resolveVtableCall(C, VtablePtrId, VtableIndex, Args, CSRetVal, CurCtx);
      UnresolvedVCalls.push_back(VCallRecord{
          .CS = C,
          .VtablePtrId = VtablePtrId,
          .VtableIndex = VtableIndex,
          .Args = std::move(Args),
          .CSRetVal = CSRetVal,
          .Ctx = CurCtx,
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
          .Ctx = CurCtx,
      };
      resolveStructVCall(Rec);
      UnresolvedStructVCalls.push_back(std::move(Rec));
      return;
    }

    // Indirect call: connect already-known targets, record for fixpoint.
    const ValueId FPId = getOrInsertVar(PAGVariable(FnPtr));
    resolveFPCall(C, FPId, Args, CSRetVal, CurCtx);
    UnresolvedFPCalls.push_back(FPCallRecord{
        .CS = C,
        .FPId = FPId,
        .Args = std::move(Args),
        .CSRetVal = CSRetVal,
        .Ctx = CurCtx,
    });
  }

  bool checkUnresolvedFPCalls() {
    bool NewEdge = false;
    for (const auto &Rec : UnresolvedFPCalls) {
      NewEdge |=
          resolveFPCall(Rec.CS, Rec.FPId, Rec.Args, Rec.CSRetVal, Rec.Ctx);
    }
    return NewEdge;
  }

  bool checkUnresolvedVCalls() {
    bool NewEdge = false;
    for (const auto &Rec : UnresolvedVCalls) {
      NewEdge |= resolveVtableCall(Rec.CS, Rec.VtablePtrId, Rec.VtableIndex,
                                   Rec.Args, Rec.CSRetVal, Rec.Ctx);
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
    // Context clones of one PAGVariable share an external id, so the reported
    // alias set of a formal is the union over its contexts, while call sites
    // and locals -- distinct PAGVariables -- keep their per-context precision.
    TypedVector<ValueId, std::optional<ValueId>> LocalToExt(NumLocal);
    for (auto VId : iota<ValueId>(NumLocal)) {
      std::optional<ValueId> FirstExtId;
      forEachVar(VId, [&](ContextualVar CVar) {
        if (CVar.Var.isObject()) {
          return;
        }
        if (!FirstExtId) {
          FirstExtId = ExternalVC.insert(CVar.Var.getBase()).first;
          LocalToExt[VId] = FirstExtId;
        } else {
          ExternalVC.addAlias(CVar.Var.getBase(), *FirstExtId);
        }
      });
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
        const auto [F, Ctx] = FunctionWorklist.pop_back_val();
        if (!Processed.insert({F, Ctx}).second) {
          continue;
        }
        processFunction(F, Ctx);
        // Drain pending pts for functions that make no pointer-relevant
        // calls (connectCallee would otherwise be the only propagate site).
        propagate();
      }
      Changed = checkUnresolvedFieldWrites();
      Changed |= checkUnresolvedFPCalls();
      Changed |= checkUnresolvedVCalls();
      Changed |= checkUnresolvedStructVCalls();
    } while (!FunctionWorklist.empty() || Changed);

    return buildResult();
  }
};

// ---- AndersenOTFSolver --------------------------------------------------

AndersenOTFSolver::AndersenOTFSolver(
    const LLVMProjectIRDB &IRDB, llvm::ArrayRef<const llvm::Function *> Entries,
    ValueCompressor<PAGVariable> &VC, Soundness S,
    ContextSensitivityOptions CSOpts) noexcept
    : IRDB(IRDB), Entries(Entries), VC(VC), S(S), CSOpts(std::move(CSOpts)) {}

AndersenOTFResult AndersenOTFSolver::solve() {
  SolverData Impl{*IRDB, Entries, *VC, S, CSOpts};
  return Impl.run();
}

// ---- Factory functions --------------------------------------------------

AndersenOTFResult
psr::computeAndersenOTFRaw(const LLVMProjectIRDB &IRDB,
                           llvm::ArrayRef<const llvm::Function *> EntryPoints,
                           MaybeUniquePtr<ValueCompressor<PAGVariable>> VC,
                           Soundness S, ContextSensitivityOptions CSOpts) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC, S, std::move(CSOpts));
  return Solver.solve();
}

LLVMUnionFindAliasIterator<AndersenOTFResult>
psr::computeAndersenOTF(const LLVMProjectIRDB &IRDB,
                        llvm::ArrayRef<const llvm::Function *> EntryPoints,
                        MaybeUniquePtr<ValueCompressor<PAGVariable>> VC,
                        Soundness S, ContextSensitivityOptions CSOpts) {
  if (!VC) {
    VC = std::make_unique<ValueCompressor<PAGVariable>>();
  }
  AndersenOTFSolver Solver(IRDB, EntryPoints, *VC, S, std::move(CSOpts));
  auto Res = Solver.solve();
  return LLVMUnionFindAliasIterator{std::move(Res), std::move(VC)};
}
