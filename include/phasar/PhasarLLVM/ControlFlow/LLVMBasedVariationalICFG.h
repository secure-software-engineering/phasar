#pragma once

#include "LLVMBasedICFG.h"
#include "LLVMBasedVariationalCFG.h"
#include "VariationalICFG.h"

namespace psr {
class LLVMBasedVariationalICFG : public virtual VariationalICFG,
                                 public virtual LLVMBasedICFG,
                                 public virtual LLVMBasedVariationalCFG {};
} // namespace psr