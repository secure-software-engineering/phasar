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

#include "../../utils/Logger.h"
#include "../../utils/SOL.h"
#include "../control_flow/LLVMBasedICFG.h"
#include "../ifds_ide/solver/LLVMIDESolver.h"
#include "../ifds_ide/solver/LLVMIFDSSolver.h"
#include "../monotone/solver/LLVMInterMonotoneSolver.h"
#include "../monotone/solver/LLVMIntraMonotoneSolver.h"
#include "plugin_ifaces/IDETabulationProblemPlugin.h"
#include "plugin_ifaces/IFDSTabulationProblemPlugin.h"
#include "plugin_ifaces/InterMonotoneProblemPlugin.h"
#include "plugin_ifaces/IntraMonotoneProblemPlugin.h"
#include <set>
#include <string>
#include <vector>
using namespace std;

class AnalysisPluginController {
private:
  vector<SOL> SharedObjectLibraries;

public:
  AnalysisPluginController(vector<string> AnalysisPlygins, LLVMBasedICFG &ICFG,
                           vector<string> EntryPoints);
};

#endif