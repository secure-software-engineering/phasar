/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_ANALYSISPLUGINCONTROLLER_H_
#define PHASAR_PHASARLLVM_PLUGINS_ANALYSISPLUGINCONTROLLER_H_

#include <set>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace psr {

class ProjectIRDB;
class LLVMTypeHierarchy;
class LLVMBasedICFG;
class LLVMPointsToInfo;

class AnalysisPluginController {
public:
  AnalysisPluginController(std::vector<std::string> AnalysisPlygins,
                           const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints = {"main"});
};

} // namespace psr

#endif
