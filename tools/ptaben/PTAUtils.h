#pragma once

/******************************************************************************
 * Copyright (c) 2026 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "phasar/Pointer/AliasResult.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"

#include "QueryLocation.h"

#include <functional>
#include <type_traits>

namespace llvm {
class Module;
class Value;
class Instruction;
class raw_ostream;
} // namespace llvm

namespace psr::ptaben {
struct QueryResult : QueryLocation {
  AliasResult Result{};

  friend llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                       const QueryResult &Result);
};

struct QuerySrcCodeLocation {
  std::string FileName{};
  size_t QueryIndex{};
};

[[nodiscard]] bool isSoundResult(const QueryResult &QR) noexcept;

void findAllQueryLocations(
    const llvm::Module &Mod, llvm::SmallVectorImpl<QueryLocation> &Locs,
    llvm::SmallVectorImpl<QuerySrcCodeLocation> *SrcLocs = nullptr);

[[nodiscard]] bool verifyAnalysisResult(AliasResult Res,
                                        const QueryLocation &QueryLoc);

template <typename CheckFn>
std::enable_if_t<std::is_invocable_r_v<bool, CheckFn, llvm::StringRef>>
checkDir(const llvm::Twine &DirName,
         llvm::SmallVectorImpl<std::string> &Failures, CheckFn Check) {
  std::error_code EC;
  llvm::sys::fs::recursive_directory_iterator It(DirName, EC, false);
  llvm::sys::fs::recursive_directory_iterator End;

  size_t NumTests = 0;

  static constexpr auto IsLLVMIRFile = [](llvm::StringRef Path) {
    return Path.endswith(".ll") || Path.endswith(".bc");
  };

  for (; It != End && !EC; It.increment(EC)) {
    auto Ty = It->type();
    llvm::StringRef Path = It->path();
    if (Ty == llvm::sys::fs::file_type::regular_file && IsLLVMIRFile(Path)) {
      ++NumTests;

      if (!std::invoke(Check, Path)) {
        Failures.push_back(Path.str());
      }
    }
  }

  llvm::outs() << "Analyzed " << NumTests << " Benchmark files\n";

  if (EC) {
    llvm::errs() << "[ERROR]: " << EC.message() << '\n';
  }
}
} // namespace psr::ptaben
