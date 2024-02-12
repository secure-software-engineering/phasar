#include "phasar/PhasarLLVM/Utils/SourceMgrPrinter.h"

#include <llvm/Support/SourceMgr.h>

namespace psr {

llvm::StringMap<unsigned> FileNameIDMap{};
llvm::SourceMgr SrcMgr{};

std::optional<unsigned> getSourceBufId(llvm::StringRef FileName) {
  if (auto It = FileNameIDMap.find(FileName); It != FileNameIDMap.end()) {
    return It->second;
  }

  auto Buf = llvm::MemoryBuffer::getFile(FileName, true);
  if (!Buf) {
    PHASAR_LOG_LEVEL(WARNING, "File not accessible: " << FileName);
    return std::nullopt;
  }

  auto Id = SrcMgr.AddNewSourceBuffer(std::move(Buf.get()), llvm::SMLoc{});
  FileNameIDMap.try_emplace(FileName, Id);
  return Id;
}
} // namespace psr
