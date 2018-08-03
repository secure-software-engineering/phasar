/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_CONTROLLER_ANALYSIS_CONTROLLER_H_
#define PHASAR_CONTROLLER_ANALYSIS_CONTROLLER_H_

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include <json.hpp>

#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>

namespace psr {

class ProjectIRDB;

enum class ExportType { JSON = 0 };

extern const std::map<std::string, ExportType> StringToExportType;

extern const std::map<ExportType, std::string> ExportTypeToString;

std::ostream &operator<<(std::ostream &os, const ExportType &e);

class AnalysisController {
public:
  using json = nlohmann::json;

private:
  json FinalResultsJson;

public:
  AnalysisController(ProjectIRDB &&IRDB,
                     std::vector<DataFlowAnalysisType> Analyses,
                     bool WPA_MODE = true, bool PrintEdgeRecorder = true,
                     std::string graph_id = "");
  ~AnalysisController() = default;
  void writeResults(std::string filename);
};

} // namespace psr

#endif
