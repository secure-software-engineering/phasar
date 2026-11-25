/******************************************************************************
 * Copyright (c) 2024 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and other
 *****************************************************************************/

#include "phasar/PhasarLLVM/ControlFlow/VTA/TypeAssignmentGraph.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMVFTableProvider.h"
#include "phasar/PhasarLLVM/ControlFlow/Resolver/Resolver.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallBitVector.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

using namespace psr;
using namespace psr::vta;

#if __cplusplus >= 202002L
static_assert(is_const_graph<TypeAssignmentGraph>);
#endif

static void printNodeImpl(llvm::raw_ostream &OS, Variable Var) {
  OS << "var-";
  OS.write_escaped(psr::llvmIRToString(Var.Val));
}

static void printNodeImpl(llvm::raw_ostream &OS, Field Fld) {
  OS << "fld-";
  OS.write_escaped(psr::llvmTypeToString(Fld.Base, true));
  OS << '+' << Fld.ByteOffset;
}

static void printNodeImpl(llvm::raw_ostream &OS, Return Ret) {
  OS << "ret-";
  OS.write_escaped(Ret.Fun->getName());
}

void vta::printNode(llvm::raw_ostream &OS, TAGNode TN) {
  std::visit([&OS](auto Nod) { printNodeImpl(OS, Nod); }, TN.Label);
}

static void addTAGNode(TAGNode TN, TypeAssignmentGraph &TAG) {
  TAG.Nodes.getOrInsert(TN);
}

static void addFields(const LLVMProjectIRDB &IRDB, TypeAssignmentGraph &TAG,
                      const llvm::DataLayout &DL) {

  size_t PointerSize = DL.getPointerSize();

  llvm::DebugInfoFinder DIF;
  DIF.processModule(*IRDB.getModule());

  PointerIndicesCache PIC{};

  for (auto *DITy : DIF.types()) {
    if (auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(DITy)) {
      auto &&Offsets = getPointerIndicesOfType(CompTy, DL, PIC);
      Offsets.foreach ([&](uint32_t Offs) {
        addTAGNode({Field{CompTy, Offs * PointerSize}}, TAG);
      });
      addTAGNode({Field{CompTy, SIZE_MAX}}, TAG);
    }
  }
}

static void addGlobals(const LLVMProjectIRDB &IRDB, TypeAssignmentGraph &TAG) {
  auto NumGlobals = IRDB.getNumGlobals();
  TAG.Nodes.reserve(TAG.Nodes.size() + NumGlobals);

  for (const auto &Glob : IRDB.getModule()->globals()) {
    if (Glob.getValueType()->isIntOrIntVectorTy() ||
        Glob.getValueType()->isFloatingPointTy()) {
      continue;
    }
    auto GlobName = Glob.getName();
    if (GlobName.starts_with("_ZTV") || GlobName.starts_with("_ZTI") ||
        GlobName.starts_with("_ZTS")) {
      continue;
    }

    addTAGNode({Variable{&Glob}}, TAG);
  }
}

static const llvm::DIType *stripTypedefsAndQualifiers(const llvm::DIType *Ty) {
  while (const auto *DerivedTy =
             llvm::dyn_cast_if_present<llvm::DIDerivedType>(Ty)) {
    switch (DerivedTy->getTag()) {
    case llvm::dwarf::DW_TAG_typedef:
    case llvm::dwarf::DW_TAG_member:
    case llvm::dwarf::DW_TAG_inheritance:
    case llvm::dwarf::DW_TAG_const_type:
    case llvm::dwarf::DW_TAG_volatile_type:
    case llvm::dwarf::DW_TAG_restrict_type:
    case llvm::dwarf::DW_TAG_atomic_type:
      Ty = DerivedTy->getBaseType();
      continue;
    default:
      return Ty;
    }
  }
  return Ty;
}

/// Calls WithField for each composite type that encloses the bit at
/// OffsetInBits of an object of type Ty, together with the byte offset
/// relative to that composite.
static void forEachEnclosingField(
    const llvm::DIType *Ty, uint64_t OffsetInBits,
    std::invocable<const llvm::DICompositeType *, size_t> auto WithField) {
  const auto *CompTy = llvm::dyn_cast_if_present<llvm::DICompositeType>(
      stripTypedefsAndQualifiers(Ty));
  if (!CompTy) {
    return;
  }

  auto Tag = CompTy->getTag();
  if (Tag == llvm::dwarf::DW_TAG_array_type) {
    std::invoke(WithField, CompTy, OffsetInBits / 8);

    const auto *ElemTy = stripTypedefsAndQualifiers(CompTy->getBaseType());
    if (!ElemTy || !ElemTy->getSizeInBits()) {
      return;
    }
    forEachEnclosingField(ElemTy, OffsetInBits % ElemTy->getSizeInBits(),
                          copyOrRef(WithField));
    return;
  }

  if (Tag != llvm::dwarf::DW_TAG_structure_type &&
      Tag != llvm::dwarf::DW_TAG_class_type &&
      Tag != llvm::dwarf::DW_TAG_union_type) {
    return;
  }

  std::invoke(WithField, CompTy, OffsetInBits / 8);

  for (const auto *Elem : CompTy->getElements()) {
    const auto *Member = llvm::dyn_cast<llvm::DIDerivedType>(Elem);
    if (!Member || Member->isStaticMember() ||
        (Member->getTag() != llvm::dwarf::DW_TAG_member &&
         Member->getTag() != llvm::dwarf::DW_TAG_inheritance)) {
      continue;
    }

    // DW_TAG_inheritance carries no size
    auto MemberSize = Member->getSizeInBits();
    if (!MemberSize) {
      if (const auto *BaseTy = stripTypedefsAndQualifiers(Member)) {
        MemberSize = BaseTy->getSizeInBits();
      }
    }

    auto MemberOffset = Member->getOffsetInBits();
    if (OffsetInBits >= MemberOffset &&
        OffsetInBits - MemberOffset < MemberSize) {
      forEachEnclosingField(Member->getBaseType(), OffsetInBits - MemberOffset,
                            copyOrRef(WithField));
    }
  }
}

/// Calls WithLeaf for each non-aggregate constant in Init, together with its
/// byte offset relative to Init.
static void forEachInitializerLeaf(
    const llvm::Constant *Init, uint64_t ByteOffset, const llvm::DataLayout &DL,
    std::invocable<const llvm::Constant *, uint64_t> auto WithLeaf) {
  if (const auto *Struct = llvm::dyn_cast<llvm::ConstantStruct>(Init)) {
    const auto *Layout = DL.getStructLayout(Struct->getType());
    for (const auto &[Idx, Op] : llvm::enumerate(Struct->operands())) {
      forEachInitializerLeaf(llvm::cast<llvm::Constant>(Op),
                             ByteOffset + Layout->getElementOffset(Idx), DL,
                             copyOrRef(WithLeaf));
    }
    return;
  }

  if (const auto *Arr = llvm::dyn_cast<llvm::ConstantArray>(Init)) {
    uint64_t ElemSize =
        DL.getTypeAllocSize(Arr->getType()->getElementType()).getFixedValue();
    for (const auto &[Idx, Op] : llvm::enumerate(Arr->operands())) {
      forEachInitializerLeaf(llvm::cast<llvm::Constant>(Op),
                             ByteOffset + Idx * ElemSize, DL,
                             copyOrRef(WithLeaf));
    }
    return;
  }

  std::invoke(WithLeaf, Init, ByteOffset);
}

/// Models each pointer P in a global's initializer like a store of P into the
/// global, resp. into the field of the global that holds P; see handleStore().
static void addGlobalInitializers(const LLVMProjectIRDB &IRDB,
                                  TypeAssignmentGraph &TAG,
                                  LLVMAliasIteratorRef AI,
                                  const llvm::DataLayout &DL) {
  llvm::SmallVector<const llvm::Function *> Funs;
  llvm::SmallVector<TAGNodeId> Froms;

  for (const auto &Glob : IRDB.getModule()->globals()) {
    // Globals without a node are filtered by addGlobals(), e.g., vtables.
    // llvm.* globals, e.g., llvm.global_ctors, are never loaded from.
    if (!Glob.hasInitializer() || Glob.getName().starts_with("llvm.") ||
        !TAG.get({Variable{&Glob}})) {
      continue;
    }

    const auto *GlobTy = getVarTypeFromIR(&Glob);
    Funs.clear();
    Froms.clear();

    forEachInitializerLeaf(
        Glob.getInitializer(), 0, DL,
        [&](const llvm::Constant *Leaf, uint64_t ByteOffset) {
          if (!Leaf->getType()->isPointerTy()) {
            return;
          }

          const auto *Base = getBaseAndOffset(Leaf, DL).first;
          const auto *Fun = llvm::dyn_cast<llvm::Function>(Base);
          std::optional<TAGNodeId> From;
          if (Fun) {
            Funs.push_back(Fun);
          } else {
            From = TAG.get({Variable{Base}});
            if (From) {
              Froms.push_back(*From);
            } else {
              return;
            }
          }

          forEachEnclosingField(
              GlobTy, ByteOffset * 8,
              [&](const llvm::DICompositeType *CompTy, size_t FieldOffset) {
                for (auto Offs : {FieldOffset, SIZE_MAX}) {
                  auto FldNodeId = TAG.get({Field{CompTy, Offs}});
                  if (!FldNodeId) {
                    continue;
                  }
                  if (Fun) {
                    TAG.TypeEntryPoints[*FldNodeId].insert(Fun);
                  } else {
                    TAG.addEdge(*From, *FldNodeId);
                  }
                }
              });
        });

    if (Funs.empty() && Froms.empty()) {
      continue;
    }

    AI.forallAliasesOf(&Glob, nullptr, [&](const llvm::Value *Dest) {
      auto DestNodeId = TAG.get({Variable{Dest}});
      if (!DestNodeId) {
        return;
      }

      if (!Funs.empty()) {
        TAG.TypeEntryPoints[*DestNodeId].insert(Funs.begin(), Funs.end());
      }
      for (auto From : Froms) {
        TAG.addEdge(From, *DestNodeId);
      }
    });
  }
}

static void initializeWithFun(const llvm::Function *Fun,
                              TypeAssignmentGraph &TAG) {
  // Add all params
  // Add all locals
  // Add return

  if (Fun->isDeclaration()) {
    return;
  }

  for (const auto &Arg : Fun->args()) {
    if (!Arg.getType()->isPointerTy()) {
      continue;
    }

    addTAGNode({Variable{&Arg}}, TAG);
  }

  for (const auto &I : llvm::instructions(Fun)) {
    if (!I.getType()->isPointerTy()) {
      // XXX: What about SSA structs that contain pointers?
      continue;
    }

    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
      if (Alloca->getAllocatedType()->isIntOrIntVectorTy() ||
          Alloca->getAllocatedType()->isFloatingPointTy()) {
        continue;
      }
    }

    addTAGNode({Variable{&I}}, TAG);
  }

  if (Fun->getReturnType() && Fun->getReturnType()->isPointerTy()) {
    addTAGNode({Return{Fun}}, TAG);
  }
}

static void handleAlloca(const llvm::AllocaInst *Alloca,
                         TypeAssignmentGraph &TAG,
                         const psr::LLVMVFTableProvider & /*VTP*/) {
  if (Alloca->getAllocatedType()->isPointerTy()) {
    return;
  }

  auto TN = TAG.get({Variable{Alloca}});
  if (!TN) {
    return;
  }

  const auto *AllocTy = getVarTypeFromIR(Alloca);
  if (!AllocTy) {
    return;
  }

  TAG.TypeEntryPoints[*TN].insert(AllocTy);
}

static std::optional<TAGNodeId> getGEPNode(const llvm::GetElementPtrInst *GEP,
                                           TypeAssignmentGraph &TAG,
                                           const llvm::DataLayout &DL) {
  auto Offs = [&]() -> size_t {
    llvm::APInt Offs(64, 0);
    if (GEP->accumulateConstantOffset(DL, Offs)) {
      return Offs.getZExtValue();
    }
    return SIZE_MAX;
  }();

  auto *VarTy = getVarTypeFromIR(GEP);
  if (!VarTy) {
    return std::nullopt;
  }

  return TAG.get({Field{VarTy, Offs}});
}

static void handleGEP(const llvm::GetElementPtrInst *GEP,
                      TypeAssignmentGraph &TAG, const llvm::DataLayout &DL) {
  auto To = TAG.get({Variable{GEP}});
  if (!To) {
    return;
  }

  if (!GEP->isInBounds()) {
    auto From = TAG.get({Variable{GEP->getPointerOperand()}});

    if (From && To) {
      TAG.addEdge(*From, *To);
    }

    return;
  }

  auto From = getGEPNode(GEP, TAG, DL);
  if (From) {
    TAG.addEdge(*From, *To);
  }
}

static bool handleEntryForStore(const llvm::StoreInst *Store,
                                TypeAssignmentGraph &TAG,
                                LLVMAliasIteratorRef AI,
                                const llvm::DataLayout &DL) {
  const auto *Base = llvm::dyn_cast<llvm::Function>(
      Store->getValueOperand()->stripPointerCastsAndAliases());

  if (!Base) {
    return false;
  }

  if (const auto *GEPDest =
          llvm::dyn_cast<llvm::GetElementPtrInst>(Store->getPointerOperand())) {
    if (auto GEPNodeId = getGEPNode(GEPDest, TAG, DL)) {
      TAG.TypeEntryPoints[*GEPNodeId].insert(Base);

      auto GEPNode = TAG[*GEPNodeId];
      if (const auto *FldDest = std::get_if<Field>(&GEPNode.Label)) {
        auto ApproxDest = TAG.get({Field{FldDest->Base, SIZE_MAX}});

        if (ApproxDest) {
          TAG.TypeEntryPoints[*ApproxDest].insert(Base);
        }
      }
    }
  }

  AI.forallAliasesOf(Store->getPointerOperand(), Store,
                     [&](const llvm::Value *Dest) {
                       // XXX: Fuse store and GEP!

                       auto DestNodeId = TAG.get({Variable{Dest}});
                       if (!DestNodeId) {
                         return;
                       }

                       TAG.TypeEntryPoints[*DestNodeId].insert(Base);
                     });
  return true;
}

static void handleStore(const llvm::StoreInst *Store, TypeAssignmentGraph &TAG,
                        LLVMAliasIteratorRef AI, const llvm::DataLayout &DL) {

  if (handleEntryForStore(Store, TAG, AI, DL)) {
    return;
  }

  auto From = TAG.get({Variable{Store->getValueOperand()}});
  if (!From) {
    return;
  }

  if (const auto *GEPDest =
          llvm::dyn_cast<llvm::GetElementPtrInst>(Store->getPointerOperand())) {
    if (auto GEPNodeId = getGEPNode(GEPDest, TAG, DL)) {
      TAG.addEdge(*From, *GEPNodeId);

      auto GEPNode = TAG[*GEPNodeId];
      if (const auto *FldDest = std::get_if<Field>(&GEPNode.Label)) {
        auto ApproxDest = TAG.get({Field{FldDest->Base, SIZE_MAX}});

        if (ApproxDest) {
          TAG.addEdge(*From, *ApproxDest);
        }
      }
    }
  }

  AI.forallAliasesOf(Store->getPointerOperand(), Store,
                     [&](const llvm::Value *Dest) {
                       // XXX: Fuse store and GEP!

                       auto DestNodeId = TAG.get({Variable{Dest}});
                       if (!DestNodeId) {
                         return;
                       }

                       TAG.addEdge(*From, *DestNodeId);
                     });
}

static void handleLoad(const llvm::LoadInst *Load, TypeAssignmentGraph &TAG,
                       const llvm::DataLayout &DL) {
  auto To = TAG.get({Variable{Load}});
  if (!To) {
    return;
  }

  auto From = TAG.get({Variable{Load->getPointerOperand()}});
  if (From) {
    TAG.addEdge(*From, *To);
  }

  if (const auto *GEPDest =
          llvm::dyn_cast<llvm::GetElementPtrInst>(Load->getPointerOperand())) {
    if (auto GEPNodeId = getGEPNode(GEPDest, TAG, DL)) {
      TAG.addEdge(*GEPNodeId, *To);
    }
  }
}

static void handlePhi(const llvm::PHINode *Phi, TypeAssignmentGraph &TAG) {
  auto To = TAG.get({Variable{Phi}});
  if (!To) {
    return;
  }

  for (const auto &Inc : Phi->incoming_values()) {
    auto From = TAG.get({Variable{Inc.get()}});
    if (From) {
      TAG.addEdge(*From, *To);
    }
  }
}

static void handleEntryForCall(const llvm::CallBase *Call, TAGNodeId CSNod,
                               TypeAssignmentGraph &TAG,
                               const llvm::Function *Callee,
                               const psr::LLVMVFTableProvider & /*VTP*/) {

  if (!psr::isHeapAllocatingFunction(Callee)) {
    return;
  }

  if (const auto *MDNode = Call->getMetadata("heapallocsite")) {

    // Shortcut
    if (const auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(MDNode);
        CompTy && (CompTy->getTag() == llvm::dwarf::DW_TAG_structure_type ||
                   CompTy->getTag() == llvm::dwarf::DW_TAG_class_type)) {

      TAG.TypeEntryPoints[CSNod].insert(CompTy);
    }
  }
}

static void handleCall(const llvm::CallBase *Call, TypeAssignmentGraph &TAG,
                       Resolver &BaseRes, const psr::LLVMVFTableProvider &VTP) {

  llvm::SmallVector<std::optional<TAGNodeId>> Args;
  llvm::SmallBitVector EntryArgs;
  bool HasArgNode = false;

  for (const auto &Arg : Call->args()) {
    auto TN = TAG.get({Variable{Arg.get()}});
    Args.push_back(TN);
    if (TN) {
      HasArgNode = true;
    }

    bool IsEntry =
        llvm::isa<llvm::Function>(Arg.get()->stripPointerCastsAndAliases());
    EntryArgs.push_back(IsEntry);
  }

  auto CSNod = TAG.get({Variable{Call}});

  // XXX: Handle struct returns that contain pointers
  if (!HasArgNode && !CSNod) {
    return;
  }

  const auto HandleCallTarget = [&](const llvm::Function *Callee) {
    if (CSNod) {
      handleEntryForCall(Call, *CSNod, TAG, Callee, VTP);
    }

    if (Callee->isDeclaration()) {
      // XXX: Integrate with getLibCSummary()
      return;
    }

    for (const auto &[Param, Arg] : llvm::zip(Callee->args(), Args)) {
      auto ParamNodId = TAG.get({Variable{&Param}});
      if (!ParamNodId) {
        continue;
      }

      if (EntryArgs.test(Param.getArgNo())) {
        TAG.TypeEntryPoints[*ParamNodId].insert(
            llvm::cast<llvm::Function>(Call->getArgOperand(Param.getArgNo())
                                           ->stripPointerCastsAndAliases()));
      }

      if (!Arg) {
        continue;
      }

      if (!Param.hasStructRetAttr()) {
        TAG.addEdge(*Arg, *ParamNodId);
      }

      // if (!Param.hasByValAttr())
      //   TAG.addEdge(*ParamNodId, *Arg);
    }
    if (CSNod) {
      auto RetNod = TAG.get({Return{Callee}});
      if (RetNod) {
        TAG.addEdge(*RetNod, *CSNod);
      }
    }
  };

  if (const auto *StaticCallee = llvm::dyn_cast<llvm::Function>(
          Call->getCalledOperand()->stripPointerCastsAndAliases())) {
    HandleCallTarget(StaticCallee);
  } else {
    for (const auto *Callee : BaseRes.resolveIndirectCall(Call)) {
      HandleCallTarget(Callee);
    }
  }
}

static void handleReturn(const llvm::ReturnInst *Ret,
                         TypeAssignmentGraph &TAG) {

  auto TNId = TAG.get({Return{Ret->getFunction()}});
  if (!TNId) {
    return;
  }

  if (const auto *RetVal = Ret->getReturnValue()) {
    const auto *Base = RetVal->stripPointerCastsAndAliases();
    if (const auto *RetFun = llvm::dyn_cast<llvm::Function>(Base)) {
      TAG.TypeEntryPoints[*TNId].insert(RetFun);
      return;
    }

    auto From = TAG.get({Variable{Base}});
    if (From) {
      TAG.addEdge(*From, *TNId);
    }
  }
}

static void dispatch(const llvm::Instruction &I, TypeAssignmentGraph &TAG,
                     Resolver &BaseRes, LLVMAliasIteratorRef AI,
                     const llvm::DataLayout &DL,
                     const psr::LLVMVFTableProvider &VTP) {
  if (llvm::isa<llvm::DbgInfoIntrinsic>(&I)) {
    return;
  }

  if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
    handleAlloca(Alloca, TAG, VTP);
    return;
  }
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(&I)) {
    handleLoad(Load, TAG, DL);
    return;
  }
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(&I)) {
    handleGEP(GEP, TAG, DL);
    return;
  }
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(&I)) {
    handleStore(Store, TAG, AI, DL);
    return;
  }
  if (const auto *Phi = llvm::dyn_cast<llvm::PHINode>(&I)) {
    handlePhi(Phi, TAG);
    return;
  }
  if (const auto *Cast = llvm::dyn_cast<llvm::CastInst>(&I)) {
    auto From = TAG.get({Variable{Cast->getOperand(0)}});
    auto To = TAG.get({Variable{Cast}});

    if (From && To) {
      TAG.addEdge(*From, *To);
    }
  }
  if (const auto *Call = llvm::dyn_cast<llvm::CallBase>(&I)) {
    handleCall(Call, TAG, BaseRes, VTP);
    return;
  }
  if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(&I)) {
    handleReturn(Ret, TAG);
    return;
  }
  // XXX: Handle more cases
}

static void buildTAGWithFun(const llvm::Function *Fun, TypeAssignmentGraph &TAG,
                            Resolver &BaseRes, LLVMAliasIteratorRef AI,
                            const llvm::DataLayout &DL,
                            const psr::LLVMVFTableProvider &VTP) {
  for (const auto &I : llvm::instructions(Fun)) {
    dispatch(I, TAG, BaseRes, AI, DL, VTP);
  }
}

static auto computeTypeAssignmentGraphImpl(const LLVMProjectIRDB &IRDB,
                                           Resolver &BaseRes,
                                           LLVMAliasIteratorRef AI,
                                           const psr::LLVMVFTableProvider &VTP,
                                           ReachableFunsTy ReachableFunctions)
    -> TypeAssignmentGraph {
  TypeAssignmentGraph TAG;

  const auto &DL = IRDB.getModule()->getDataLayout();

  addFields(IRDB, TAG, DL);
  addGlobals(IRDB, TAG);

  assert(ReachableFunctions);

  ReachableFunctions(IRDB,
                     [&TAG](const auto *Fun) { initializeWithFun(Fun, TAG); });

  TAG.Adj.resize(TAG.Nodes.size());

  addGlobalInitializers(IRDB, TAG, AI, DL);

  ReachableFunctions(IRDB, [&](const auto *Fun) {
    buildTAGWithFun(Fun, TAG, BaseRes, AI, DL, VTP);
  });

  return TAG;
}

auto vta::computeTypeAssignmentGraph(const LLVMProjectIRDB &IRDB,
                                     const psr::LLVMVFTableProvider &VTP,
                                     LLVMAliasIteratorRef AS, Resolver &BaseRes,
                                     ReachableFunsTy ReachableFunctions)
    -> TypeAssignmentGraph {

  return computeTypeAssignmentGraphImpl(IRDB, BaseRes, AS, VTP,
                                        ReachableFunctions);
}

void TypeAssignmentGraph::print(llvm::raw_ostream &OS) {
  OS << "digraph TAG {\n";
  psr::scope_exit CloseBrace = [&OS] { OS << "}\n"; };

  size_t Ctr = 0;
  for (const auto &TN : Nodes) {
    OS << "  " << Ctr << "[label=\"";
    printNode(OS, TN);
    OS << "\"];\n";

    ++Ctr;
  }

  OS << '\n';

  Ctr = 0;
  for (const auto &Targets : Adj) {
    for (auto Tgt : Targets) {
      OS << "  " << Ctr << "->" << uint32_t(Tgt) << ";\n";
    }
    ++Ctr;
  }
}
