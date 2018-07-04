/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/Plugins/AnalysisPluginController.h>

using namespace std;
using namespace psr;

namespace psr {

AnalysisPluginController::AnalysisPluginController(
    vector<string> AnalysisPlygins, LLVMBasedICFG &ICFG,
    vector<string> EntryPoints, json &Results)
    : FinalResultsJson(Results) {
  auto &lg = lg::get();
  for (const auto &AnalysisPlugin : AnalysisPlygins) {
    SOL SharedLib(AnalysisPlugin);
    if (!IDETabulationProblemPluginFactory.empty()) {
      for (auto Problem : IDETabulationProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first);
      }
    }
    if (!IFDSTabulationProblemPluginFactory.empty()) {
      for (auto &Problem : IFDSTabulationProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first);
        unique_ptr<IFDSTabulationProblemPlugin> plugin(
            Problem.second(ICFG, {"main"}));
        cout << "DONE" << endl;
        LLVMIFDSSolver<const llvm::Value *, LLVMBasedICFG &> llvmifdstestsolver(
            *plugin, true);
        llvmifdstestsolver.solve();
        FinalResultsJson += llvmifdstestsolver.getAsJson();
      }
    }
    if (!InterMonotoneProblemPluginFactory.empty()) {
      for (auto Problem : InterMonotoneProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first);
      }
    }
    if (!IntraMonotoneProblemPluginFactory.empty()) {
      for (auto Problem : IntraMonotoneProblemPluginFactory) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, INFO) << "Solving plugin: " << Problem.first);
      }
    }
  }
}

} // namespace psr
