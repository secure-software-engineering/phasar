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

#include <string>
#include <vector>

#include <json.hpp>

namespace psr {

class LLVMBasedICFG;

using json = nlohmann::json;

class AnalysisPluginController {
private:
  json &FinalResultsJson;

public:
  AnalysisPluginController(std::vector<std::string> AnalysisPlygins,
                           LLVMBasedICFG &ICFG,
                           std::vector<std::string> EntryPoints, json &Results);
};

} // namespace psr

#endif
