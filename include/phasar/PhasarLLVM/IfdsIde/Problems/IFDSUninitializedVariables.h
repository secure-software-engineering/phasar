/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSSummaryPool.h>

namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class IFDSUnitializedVariables
    : public DefaultIFDSTabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, LLVMBasedICFG &> {

public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedICFG &i_t;

private:
  IFDSSummaryPool<d_t, n_t> dynSum;
  std::vector<std::string> EntryPoints;

public:
  IFDSUnitializedVariables(i_t icfg,
                           std::vector<std::string> EntryPoints = {"main"});

  virtual ~IFDSUnitializedVariables() = default;

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         m_t destMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        m_t calleeMthd,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<m_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  std::string DtoString(d_t d) const override;

  std::string NtoString(n_t n) const override;

  std::string MtoString(m_t m) const override;
};

} // namespace psr
