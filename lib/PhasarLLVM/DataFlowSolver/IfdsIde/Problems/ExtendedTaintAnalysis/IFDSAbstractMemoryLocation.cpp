/******************************************************************************
 * Copyright (c) 2020 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#include "llvm/IR/Operator.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/ExtendedTaintAnalysis/IFDSAbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/Utils/BasicBlockOrdering.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

namespace psr {
IFDSAbstractMemoryLocation::IFDSAbstractMemoryLocation(
    const detail::AbstractMemoryLocationImpl *Impl,
    const llvm::Instruction *Sani)
    : AbstractMemoryLocation(Impl), loadSanitizer(Sani) {}

[[nodiscard]] IFDSAbstractMemoryLocation
IFDSAbstractMemoryLocation::withSanitizedLoad(
    const llvm::Instruction *Sani) const {
  return IFDSAbstractMemoryLocation(pImpl, Sani);
}

bool IFDSAbstractMemoryLocation::isLoadSanitized(
    const llvm::Instruction *CurrInst) const {
  static BasicBlockOrdering Order;
  if (CurrInst == nullptr) {
    Order.clear();
    return false;
  }

  if (loadSanitizer == nullptr)
    return false;

  if (CurrInst->getFunction() != loadSanitizer->getFunction()) {
    // TODO: Check whether it is worth having a partial ordering on functions
    // too
    return false;
  }

  return Order.mustComeBefore(loadSanitizer, CurrInst);
}

llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                              const IFDSAbstractMemoryLocation &TV) {
  // TODO: better representation
  os << "(";
  if (LLVMZeroValue::getInstance()->isLLVMZeroValue(TV->base())) {
    os << "<ZERO>";
  } else {
    os << llvmIRToShortString(TV->base());
  }
  os << "; Offsets=" << SequencePrinter(TV->offsets());
  if (TV.loadSanitizer) {
    os << ", L(";
    TV.loadSanitizer->print(os, getModuleSlotTrackerFor(TV.loadSanitizer));
    os << ")";
  }
  return os << ")";
}

std::string DToString(const IFDSAbstractMemoryLocation &AML) {
  std::string ret;
  llvm::raw_string_ostream os(ret);
  os << AML;
  return os.str();
}
} // namespace psr

namespace llvm {
hash_code hash_value(const psr::IFDSAbstractMemoryLocation &Val) {
  return hash_combine(
      Val->base(), Val->lifetime() == 0,
      hash_combine_range(Val->offsets().begin(), Val->offsets().end()),
      Val.getLoadSanitizer());
}
} // namespace llvm