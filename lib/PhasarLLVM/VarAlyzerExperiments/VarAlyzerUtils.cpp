/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "phasar/PhasarLLVM/VarAlyzerExperiments/VarAlyzerUtils.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/Utils/Logger.h"

#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/IntrinsicInst.h"

#include <cassert>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace psr;

std::vector<std::string>
psr::getEntryPointsForCallersOf(llvm::StringRef FunName, LLVMProjectIRDB &IR,
                                LLVMBasedICFG &ICF,
                                llvm::StringRef TypeNameOfInterest) {

  // Deduplication is easier on pointers than on fresh allocated strings
  llvm::SmallPtrSet<const llvm::Function *, 8> EntrypointFunctions;

  const auto *F = IR.getFunction(FunName);
  auto CallSites = ICF.getCallersOf(F);

  for (const auto *CallSite : CallSites) {
    EntrypointFunctions.insert(CallSite->getFunction());
  }

  // if (EntrypointFunctions.empty()) {
  //   // Exceptional case that we have no context-constructor calls
  //   // -> Detect all allocas of the typenameOfInterest

  //   auto Ty = IR.getType(TypeNameOfInterest);
  //   // some individual modules don't use OpenSSL functionalities at all when
  //   a
  //   // certain configuration has been chosen
  //   // assert(Ty);

  //   for (auto Alloca : IR.getAllocaInstructions()) {
  //     if (isOfType(Ty, Alloca->getType())) {
  //       EntrypointFunctions.insert(Alloca->getFunction());
  //     }
  //   }
  // }
  // Retrieve the entrypoint-names
  std::vector<std::string> EntryPoints;

  for (auto Fn : EntrypointFunctions)
    EntryPoints.push_back(Fn->getName().str());

  return EntryPoints;
}

std::vector<std::string> psr::getEntryPointsForCallersOfDesugared(
    llvm::StringRef FunName, LLVMProjectIRDB &IR, LLVMBasedICFG &ICF,
    const stringstringmap_t &FNameMap, llvm::StringRef TypeNameOfInterest) {
  auto Search = staticRename(FunName, FNameMap);
  return getEntryPointsForCallersOf(Search, IR, ICF, TypeNameOfInterest);
}

llvm::StringRef psr::staticRename(llvm::StringRef Name,
                                  const stringstringmap_t &Renaming) {
  if (auto it = Renaming.find(Name); it != Renaming.end())
    return it->getValue();

  PHASAR_LOG_LEVEL(DEBUG, "Renaming fallthrough: " << Name);
  return Name;
}

static std::optional<llvm::StringRef>
getBaseTypeNameIfUsingTypeDef(llvm::StringRef Target, const llvm::Function *F) {
  for (auto ii = llvm::inst_begin(F), end = llvm::inst_end(F); ii != end;
       ++ii) {
    if (const auto *DbgDeclare = llvm::dyn_cast<llvm::DbgDeclareInst>(&*ii)) {
      const auto *LocalVar = DbgDeclare->getVariable();

      if (const auto *DerivedTy =
              llvm::dyn_cast<llvm::DIDerivedType>(LocalVar->getType())) {

        while ((DerivedTy = llvm::dyn_cast<llvm::DIDerivedType>(
                    DerivedTy->getBaseType()))) {
          if (DerivedTy->getTag() == llvm::dwarf::DW_TAG_typedef) {
            if (DerivedTy->getName() == Target) {
              return DerivedTy->getBaseType()->getName();
            }
          }
        }
      }
    }
  }
  return std::nullopt;
}

std::optional<llvm::StringRef> psr::extractDesugaredTypeNameOfInterest(
    llvm::StringRef OriginalTOI, const LLVMProjectIRDB &IRDB,
    const stringstringmap_t &ForwardRenaming) {
  auto renamedName = staticRename(OriginalTOI, ForwardRenaming);

  for (auto F : IRDB.getAllFunctions()) {
    if (auto name = getBaseTypeNameIfUsingTypeDef(renamedName, F))
      return *name;
  }
  return std::nullopt;
}

llvm::StringRef psr::extractDesugaredTypeNameOfInterestOrFail(
    llvm::StringRef OriginalTOI, const LLVMProjectIRDB &IRDB,
    const stringstringmap_t &ForwardRenaming, llvm::StringRef ErrorMsg,
    int errorExitCode) {
  if (auto ret = extractDesugaredTypeNameOfInterest(OriginalTOI, IRDB,
                                                    ForwardRenaming)) {
    return *ret;
  }
  return "";
}

// bool psr::isOfType(const llvm::Type *OfTy, const llvm::Type *IsTy) {
//   while (IsTy) {
//     if (OfTy == IsTy)
//       return true;
//     if (!IsTy->isPointerTy())
//       return false;
//     IsTy = IsTy->getPointerElementType();
//   }
//   return false;
// }
