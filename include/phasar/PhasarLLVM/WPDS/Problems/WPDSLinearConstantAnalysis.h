/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H_
#define PHASAR_PHASARLLVM_WPDS_PROBLEMS_WPDSLINEARCONSTANTANALYSIS_H_

#include <memory>
#include <vector>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/WPDS/WPDSOptions.h>
#include <phasar/PhasarLLVM/WPDS/WPDSProblem.h>

namespace llvm {
class Value;
class Instruction;
class Function;
} // namespace llvm

namespace psr {

class WPDSLinearConstantAnalysis
    : public WPDSProblem<const llvm::Instruction *, const llvm::Value *,
                         const llvm::Function *, int64_t, LLVMBasedICFG &> {
public:
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Value *d_t;
  typedef const llvm::Function *m_t;
  typedef int64_t v_t;
  typedef LLVMBasedICFG &i_t;

  static const v_t TOP;
  static const v_t BOTTOM;

  WPDSLinearConstantAnalysis(LLVMBasedICFG &I, WPDSType WPDS,
                             SearchDirection Direction,
                             std::vector<n_t> Stack = {},
                             bool Witnesses = false);

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
  getSummaryFlowFunction(n_t curr, m_t destMthd) override;

  std::shared_ptr<EdgeFunction<v_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;
  std::shared_ptr<EdgeFunction<v_t>> getCallEdgeFunction(n_t callStmt,
                                                         d_t srcNode,
                                                         m_t destiantionMethod,
                                                         d_t destNode) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getReturnEdgeFunction(n_t callSite, m_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<m_t> callees) override;
  std::shared_ptr<EdgeFunction<v_t>>
  getSummaryEdgeFunction(n_t curr, d_t currNode, n_t succ,
                         d_t succNode) override;

  v_t topElement() override;
  v_t bottomElement() override;
  v_t join(v_t lhs, v_t rhs) override;

  d_t zeroValue() override;

  //   std::shared_ptr<EdgeFunction<v_t>> allTopFunction() override;

  //   class LCAEdgeFunctionComposer : public EdgeFunctionComposer<v_t> {
  //    public:
  //     LCAEdgeFunctionComposer(std::shared_ptr<EdgeFunction<v_t>> F,
  //                             std::shared_ptr<EdgeFunction<v_t>> G)
  //         : EdgeFunctionComposer<v_t>(F, G){};

  //     std::shared_ptr<EdgeFunction<v_t>> composeWith(
  //         std::shared_ptr<EdgeFunction<v_t>> secondFunction) override;

  //     std::shared_ptr<EdgeFunction<v_t>> joinWith(
  //         std::shared_ptr<EdgeFunction<v_t>> otherFunction) override;
  //   };

  //   class StoreConstant : public EdgeFunction<v_t>,
  //                         public std::enable_shared_from_this<StoreConstant>
  //                         {
  //    private:
  //     const unsigned StoreConst_Id;
  //     const v_t IntConst;

  //    public:
  //     explicit StoreConstant(v_t IntConst);

  //     v_t computeTarget(v_t source) override;

  //     std::shared_ptr<EdgeFunction<v_t>> composeWith(
  //         std::shared_ptr<EdgeFunction<v_t>> secondFunction) override;

  //     std::shared_ptr<EdgeFunction<v_t>> joinWith(
  //         std::shared_ptr<EdgeFunction<v_t>> otherFunction) override;

  //     bool equal_to(std::shared_ptr<EdgeFunction<v_t>> other) const override;

  //     void print(std::ostream &OS, bool isForDebug = false) const override;
  //   };

  //   class LoadStoreValueIdentity
  //       : public EdgeFunction<v_t>,
  //         public std::enable_shared_from_this<LoadStoreValueIdentity> {
  //    private:
  //     const unsigned LoadStoreV_Id;

  //    public:
  //     explicit LoadStoreValueIdentity();

  //     v_t computeTarget(v_t source) override;

  //     std::shared_ptr<EdgeFunction<v_t>> composeWith(
  //         std::shared_ptr<EdgeFunction<v_t>> secondFunction) override;

  //     std::shared_ptr<EdgeFunction<v_t>> joinWith(
  //         std::shared_ptr<EdgeFunction<v_t>> otherFunction) override;

  //     bool equal_to(std::shared_ptr<EdgeFunction<v_t>> other) const override;

  //     void print(std::ostream &OS, bool isForDebug = false) const override;
  //   };

  //   // Helper functions

  //   /**
  //    * The following binary operations are computed:
  //    *  - addition
  //    *  - subtraction
  //    *  - multiplication
  //    *  - division (signed/unsinged)
  //    *  - remainder (signed/unsinged)
  //    *
  //    * @brief Computes the result of a binary operation.
  //    * @param op operator
  //    * @param lop left operand
  //    * @param rop right operand
  //    * @return Result of binary operation
  //    */
  //   static v_t executeBinOperation(const unsigned op, v_t lop, v_t rop);
};

} // namespace psr

#endif
