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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>

namespace llvm {
class Instruction;
class Function;
class Value;
} // namespace llvm

namespace psr {
class LLVMBasedICFG;

// caution the underlying intergers do matter!
enum class State {
  TOP = 42,
  UNINIT = 0,
  OPENED = 1,
  CLOSED = 2,
  ERROR = 3,
  BOT = 13
};

// the tokens of interest that may change state (the underlying intergers do
// matter!)
enum class Token { FOPEN = 0, FCLOSE = 1, STAR = 2, FREOPEN = 3 };

// delta matrix to implement the state machine's delta function
constexpr State delta[4][4] = {
    {State::OPENED, State::ERROR, State::OPENED, State::ERROR},
    {State::UNINIT, State::CLOSED, State::ERROR, State::ERROR},
    {State::ERROR, State::OPENED, State::ERROR, State::ERROR},
    {State::OPENED, State::OPENED, State::OPENED, State::ERROR},
};

// delta function that computes the next state
static State getNextState(Token tok, State state);

class IDETypeStateAnalysis
    : public DefaultIDETabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, State, LLVMBasedICFG &> {
public:
  typedef const llvm::Value *d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *m_t;
  typedef State v_t;
  typedef LLVMBasedICFG &i_t;

private:
  std::vector<std::string> EntryPoints;
  static const std::set<std::string> STDIOFunctions;

public:
  static const State TOP;
  static const State BOTTOM;
  std::string TypeOfInterest;
  static const std::shared_ptr<AllBottom<v_t>> AllBotFunction;

  IDETypeStateAnalysis(i_t icfg, std::string TypeOfInterest,
                       std::vector<std::string> EntryPoints = {"main"});

  virtual ~IDETypeStateAnalysis() = default;

  // start formulating our analysis by specifying the parts required for IFDS

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

  // in addition provide specifications for the IDE parts

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
  getSummaryEdgeFunction(n_t callStmt, d_t callNode, n_t retSite,
                         d_t retSiteNode) override;

  v_t topElement() override;

  v_t bottomElement() override;

  v_t join(v_t lhs, v_t rhs) override;

  std::shared_ptr<EdgeFunction<v_t>> allTopFunction() override;

  void printNode(std::ostream &os, n_t d) const override;

  void printDataFlowFact(std::ostream &os, d_t d) const override;

  void printMethod(std::ostream &os, m_t m) const override;

  void printValue(std::ostream &os, v_t v) const override;

  // customize the edge function composer
  class TSEdgeFunctionComposer : public EdgeFunctionComposer<v_t> {
  public:
    TSEdgeFunctionComposer(std::shared_ptr<EdgeFunction<v_t>> F,
                           std::shared_ptr<EdgeFunction<v_t>> G)
        : EdgeFunctionComposer<v_t>(F, G){};
    std::shared_ptr<EdgeFunction<v_t>>
    joinWith(std::shared_ptr<EdgeFunction<v_t>> otherFunction) override;
  };

  // simplify the art of encoding edge functions
  // the composeWith(), joinWith() and equal_to() implementation
  // can stay the same for each instance
  class TSEdgeFunction : public EdgeFunction<v_t>,
                         public std::enable_shared_from_this<TSEdgeFunction> {
  private:
    v_t value;

  public:
    // to be implemented by a concrete edge function type
    // virtual V computeTarget(V source) = 0;

    std::shared_ptr<EdgeFunction<v_t>>
    composeWith(std::shared_ptr<EdgeFunction<v_t>> secondFunction) override;

    std::shared_ptr<EdgeFunction<v_t>>
    joinWith(std::shared_ptr<EdgeFunction<v_t>> otherFunction) override;

    bool equal_to(std::shared_ptr<EdgeFunction<v_t>> other) const override;
  };
};

} // namespace psr

#endif
