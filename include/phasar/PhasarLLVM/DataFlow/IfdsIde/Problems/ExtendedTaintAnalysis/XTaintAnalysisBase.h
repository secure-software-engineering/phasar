/******************************************************************************
 * Copyright (c) 2021 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTANALYSISBASE_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_EXTENDEDTAINTANALYSIS_XTAINTANALYSISBASE_H

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocation.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/ExtendedTaintAnalysis/AbstractMemoryLocationFactory.h"

#include "llvm/ADT/SmallPtrSet.h"

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {
class LLVMTaintConfig;
} // namespace psr

namespace psr::XTaint {
class AnalysisBase {
protected:
  const LLVMTaintConfig *TSF;
  AbstractMemoryLocationFactory<AbstractMemoryLocation> FactFactory;

  explicit AnalysisBase(const LLVMTaintConfig *TSF, size_t NumInstructions);

  using SourceConfigTy = llvm::SmallPtrSet<const llvm::Value *, 4>;
  using SinkConfigTy = llvm::SmallPtrSet<const llvm::Value *, 4>;
  using SanitizerConfigTy = SinkConfigTy;

  [[nodiscard]] std::pair<SourceConfigTy, SinkConfigTy>
  getConfigurationAt(const llvm::Instruction *Inst,
                     const llvm::Function *Callee = nullptr) const;

  [[nodiscard]] SourceConfigTy
  getSourceConfigAt(const llvm::Instruction *Inst,
                    const llvm::Function *Callee = nullptr) const;

  [[nodiscard]] SinkConfigTy
  getSinkConfigAt(const llvm::Instruction *Inst,
                  const llvm::Function *Callee = nullptr) const;

  [[nodiscard]] bool isSink(const llvm::Value *SinkCandidate,
                            const llvm::Instruction *AtInst) const;

  [[nodiscard]] SanitizerConfigTy
  getSanitizerConfigAt(const llvm::Instruction *Inst,
                       const llvm::Function *Callee = nullptr) const;

  [[nodiscard]] AbstractMemoryLocation createZeroValue() const {
    return FactFactory.getOrCreateZero();
  }
};
} // namespace psr::XTaint

#endif
