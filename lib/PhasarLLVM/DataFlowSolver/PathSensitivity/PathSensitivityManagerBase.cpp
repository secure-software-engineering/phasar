#include "phasar/PhasarLLVM/DataFlowSolver/PathSensitivity/PathSensitivityManagerBase.h"

namespace psr {
template class PathSensitivityManagerBase<const llvm::Instruction *>;
} // namespace psr
