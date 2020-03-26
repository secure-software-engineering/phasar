/******************************************************************************
 * Copyright (c) 2018 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDETYPESTATEANALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDETYPESTATEANALYSIS_H_

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h"

namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

class IDETypeStateAnalysis
    : public IDETabulationProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::StructType *, const llvm::Value *,
                                  int, LLVMBasedICFG> {
public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *f_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef int l_t;
  typedef LLVMBasedICFG i_t;
  using ConfigurationTy = TypeStateDescription;

private:
  const TypeStateDescription &TSD;
  std::map<const llvm::Value *, std::set<const llvm::Value *>> PointsToCache;
  std::map<const llvm::Value *, std::set<const llvm::Value *>>
      RelevantAllocaCache;

  /**
   * @brief Returns all alloca's that are (indirect) aliases of V.
   *
   * Currently PhASAR's points-to information does not include alloca
   * instructions, since alloca instructions, i.e. memory locations, are of
   * type T* for a target type T. Thus they do not alias directly. Therefore,
   * for each alias of V we collect related alloca instructions by checking
   * load and store instructions for used alloca's.
   */
  std::set<d_t> getRelevantAllocas(d_t V);

  /**
   * @brief Returns whole-module aliases of V.
   *
   * This function retrieves whole-module points-to information. We store
   * already computed points-to information in a cache to prevent expensive
   * recomputation since the whole module points-to graph can be huge. This
   * might become unnecessary once PhASAR's PointsToGraph starts using a cache
   * itself.
   */
  std::set<d_t> getWMPointsToSet(d_t V);

  /**
   * @brief Provides whole module aliases and relevant alloca's of V.
   */
  std::set<d_t> getWMAliasesAndAllocas(d_t V);

  /**
   * @brief Provides local aliases and relevant alloca's of V.
   */
  std::set<d_t> getLocalAliasesAndAllocas(d_t V, const std::string &Fname);

  /**
   * @brief Checks if the type machtes the type of interest.
   */
  bool hasMatchingType(d_t V);

public:
  const l_t TOP;
  const l_t BOTTOM;

  IDETypeStateAnalysis(const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
                       const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
                       const TypeStateDescription &TSD,
                       std::set<std::string> EntryPoints = {"main"});

  ~IDETypeStateAnalysis() override = default;

  // start formulating our analysis by specifying the parts required for IFDS

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override;

  std::shared_ptr<FlowFunction<d_t>> getCallFlowFunction(n_t callStmt,
                                                         f_t destFun) override;

  std::shared_ptr<FlowFunction<d_t>> getRetFlowFunction(n_t callSite,
                                                        f_t calleeFun,
                                                        n_t exitStmt,
                                                        n_t retSite) override;

  std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) override;

  std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, f_t destFun) override;

  std::map<n_t, std::set<d_t>> initialSeeds() override;

  d_t createZeroValue() const override;

  bool isZeroValue(d_t d) const override;

  // in addition provide specifications for the IDE parts

  std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t callStmt, d_t srcNode, f_t destinationFunction,
                      d_t destNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeFunction, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override;

  std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  l_t topElement() override;

  l_t bottomElement() override;

  /**
   * We have a lattice with BOTTOM representing all information
   * and TOP representing no information. The other lattice elements
   * are defined by the type state description, i.e. represented by the
   * states of the finite state machine.
   *
   * @note Only one-level lattice's are handled currently
   */
  l_t join(l_t lhs, l_t rhs) override;

  std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t d) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printFunction(std::ostream &os, f_t m) const override;

  void printEdgeFact(std::ostream &os, l_t l) const override;

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      std::ostream &OS = std::cout) override;

  // customize the edge function composer
  class TSEdgeFunctionComposer : public EdgeFunctionComposer<l_t> {
  private:
    l_t botElement;

  public:
    TSEdgeFunctionComposer(std::shared_ptr<EdgeFunction<l_t>> F,
                           std::shared_ptr<EdgeFunction<l_t>> G, l_t bot)
        : EdgeFunctionComposer<l_t>(F, G), botElement(bot){};
    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override;
  };

  class TSEdgeFunction : public EdgeFunction<l_t>,
                         public std::enable_shared_from_this<TSEdgeFunction> {
  protected:
    const TypeStateDescription &TSD;
    // Do not use a reference here, since LLVM's StringRef's (obtained by str())
    // might turn to nullptr for whatever reason...
    const std::string Token;
    l_t CurrentState;
    llvm::ImmutableCallSite CS;

  public:
    TSEdgeFunction(const TypeStateDescription &tsd, const std::string tok,
                   llvm::ImmutableCallSite cs)
        : TSD(tsd), Token(tok), CurrentState(TSD.top()), CS(cs){};

    l_t computeTarget(l_t source) override;

    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override;

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override;

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override;

    void print(std::ostream &OS, bool isForDebug = false) const override;

    l_t getCurrentState() const { return CurrentState; }
  };
};

} // namespace psr

#endif
