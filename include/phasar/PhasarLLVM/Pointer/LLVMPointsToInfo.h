/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_
#define PHASAR_PHASARLLVM_POINTER_LLVMPOINTSTOINFO_H_

#include <map>
#include <memory>
#include <string>

namespace psr {

class ProjectIRDB;
class PointstoGraph;

class LLVMPointsToInfo {
private:
  std::map<std::string, std::unique_ptr<PointsToGraph>> PointsToGraphs;

public:
  LLVMPointsToInfo(ProjectIRDB &IRDB);
  ~LLVMPointsToInfo() = default;

  PointsToGraph *getPointsToGraph(const std::string &FunctionName) const;
};

} // namespace psr

#endif
