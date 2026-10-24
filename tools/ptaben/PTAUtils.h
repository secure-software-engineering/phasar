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
#include "phasar/Utils/TypeTraits.h"
#include "phasar/Utils/Utilities.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/WithColor.h"

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
size_t checkDir(const llvm::Twine &DirName,
                llvm::SmallVectorImpl<std::string> &Failures, CheckFn Check)
  requires(std::is_invocable_r_v<bool, CheckFn, llvm::StringRef>)
{
  std::error_code EC;
  llvm::sys::fs::recursive_directory_iterator It(DirName, EC, false);
  llvm::sys::fs::recursive_directory_iterator End;

  size_t NumTests = 0;

  static constexpr auto IsLLVMIRFile = [](llvm::StringRef Path) {
    return Path.ends_with(".ll") || Path.ends_with(".bc");
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

  if (EC) {
    llvm::WithColor::error() << EC.message() << '\n';
  }

  return NumTests;
}

template <typename CheckFn>
void checkDirs(is_iterable_over_v<llvm::StringRef> auto &&DirNames,
               llvm::SmallVectorImpl<std::string> &Failures, CheckFn Check)
  requires(std::is_invocable_r_v<bool, CheckFn, llvm::StringRef>)
{

  size_t NumTests = 0;
  for (const auto &Path : DirNames) {
    NumTests += checkDir(Path, Failures, copyOrRef(Check));
  }

  llvm::outs() << "Analyzed " << NumTests << " Benchmark files\n";
}
} // namespace psr::ptaben
