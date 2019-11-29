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
#include <set>
#include <string>

#include <phasar/DB/ProjectIRDB.h>
#include <phasar/PhasarLLVM/AnalysisStrategy/Strategies.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

namespace psr {

class AnalysisController {
private:
  ProjectIRDB IRDB;
  LLVMTypeHierarchy TH;
  LLVMPointsToInfo PT;
  LLVMBasedICFG ICF;
  std::set<std::string> EntryPoints;

  void executeDemandDriven();
  void executeIncremental();
  void executeModuleWise();
  void executeVariational();
  void executeWholeProgram();

public:
  AnalysisController();
  ~AnalysisController() = default;
  AnalysisController(const AnalysisController &) = delete;
  AnalysisController(AnalysisController &&) = delete;

  void executeAs(AnalysisStrategy Strategy);
};

} // namespace psr

#endif
