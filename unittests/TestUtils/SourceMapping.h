#ifndef UNITTEST_TESTUTILS_SOURCEMAPPING_H
#define UNITTEST_TESTUTILS_SOURCEMAPPING_H

#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"
#include "phasar/Utils/TypeTraits.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"

#include <functional>

namespace psr::unittest {

template <typename PredFn = psr::TrueFn>
inline const llvm::Instruction *
getInstAtOrNull(const llvm::Function *F, uint32_t ReqLine,
                uint32_t ReqColumn = 0, PredFn Pred = {}) {
  assert(F != nullptr);
  for (const auto &I : llvm::instructions(F)) {
    auto [Line, Column] = psr::getLineAndColFromIR(&I);
    if (Line == ReqLine && (ReqColumn == 0 || ReqColumn == Column) &&
        std::invoke(Pred, &I)) {
      return &I;
    }
  }
  return nullptr;
}

} // namespace psr::unittest

#endif // UNITTEST_TESTUTILS_SOURCEMAPPING_H
