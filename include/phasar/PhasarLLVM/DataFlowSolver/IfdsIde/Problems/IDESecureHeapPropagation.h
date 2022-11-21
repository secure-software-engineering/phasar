/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H

#include "llvm/ADT/StringRef.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

namespace llvm {
class Instruction;
class Value;
class StructType;
class Function;
} // namespace llvm

namespace psr {
enum class SecureHeapFact { ZERO, INITIALIZED };
enum class SecureHeapValue { TOP, INITIALIZED, BOT };

struct IDESecureHeapPropagationAnalysisDomain : LLVMAnalysisDomainDefault {
  using d_t = SecureHeapFact;
  using l_t = SecureHeapValue;
};

class IDESecureHeapPropagation
    : public IDETabulationProblem<IDESecureHeapPropagationAnalysisDomain> {

  const llvm::StringLiteral InitializerFn = "CRYPTO_secure_malloc_init";
  const llvm::StringLiteral ShutdownFn = "CRYPTO_secure_malloc_done";

public:
  using IDETabProblemType =
      IDETabulationProblem<IDESecureHeapPropagationAnalysisDomain>;
  using typename IDETabProblemType::d_t;
  using typename IDETabProblemType::f_t;
  using typename IDETabProblemType::i_t;
  using typename IDETabProblemType::l_t;
  using typename IDETabProblemType::n_t;
  using typename IDETabProblemType::t_t;
  using typename IDETabProblemType::v_t;

  IDESecureHeapPropagation(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints = {"main"});
  ~IDESecureHeapPropagation() override = default;

  FlowFunctionPtrType getNormalFlowFunction(n_t Curr, n_t Succ) override;

  FlowFunctionPtrType getCallFlowFunction(n_t CallSite, f_t DestFunc) override;

  FlowFunctionPtrType getRetFlowFunction(n_t CallSite, f_t CalleeMthd,
                                         n_t ExitInst, n_t RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(n_t CallSite, n_t RetSite,
                           llvm::ArrayRef<f_t> Callees) override;

  FlowFunctionPtrType getSummaryFlowFunction(n_t CallSite,
                                             f_t DestMthd) override;

  InitialSeeds<n_t, d_t, l_t> initialSeeds() override;

  [[nodiscard]] d_t createZeroValue() const override;

  [[nodiscard]] bool isZeroValue(d_t Fact) const override;

  void printNode(llvm::raw_ostream &OS, n_t Stmt) const override;

  void printDataFlowFact(llvm::raw_ostream &OS, d_t Fact) const override;

  void printFunction(llvm::raw_ostream &OS, f_t Func) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t Curr, d_t CurrNode, n_t Succ,
                        d_t SuccNode) override;

  std::shared_ptr<EdgeFunction<l_t>> getCallEdgeFunction(n_t CallSite,
                                                         d_t SrcNode,
                                                         f_t DestinationMethod,
                                                         d_t DestNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t CallSite, f_t CalleeMethod, n_t ExitInst,
                        d_t ExitNode, n_t RetSite, d_t RetNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                           d_t RetSiteNode,
                           llvm::ArrayRef<f_t> Callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t CallSite, d_t CallNode, n_t RetSite,
                         d_t RetSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t Lhs, l_t Rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printEdgeFact(llvm::raw_ostream &OS, l_t L) const override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      llvm::raw_ostream &OS) override;

  struct SHPEdgeFn : public EdgeFunction<l_t>,
                     public std::enable_shared_from_this<SHPEdgeFn> {
    ~SHPEdgeFn() override = default;

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> OtherFunction) override;
  };

  struct SHPEdgeFunctionComposer
      : public EdgeFunctionComposer<l_t>,
        public std::enable_shared_from_this<SHPEdgeFn> {
    ~SHPEdgeFunctionComposer() override = default;
    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> OtherFunction) override;
  };
  struct SHPGenEdgeFn : public SHPEdgeFn {
    SHPGenEdgeFn(l_t Val);
    ~SHPGenEdgeFn() override = default;

    l_t computeTarget(l_t Source) override;

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> Other) const override;
    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> SecondFunction) override;
    void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;
    static std::shared_ptr<SHPGenEdgeFn> getInstance(l_t Val);

  private:
    l_t Value;
  };

  struct IdentityEdgeFunction : public SHPEdgeFn {
    ~IdentityEdgeFunction() override = default;

    l_t computeTarget(l_t Source) override;
    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> SecondFunction) override;
    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> Other) const override;

    void print(llvm::raw_ostream &OS, bool IsForDebug = false) const override;

    static std::shared_ptr<IdentityEdgeFunction> getInstance();
  };
};
} // namespace psr

#endif
