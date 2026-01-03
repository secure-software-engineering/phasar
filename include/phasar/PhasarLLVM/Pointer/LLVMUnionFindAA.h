#pragma once

#include "phasar/PhasarLLVM/Pointer/LLVMPointerAssignmentGraph.h"
#include "phasar/Pointer/UnionFindAA.h"

namespace psr {
extern template class CallingContextSensUnionFindAA<LLVMPAGDomain>;
extern template class IndirectionSensUnionFindAA<LLVMPAGDomain>;
} // namespace psr
