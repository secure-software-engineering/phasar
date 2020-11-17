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

#include <set>
#include <string>
#include <vector>

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"

#include "boost/filesystem/path.hpp"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/VarStaticRenaming.h"

namespace llvm {
class AllocaInst;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class LLVMBasedICFG;

bool isValidLLVMIRFile(const boost::filesystem::path &FilePath);

std::vector<std::string> makeStringVectorFromPathVector(
    const std::vector<boost::filesystem::path> &Paths);

enum class OpenSSLEVPAnalysisType { CIPHER, MAC, MD };

OpenSSLEVPAnalysisType to_OpenSSLEVPAnalysisType(const std::string &Str);

std::set<std::string> getEntryPointsForCallersOf(const std::string &FunName,
                                                 ProjectIRDB &IR,
                                                 LLVMBasedICFG &ICF);

std::set<std::string>
getEntryPointsForCallersOfDesugared(const std::string &FunName, ProjectIRDB &IR,
                                    LLVMBasedICFG &ICF,
                                    const stringstringmap_t &FNameMap);

llvm::StringRef getBaseTypeNameIfUsingTypeDef(const llvm::AllocaInst *A);

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

template <typename Iter, typename EndIter> class SequencePrinter {
  Iter begIt;
  EndIter endIt;

public:
  template <typename It, typename EndIt>
  explicit SequencePrinter(It &&it, EndIt &&end)
      : begIt(std::forward<It>(it)), endIt(std::forward<EndIt>(end)) {}
  Iter begin() const { return begIt; }
  EndIter end() const { return endIt; }
  friend std::ostream &operator<<(std::ostream &OS, const SequencePrinter &SP) {
    OS << "[";
    auto it = SP.begIt;
    const auto end = SP.endIt;
    if (it != end) {
      OS << *it;
      while (++it != end)
        OS << ", " << *it;
    }
    return OS << "]";
  }
};

template <typename Iter, typename EndIter>
auto printAll(Iter &&it, EndIter &&endIt) {
  return SequencePrinter<std::decay_t<Iter>, std::decay_t<EndIter>>(
      std::forward<Iter>(it), std::forward<EndIter>(endIt));
}

template <typename ContainerTy> auto printAll(const ContainerTy &Cont) {
  return printAll(Cont.begin(), Cont.end());
}

} // namespace psr

#endif
