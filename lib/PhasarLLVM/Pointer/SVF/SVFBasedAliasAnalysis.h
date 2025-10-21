/******************************************************************************
 * Copyright (c) 2025 Fabian Schiebel.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_SVFBASEDALIASANALYSIS_H
#define PHASAR_PHASARLLVM_POINTER_SVFBASEDALIASANALYSIS_H

#include "phasar/PhasarLLVM/Pointer/AliasAnalysisView.h"

#include <memory>

namespace psr {
[[nodiscard]] std::unique_ptr<AliasAnalysisView>
createSVFVFSAnalysis(LLVMProjectIRDB &IRDB);

[[nodiscard]] std::unique_ptr<AliasAnalysisView>
createSVFDDAAnalysis(LLVMProjectIRDB &IRDB);
} // namespace psr

#endif // PHASAR_PHASARLLVM_POINTER_SVFBASEDALIASANALYSIS_H
