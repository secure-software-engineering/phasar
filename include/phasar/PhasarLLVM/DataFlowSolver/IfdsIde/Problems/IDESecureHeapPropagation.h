/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Fabian Schiebel and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDESECUREHEAPPROPAGATION_H_

#include "llvm/ADT/StringRef.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
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
class IDESecureHeapPropagation
    : public IDETabulationProblem<const llvm::Instruction *, SecureHeapFact,
                                  const llvm::Function *,
                                  const llvm::StructType *, const llvm::Value *,
                                  SecureHeapValue, LLVMBasedICFG> {
  const llvm::StringLiteral initializerFn = "CRYPTO_secure_malloc_init";
  const llvm::StringLiteral shutdownFn = "CRYPTO_secure_malloc_done";

public:
  typedef SecureHeapFact d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef SecureHeapValue l_t;
  typedef LLVMBasedICFG i_t;
  IDESecureHeapPropagation(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                           const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                           std::set<std::string> EntryPoints = {"main"});
  ~IDESecureHeapPropagation() override = default;

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

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  void printNode(std::ostream &os, n_t n) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, m_t m) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  std::shared_ptr<EdgeFunction<l_t>> getCallEdgeFunction(n_t callStmt,
                                                         d_t srcNode,
                                                         m_t destinationMethod,
                                                         d_t destNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, m_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<m_t> callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  l_t join(l_t lhs, l_t rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printEdgeFact(std::ostream &os, l_t l) const override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      std::ostream &os) override;

  struct SHPEdgeFn : public EdgeFunction<l_t>,
                     public std::enable_shared_from_this<SHPEdgeFn> {
    virtual ~SHPEdgeFn() = default;

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override;
  };

  struct SHPEdgeFunctionComposer
      : public EdgeFunctionComposer<l_t>,
        public std::enable_shared_from_this<SHPEdgeFn> {
    virtual ~SHPEdgeFunctionComposer() = default;
    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override;
  };
  struct SHPGenEdgeFn : public SHPEdgeFn {
    SHPGenEdgeFn(l_t val);
    virtual ~SHPGenEdgeFn() = default;

    l_t computeTarget(l_t source) override;

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override;
    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override;
    void print(std::ostream &OS, bool isForDebug = false) const override;
    static std::shared_ptr<SHPGenEdgeFn> getInstance(l_t val);

  private:
    l_t value;
  };

  struct IdentityEdgeFunction : public SHPEdgeFn {
    virtual ~IdentityEdgeFunction() = default;

    l_t computeTarget(l_t source) override;
    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override;
    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override;

    void print(std::ostream &OS, bool isForDebug = false) const override;

    static std::shared_ptr<IdentityEdgeFunction> getInstance();
  };
};
} // namespace psr

#endif
