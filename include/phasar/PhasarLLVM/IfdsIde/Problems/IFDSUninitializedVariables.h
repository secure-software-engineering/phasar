/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSUNINITIALIZEDVARIABLES_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSUNINITIALIZEDVARIABLES_H_

#include <map>
#include <memory>
#include <set>
#include <string>

#include <phasar/PhasarLLVM/IfdsIde/LLVMDefaultIFDSTabulationProblem.h>

namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {
class LLVMBasedICFG;

class IFDSUninitializedVariables
    : public LLVMDefaultIFDSTabulationProblem<const llvm::Value *,
                                              LLVMBasedICFG &> {
public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedICFG &i_t;

private:
  struct UninitResult {
    UninitResult() = default;
    unsigned int line = 0;
    std::string func_name;
    std::string file_path;
    std::string src_code;
    std::vector<std::string> var_names;
    std::map<IFDSUninitializedVariables::n_t,
             std::set<IFDSUninitializedVariables::d_t>>
        ir_trace;
    bool empty();
    void print(std::ostream &os);
  };
  std::map<n_t, std::set<d_t>> UndefValueUses;
  std::vector<std::string> EntryPoints;

public:
  IFDSUninitializedVariables(i_t icfg, const LLVMTypeHierarchy &th,
                             const ProjectIRDB &irdb,
                             std::vector<std::string> EntryPoints = {"main"});

  ~IFDSUninitializedVariables() override = default;

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

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;

  void emitTextReport(std::ostream &os,
                      const SolverResults<n_t, d_t, BinaryDomain> &SR) override;

  const std::map<n_t, std::set<d_t>> &getAllUndefUses() const;

  std::vector<UninitResult> aggregateResults();
};

} // namespace psr

#endif
