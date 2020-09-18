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
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToUtils.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"
#include "phasar/PhasarLLVM/Utils/LatticeDomain.h"
#include "phasar/Utils/BitVectorSet.h"
#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

// have some handy helper functionalities
namespace {

const llvm::AllocaInst *
getAllocaInstruction(const llvm::GetElementPtrInst *GEP) {
  if (!GEP) {
    return nullptr;
  }
  const auto *Alloca = GEP->getPointerOperand();
  while (const auto *NestedGEP =
             llvm::dyn_cast<llvm::GetElementPtrInst>(Alloca)) {
    Alloca = NestedGEP->getPointerOperand();
  }
  return llvm::dyn_cast<llvm::AllocaInst>(Alloca);
}

} // namespace

namespace psr {

template <typename EdgeFactType>
struct IDEInstInteractionAnalysisDomain : public LLVMAnalysisDomainDefault {
  // type of the element contained in the sets of edge functions
  using e_t = EdgeFactType;
  using l_t = LatticeDomain<BitVectorSet<e_t>>;
};

///
/// SyntacticAnalysisOnly: Can be set if a syntactic-only analysis is desired
/// (without using points-to information)
///
/// IndirectTaints: Can be set to ensure non-interference
///
template <typename EdgeFactType = std::string,
          bool SyntacticAnalysisOnly = false, bool EnableIndirectTaints = false>
class IDEInstInteractionAnalysisT
    : public IDETabulationProblem<
          IDEInstInteractionAnalysisDomain<EdgeFactType>> {
public:
  using AnalysisDomainTy = IDEInstInteractionAnalysisDomain<EdgeFactType>;

  using IDETabProblemType = IDETabulationProblem<AnalysisDomainTy>;
  using typename IDETabProblemType::container_type;
  using typename IDETabProblemType::FlowFunctionPtrType;

  using d_t = typename AnalysisDomainTy::d_t;
  using n_t = typename AnalysisDomainTy::n_t;
  using f_t = typename AnalysisDomainTy::f_t;
  using t_t = typename AnalysisDomainTy::t_t;
  using v_t = typename AnalysisDomainTy::v_t;

  // type of the element contained in the sets of edge functions
  using e_t = typename AnalysisDomainTy::e_t;
  using l_t = typename AnalysisDomainTy::l_t;
  using i_t = typename AnalysisDomainTy::i_t;

  using EdgeFactGeneratorTy = std::set<e_t>(n_t curr);

private:
  std::function<EdgeFactGeneratorTy> edgeFactGen;
  static inline const l_t BottomElement = Bottom{};
  static inline const l_t TopElement = Top{};
  // bool GeneratedGlobalVariables = false;

  inline BitVectorSet<e_t> edgeFactGenToBitVectorSet(n_t curr) {
    if (edgeFactGen) {
      auto Results = edgeFactGen(curr);
      BitVectorSet<e_t> BVS(Results.begin(), Results.end());
      return BVS;
    }
    return {};
  }

public:
  IDEInstInteractionAnalysisT(const ProjectIRDB *IRDB,
                              const LLVMTypeHierarchy *TH,
                              const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
                              std::set<std::string> EntryPoints = {"main"})
      : IDETabulationProblem<AnalysisDomainTy, container_type>(
            IRDB, TH, ICF, PT, std::move(EntryPoints)) {
    this->ZeroValue =
        IDEInstInteractionAnalysisT<EdgeFactType, SyntacticAnalysisOnly,
                                    EnableIndirectTaints>::createZeroValue();
    IIAAAddLabelsEF::initEdgeFunctionCleaner();
    IIAAKillOrReplaceEF::initEdgeFunctionCleaner();
  }

  ~IDEInstInteractionAnalysisT() override = default;

  // Offer a special hook to the user that allows to generate additional
  // edge facts on-the-fly. Above the generator function, the ordinary
  // edge facts are generated according to the usual edge functions.

  inline void registerEdgeFactGenerator(
      std::function<EdgeFactGeneratorTy> EdgeFactGenerator) {
    edgeFactGen = std::move(EdgeFactGenerator);
  }

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override {
    // TODO generate global and heap allocated variables

    // Generate all local variables
    //
    // Flow function:
    //
    //                0
    //                |\
    // x = alloca y   | \
    //                v  v
    //                0  x
    //
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << "AllocaInst");
      return std::make_shared<Gen<d_t>>(Alloca, this->getZeroValue());
    }

    // Handle indirect taints, i. e., propagate values that depend on branch
    // conditions whose operands are tainted.
    if (EnableIndirectTaints) {
      if (auto br = llvm::dyn_cast<llvm::BranchInst>(curr);
          br && br->isConditional()) {
        return std::make_shared<LambdaFlow<d_t>>([=](d_t src) {
          container_type ret = {src, br};
          if (src == br->getCondition()) {
            for (auto succ : br->successors()) {
              // this->indirecrTaints[succ].insert(src);
              for (auto &inst : succ->instructionsWithoutDebug()) {
                ret.insert(&inst);
              }
            }
          }
          return ret;
        });
      }
    }

    // Handle points is the user wishes to conduct a non-syntax-only
    // inst-interaction analysis.
    if constexpr (!SyntacticAnalysisOnly) {
      // (ii) Handle semantic propagation (pointers) for load instructions.
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
        // If one of the potentially many loaded values holds, the load itself
        // (dereferenced value) must also be generated and populated.
        //
        // Flow function:
        //
        // Let Y = pts(y), be the points-to set of y.
        //
        //              0  Y  x
        //              |  |\ |
        // x = load y   |  | \|
        //              v  v  v
        //              0  y  x
        //
        struct IIAFlowFunction : FlowFunction<d_t, container_type> {
          const llvm::LoadInst *Load;
          std::shared_ptr<std::unordered_set<d_t>> PTS;

          IIAFlowFunction(IDEInstInteractionAnalysisT &Problem,
                          const llvm::LoadInst *Load)
              : Load(Load),
                PTS(Problem.PT->getPointsToSet(Load->getPointerOperand())) {}

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (PTS->count(src)) {
              Facts.insert(Load);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Load);
      }

      // (ii) Handle semantic propagation (pointers) for store instructions.
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
        // If the value to be stored holds the potentially memory location(s)
        // that it is stored to must be generated and populated, too.
        //
        // Flow function:
        //
        // Let X be
        //    - pts(x), the points-to set of x if x is an intersting pointer.
        //    - a singleton set containing x, otherwise.
        // Let Y be pts(y), the points-to set of y.
        //
        //             0  X  y
        //             |  |\ |
        // store x y   |  | \|
        //             v  v  v
        //             0  x  Y
        //
        struct IIAFlowFunction : FlowFunction<d_t, container_type> {
          const llvm::StoreInst *Store;
          std::shared_ptr<std::unordered_set<d_t>> ValuePTS;
          std::shared_ptr<std::unordered_set<d_t>> PointerPTS;

          IIAFlowFunction(IDEInstInteractionAnalysisT &Problem,
                          const llvm::StoreInst *Store)
              : Store(Store), ValuePTS([&]() {
                  if (isInterestingPointer(Store->getValueOperand())) {
                    return Problem.PT->getPointsToSet(Store->getValueOperand());
                  } else {
                    return std::make_shared<std::unordered_set<d_t>>(
                        std::unordered_set<d_t>{Store->getValueOperand()});
                  }
                }()),
                PointerPTS(
                    Problem.PT->getPointsToSet(Store->getPointerOperand())) {}

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (IDEInstInteractionAnalysisT::isZeroValueImpl(src)) {
              return Facts;
            }
            // If a value is stored that holds we must generate all potential
            // memory locations the store might write to.
            if (Store->getValueOperand() == src || ValuePTS->count(src)) {
              Facts.insert(Store->getPointerOperand());
              Facts.insert(PointerPTS->begin(), PointerPTS->end());
            }
            // If the value to be stored does not hold we must at least add
            // the store instruction and the points-to set as the instruction
            // still interacts with the memory locations pointed to be PTS.
            if (Store->getPointerOperand() == src || PointerPTS->count(src)) {
              Facts.insert(Store);
              Facts.erase(src);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Store);
      }
    }

    // (i) Handle syntactic propagation for store instructions
    // In case store x y, we need to draw the edge x --> y such that we can
    // transfer x's labels to y
    //
    // Flow function:
    //
    //             0  x
    //             |  |\
    // store x y   |  | \
    //             v  v  v
    //             0  x  y
    //
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
      // Case x is a load instruction
      if (const auto *Load =
              llvm::dyn_cast<llvm::LoadInst>(Store->getValueOperand())) {
        struct IIAAFlowFunction : FlowFunction<d_t> {
          const llvm::StoreInst *Store;
          const llvm::LoadInst *Load;

          IIAAFlowFunction(const llvm::StoreInst *S, const llvm::LoadInst *L)
              : Store(S), Load(L) {}
          ~IIAAFlowFunction() override = default;

          container_type computeTargets(d_t src) override {
            container_type Facts;
            if (Load == src || Load->getPointerOperand() == src) {
              Facts.insert(src);
              Facts.insert(Load->getPointerOperand());
              Facts.insert(Store->getPointerOperand());
            } else {
              Facts.insert(src);
            }
            LOG_IF_ENABLE([&]() {
              for (const auto s : Facts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "Create edge: " << llvmIRToShortString(src) << " --"
                    << llvmIRToShortString(Store) << "--> "
                    << llvmIRToShortString(s);
              }
            }());
            return Facts;
          }
        };
        return std::make_shared<IIAAFlowFunction>(Store, Load);
      } else {
        // Otherwise
        struct IIAAFlowFunction : FlowFunction<d_t> {
          const llvm::StoreInst *Store;

          IIAAFlowFunction(const llvm::StoreInst *S) : Store(S) {}
          ~IIAAFlowFunction() override = default;

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (Store->getValueOperand() == src) {
              Facts.insert(Store->getPointerOperand());
            }
            LOG_IF_ENABLE([&]() {
              for (const auto s : Facts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "Create edge: " << llvmIRToShortString(src) << " --"
                    << llvmIRToShortString(Store) << "--> "
                    << llvmIRToShortString(s);
              }
            }());
            return Facts;
          }
        };
        return std::make_shared<IIAAFlowFunction>(Store);
      }
    }
    // At last, we can handle all other (unary/binary) instructions
    //
    // Flow function:
    //
    //                       0  x  o  p
    //                       |  | /| /|
    // x = instruction o p   |  |/ |/ |
    //                       |  | /|  |
    //                       |  |/ |  |
    //                       v  v  v  v
    //                       0  x  o  p
    //
    struct IIAFlowFunction : FlowFunction<d_t> {
      n_t Inst;

      IIAFlowFunction(n_t Inst) : Inst(Inst) {}

      ~IIAFlowFunction() override = default;

      container_type computeTargets(d_t src) override {
        container_type Facts;
        if (IDEInstInteractionAnalysisT::isZeroValueImpl(src)) {
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
        LOG_IF_ENABLE([&]() {
          for (const auto s : Facts) {
            BOOST_LOG_SEV(lg::get(), DFADEBUG)
                << "Create edge: " << llvmIRToShortString(src) << " --"
                << llvmIRToShortString(Inst) << "--> "
                << llvmIRToShortString(s);
          }
        }());
        return Facts;
      }
    };
    return std::make_shared<IIAFlowFunction>(curr);
  }

  inline FlowFunctionPtrType getCallFlowFunction(n_t callStmt,
                                                 f_t destMthd) override {
    if (this->ICF->isHeapAllocatingFunction(destMthd)) {
      // Kill add facts and model the effects in getCallToRetFlowFunction().
      return KillAll<d_t>::getInstance();
    } else if (destMthd->isDeclaration()) {
      // We don't have anything that we could analyze, kill all facts.
      return KillAll<d_t>::getInstance();
    }
    // Map actual to formal parameters.
    return std::make_shared<MapFactsToCallee<container_type>>(
        llvm::ImmutableCallSite(callStmt), destMthd);
  }

  inline FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeMthd,
                                                n_t exitStmt,
                                                n_t retSite) override {
    // Map return value back to the caller. If pointer parameters hold at the
    // end of a callee function generate all of those in the caller context.
    return std::make_shared<MapFactsToCaller<container_type>>(
        llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt);
  }

  inline FlowFunctionPtrType
  getCallToRetFlowFunction(n_t callSite, n_t retSite,
                           std::set<f_t> callees) override {
    // Model call to heap allocating functions (new, new[], malloc, etc.) --
    // only model direct calls, though.
    if (callees.size() == 1) {
      for (const auto *Callee : callees) {
        if (this->ICF->isHeapAllocatingFunction(Callee)) {
          // In case a heap allocating function is called, generate the pointer
          // that is returned.
          //
          // Flow function:
          //
          // Let H be a heap allocating function.
          //
          //              0
          //              |\
          // x = call H   | \
          //              v  v
          //              0  x
          //
          return std::make_shared<Gen<d_t>>(callSite, this->getZeroValue());
        }
      }
    }
    // Just use the auto mapping for values, pointer parameters are killed and
    // handled by getCallFlowfunction() and getRetFlowFunction().
    return std::make_shared<MapFactsAlongsideCallSite<container_type>>(
        llvm::ImmutableCallSite(callSite));
  }

  inline FlowFunctionPtrType getSummaryFlowFunction(n_t callStmt,
                                                    f_t destMthd) override {
    // Do not use user-crafted summaries.
    return nullptr;
  }

  inline std::map<n_t, container_type> initialSeeds() override {
    std::map<n_t, container_type> SeedMap;
    for (const auto &EntryPoint : this->EntryPoints) {
      for (const auto *StartPoint :
           this->ICF->getStartPointsOf(this->ICF->getFunction(EntryPoint))) {
        SeedMap.insert(
            std::make_pair(StartPoint, container_type({this->getZeroValue()})));
      }
    }
    return SeedMap;
  }

  [[nodiscard]] inline d_t createZeroValue() const override {
    // Create a special value to represent the zero value!
    return LLVMZeroValue::getInstance();
  }

  inline bool isZeroValue(d_t d) const override { return isZeroValueImpl(d); }

  // In addition provide specifications for the IDE parts.

  inline std::shared_ptr<EdgeFunction<l_t>>
  getNormalEdgeFunction(n_t curr, d_t currNode, n_t succ,
                        d_t succNode) override {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                  << "Process edge: " << llvmIRToShortString(currNode) << " --"
                  << llvmIRToShortString(curr) << "--> "
                  << llvmIRToShortString(succNode));

    // Propagate zero edges as identity
    if (isZeroValue(currNode) && isZeroValue(succNode)) {
      return EdgeIdentity<l_t>::getInstance();
    }

    // check if the user has registered a fact generator function
    l_t UserEdgeFacts;
    std::set<e_t> EdgeFacts;
    if (edgeFactGen) {
      EdgeFacts = edgeFactGen(curr);
      // fill BitVectorSet
      UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
    }

    // override at store instructions
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
      if (SyntacticAnalysisOnly) {
        // check for the overriding edges at store instructions
        // store x y
        // case x and y ordinary variables
        // y obtains its values from x (and from the store itself)
        if (const auto *Load =
                llvm::dyn_cast<llvm::LoadInst>(Store->getValueOperand())) {
          if (Load->getPointerOperand() == currNode &&
              succNode == Store->getPointerOperand()) {
            LOG_IF_ENABLE([&]() {
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << "Var-Override: ";
              for (const auto &EF : EdgeFacts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG) << EF << ", ";
              }
              BOOST_LOG_SEV(lg::get(), DFADEBUG)
                  << "at '" << llvmIRToString(curr) << "'\n";
            }());
            return IIAAKillOrReplaceEF::createEdgeFunction(UserEdgeFacts);
          }
        }
        // kill all labels that are propagated along the edge of the value that
        // is overridden
        if ((currNode == succNode) &&
            (currNode == Store->getPointerOperand())) {
          if (llvm::isa<llvm::ConstantData>(Store->getValueOperand())) {
            // case x is a literal (and y an ordinary variable)
            // y obtains its values from its original allocation and this store
            LOG_IF_ENABLE([&]() {
              BOOST_LOG_SEV(lg::get(), DFADEBUG)
                  << "Const-Replace at '" << llvmIRToString(curr) << "'\n";
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << "Replacement label(s): ";
              for (const auto &Item : EdgeFacts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG) << Item << ", ";
              }
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n';
            }());
            // obtain label from the original allocation
            const llvm::AllocaInst *OrigAlloca = nullptr;
            if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(
                    Store->getPointerOperand())) {
              OrigAlloca = Alloca;
            }
            if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
                    Store->getPointerOperand())) {
              OrigAlloca = getAllocaInstruction(GEP);
            }
            // obtain the label
            if (OrigAlloca) {
              if (auto *UEF = std::get_if<BitVectorSet<e_t>>(&UserEdgeFacts)) {
                UEF->insert(edgeFactGenToBitVectorSet(OrigAlloca));
              }
            }
            return IIAAKillOrReplaceEF::createEdgeFunction(UserEdgeFacts);
          } else {
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "Kill at '" << llvmIRToString(curr) << "'\n");
            // obtain label from original allocation and add it
            const llvm::AllocaInst *OrigAlloca = nullptr;
            if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(
                    Store->getPointerOperand())) {
              OrigAlloca = Alloca;
            }
            if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(
                    Store->getPointerOperand())) {
              OrigAlloca = getAllocaInstruction(GEP);
            }
            // obtain the label
            if (OrigAlloca) {
              if (auto *UEF = std::get_if<BitVectorSet<e_t>>(&UserEdgeFacts)) {
                UEF->insert(edgeFactGenToBitVectorSet(OrigAlloca));
              }
            }
            return IIAAKillOrReplaceEF::createEdgeFunction(UserEdgeFacts);
          }
        }
      } else {
        // consider points-to information and find all possible overriding edges
        // using points-to sets
        std::shared_ptr<std::unordered_set<d_t>> ValuePTS;
        if (Store->getValueOperand()->getType()->isPointerTy()) {
          ValuePTS = this->PT->getPointsToSet(Store->getValueOperand());
        }
        auto PointerPTS = this->PT->getPointsToSet(Store->getPointerOperand());
        // overriding edge
        if ((currNode == Store->getValueOperand() ||
             (ValuePTS && ValuePTS->count(Store->getValueOperand())) ||
             llvm::isa<llvm::ConstantData>(Store->getValueOperand())) &&
            PointerPTS->count(Store->getPointerOperand())) {
          return IIAAKillOrReplaceEF::createEdgeFunction(UserEdgeFacts);
        }
        // kill all labels that are propagated along the edge of the
        // value/values that is/are overridden
        if (currNode == succNode && PointerPTS->count(currNode)) {
          return IIAAKillOrReplaceEF::createEdgeFunction(BitVectorSet<e_t>());
        }
      }
    }

    // check if the user has registered a fact generator function
    if (auto UEF = std::get_if<BitVectorSet<e_t>>(&UserEdgeFacts)) {
      if (!UEF->empty()) {
        // handle generating edges from zero
        // generate labels from zero when the instruction itself is the flow
        // fact that is generated
        if (isZeroValue(currNode) && curr == succNode) {
          return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
        }
        // handle edges that may add new labels to existing facts
        if (curr == currNode && currNode == succNode) {
          return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
        }
        // generate labels from zero when an operand of the current instruction
        // is a flow fact that is generated
        for (const auto &Op : curr->operands()) {
          // also propagate the labels if one of the operands holds
          if (isZeroValue(currNode) && Op == succNode) {
            return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
          }
          // handle edges that may add new labels to existing facts
          if (Op == currNode && currNode == succNode) {
            LOG_IF_ENABLE([&]() {
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << "this is 'i'\n";
              for (auto &EdgeFact : EdgeFacts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG) << EdgeFact << ", ";
              }
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n';
            }());
            return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
          }
          // handle edge that are drawn from existing facts
          if (Op == currNode && curr == succNode) {
            LOG_IF_ENABLE([&]() {
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << "this is '0'\n";
              for (auto &EdgeFact : EdgeFacts) {
                BOOST_LOG_SEV(lg::get(), DFADEBUG) << EdgeFact << ", ";
              }
              BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n';
            }());
            return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
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
    // Can be passed as identity.
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeMethod, n_t exitStmt,
                        d_t exitNode, n_t reSite, d_t retNode) override {
    // Can be passed as identity.
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override {
    // Just forward to getNormalEdgeFunction() to check whether a user has
    // additional labels for this call site.
    return getNormalEdgeFunction(callSite, callNode, retSite, retSiteNode);
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getSummaryEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                         d_t retSiteNode) override {
    // Do not use user-crafted summaries.
    return nullptr;
  }

  inline l_t topElement() override { return TopElement; }

  inline l_t bottomElement() override { return BottomElement; }

  inline l_t join(l_t Lhs, l_t Rhs) override { return joinImpl(Lhs, Rhs); }

  inline std::shared_ptr<EdgeFunction<l_t>> allTopFunction() override {
    return std::make_shared<AllTop<l_t>>(topElement());
  }

  // Provide some handy helper edge functions to improve reuse.

  // Edge function that kills all labels in a set (and may replaces them with
  // others).
  class IIAAKillOrReplaceEF
      : public EdgeFunction<l_t>,
        public std::enable_shared_from_this<IIAAKillOrReplaceEF>,
        public EdgeFunctionSingletonFactory<IIAAKillOrReplaceEF, l_t> {
  public:
    l_t Replacement;

    explicit IIAAKillOrReplaceEF() : Replacement(BitVectorSet<e_t>()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "IIAAKillOrReplaceEF");
    }

    explicit IIAAKillOrReplaceEF(l_t Replacement) : Replacement(Replacement) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "IIAAKillOrReplaceEF");
    }

    ~IIAAKillOrReplaceEF() override = default;

    l_t computeTarget(l_t Src) override { return Replacement; }

    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "IIAAKillOrReplaceEF::composeWith(): " << this->str()
                    << " * " << secondFunction->str());
      // kill or replace, previous functions are ignored
      if (auto *KR =
              dynamic_cast<IIAAKillOrReplaceEF *>(secondFunction.get())) {
        if (KR->isKillAll()) {
          return secondFunction;
        }
      }
      return this->shared_from_this();
    }

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) <<
      // "IIAAKillOrReplaceEF::joinWith");
      if (auto *AB = dynamic_cast<AllBottom<l_t> *>(otherFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *ID = dynamic_cast<EdgeIdentity<l_t> *>(otherFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *AD = dynamic_cast<IIAAAddLabelsEF *>(otherFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *KR = dynamic_cast<IIAAKillOrReplaceEF *>(otherFunction.get())) {
        Replacement =
            IDEInstInteractionAnalysisT::joinImpl(Replacement, KR->Replacement);
        return this->shared_from_this();
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAKillOrReplaceEF'");
    }

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) <<
      // "IIAAKillOrReplaceEF::equal_to");
      if (auto *I = dynamic_cast<IIAAKillOrReplaceEF *>(other.get())) {
        return Replacement == I->Replacement;
      }
      return this == other.get();
    }

    void print(std::ostream &OS, bool isForDebug = false) const override {
      OS << "EF: (IIAAKillOrReplaceEF)<->";
      if (isKillAll()) {
        OS << "(KillAll";
      } else {
        IDEInstInteractionAnalysisT::printEdgeFactImpl(OS, Replacement);
      }
      OS << ")";
    }

    [[nodiscard]] bool isKillAll() const {
      if (auto *RSet = std::get_if<BitVectorSet<e_t>>(&Replacement)) {
        return RSet->empty();
      }
      return false;
    }
  };

  // Edge function that adds the given labels to existing labels
  // add all labels provided by Data.
  class IIAAAddLabelsEF
      : public EdgeFunction<l_t>,
        public std::enable_shared_from_this<IIAAAddLabelsEF>,
        public EdgeFunctionSingletonFactory<IIAAAddLabelsEF, l_t> {
  public:
    const l_t Data;

    explicit IIAAAddLabelsEF(l_t Data) : Data(Data) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << "IIAAAddLabelsEF");
    }

    ~IIAAAddLabelsEF() override = default;

    l_t computeTarget(l_t Src) override {
      return IDEInstInteractionAnalysisT::joinImpl(Src, Data);
    }

    std::shared_ptr<EdgeFunction<l_t>>
    composeWith(std::shared_ptr<EdgeFunction<l_t>> secondFunction) override {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "IIAAAddLabelEF::composeWith(): " << this->str() << " * "
                    << secondFunction->str());
      if (auto *AB = dynamic_cast<AllBottom<l_t> *>(secondFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *EI = dynamic_cast<EdgeIdentity<l_t> *>(secondFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *AS = dynamic_cast<IIAAAddLabelsEF *>(secondFunction.get())) {
        auto Union = IDEInstInteractionAnalysisT::joinImpl(Data, AS->Data);
        return IIAAAddLabelsEF::createEdgeFunction(Union);
      }
      if (auto *KR =
              dynamic_cast<IIAAKillOrReplaceEF *>(secondFunction.get())) {
        return IIAAAddLabelsEF::createEdgeFunction(KR->Replacement);
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAAddLabelsEF'");
    }

    std::shared_ptr<EdgeFunction<l_t>>
    joinWith(std::shared_ptr<EdgeFunction<l_t>> otherFunction) override {
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) <<
      // "IIAAAddLabelsEF::joinWith");
      if (otherFunction.get() == this ||
          otherFunction->equal_to(this->shared_from_this())) {
        return this->shared_from_this();
      }
      if (auto *AT = dynamic_cast<AllTop<l_t> *>(otherFunction.get())) {
        return this->shared_from_this();
      }
      if (auto *AS = dynamic_cast<IIAAAddLabelsEF *>(otherFunction.get())) {
        auto Union = IDEInstInteractionAnalysisT::joinImpl(Data, AS->Data);
        return IIAAAddLabelsEF::createEdgeFunction(Union);
      }
      if (auto *KR = dynamic_cast<IIAAKillOrReplaceEF *>(otherFunction.get())) {
        auto Union =
            IDEInstInteractionAnalysisT::joinImpl(Data, KR->Replacement);
        return IIAAAddLabelsEF::createEdgeFunction(Union);
      }
      return std::make_shared<AllBottom<l_t>>(
          IDEInstInteractionAnalysisT::BottomElement);
    }

    [[nodiscard]] bool
    equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
      // std::cout << "IIAAAddLabelsEF::equal_to\n";
      if (auto *I = dynamic_cast<IIAAAddLabelsEF *>(other.get())) {
        return (I->Data == this->Data);
      }
      return this == other.get();
    }

    void print(std::ostream &OS, bool isForDebug = false) const override {
      OS << "EF: (IIAAAddLabelsEF: ";
      IDEInstInteractionAnalysisT::printEdgeFactImpl(OS, Data);
      OS << ")";
    }
  };

  // Provide functionalities for printing things and emitting text reports.

  void printNode(std::ostream &os, n_t n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os, d_t d) const override {
    os << llvmIRToString(d);
  }

  void printFunction(std::ostream &os, f_t m) const override {
    os << m->getName().str();
  }

  inline void printEdgeFact(std::ostream &os, l_t l) const override {
    printEdgeFactImpl(os, l);
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

protected:
  static inline bool isZeroValueImpl(d_t d) {
    return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
  }

  static void printEdgeFactImpl(std::ostream &os, l_t l) {
    if (std::holds_alternative<Top>(l)) {
      os << std::get<Top>(l);
    } else if (std::holds_alternative<Bottom>(l)) {
      os << std::get<Bottom>(l);
    } else {
      auto lset = std::get<BitVectorSet<e_t>>(l);
      os << "(set size: " << lset.size() << "), values: ";
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

  static inline l_t joinImpl(l_t Lhs, l_t Rhs) {
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

}; // namespace psr

using IDEInstInteractionAnalysis = IDEInstInteractionAnalysisT<>;

} // namespace psr

#endif
