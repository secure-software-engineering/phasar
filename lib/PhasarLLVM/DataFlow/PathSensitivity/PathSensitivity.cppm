module;

#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/LLVMPathConstraints.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitivityConfig.h"
#include "phasar/PhasarLLVM/DataFlow/PathSensitivity/Z3BasedPathSensitvityManager.h"

export module phasar.llvm.dataflow.pathsensitivity;

export namespace psr {
using psr::LLVMPathConstraints;
using psr::Z3BasedPathSensitivityConfig;
using psr::Z3BasedPathSensitivityManager;
using psr::Z3BasedPathSensitivityManagerBase;
} // namespace psr

export namespace llvm {
using llvm::AllocaInst;
using llvm::BinaryOperator;
using llvm::BranchInst;
using llvm::CallBase;
using llvm::CmpInst;
using llvm::GetElementPtrInst;
using llvm::Instruction;
using llvm::LoadInst;
using llvm::PHINode;
using llvm::Value;
} // namespace llvm
