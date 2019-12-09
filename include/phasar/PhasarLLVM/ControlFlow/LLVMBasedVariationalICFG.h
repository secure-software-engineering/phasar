#pragma once

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedVariationalCFG.h>
#include <phasar/PhasarLLVM/ControlFlow/VariationalICFG.h>

namespace psr {
class LLVMBasedVariationalICFG : public virtual VariationalICFG,
                                 public virtual LLVMBasedICFG,
                                 public virtual LLVMBasedVariationalCFG {};
} // namespace psr