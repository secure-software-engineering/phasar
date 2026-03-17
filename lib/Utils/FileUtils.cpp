#include "phasar/Utils/FileUtils.hpp"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <system_error>

std::unique_ptr<llvm::raw_ostream>
psr::openFileForWrite(const llvm::Twine &FilePath) {
  llvm::SmallString<256> Buf;
  auto FileName = FilePath.toStringRef(Buf);

  std::error_code EC;
  auto Ret = std::make_unique<llvm::raw_fd_ostream>(FileName, EC);
  if (EC) {
    llvm::errs() << "[ERROR]: Failed to open file '" << FileName
                 << "' for writing:\n";
    llvm::errs() << "  > " << EC.message() << '\n';
    Ret.reset();
  }

  return Ret;
}
