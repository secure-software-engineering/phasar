#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Type.h"

#include <map>

namespace psr {

class OpaquePtrTypeInfoMap {
public:
  // maps name of pointer to name of variable
  std::map<const char *, llvm::StringRef> TypeInfo;
  OpaquePtrTypeInfoMap(const psr::LLVMProjectIRDB *Code);
};

} // namespace psr
