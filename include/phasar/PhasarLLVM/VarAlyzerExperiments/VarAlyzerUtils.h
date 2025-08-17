/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_VARALYZEREXPERIMENTS_VARALYZERUTILS_H_
#define PHASAR_VARALYZEREXPERIMENTS_VARALYZERUTILS_H_

#include "phasar/PhasarLLVM/VarStaticRenaming.h"

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"

#include <optional>
#include <set>
#include <string>
#include <vector>

namespace llvm {
class AllocaInst;
} // namespace llvm

namespace psr {

class LLVMProjectIRDB;
class LLVMBasedICFG;

// bool isValidLLVMIRFile(const boost::filesystem::path &FilePath);

// std::vector<std::string> makeStringVectorFromPathVector(
//     const std::vector<boost::filesystem::path> &Paths);

std::vector<std::string>
getEntryPointsForCallersOf(llvm::StringRef FunName, const LLVMProjectIRDB &IR,
                           const LLVMBasedICFG &ICF,
                           llvm::StringRef TypeNameOfInterest);

std::vector<std::string> getEntryPointsForCallersOfDesugared(
    llvm::StringRef FunName, const LLVMProjectIRDB &IR,
    const LLVMBasedICFG &ICF, const stringstringmap_t &FNameMap,
    llvm::StringRef TypeNameOfInterest);

std::optional<llvm::StringRef>
extractDesugaredTypeNameOfInterest(llvm::StringRef OriginalTOI,
                                   const LLVMProjectIRDB &IRDB,
                                   const stringstringmap_t &ForwardRenaming);

llvm::StringRef extractDesugaredTypeNameOfInterestOrFail(
    llvm::StringRef OriginalTOI, const LLVMProjectIRDB &IRDB,
    const stringstringmap_t &ForwardRenaming, llvm::StringRef ErrorMsg,
    int errorExitCode = 1);

llvm::StringRef staticRename(llvm::StringRef Name,
                             const stringstringmap_t &Renaming);
template <typename Iter, typename EndIter>
std::set<std::string> staticRenameAll(Iter NamesBegin, EndIter &&NamesEnd,
                                      const stringstringmap_t &Renaming) {
  std::set<std::string> ret;
  while (NamesBegin != NamesEnd) {
    ret.insert(staticRename(*NamesBegin, Renaming).str());
    ++NamesBegin;
  }
  return ret;
}

// bool isOfType(const llvm::Type *OfTy, const llvm::Type *IsTy);

} // namespace psr

#endif
