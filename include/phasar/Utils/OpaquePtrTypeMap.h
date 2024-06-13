#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "llvm/IR/Type.h"

#include <map>

namespace psr {

class OpaquePtrTypeInfoMap {
public:
  // maps ValueID of pointer to ValueID of variable
  std::map<unsigned int, unsigned int> TypeInfo;
  OpaquePtrTypeInfoMap(const psr::LLVMProjectIRDB *Code);
};

} // namespace psr
