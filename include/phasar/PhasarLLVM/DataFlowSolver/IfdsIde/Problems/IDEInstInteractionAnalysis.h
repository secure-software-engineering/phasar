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
#include <variant>
#include <vector>

#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsAlongsideCallSite.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace psr {

template <typename EdgeFactType = std::string>
class IDEInstInteractionAnalysisT
    : public IDETabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, const llvm::StructType *, const llvm::Value *,
          LatticeDomain<BitVectorSet<EdgeFactType>>, LLVMBasedICFG> {
public:
  using d_t = const llvm::Value *;
  using n_t = const llvm::Instruction *;
  using f_t = const llvm::Function *;
  using t_t = const llvm::StructType *;
  using v_t = const llvm::Value *;

  // type of the element contained in the sets of edge functions
  using e_t = EdgeFactType;
  using l_t = LatticeDomain<BitVectorSet<e_t>>;
  using i_t = LLVMBasedICFG;

  using EdgeFactGeneratorTy = std::set<e_t>(n_t curr, d_t srcNode,
                                            d_t destNode);

private:
  std::function<EdgeFactGeneratorTy> EdgeFactGen;
  static inline l_t BottomElement = Bottom{};
  static inline l_t TopElement = Top{};
  // can be set if a syntactic-only analysis is desired
  // (without using points-to information)
  const bool SyntacticAnalysisOnly = false;

public:
  IDEInstInteractionAnalysisT(const ProjectIRDB *IRDB,
                              const LLVMTypeHierarchy *TH,
                              const LLVMBasedICFG *ICF,
                              const LLVMPointsToInfo *PT,
                              std::set<std::string> EntryPoints = {"main"})
      : IDETabulationProblem<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Function *, const llvm::StructType *,
                             const llvm::Value *,
                             LatticeDomain<BitVectorSet<EdgeFactType>>,
                             LLVMBasedICFG>(IRDB, TH, ICF, PT, EntryPoints) {
    this->ZeroValue = createZeroValue();
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
      return std::make_shared<Gen<d_t>>(Alloca, this->getZeroValue());
    }

    if (!SyntacticAnalysisOnly) {

      // (ii) handle semantic propagation (pointers)
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
        // if one of the potentially many loaded values holds, the load itself
        // must also be populated
        struct IIAFlowFunction : FlowFunction<d_t> {
          IDEInstInteractionAnalysisT &Problem;
          const llvm::LoadInst *Load;
          std::set<d_t> PTS;

          IIAFlowFunction(IDEInstInteractionAnalysisT &Problem,
                          const llvm::LoadInst *Load)
              : Problem(Problem), Load(Load),
                PTS(Problem.PT->getPointsToSet(Load->getPointerOperand())) {}

          std::set<d_t> computeTargets(d_t src) override {
            std::set<d_t> Facts;
            Facts.insert(src);
            if (PTS.count(src)) {
              Facts.insert(Load);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Load);
      }

      // (ii) handle semantic propagation (pointers)
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
        // if the value to be stored holds the potentially memory location
        // that it is stored to must be populated as well
        struct IIAFlowFunction : FlowFunction<d_t> {
          IDEInstInteractionAnalysisT &Problem;
          const llvm::StoreInst *Store;
          std::set<d_t> ValuePTS;
          std::set<d_t> PointerPTS;

          IIAFlowFunction(IDEInstInteractionAnalysisT &Problem,
                          const llvm::StoreInst *Store)
              : Problem(Problem), Store(Store),
                ValuePTS(Problem.PT->getPointsToSet(Store->getValueOperand())),
                PointerPTS(
                    Problem.PT->getPointsToSet(Store->getPointerOperand())) {}

          std::set<d_t> computeTargets(d_t src) override {
            std::set<d_t> Facts;
            Facts.insert(src);
            // if a value is stored that holds we must generate all potential
            // memory locations the store might write to
            if (Store->getValueOperand() == src || ValuePTS.count(src)) {
              Facts.insert(Store->getPointerOperand());
              Facts.insert(PointerPTS.begin(), PointerPTS.end());
            }
            // if the value to be stored does not hold then we must at least add
            // the store instruction and the points-to set as the instruction
            // still interacts with the memory locations pointed to be PTS
            if (Store->getPointerOperand() == src || PointerPTS.count(src)) {
              Facts.insert(Store);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Store);
      }
    }

    // (i) handle syntactic propagation for all other statements
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
        // (i) syntactic propagation
        if (Inst == src) {
          Facts.insert(Inst);
        }
        // continue syntactic propagation: populate and propagate other existing
        // facts
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
    // if pointer parameters hold at the end of a callee function generate all
    // of the
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
    for (auto &EntryPoint : this->EntryPoints) {
      SeedMap.insert(
          std::make_pair(&this->ICF->getFunction(EntryPoint)->front().front(),
                         std::set<d_t>({this->getZeroValue()})));
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
    if (EdgeFactGen) {
      auto EdgeFacts = EdgeFactGen(curr, currNode, succNode);
      // construct BitVectorSet
      l_t UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
      if (!EdgeFacts.empty()) {
        // handle generating edges from zero
        // generate labels from zero when the instruction itself is the flow
        // fact that is generated
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
            return std::make_shared<IIAALabelEdgeFunction>(*this,
                                                           UserEdgeFacts);
          }
          // handle edges that may add new labels to existing facts
          if (Op == currNode && currNode == succNode) {
            return std::make_shared<IIAALabelEdgeFunction>(*this,
                                                           UserEdgeFacts);
          }
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

  inline l_t topElement() override { return TopElement; }

  inline l_t bottomElement() override { return BottomElement; }

  inline l_t join(l_t Lhs, l_t Rhs) override {
    if (Lhs == BottomElement || Rhs == BottomElement) {
      return BottomElement;
    }
    if (Lhs == TopElement) {
      return Rhs;
    }
    if (Rhs == TopElement) {
      return Lhs;
    }
    auto LhsSet = std::get<BitVectorSet<e_t>>(Lhs);
    auto RhsSet = std::get<BitVectorSet<e_t>>(Rhs);
    return LhsSet.setUnion(RhsSet);
  }

  inline std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    return std::make_shared<AllTop<l_t>>(topElement());
  }

  // provide some handy helper edge functions to improve reuse

  class IIAALabelEdgeFunction
      : public EdgeFunction<l_t>,
        public std::enable_shared_from_this<IIAALabelEdgeFunction> {
  private:
    IDEInstInteractionAnalysisT<e_t> &Analysis;
    l_t Data;

  public:
    explicit IIAALabelEdgeFunction(IDEInstInteractionAnalysisT<e_t> &Analysis,
                                   l_t Data)
        : Analysis(Analysis), Data(Data) {}

    l_t computeTarget(l_t Src) override {
      return Analysis.join(Src, Data);
      // return Src.setUnion(Data);
    }

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
        // auto Union = Data.setUnion(AS->Data);
        auto Union = Analysis.join(Data, AS->Data);
        return std::make_shared<IIAALabelEdgeFunction>(this->Analysis, Union);
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
        // auto Union = Data.setUnion(AS->Data);
        auto Union = Analysis.join(Data, AS->Data);
        return std::make_shared<IIAALabelEdgeFunction>(this->Analysis, Union);
      }
      return std::make_shared<AllBottom<l_t>>(Analysis.BottomElement);
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
    if (std::holds_alternative<Top>(l)) {
      os << std::get<Top>(l);
    } else if (std::holds_alternative<Bottom>(l)) {
      os << std::get<Bottom>(l);
    } else {
      auto lset = std::get<BitVectorSet<e_t>>(l).getAsSet();
      size_t idx = 0;
      for (const auto &s : lset) {
        os << s;
        if (idx != lset.size() - 1) {
          os << ", ";
        }
        ++idx;
      }
    }
  }

  void stripBottomResults(std::unordered_map<d_t, l_t> &Res) {
    for (auto it = Res.begin(); it != Res.end();) {
      if (it->second == BottomElement) {
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
    for (const auto *f : this->ICF->getAllFunctions()) {
      std::string fName = getFunctionNameFromIR(f);
      OS << "\nFunction: " << fName << "\n----------"
         << std::string(fName.size(), '-') << '\n';
      for (const auto *stmt : this->ICF->getAllInstructionsOf(f)) {
        auto results = SR.resultsAt(stmt, true);
        stripBottomResults(results);
        if (!results.empty()) {
          OS << "At IR statement: " << this->NtoString(stmt) << '\n';
          for (auto res : results) {
            if (res.second != BottomElement) {
              OS << "   Fact: " << this->DtoString(res.first)
                 << "\n  Value: " << this->LtoString(res.second) << '\n';
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
