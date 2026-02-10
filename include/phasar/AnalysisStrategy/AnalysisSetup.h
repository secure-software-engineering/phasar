/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_ANALYSISSTRATEGY_ANALYSISSETUP_H
#define PHASAR_ANALYSISSTRATEGY_ANALYSISSETUP_H

namespace psr {
class LLVMAliasSet;
class LLVMBasedICFG;
class DIBasedTypeHierarchy;

/// Indicates that an analysis does not need a special configuration (file).
struct HasNoConfigurationType {};

struct AnalysisSetup {
  struct UnsupportedAnalysisType {};
  using PointerAnalysisTy = UnsupportedAnalysisType;
  using CallGraphAnalysisTy = UnsupportedAnalysisType;
  using TypeHierarchyTy = UnsupportedAnalysisType;
};

struct DefaultAnalysisSetup : AnalysisSetup {
  using PointerAnalysisTy = LLVMAliasSet;
  using CallGraphAnalysisTy = LLVMBasedICFG;
  using TypeHierarchyTy = DIBasedTypeHierarchy;
};

} // namespace psr

#endif
