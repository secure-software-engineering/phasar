#ifndef ANALYSIS_PLUGIN_H_
#define ANALYSIS_PLUGIN_H_

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

extern const set<string> AvailablePluginInterfaces;

class AnalysisPlugin {
public:
  AnalysisPlugin(const string &AnalysisIface, const string &AnalysisPlugin,
                 LLVMBasedICFG &ICFG, vector<string> EntryPoints);
};

#endif