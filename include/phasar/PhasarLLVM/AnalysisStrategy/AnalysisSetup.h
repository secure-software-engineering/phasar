/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_ANALYSISSTRATEGY_ANALYSISSETUP_H_
#define PHASAR_PHASARLLVM_ANALYSISSTRATEGY_ANALYSISSETUP_H_

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToSet.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

namespace psr {

// Indicates that an analysis does not need a special configuration (file).
struct HasNoConfigurationType {};

struct AnalysisSetup {
  struct UnsupportedAnalysisType {};
  using PointerAnalysisTy = UnsupportedAnalysisType;
  using CallGraphAnalysisTy = UnsupportedAnalysisType;
  using TypeHierarchyTy = UnsupportedAnalysisType;
};

struct DefaultAnalysisSetup : AnalysisSetup {
  using PointerAnalysisTy = LLVMPointsToSet;
  using CallGraphAnalysisTy = LLVMBasedICFG;
  using TypeHierarchyTy = LLVMTypeHierarchy;
};

} // namespace psr

#endif
