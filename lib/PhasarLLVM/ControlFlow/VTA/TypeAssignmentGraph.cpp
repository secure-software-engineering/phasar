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
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

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

static const llvm::DIType *stripMemberAndTypedef(const llvm::DIType *Ty) {
  while (const auto *DerivedTy = llvm::dyn_cast<llvm::DIDerivedType>(Ty)) {
    if (DerivedTy->getTag() == llvm::dwarf::DW_TAG_typedef ||
        DerivedTy->getTag() == llvm::dwarf::DW_TAG_member) {
      Ty = DerivedTy->getBaseType();
      continue;
    }
    break;
  }
  return Ty;
}

static bool isPointerTy(const llvm::DIType *Ty) {
  if (const auto *DerivedTy =
          llvm::dyn_cast<llvm::DIDerivedType>(stripMemberAndTypedef(Ty))) {
    return DerivedTy->getTag() == llvm::dwarf::DW_TAG_pointer_type ||
           DerivedTy->getTag() == llvm::dwarf::DW_TAG_reference_type;
  }
  return false;
}

static const llvm::DICompositeType *isCompositeTy(const llvm::DIType *Ty) {
  return llvm::dyn_cast<llvm::DICompositeType>(stripMemberAndTypedef(Ty));
}

static llvm::SmallBitVector
getPointerIndicesOfType(llvm::DICompositeType *Ty, const llvm::DataLayout &DL) {
  llvm::SmallBitVector Ret;

  auto PointerSize = DL.getPointerSizeInBits();

  // XXX: Does every type provide a meaningful getSizeInBits?
  auto MaxNumPointers = Ty->getSizeInBits() / PointerSize;
  if (!MaxNumPointers) {
    return Ret;
  }
  Ret.resize(MaxNumPointers);

  llvm::SmallVector<std::pair<llvm::DIType *, ptrdiff_t>> WorkList = {{Ty, 0}};

  while (!WorkList.empty()) {
    auto [CurrTy, CurrBitOffs] = WorkList.pop_back_val();

    if (isPointerTy(CurrTy)) {
      size_t Idx = CurrBitOffs / PointerSize;
      if (CurrBitOffs % PointerSize) [[unlikely]] {
        PHASAR_LOG_LEVEL(WARNING, "Unaligned pointer..");
      }
      assert(Ret.size() > Idx &&
             "reserved unsufficient space for pointer indices");
      Ret.set(Idx);
      continue;
    }

    const auto *CompTy = isCompositeTy(CurrTy);
    if (!CompTy) {
      continue;
    }

    auto Tag = CompTy->getTag();

    if (Tag == llvm::dwarf::DW_TAG_array_type) {
      auto *ElemTy = CompTy->getBaseType();
      const auto *ArrayLenRange =
          llvm::cast<llvm::DISubrange>(CompTy->getElements()[0]);
      auto ArrayLenBound = ArrayLenRange->getCount();
      if (const auto *ArrayLenCInt =
              ArrayLenBound.dyn_cast<llvm::ConstantInt *>()) {
        auto ArrayLen = ArrayLenCInt->getSExtValue();
        // Count is -1 for flexible array members;
        if (ArrayLen < 0) {
          continue;
        }

        auto ElemSize = int64_t(ElemTy->getSizeInBits());
        for (int64_t I = 0, Offs = CurrBitOffs; I < ArrayLen;
             ++I, Offs += ElemSize) {
          WorkList.emplace_back(ElemTy, Offs);
        }
      }

      continue;
    }

    if (Tag == llvm::dwarf::DW_TAG_structure_type ||
        Tag == llvm::dwarf::DW_TAG_class_type) {

      auto Elems = CompTy->getElements();
      uint64_t Offs = CurrBitOffs;
      for (auto *Elem : Elems) {
        auto *ElemTy = llvm::dyn_cast<llvm::DIType>(Elem);
        if (!ElemTy) {
          continue;
        }

        scope_exit IncOffs = [&] { Offs += ElemTy->getSizeInBits(); };

        if (Elem->getTag() != llvm::dwarf::DW_TAG_inheritance &&
            Elem->getTag() != llvm::dwarf::DW_TAG_member) {
          continue;
        }

        WorkList.emplace_back(ElemTy, Offs);
      }

      continue;
    }
  }

  return Ret;
}

static void addTAGNode(TAGNode TN, TypeAssignmentGraph &TAG) {
  TAG.Nodes.getOrInsert(TN);
}

static void addFields(const LLVMProjectIRDB &IRDB, TypeAssignmentGraph &TAG,
                      const llvm::DataLayout &DL) {

  size_t PointerSize = DL.getPointerSize();

  llvm::DebugInfoFinder DIF;
  DIF.processModule(*IRDB.getModule());

  for (auto *DITy : DIF.types()) {
    if (auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(DITy)) {
      auto Offsets = getPointerIndicesOfType(CompTy, DL);
      for (auto Offs : Offsets.set_bits()) {
        addTAGNode({Field{CompTy, Offs * PointerSize}}, TAG);
      }
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
    if (GlobName.startswith("_ZTV") || GlobName.startswith("_ZTI") ||
        GlobName.startswith("_ZTS")) {
      continue;
    }

    addTAGNode({Variable{&Glob}}, TAG);
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
    handleEntryForCall(Call, *CSNod, TAG, Callee, VTP);

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
