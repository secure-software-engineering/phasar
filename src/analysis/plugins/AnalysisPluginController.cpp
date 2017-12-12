/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "AnalysisPluginController.h"

AnalysisPluginController::AnalysisPluginController(
    vector<string> AnalysisPlygins, LLVMBasedICFG &ICFG,
    vector<string> EntryPoints) {
  auto &lg = lg::get();
  for (const auto &AnalysisPlugin : AnalysisPlygins) {
    SharedObjectLibraries.push_back(SOL(AnalysisPlugin));
  }
  if (!IDETabulationProblemPluginFactory.empty()) {
    for (auto Problem : IDETabulationProblemPluginFactory) {
      BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first;
    }
  }
  if (!IFDSTabulationProblemPluginFactory.empty()) {
    for (auto Problem : IFDSTabulationProblemPluginFactory) {
      BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first;
      // unique_ptr<IFDSTabulationProblemPlugin> plugin(Problem.second(ICFG,
      // EntryPoints)); LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &>
      // llvmifdstestsolver(
      //       *plugin, true);
      // llvmifdstestsolver.solve();
    }
  }
  if (!InterMonotoneProblemPluginFactory.empty()) {
    for (auto Problem : InterMonotoneProblemPluginFactory) {
      BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first;
    }
  }
  if (!IntraMonotoneProblemPluginFactory.empty()) {
    for (auto Problem : IntraMonotoneProblemPluginFactory) {
      BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first;
    }
  }
}
