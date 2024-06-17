
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"

#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include <map>

namespace psr {

class IRDBOpaquePtrTypes {
public:
  IRDBOpaquePtrTypes(const LLVMProjectIRDB *IRDB);
  const llvm::Type *getTypeOfPtr(const llvm::Value *Value);

private:
  std::map<const llvm::Value *, const llvm::DIType *> ValueToDIType;
  std::map<const llvm::DIType *, const llvm::Value *> DITypeToValue;
  std::map<const llvm::DISubprogram *,
           std::vector<const llvm::DILocalVariable *>>
      SubprogamVars;
  std::map<const llvm::Value *, const llvm::Type *> ValueToType;

  const llvm::DIType *getBaseType(const llvm::DIDerivedType *DerivedTy);
  const llvm::Type *getPtrType(const llvm::Value *Value);
};

} // namespace psr