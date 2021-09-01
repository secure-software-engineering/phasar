/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include <iostream>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/LeakedValue.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr::detail {

LeakedValueBase::LeakedValueBase(const llvm::Value *leak) : leak(leak) {
  assert(leak);
}

std::ostream &operator<<(std::ostream &os, const detail::LeakedValueBase &LV) {
  return os << llvmIRToString(LV.leak);
}
bool operator==(const detail::LeakedValueBase &LHS,
                const detail::LeakedValueBase &RHS) {
  return LHS.leak == RHS.leak;
}
bool operator<(const detail::LeakedValueBase &LHS,
               const detail::LeakedValueBase &RHS) {
  return llvmValueIDLess()(LHS.leak, RHS.leak);
}
} // namespace psr::detail