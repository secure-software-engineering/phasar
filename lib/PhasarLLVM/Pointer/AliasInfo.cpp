#include "phasar/PhasarLLVM/Pointer/AliasInfo.h"

namespace psr {
template class AliasInfoBase<
    AliasInfoRef<const llvm::Value *, const llvm::Instruction *>>;
template class AliasInfoRef<const llvm::Value *, const llvm::Instruction *>;
template class AliasInfo<const llvm::Value *, const llvm::Instruction *>;
} // namespace psr
