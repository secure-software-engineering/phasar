#include "phasar/ControlFlow/CallGraph.h"

template class psr::CallGraph<const llvm::Instruction *,
                              const llvm::Function *>;
template class psr::CallGraphBuilder<const llvm::Instruction *,
                                     const llvm::Function *>;
