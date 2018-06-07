/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef ANALYSIS_PLUGIN_CONTROLLER_H_
#define ANALYSIS_PLUGIN_CONTROLLER_H_

#include <json.hpp>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIDESolver.h>
#include <phasar/PhasarLLVM/IfdsIde/Solver/LLVMIFDSSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMInterMonotoneSolver.h>
#include <phasar/PhasarLLVM/Mono/Solver/LLVMIntraMonotoneSolver.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSTabulationProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/InterMonotoneProblemPlugin.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/Mono/IntraMonotoneProblemPlugin.h>
#include <phasar/Utils/Logger.h>
#include <phasar/Utils/SOL.h>
#include <set>
#include <string>
#include <vector>
using namespace std;
using json = nlohmann::json;

namespace psr {

class AnalysisPluginController {
private:
  json &FinalResultsJson;

public:
  AnalysisPluginController(vector<string> AnalysisPlygins, LLVMBasedICFG &ICFG,
                           vector<string> EntryPoints, json &Results);
};

} // namespace psr

#endif