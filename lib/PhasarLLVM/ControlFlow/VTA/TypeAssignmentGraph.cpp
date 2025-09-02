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
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

using namespace psr;
using namespace psr::vta;

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

static llvm::SmallBitVector
getPointerIndicesOfType(llvm::Type *Ty, const llvm::DataLayout &DL) {
  llvm::SmallBitVector Ret;

  auto PointerSize = DL.getPointerSize();
  // LOGS("[getPointerIndicesOfType]: " << *Ty );
  auto MaxNumPointers =
      !Ty->isSized() ? 1 : DL.getTypeAllocSize(Ty) / PointerSize;
  if (!MaxNumPointers) {
    return Ret;
  }
  Ret.resize(MaxNumPointers);

  llvm::SmallVector<std::pair<llvm::Type *, ptrdiff_t>> WorkList = {{Ty, 0}};

  while (!WorkList.empty()) {
    auto [CurrTy, CurrByteOffs] = WorkList.pop_back_val();

    if (CurrTy->isPointerTy()) {
      size_t Idx = CurrByteOffs / PointerSize;
      if (CurrByteOffs % PointerSize) [[unlikely]] {
        PHASAR_LOG_LEVEL(WARNING, "Unaligned pointer..");
        /*llvm::errs() << "[WARNING][getPointerIndicesOfType]: Unaligned pointer
           " "found at offset "
                     << CurrByteOffs << " in type " << *Ty;*/
      }
      assert(Ret.size() > Idx &&
             "reserved unsufficient space for pointer indices");
      Ret.set(Idx);
      continue;
    }

    if (CurrTy->isArrayTy()) {
      auto *ElemTy = CurrTy->getArrayElementType();
      auto ArrayLen = CurrTy->getArrayNumElements();
      auto ElemSize = DL.getTypeAllocSize(ElemTy);
      for (size_t I = 0, Offs = CurrByteOffs; I < ArrayLen;
           ++I, Offs += ElemSize) {
        WorkList.emplace_back(ElemTy, Offs);
      }
      continue;
    }

    if (auto *Struct = llvm::dyn_cast<llvm::StructType>(CurrTy)) {
      auto NumElems = Struct->getNumElements();
      const auto *SL = DL.getStructLayout(Struct);
      for (size_t I = 0; I < NumElems; ++I) {
        auto Offs = CurrByteOffs + SL->getElementOffset(I);
        WorkList.emplace_back(Struct->getElementType(I), Offs);
      }
      continue;
    }
  }

  return Ret;
}

static void addTAGNode(TAGNode TN, TypeAssignmentGraph &TAG) {
  TAG.Nodes.getOrInsert(TN);
}

static void addFields(const llvm::Module &Mod, TypeAssignmentGraph &TAG,
                      const llvm::DataLayout &DL) {
  auto &&Structs = Mod.getIdentifiedStructTypes();
  TAG.Nodes.reserve(TAG.Nodes.size() + Structs.size());

  size_t PointerSize = DL.getPointerSize();

  for (auto *ST : Structs) {
    auto Offsets = getPointerIndicesOfType(ST, DL);
    for (auto Offs : Offsets.set_bits()) {
      addTAGNode({Field{ST, Offs * PointerSize}}, TAG);
    }
    addTAGNode({Field{ST, SIZE_MAX}}, TAG);
  }
}

static void addGlobals(const llvm::Module &Mod, TypeAssignmentGraph &TAG) {
  auto NumGlobals = Mod.global_size();
  TAG.Nodes.reserve(TAG.Nodes.size() + NumGlobals);

  for (const auto &Glob : Mod.globals()) {
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
      // TODO: What about SSA structs that contain pointers?
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

[[nodiscard]] static bool isVTableOrFun(const llvm::Value *Val) {
  const auto *Base = Val->stripPointerCastsAndAliases();
  if (llvm::isa<llvm::Function>(Base)) {
    return true;
  }

  if (const auto *Glob = llvm::dyn_cast<llvm::GlobalVariable>(Base)) {
    return Glob->isConstant() && Glob->getName().startswith("_ZTV");
  }

  return false;
}

static void handleAlloca(const llvm::AllocaInst *Alloca,
                         TypeAssignmentGraph &TAG,
                         const psr::LLVMVFTableProvider &VTP) {
  auto TN = TAG.get({Variable{Alloca}});
  if (!TN) {
    return;
  }

  const auto *AllocTy = getVarTypeFromIR(Alloca);
  if (!AllocTy) {
    return;
  }

  if (const auto *TV = VTP.getVFTableGlobal(AllocTy)) {
    TAG.TypeEntryPoints[*TN].insert(TV);
  }
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

  return TAG.get({Field{GEP->getSourceElementType(), Offs}});
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
  // TODO: Is this correct? -- also check load

  auto From = getGEPNode(GEP, TAG, DL);
  if (From) {
    TAG.addEdge(*From, *To);
  }
}

static bool handleEntryForStore(const llvm::StoreInst *Store,
                                TypeAssignmentGraph &TAG, AliasInfoTy AI,
                                const llvm::DataLayout &DL) {
  const auto *Base = Store->getValueOperand()->stripPointerCastsAndAliases();
  bool IsEntry = isVTableOrFun(Base);

  if (!IsEntry) {
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

  AI(Store->getPointerOperand(), Store, [&](const llvm::Value *Dest) {
    // TODO: Fuse store and GEP!

    auto DestNodeId = TAG.get({Variable{Dest}});
    if (!DestNodeId) {
      return;
    }

    TAG.TypeEntryPoints[*DestNodeId].insert(Base);
  });
  return true;
}

static void handleStore(const llvm::StoreInst *Store, TypeAssignmentGraph &TAG,
                        AliasInfoTy AI, const llvm::DataLayout &DL) {

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

  AI(Store->getPointerOperand(), Store, [&](const llvm::Value *Dest) {
    // TODO: Fuse store and GEP!

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

static const llvm::Value *getTypeFromDI(const llvm::DICompositeType *CompTy,
                                        const llvm::Module &Mod,
                                        const psr::LLVMVFTableProvider &VTP) {
  if (!CompTy->getIdentifier().empty()) {

    std::string Buf;
    auto TypeName = CompTy->getIdentifier();
    if (TypeName.startswith("_ZTS") || TypeName.startswith("_ZTI")) {
      Buf = TypeName.str();
      Buf[3] = 'V';
      TypeName = Buf;
    }

    if (const auto *GlobTV = Mod.getNamedGlobal(TypeName)) {
      return GlobTV;
    }
    if (const auto *Alias = Mod.getNamedAlias(TypeName)) {
      return Alias->getAliasee()->stripPointerCastsAndAliases();
    }

    return nullptr;
  }

  // TODO: With latest changes from f-TestingAPIChanges, we don't need the below
  // loop!
  auto ClearName = CompTy->getName().str();
  const auto *Scope = CompTy->getScope();
  while (llvm::isa_and_nonnull<llvm::DINamespace, llvm::DISubprogram,
                               llvm::DIType>(Scope)) {
    ClearName = Scope->getName().str().append("::").append(ClearName);
    Scope = Scope->getScope();
  }

  return VTP.getVFTableGlobal(ClearName);
}

static void handleEntryForCall(const llvm::CallBase *Call, TAGNodeId CSNod,
                               TypeAssignmentGraph &TAG,
                               const llvm::Function *Callee,
                               const psr::LLVMVFTableProvider &VTP) {

  if (!psr::isHeapAllocatingFunction(Callee)) {
    return;
  }

  if (const auto *MDNode = Call->getMetadata("heapallocsite")) {

    // Shortcut
    if (const auto *CompTy = llvm::dyn_cast<llvm::DICompositeType>(MDNode);
        CompTy && (CompTy->getTag() == llvm::dwarf::DW_TAG_structure_type ||
                   CompTy->getTag() == llvm::dwarf::DW_TAG_class_type)) {

      if (const auto *Ty = getTypeFromDI(CompTy, *Call->getModule(), VTP)) {

        TAG.TypeEntryPoints[CSNod].insert(Ty);
        return;
      }
    }
  }
}

static void handleCall(const llvm::CallBase *Call, TypeAssignmentGraph &TAG,
                       const psr::CallGraph<const llvm::Instruction *,
                                            const llvm::Function *> &BaseCG,
                       const psr::LLVMVFTableProvider &VTP) {

  llvm::SmallVector<std::optional<TAGNodeId>> Args;
  llvm::SmallBitVector EntryArgs;
  bool HasArgNode = false;

  for (const auto &Arg : Call->args()) {
    auto TN = TAG.get({Variable{Arg.get()}});
    Args.push_back(TN);
    if (TN) {
      HasArgNode = true;
    }

    bool IsEntry = isVTableOrFun(Arg.get());
    EntryArgs.push_back(IsEntry);
  }

  auto CSNod = TAG.get({Variable{Call}});

  // TODO: Handle struct returns that contain pointers
  if (!HasArgNode && !CSNod) {
    return;
  }

  for (const auto *Callee : BaseCG.getCalleesOfCallAt(Call)) {
    handleEntryForCall(Call, *CSNod, TAG, Callee, VTP);

    for (const auto &[Param, Arg] : llvm::zip(Callee->args(), Args)) {
      auto ParamNodId = TAG.get({Variable{&Param}});
      if (!ParamNodId) {
        continue;
      }

      if (EntryArgs.test(Param.getArgNo())) {
        TAG.TypeEntryPoints[*ParamNodId].insert(
            Call->getArgOperand(Param.getArgNo())
                ->stripPointerCastsAndAliases());
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
    if (isVTableOrFun(Base)) {
      TAG.TypeEntryPoints[*TNId].insert(Base);
      return;
    }

    auto From = TAG.get({Variable{Base}});
    if (From) {
      TAG.addEdge(*From, *TNId);
    }
  }
}

static void dispatch(const llvm::Instruction &I, TypeAssignmentGraph &TAG,
                     const psr::CallGraph<const llvm::Instruction *,
                                          const llvm::Function *> &BaseCG,
                     AliasInfoTy AI, const llvm::DataLayout &DL,
                     const psr::LLVMVFTableProvider &VTP) {
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
    handleCall(Call, TAG, BaseCG, VTP);
    return;
  }
  if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(&I)) {
    handleReturn(Ret, TAG);
    return;
  }
  // TODO: Handle more cases
}

static void buildTAGWithFun(
    const llvm::Function *Fun, TypeAssignmentGraph &TAG,
    const psr::CallGraph<const llvm::Instruction *, const llvm::Function *>
        &BaseCG,
    AliasInfoTy AI, const llvm::DataLayout &DL,
    const psr::LLVMVFTableProvider &VTP) {
  for (const auto &I : llvm::instructions(Fun)) {
    dispatch(I, TAG, BaseCG, AI, DL, VTP);
  }
}

static auto computeTypeAssignmentGraphImpl(
    const llvm::Module &Mod,
    const psr::CallGraph<const llvm::Instruction *, const llvm::Function *>
        &BaseCG,
    AliasInfoTy AI, const psr::LLVMVFTableProvider &VTP)
    -> TypeAssignmentGraph {
  TypeAssignmentGraph TAG;

  const auto &DL = Mod.getDataLayout();

  addFields(Mod, TAG, DL);
  addGlobals(Mod, TAG);

  for (const auto &Fun : Mod) {
    initializeWithFun(&Fun, TAG);
  }

  TAG.Adj.resize(TAG.Nodes.size());

  for (const auto &Fun : Mod) {
    buildTAGWithFun(&Fun, TAG, BaseCG, AI, DL, VTP);
  }

  return TAG;
}

auto vta::computeTypeAssignmentGraph(
    const llvm::Module &Mod,
    const psr::CallGraph<const llvm::Instruction *, const llvm::Function *>
        &BaseCG,
    AliasInfoTy AS, const psr::LLVMVFTableProvider &VTP)
    -> TypeAssignmentGraph {

  return computeTypeAssignmentGraphImpl(Mod, BaseCG, AS, VTP);
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
