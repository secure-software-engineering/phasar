/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert, Richard Leer, and Florian Sattler.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEINSTINTERACTIONALYSIS_H_
#define PHASAR_PHASARLLVM_IFDSIDE_PROBLEMS_IDEINSTINTERACTIONALYSIS_H_

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsAlongsideCallSite.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename EdgeFactType = std::string>
class IDEInstInteractionAnalysisT
    : public IDETabulationProblem<const llvm::Instruction *,
                                  const llvm::Value *, const llvm::Function *,
                                  const llvm::StructType *, const llvm::Value *,
                                  BitVectorSet<std::string>, LLVMBasedICFG> {
public:
  using d_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;

  // type of the element contained in the sets of edge functions
  using e_t = EdgeFactType;
  using l_t = BitVectorSet<e_t>;
  using i_t = LLVMBasedICFG;

  using EdgeFactGeneratorTy = std::set<e_t>(n_t curr, d_t srcNode,
                                            d_t destNode);

private:
  std::function<EdgeFactGeneratorTy> EdgeFactGen;
  static inline l_t BOTTOM = {"__BOTTOM__"};
  static inline l_t TOP = {"__TOP__"};

public:
  IDEInstInteractionAnalysisT(const ProjectIRDB *IRDB,
                              const LLVMTypeHierarchy *TH,
                              const LLVMBasedICFG *ICF,
                              const LLVMPointsToInfo *PT,
                              std::set<std::string> EntryPoints = {"main"})
      : IDETabulationProblem(IRDB, TH, ICF, PT, EntryPoints) {
    IDETabulationProblem::ZeroValue = createZeroValue();
  }

  ~IDEInstInteractionAnalysisT() override = default;

  // Offer a special hook to the user that allows to generate additional
  // edge facts on-the-fly. Above the generator function, the ordinary
  // edge facts are generated according to the usual edge functions.

  inline void registerEdgeFactGenerator(
      std::function<EdgeFactGeneratorTy> EdgeFactGenerator) {
    EdgeFactGen = std::move(EdgeFactGenerator);
  }

  // start formulating our analysis by specifying the parts required for IFDS

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override {
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
      return std::make_shared<Gen<d_t>>(Alloca, getZeroValue());
    }

    struct IIAFlowFunction : FlowFunction<d_t> {
      IDEInstInteractionAnalysisT &Problem;
      n_t Inst;

      IIAFlowFunction(IDEInstInteractionAnalysisT &Problem, n_t Inst)
          : Problem(Problem), Inst(Inst) {}

      std::set<d_t> computeTargets(d_t src) override {
        std::set<d_t> Facts;
        if (Problem.isZeroValue(src)) {
          // keep the zero flow fact
          Facts.insert(src);
          return Facts;
        }
        if (Inst == src) {
          Facts.insert(Inst);
        }
        // populate and propagate other existing facts
        for (auto &Op : Inst->operands()) {
          // if one of the operands holds, also generate the instruction using
          // it
          if (Op == src) {
            Facts.insert(Inst);
            Facts.insert(src);
          }
        }
        // pass everything that already holds as identity
        Facts.insert(src);
        return Facts;
      }
    };
    return std::make_shared<IIAFlowFunction>(*this, curr);
  }

  inline std::shared_ptr<FlowFunction<d_t>>
  getCallFlowFunction(n_t callStmt, f_t destMthd) override {
    // just use the auto mapping
    return std::make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                              destMthd);
  }

  inline std::shared_ptr<FlowFunction<d_t>>
  getRetFlowFunction(n_t callSite, f_t calleeMthd, n_t exitStmt,
                     n_t retSite) override {
    // just use the auto mapping
    return std::make_shared<MapFactsToCaller>(llvm::ImmutableCallSite(callSite),
                                              calleeMthd, exitStmt);
  }

  inline std::shared_ptr<FlowFunction<d_t>>
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) override {
    // just use the auto mapping, pointer parameters are killed and handled by
    // getCallFlowfunction() and getRetFlowFunction()
    return std::make_shared<MapFactsAlongsideCallSite>(
        llvm::ImmutableCallSite(callSite));
  }

  inline std::shared_ptr<FlowFunction<d_t>>
  getSummaryFlowFunction(n_t callStmt, f_t destMthd) override {
    // do not use summaries
    return nullptr;
  }

  inline std::map<n_t, std::set<d_t>> initialSeeds() override {
    std::map<n_t, std::set<d_t>> SeedMap;
    for (auto &EntryPoint : EntryPoints) {
      SeedMap.insert(
          std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                         std::set<d_t>({getZeroValue()})));
    }
    return SeedMap;
  }

  inline d_t createZeroValue() const override {
    // create a special value to represent the zero value!
    return LLVMZeroValue::getInstance();
  }

  inline bool isZeroValue(d_t d) const override {
    return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
  }

  // in addition provide specifications for the IDE parts

  inline std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override {
    // propagate zero edges as identity
    if (isZeroValue(currNode) && isZeroValue(succNode)) {
      return EdgeIdentity<l_t>::getInstance();
    }
    // check if the user has registered a fact generator function
    std::set<e_t> UserEdgeFacts;
    if (EdgeFactGen) {
      UserEdgeFacts = EdgeFactGen(curr, currNode, succNode);
    }
    if (!UserEdgeFacts.empty()) {
      // handle generating edges from zero
      // generate labels from zero when the instruction itself is the flow fact
      // that is generated
      if (isZeroValue(currNode) && curr == succNode) {
        return std::make_shared<IIAALabelEdgeFunction>(*this, UserEdgeFacts);
      }
      // handle edges that may add new labels to existing facts
      if (curr == currNode && currNode == succNode) {
        return std::make_shared<IIAALabelEdgeFunction>(*this, UserEdgeFacts);
      }
      // generate labels from zero when an operand of the current instruction
      // is a flow fact that is generated
      for (auto &Op : curr->operands()) {
        // also propagate the labels if one of the operands holds
        if (isZeroValue(currNode) && Op == succNode) {
          return std::make_shared<IIAALabelEdgeFunction>(*this, UserEdgeFacts);
        }
        // handle edges that may add new labels to existing facts
        if (Op == currNode && currNode == succNode) {
          return std::make_shared<IIAALabelEdgeFunction>(*this, UserEdgeFacts);
        }
      }
    }
    // otherwise stick to identity
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t callStmt, d_t srcNode, f_t destinationMethod,
                      d_t destNode) override {
    // can be passed as identity
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override {
    // can be passed as identity
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override {
    // just forward to getNormalEdgeFunction() to check whether a user has
    // additional labels for this call site
    return getNormalEdgeFunction(callSite, callNode, retSite, retSiteNode);
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                         d_t retSiteNode) override {
    // do not use summaries
    return nullptr;
  }

  inline l_t topElement() override { return TOP; }

  inline l_t bottomElement() override { return BOTTOM; }

  inline l_t join(l_t lhs, l_t rhs) override {
    if (lhs == BOTTOM || rhs == BOTTOM) {
      return BOTTOM;
    }
    if (lhs == TOP) {
      return rhs;
    }
    if (rhs == TOP) {
      return lhs;
    }
    return lhs.setUnion(rhs);
  }

  inline std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    return std::make_shared<AllTop<l_t>>(topElement());
  }

  // provide some handy helper edge functions to improve reuse

  class IIAALabelEdgeFunction
      : public EdgeFunction<l_t>,
        public std::enable_shared_from_this<IIAALabelEdgeFunction> {
  private:
    const IDEInstInteractionAnalysisT<e_t> &Analysis;
    l_t Data;

  public:
    explicit IIAALabelEdgeFunction(
        const IDEInstInteractionAnalysisT<e_t> &Analysis, std::set<e_t> Data)
        : Analysis(Analysis), Data(Data) {}

    l_t computeTarget(l_t Src) override { return Src.setUnion(Data); }

    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
      // std::cout << "IIAALabelEdgeFunction::composeWith\n";
      if (auto *AB = dynamic_cast<AllBottom<l_t> *>(secondFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *EI = dynamic_cast<EdgeIdentity<l_t> *>(secondFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *AS =
              dynamic_cast<IIAALabelEdgeFunction *>(secondFunction.get())) {
        auto Union = Data.setUnion(AS->Data);
        return std::make_shared<IIAALabelEdgeFunction>(this->Analysis,
                                                       Union.getAsSet());
      }
      llvm::report_fatal_error("found unexpected edge function");
    }

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
      // std::cout << "IIAALabelEdgeFunction::joinWith\n";
      if (otherFunction.get() == this ||
          otherFunction->equal_to(this->shared_from_this())) {
        return this->shared_from_this();
      }
      if (auto *AT = dynamic_cast<AllTop<l_t> *>(otherFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *AS =
              dynamic_cast<IIAALabelEdgeFunction *>(otherFunction.get())) {
        auto Union = Data.setUnion(AS->Data);
        return std::make_shared<IIAALabelEdgeFunction>(this->Analysis,
                                                       Union.getAsSet());
      }
      return std::make_shared<AllBottom<l_t>>(
          IDEInstInteractionAnalysisT<e_t>::BOTTOM);
    }

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
      // std::cout << "IIAALabelEdgeFunction::equal_to\n";
      if (auto *I = dynamic_cast<IIAALabelEdgeFunction *>(other.get())) {
        return (I->Data == this->Data);
      }
      return this == other.get();
    }

    void print(std::ostream &OS, bool isForDebug = false) const override {
      OS << "EF: (IIAALabelEdgeFunction: ";
      Analysis.printEdgeFact(OS, Data);
      OS << ")";
    }
  };

  // provide functionalities for printing things and emitting text reports

  void printNode(std::ostream &os, n_t n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    os << llvmIRToString(d);
  }

  void printFunction(std::ostream &os, f_t m) const override {
    os << m->getName().str();
  }

  void printEdgeFact(std::ostream &os, l_t l) const override {
    auto lset = l.getAsSet();
    size_t idx = 0;
    for (const auto &s : lset) {
      os << s;
      if (idx != lset.size() - 1) {
        os << ", ";
      }
      ++idx;
    }
  }

  void stripBottomResults(std::unordered_map<d_t, l_t> &Res) {
    for (auto it = Res.begin(); it != Res.end();) {
      if (it->second == BOTTOM) {
        it = Res.erase(it);
      } else {
        ++it;
      }
    }
  }

  void emitTextReport(const SolverResults<n_t, d_t, l_t> &SR,
                      std::ostream &OS = std::cout) override {
    OS << "\n====================== IDE-Inst-Interaction-Analysis Report "
          "======================\n";
    // if (!IRDB->debugInfoAvailable()) {
    //   // Emit only IR code, function name and module info
    //   OS << "\nWARNING: No Debug Info available - emiting results without "
    //         "source code mapping!\n";
    for (const auto *f : ICF->getAllFunctions()) {
      std::string fName = getFunctionNameFromIR(f);
      OS << "\nFunction: " << fName << "\n----------"
         << std::string(fName.size(), '-') << '\n';
      for (const auto *stmt : ICF->getAllInstructionsOf(f)) {
        auto results = SR.resultsAt(stmt, true);
        stripBottomResults(results);
        if (!results.empty()) {
          OS << "At IR statement: " << NtoString(stmt) << '\n';
          for (auto res : results) {
            if (res.second != BOTTOM) {
              OS << "   Fact: " << DtoString(res.first)
                 << "\n  Value: " << LtoString(res.second) << '\n';
            }
          }
          OS << '\n';
        }
      }
      OS << '\n';
    }
    // } else {
    // TODO: implement better report in case debug information are available
    //   }
  }
}; // namespace psr

using IDEInstInteractionAnalysis = IDEInstInteractionAnalysisT<>;

} // namespace psr

#endif
