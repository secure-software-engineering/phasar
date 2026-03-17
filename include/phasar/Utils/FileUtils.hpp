#pragma once

#include "llvm/ADT/Twine.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

namespace psr {
std::unique_ptr<llvm::raw_ostream>
openFileForWrite(const llvm::Twine &FilePath);
} // namespace psr
