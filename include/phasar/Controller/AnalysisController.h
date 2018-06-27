/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <iosfwd>
#include <map>
#include <vector>

#include <json.hpp>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/Utils/DataFlowAnalysisType.h>

using json = nlohmann::json;
namespace psr {

enum class ExportType { JSON = 0 };

extern const std::map<std::string, ExportType> StringToExportType;

extern const std::map<ExportType, std::string> ExportTypeToString;

std::ostream &operator<<(std::ostream &os, const ExportType &e);

class AnalysisController {
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
