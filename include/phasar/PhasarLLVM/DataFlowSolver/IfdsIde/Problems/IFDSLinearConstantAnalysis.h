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

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"

// Forward declaration of types for which we only use its pointer or ref type
namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

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

struct IFDSLinearConstantAnalysisDomain : public LLVMAnalysisDomainDefault {
  using d_t = LCAPair;
};

class IFDSLinearConstantAnalysis
    : public IFDSTabulationProblem<IFDSLinearConstantAnalysisDomain> {
public:
  IFDSLinearConstantAnalysis(const ProjectIRDB *IRDB,
                             const LLVMTypeHierarchy *TH,
                             const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                             std::set<std::string> EntryPoints = {"main"});

  ~IFDSLinearConstantAnalysis() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t callStmt, f_t destFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeFun,
                                         n_t exitStmt, n_t retSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(
      n_t callSite, n_t retSite,
      std::set<IFDSLinearConstantAnalysis::f_t> callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt,
                                             f_t destFun) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t m) const override;
};

} // namespace psr

#endif
