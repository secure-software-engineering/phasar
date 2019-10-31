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

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMTypeHierarchy.h>
#include <phasar/PhasarLLVM/Pointer/PointsToGraph.h>

namespace psr {

struct AnalysisSetup {
  struct UnsupportedAnalysisType {};
  using PointerAnalysisTy = UnsupportedAnalysisType;
  using CallGraphAnalysisTy = UnsupportedAnalysisType;
  using TypeHierarchyTy = UnsupportedAnalysisType;
};

struct DefaultAnalysisSetup : AnalysisSetup {
  using PointerAnalysisTy = PointsToGraph;
  using CallGraphAnalysisTy = LLVMBasedICFG;
  using TypeHierarchyTy = LLVMTypeHierarchy;
};

} // namespace psr

#endif
