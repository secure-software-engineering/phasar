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

#include <json.hpp>

#include <phasar/PhasarLLVM/Pointer/PointsToInfo.h>

namespace llvm {
class Value;
class Function;
} // namespace llvm

namespace psr {

class ProjectIRDB;
class PointsToGraph;

class LLVMPointsToInfo : public PointsToInfo<const llvm::Value *> {
private:
  std::map<const llvm::Function *, std::unique_ptr<PointsToGraph>>
      PointsToGraphs;

public:
  LLVMPointsToInfo(ProjectIRDB &IRDB);

  ~LLVMPointsToInfo() override = default;

  AliasResult alias(const llvm::Value *V1,
                    const llvm::Value *V2) const override;

  std::set<const llvm::Value *>
  getPointsToSet(const llvm::Value *V1) const override;

  nlohmann::json getAsJson() const override;

  PointsToGraph *getPointsToGraph(const llvm::Function *F) const;
};

} // namespace psr

#endif
