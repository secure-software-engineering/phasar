#include "phasar/PhasarLLVM/Pointer/AliasInfo.h"

#include "phasar/PhasarLLVM/Pointer/AliasInfoBase.h"
#include "phasar/PhasarLLVM/Pointer/DynamicAliasSetPtr.h"

namespace psr {
static_assert(
    IsAliasInfo<AliasInfoRef<const llvm::Value *, const llvm::Instruction *>>);

template class AliasInfoRef<const llvm::Value *, const llvm::Instruction *>;
template class AliasInfo<const llvm::Value *, const llvm::Instruction *>;
} // namespace psr
