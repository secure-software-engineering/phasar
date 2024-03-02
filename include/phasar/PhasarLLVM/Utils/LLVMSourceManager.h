#ifndef PHASAR_PHASARLLVM_UTILS_LLVMSOURCEMANAGER_H
#define PHASAR_PHASARLLVM_UTILS_LLVMSOURCEMANAGER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/SourceMgr.h"

#include <optional>

namespace llvm {
class Value;
class DIFile;
} // namespace llvm

namespace psr {
struct ManagedDebugLocation {
  unsigned Line{};
  unsigned Column{};
  unsigned File{};
};

class LLVMSourceManager {
public:
  [[nodiscard]] std::optional<ManagedDebugLocation>
  getDebugLocation(const llvm::Value *V);

  void print(llvm::raw_ostream &OS, ManagedDebugLocation Loc,
             llvm::SourceMgr::DiagKind DiagKind, const llvm::Twine &Message);

private:
  llvm::DenseMap<const llvm::DIFile *, unsigned> FileIdMap{};
  llvm::SourceMgr SrcMgr{};
};
} // namespace psr

#endif // PHASAR_PHASARLLVM_UTILS_LLVMSOURCEMANAGER_H
