/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IFDSLINEARCONSTANTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IFDSLINEARCONSTANTANALYSIS_H

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
  const llvm::Value *First{nullptr};
  int Second{0};
  LCAPair() = default;
  LCAPair(const llvm::Value *V, int I);
  friend bool operator==(const LCAPair &Lhs, const LCAPair &Rhs);
  friend bool operator!=(const LCAPair &Lhs, const LCAPair &Rhs);
  friend bool operator<(const LCAPair &Lhs, const LCAPair &Rhs);
};

} // namespace psr

// Specialize hash to be used in containers like std::unordered_map
namespace std {
template <> struct hash<psr::LCAPair> {
  std::size_t operator()(const psr::LCAPair &K) const;
};
} // namespace std

namespace psr {

struct IFDSLinearConstantAnalysisDomain : public LLVMIFDSAnalysisDomainDefault {
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

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitStmt, n_t RetSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(
      n_t CallSite, n_t RetSite,
      std::set<IFDSLinearConstantAnalysis::f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const override;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  void printNode(std::ostream &OS, n_t Stmt) const override;

  void printDataFlowFact(std::ostream &OS, d_t Fact) const override;

  void printFunction(std::ostream &OS, f_t Func) const override;
};

} // namespace psr

#endif
