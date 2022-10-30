/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDELINEARCONSTANTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDELINEARCONSTANTANALYSIS_H

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"

#include "llvm/Support/raw_ostream.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace llvm {
class Instruction;
class Function;
class StructType;
class Value;
} // namespace llvm

namespace psr {

struct IDELinearConstantAnalysisDomain : public LLVMAnalysisDomainDefault {
  // int64_t corresponds to llvm's type of constant integer
  using l_t = LatticeDomain<int64_t>;
};

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

class IDELinearConstantAnalysis
    : public IDETabulationProblem<IDELinearConstantAnalysisDomain> {

public:
  using IDETabProblemType =
      IDETabulationProblem<IDELinearConstantAnalysisDomain>;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  static const l_t TOP;
  static const l_t BOTTOM;

  IDELinearConstantAnalysis(const ProjectIRDB *IRDB, const LLVMBasedICFG *ICF,
                            std::set<std::string> EntryPoints = {"main"});

  ~IDELinearConstantAnalysis() override;

  IDELinearConstantAnalysis(const IDELinearConstantAnalysis &) = delete;
  IDELinearConstantAnalysis &
  operator=(const IDELinearConstantAnalysis &) = delete;

  struct LCAResult {
    LCAResult() = default;
    unsigned LineNr = 0;
    std::string SrcNode;
    std::map<std::string, l_t> VariableToValue;
    std::vector<n_t> IRTrace;
    void print(llvm::raw_ostream &OS);
    inline bool operator==(const LCAResult &Rhs) const {
      return SrcNode == Rhs.SrcNode && VariableToValue == Rhs.VariableToValue &&
             IRTrace == Rhs.IRTrace;
    }

    operator std::string() const {
      std::string Buffer;
      llvm::raw_string_ostream OS(Buffer);
      OS << "Line " << LineNr << ": " << SrcNode << '\n';
      OS << "Var(s): ";
      for (auto It = VariableToValue.begin(); It != VariableToValue.end();
           ++It) {
        if (It != VariableToValue.begin()) {
          OS << ", ";
        }
        OS << It->first << " = " << It->second;
      }
      OS << "\nCorresponding IR Instructions:\n";
      for (const auto *Ir : IRTrace) {
        OS << "  " << llvmIRToString(Ir) << '\n';
      }
      return OS.str();
    }
  };

  using lca_results_t = std::map<std::string, std::map<unsigned, LCAResult>>;

  static void stripBottomResults(std::unordered_map<d_t, l_t> &Res);

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFun) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeFun,
                                         n_t ExitInst, n_t RetSite) override;

  FlowFunctionPtrType getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                                               std::set<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestFun) override;

  [[nodiscard]] InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                        d_t SuccNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t CallSite, d_t SrcNode, f_t DestinationFunction,
                      d_t DestNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t CallSite, f_t CalleeFunction, n_t ExitStmt,
                        d_t ExitNode, n_t RetSite, d_t RetNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode, std::set<f_t> Callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                         d_t RetSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t Lhs, l_t Rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

private:
  const LLVMBasedICFG *ICF{};

  // Helper functions

  [[nodiscard]] bool isEntryPoint(const std::string &FunctionName) const;

  void printNode(llvm::raw_ostream &OS, n_t Stmt) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Func) const override;

  void printEdgeFact(llvm::raw_ostream &OS, l_t L) const override;

  [[nodiscard]] lca_results_t getLCAResults(SolverResults<n_t, d_t, l_t> SR);

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS = llvm::outs()) override;
};

} // namespace psr

#endif
