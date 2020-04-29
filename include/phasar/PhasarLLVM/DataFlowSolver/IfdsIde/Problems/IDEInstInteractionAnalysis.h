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
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Gen.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/LambdaFlow.h"
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

  using EdgeFactGeneratorTy = std::set<e_t>(n_t curr);

private:
  std::function<EdgeFactGeneratorTy> edgeFactGen;
  static inline l_t BottomElement = Bottom{};
  static inline l_t TopElement = Top{};

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
                              const LLVMBasedICFG *ICF,
                              const LLVMPointsToInfo *PT,
                              std::set<std::string> EntryPoints = {"main"})
      : IDETabulationProblem<const llvm::Instruction *, const llvm::Value *,
                             const llvm::Function *, const llvm::StructType *,
                             const llvm::Value *,
                             LatticeDomain<BitVectorSet<EdgeFactType>>,
                             LLVMBasedICFG>(IRDB, TH, ICF, PT, EntryPoints) {
    this->ZeroValue =
        IDEInstInteractionAnalysisT<EdgeFactType, SyntacticAnalysisOnly,
                                    EnableIndirectTaints>::createZeroValue();
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

  std::shared_ptr<FlowFunction<d_t>> getNormalFlowFunction(n_t curr,
                                                           n_t succ) override {
    // generate all variables
    if (const auto *Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << "AllocaInst");
      return std::make_shared<Gen<d_t>>(Alloca, this->getZeroValue());
    }

    // handle indirect taints (propagate values that depend on branch conditions
    // whose operands are tainted)
    if (EnableIndirectTaints) {
      if (auto br = llvm::dyn_cast<llvm::BranchInst>(curr);
          br && br->isConditional()) {
        return std::make_shared<LambdaFlow<d_t>>([=](d_t src) {
          std::set<d_t> ret = {src, br};
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
              Facts.erase(src);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Store);
      }
    }

    // (i) handle syntactic propagation store instructions
    // in case store x y, we need to draw the edge x --> y such that we can
    // transfer x's labels to y
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
      if (const auto *Load =
              llvm::dyn_cast<llvm::LoadInst>(Store->getValueOperand())) {
        struct IIAAFlowFunction : FlowFunction<d_t> {
          const llvm::StoreInst *Store;
          const llvm::LoadInst *Load;
          IIAAFlowFunction(const llvm::StoreInst *S, const llvm::LoadInst *L)
              : Store(S), Load(L) {}
          ~IIAAFlowFunction() override = default;

          std::set<d_t> computeTargets(d_t src) override {
            std::set<d_t> Facts;
            if (Load == src || Load->getPointerOperand() == src) {
              Facts.insert(src);
              Facts.insert(Load->getPointerOperand());
              Facts.insert(Store->getPointerOperand());
            } else {
              Facts.insert(src);
            }
            for (const auto s : Facts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                            << "Create edge: " << llvmIRToShortString(src)
                            << " --" << llvmIRToShortString(Store) << "--> "
                            << llvmIRToShortString(s));
            }
            return Facts;
          }
        };
        return std::make_shared<IIAAFlowFunction>(Store, Load);
      } else {
        struct IIAAFlowFunction : FlowFunction<d_t> {
          const llvm::StoreInst *Store;
          IIAAFlowFunction(const llvm::StoreInst *S) : Store(S) {}
          ~IIAAFlowFunction() override = default;

          std::set<d_t> computeTargets(d_t src) override {
            std::set<d_t> Facts;
            if (Store->getValueOperand() == src) {
              Facts.insert(src);
              Facts.insert(Store->getPointerOperand());
            } else {
              Facts.insert(src);
            }
            for (const auto s : Facts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                            << "Create edge: " << llvmIRToShortString(src)
                            << " --" << llvmIRToShortString(Store) << "--> "
                            << llvmIRToShortString(s));
            }
            return Facts;
          }
        };
        return std::make_shared<IIAAFlowFunction>(Store);
      }
    }
    // and now we can handle all other statements
    struct IIAFlowFunction : FlowFunction<d_t> {
      IDEInstInteractionAnalysisT &Problem;
      n_t Inst;

      IIAFlowFunction(IDEInstInteractionAnalysisT &Problem, n_t Inst)
          : Problem(Problem), Inst(Inst) {}

      ~IIAFlowFunction() override = default;

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
        for (const auto s : Facts) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                        << "Create edge: " << llvmIRToShortString(src) << " --"
                        << llvmIRToShortString(Inst) << "--> "
                        << llvmIRToShortString(s));
        }
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
    // do not use user-crafted summaries
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
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                  << "Process edge: " << llvmIRToShortString(currNode) << " --"
                  << llvmIRToShortString(curr) << "--> "
                  << llvmIRToShortString(succNode));

    // propagate zero edges as identity
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
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "Var-Override: ");
            for (const auto &EF : EdgeFacts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << EF << ", ");
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "at '" << llvmIRToString(curr) << "'\n");
            return std::make_shared<IIAAKillOrReplaceEF>(*this, UserEdgeFacts);
          }
        }
        // kill all labels that are propagated along the edge of the value that
        // is overridden
        if ((currNode == succNode) &&
            (currNode == Store->getPointerOperand())) {
          if (llvm::isa<llvm::ConstantData>(Store->getValueOperand())) {
            // case x is a literal (and y an ordinary variable)
            // y obtains its values from its original allocation and this store
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "Const-Replace at '" << llvmIRToString(curr)
                          << "'\n");
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "Replacement label(s): ");
            for (const auto &Item : EdgeFacts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << Item << ", ");
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n');
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
            return std::make_shared<IIAAKillOrReplaceEF>(*this, UserEdgeFacts);
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
            return std::make_shared<IIAAKillOrReplaceEF>(*this, UserEdgeFacts);
          }
        }
      } else {
        // consider points-to information and find all possible overriding edges
        // using points-to sets
        std::set<d_t> ValuePTS;
        if (Store->getValueOperand()->getType()->isPointerTy()) {
          ValuePTS = this->PT->getPointsToSet(Store->getValueOperand());
        }
        std::set<d_t> PointerPTS =
            this->PT->getPointsToSet(Store->getPointerOperand());
        // overriding edge
        if ((currNode == Store->getValueOperand() ||
             ValuePTS.count(Store->getValueOperand()) ||
             llvm::isa<llvm::ConstantData>(Store->getValueOperand())) &&
            PointerPTS.count(Store->getPointerOperand())) {
          return std::make_shared<IIAAKillOrReplaceEF>(*this, UserEdgeFacts);
        }
        // kill all labels that are propagated along the edge of the
        // value/values that is/are overridden
        if (currNode == succNode && PointerPTS.count(currNode)) {
          return std::make_shared<IIAAKillOrReplaceEF>(*this);
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
          return std::make_shared<IIAAAddLabelsEF>(*this, UserEdgeFacts);
        }
        // handle edges that may add new labels to existing facts
        if (curr == currNode && currNode == succNode) {
          return std::make_shared<IIAAAddLabelsEF>(*this, UserEdgeFacts);
        }
        // generate labels from zero when an operand of the current instruction
        // is a flow fact that is generated
        for (auto &Op : curr->operands()) {
          // also propagate the labels if one of the operands holds
          if (isZeroValue(currNode) && Op == succNode) {
            return std::make_shared<IIAAAddLabelsEF>(*this, UserEdgeFacts);
          }
          // handle edges that may add new labels to existing facts
          if (Op == currNode && currNode == succNode) {
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "this is 'i'\n");
            for (auto &EdgeFact : EdgeFacts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                            << EdgeFact << ", ");
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n');
            return std::make_shared<IIAAAddLabelsEF>(*this, UserEdgeFacts);
          }
          // handle edge that are drawn from existing facts
          if (Op == currNode && curr == succNode) {
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                          << "this is '0'\n");
            for (auto &EdgeFact : EdgeFacts) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                            << EdgeFact << ", ");
            }
            LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << '\n');
            return std::make_shared<IIAAAddLabelsEF>(*this, UserEdgeFacts);
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
    // do not use user-crafted summaries
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

  // edge function that kills all labels in a set (and may replaces them with
  // others)
  class IIAAKillOrReplaceEF
      : public EdgeFunction<l_t>,
        public std::enable_shared_from_this<IIAAKillOrReplaceEF> {
  private:
    IDEInstInteractionAnalysisT<e_t, SyntacticAnalysisOnly,
                                EnableIndirectTaints> &Analysis;

  public:
    l_t Replacement;

    explicit IIAAKillOrReplaceEF(
        IDEInstInteractionAnalysisT<e_t, SyntacticAnalysisOnly,
                                    EnableIndirectTaints> &Analysis)
        : Analysis(Analysis), Replacement(BitVectorSet<e_t>()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG)
                    << "IIAAKillOrReplaceEF");
    }

    explicit IIAAKillOrReplaceEF(
        IDEInstInteractionAnalysisT<e_t, SyntacticAnalysisOnly,
                                    EnableIndirectTaints> &Analysis,
        l_t Replacement)
        : Analysis(Analysis), Replacement(Replacement) {
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
        // auto Union = Analysis.join(Replacement, AD->Data);
        // return std::make_shared<IIAAAddLabelsEF>(Analysis, Union);
        return this->shared_from_this();
      }
      if (auto *KR = dynamic_cast<IIAAKillOrReplaceEF *>(otherFunction.get())) {
        auto Union = Analysis.join(Replacement, KR->Replacement);
        return std::make_shared<IIAAAddLabelsEF>(Analysis, Union);
      }
      llvm::report_fatal_error(
          "found unexpected edge function in 'IIAAKillOrReplaceEF'");
      // return otherFunction;
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
        Analysis.printEdgeFact(OS, Replacement);
      }
      OS << ")";
    }

    bool isKillAll() const {
      if (auto *RSet = std::get_if<BitVectorSet<e_t>>(&Replacement)) {
        return RSet->empty();
      }
      return false;
    }
  };

  // edge function that adds the given labels to existing labels
  // add all labels provided by 'Data'
  class IIAAAddLabelsEF : public EdgeFunction<l_t>,
                          public std::enable_shared_from_this<IIAAAddLabelsEF> {
  private:
    IDEInstInteractionAnalysisT<e_t, SyntacticAnalysisOnly,
                                EnableIndirectTaints> &Analysis;

  public:
    l_t Data;

    explicit IIAAAddLabelsEF(
        IDEInstInteractionAnalysisT<e_t, SyntacticAnalysisOnly,
                                    EnableIndirectTaints> &Analysis,
        l_t Data)
        : Analysis(Analysis), Data(Data) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) << "IIAAAddLabelsEF");
    }

    ~IIAAAddLabelsEF() override = default;

    l_t computeTarget(l_t Src) override {
      return Analysis.join(Src, Data);
      // return Src.setUnion(Data);
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
        auto Union = Analysis.join(Data, AS->Data);
        return std::make_shared<IIAAAddLabelsEF>(Analysis, Union);
      }
      if (auto *KR =
              dynamic_cast<IIAAKillOrReplaceEF *>(secondFunction.get())) {
        // auto Union = Analysis.join(Data, KR->Replacement);
        return std::make_shared<IIAAAddLabelsEF>(Analysis, KR->Replacement);
        // return secondFunction;
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
        // auto Union = Data.setUnion(AS->Data);
        auto Union = Analysis.join(Data, AS->Data);
        return std::make_shared<IIAAAddLabelsEF>(Analysis, Union);
      }
      if (auto *KR = dynamic_cast<IIAAKillOrReplaceEF *>(otherFunction.get())) {
        auto Union = Analysis.join(Data, KR->Replacement);
        return std::make_shared<IIAAAddLabelsEF>(Analysis, Union);
      }
      return std::make_shared<AllBottom<l_t>>(Analysis.BottomElement);
    }

    bool equal_to(std::shared_ptr<EdgeFunction<l_t>> other) const override {
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DFADEBUG) <<
      // "IIAAAddLabelsEF::equal_to");
      if (auto *I = dynamic_cast<IIAAAddLabelsEF *>(other.get())) {
        return (I->Data == this->Data);
      }
      return this == other.get();
    }

    void print(std::ostream &OS, bool isForDebug = false) const override {
      OS << "EF: (IIAAAddLabelsEF: ";
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
