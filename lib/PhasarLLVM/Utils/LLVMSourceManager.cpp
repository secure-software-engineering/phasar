#include "phasar/PhasarLLVM/Utils/LLVMSourceManager.h"

#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/Logger.h"

#include "llvm/IR/DebugInfoMetadata.h"

#include <optional>

using namespace psr;

static std::optional<unsigned>
getSourceBufId(const llvm::DIFile *File,
               llvm::DenseMap<const llvm::DIFile *, unsigned> &FileIdMap,
               llvm::SourceMgr &SrcMgr) {
  if (auto It = FileIdMap.find(File); It != FileIdMap.end()) {
    return It->second;
  }

  auto FileName = psr::getFilePathFromIR(File);
  auto Buf = llvm::MemoryBuffer::getFile(FileName, true);
  if (!Buf) {
    PHASAR_LOG_LEVEL(WARNING, "Source File not accessible: " << FileName);
    PHASAR_LOG_LEVEL(INFO, "> " << Buf.getError().message());
    return std::nullopt;
  }

  auto Id = SrcMgr.AddNewSourceBuffer(std::move(Buf.get()), llvm::SMLoc{});
  FileIdMap.try_emplace(File, Id);
  return Id;
}

std::optional<ManagedDebugLocation>
LLVMSourceManager::getDebugLocation(const llvm::Value *V) {
  auto Loc = psr::getDebugLocation(V);
  if (!Loc) {
    return std::nullopt;
  }

  auto BufId = getSourceBufId(Loc->File, FileIdMap, SrcMgr);
  if (!BufId) {
    return std::nullopt;
  }

  return ManagedDebugLocation{Loc->Line, Loc->Column, *BufId};
}

void LLVMSourceManager::print(llvm::raw_ostream &OS, ManagedDebugLocation Loc,
                              llvm::SourceMgr::DiagKind DiagKind,
                              const llvm::Twine &Message) {
  auto SMLoc = SrcMgr.FindLocForLineAndColumn(Loc.File, Loc.Line, Loc.Column);

  SrcMgr.PrintMessage(OS, SMLoc, DiagKind, Message);
}
