/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSLINEARCONSTANTANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IFDSLINEARCONSTANTANALYSIS_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {
class LLVMBasedICFG;

// A small pair data type to encode data flow facts for this LCA
struct LCAPair {
  const llvm::Value *first;
  int second;
  LCAPair();
  LCAPair(const llvm::Value *V, int i);
  friend bool operator==(const LCAPair &lhs, const LCAPair &rhs);
  friend bool operator!=(const LCAPair &lhs, const LCAPair &rhs);
  friend bool operator<(const LCAPair &lhs, const LCAPair &rhs);
};

} // namespace psr

// Specialize hash to be used in containers like std::unordered_map
namespace std {
template <> struct hash<psr::LCAPair> {
  std::size_t operator()(const psr::LCAPair &k) const;
};
} // namespace std

namespace psr {

class IFDSLinearConstantAnalysis
    : public DefaultIFDSTabulationProblem<const llvm::Instruction *, LCAPair,
                                          const llvm::Function *,
                                          LLVMBasedICFG &> {
private:
  std::vector<std::string> EntryPoints;

public:
  typedef LCAPair d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef LLVMBasedICFG &i_t;

  IFDSLinearConstantAnalysis(LLVMBasedICFG &icfg,
                             std::vector<std::string> EntryPoints = {"main"});

  virtual ~IFDSLinearConstantAnalysis() = default;

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         m_t destMthd) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        m_t calleeMthd,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>> getCallToRetFlowFunction(
      n_t callSite, n_t retSite,
      std::set<IFDSLinearConstantAnalysis::m_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, m_t destMthd) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() override;

  bool isZeroValue(d_t d) const override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;
};

} // namespace psr

#endif
