module;

#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"

export module phasar.llvm.domain;

export namespace psr {
using psr::LLVMAnalysisDomainDefault;
using LLVMIFDSAnalysisDomainDefault =
    WithBinaryValueDomain<LLVMAnalysisDomainDefault>;
} // namespace psr
