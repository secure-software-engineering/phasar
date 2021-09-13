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
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "llvm/IR/Attributes.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctionComposer.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Solver/SolverResults.h"
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

namespace vara {
class Taint;
} // namespace vara

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

  using EdgeFactGeneratorTy =
      std::set<e_t>(std::variant<n_t, const llvm::GlobalVariable *> curr);

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

  /// Offer a special hook to the user that allows to generate additional
  /// edge facts on-the-fly. Above the generator function, the ordinary
  /// edge facts are generated according to the usual edge functions.
  inline void registerEdgeFactGenerator(
      std::function<EdgeFactGeneratorTy> EdgeFactGenerator) {
    edgeFactGen = std::move(EdgeFactGenerator);
  }

  // start formulating our analysis by specifying the parts required for IFDS

  FlowFunctionPtrType getNormalFlowFunction(n_t curr, n_t succ) override {
    // Generate all global variables and handle the instruction that we
    // currently misuse to generate them.
    // TODO The handling of global variables, global constructors and global
    // destructors will soon be possible using a dedicated mechanism.
    //
    // Flow function:
    //
    // Let G be the set of global variables.
    //
    //                    0
    //                    |\
    // some instruction   | \--\
    //                    v  v  v
    //                    0  x  G
    //
    static auto Seeds = this->initialSeeds();
    static bool initGlobals = true;
    if (initGlobals && Seeds.containsInitialSeedsFor(curr)) {
      initGlobals = false;
      std::set<d_t> Globals;
      for (const auto *Mod : this->IRDB->getAllModules()) {
        for (const auto &Global : Mod->globals()) {
          Globals.insert(&Global); // collect all global variables
        }
      }
      // Create the flow function that generates the globals.
      auto GlobalFlowFun =
          std::make_shared<GenAll<d_t>>(Globals, this->getZeroValue());
      // Create the flow function for the instruction we are currently misusing.
      auto FlowFun = getNormalFlowFunction(curr, succ);
      return std::make_shared<Union<d_t>>(
          std::vector<FlowFunctionPtrType>({FlowFun, GlobalFlowFun}));
    }

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
    if constexpr (EnableIndirectTaints) {
      if (const auto *Br = llvm::dyn_cast<llvm::BranchInst>(curr);
          Br && Br->isConditional()) {
        // If the branch is conditional and its condition is tainted, then we
        // need to propagates the instructions that are depending on this
        // branch, too.
        //
        // Flow function:
        //
        // Let I be the set of instructions of the branch instruction's
        // successors.
        //
        //                                          0  c  x
        //                                          |  |\
        // x = br C, label if.then, label if.else   |  | \--\
        //                                          v  v  v  v
        //                                          0  c  x  I
        //
        struct IIAFlowFunction : FlowFunction<d_t, container_type> {
          const llvm::BranchInst *Br;

          IIAFlowFunction(const llvm::BranchInst *Br) : Br(Br) {}

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (src == Br->getCondition()) {
              Facts.insert(Br);
              for (const auto *Succs : Br->successors()) {
                for (const auto &Inst : Succs->instructionsWithoutDebug()) {
                  Facts.insert(&Inst);
                }
              }
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(Br);
      }
    }

    // Handle points-to information if the user wishes to conduct a
    // non-syntax-only inst-interaction analysis.
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
        //              |  |\
        // x = load y   |  | \
        //              v  v  v
        //              0  Y  x
        //
        struct IIAFlowFunction : FlowFunction<d_t, container_type> {
          const llvm::LoadInst *Load;
          std::shared_ptr<std::unordered_set<d_t>> PTS;

          IIAFlowFunction(IDEInstInteractionAnalysisT &Problem,
                          const llvm::LoadInst *Load)
              : Load(Load), PTS(Problem.PT->getReachableAllocationSites(
                                Load->getPointerOperand(),
                                Problem.OnlyConsiderLocalAliases)) {}

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (PTS->count(src)) {
              Facts.insert(Load);
            }
            // Handle global variables which behave a bit special.
            if (PTS->empty() && src == Load->getPointerOperand()) {
              Facts.insert(Load);
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Load);
      }

      // (ii) Handle semantic propagation (pointers) for store instructions.
      if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
        // If the value to be stored holds, the potential memory location(s)
        // that it is stored to must be generated and populated, too.
        //
        // Flow function:
        //
        // Let X be
        //    - pts(x), the points-to set of x, if x is an intersting pointer.
        //    - a singleton set containing x, otherwise.
        //
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
                    return Problem.PT->getReachableAllocationSites(
                        Store->getValueOperand(),
                        Problem.OnlyConsiderLocalAliases);
                  } else {
                    return std::make_shared<std::unordered_set<d_t>>(
                        std::unordered_set<d_t>{Store->getValueOperand()});
                  }
                }()),
                PointerPTS(Problem.PT->getReachableAllocationSites(
                    Store->getPointerOperand(),
                    Problem.OnlyConsiderLocalAliases)) {}

          container_type computeTargets(d_t src) override {
            container_type Facts;
            Facts.insert(src);
            if (IDEInstInteractionAnalysisT::isZeroValueImpl(src)) {
              return Facts;
            }
            // If a value is stored that holds we must generate all potential
            // memory locations the store might write to.
            if (Store->getValueOperand() == src || ValuePTS->count(src)) {
              Facts.insert(Store->getValueOperand());
              Facts.insert(Store->getPointerOperand());
              Facts.insert(PointerPTS->begin(), PointerPTS->end());
            }
            return Facts;
          }
        };
        return std::make_shared<IIAFlowFunction>(*this, Store);
      }
    }

    // (i) Handle syntactic propagation

    // Handle load instruction
    //
    // Flow function:
    //
    //              0  y  x
    //              |  |\
    // x = load y   |  | \
    //              v  v  v
    //              0  y  x
    //
    if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
      return std::make_shared<Gen<d_t>>(Load, Load->getPointerOperand());
    }
    // Handle store instructions
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
    // At last, we can handle all other (unary/binary) instructions.
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

  inline FlowFunctionPtrType getCallFlowFunction(n_t callSite,
                                                 f_t destMthd) override {
    if (this->ICF->isHeapAllocatingFunction(destMthd)) {
      // Kill add facts and model the effects in getCallToRetFlowFunction().
      return KillAll<d_t>::getInstance();
    } else if (destMthd->isDeclaration()) {
      // We don't have anything that we could analyze, kill all facts.
      return KillAll<d_t>::getInstance();
    }
    const auto *CS = llvm::cast<llvm::CallBase>(callSite);
    // Map actual to formal parameters.
    auto AutoMapping = std::make_shared<MapFactsToCallee<container_type>>(
        CS, destMthd, true /* map globals to callee, too */,
        // Do not map parameters that have been artificially introduced by the
        // compiler for RVO (return value optimization). Instead, these values
        // need to be generated from the zero value.
        [CS](const llvm::Value *V) {
          bool PassParameter = true;
          for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
            if (V == CS->getArgOperand(Idx)) {
              return !CS->paramHasAttr(Idx, llvm::Attribute::StructRet);
            }
          }
          return PassParameter;
        });
    // Generate the artificially introduced RVO parameters from zero value.
    std::set<d_t> SRetFormals;
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      if (CS->paramHasAttr(Idx, llvm::Attribute::StructRet)) {
        SRetFormals.insert(destMthd->getArg(Idx));
      }
    }
    auto GenSRetFormals = std::make_shared<GenAllAndKillAllOthers<d_t>>(
        SRetFormals, this->getZeroValue());
    return std::make_shared<Union<d_t>>(
        std::vector<FlowFunctionPtrType>({AutoMapping, GenSRetFormals}));
  }

  inline FlowFunctionPtrType getRetFlowFunction(n_t callSite, f_t calleeMthd,
                                                n_t exitInst,
                                                n_t retSite) override {
    // Map return value back to the caller. If pointer parameters hold at the
    // end of a callee function generate all of those in the caller context.
    auto AutoMapping = std::make_shared<MapFactsToCaller<container_type>>(
        llvm::cast<llvm::CallBase>(callSite), calleeMthd, exitInst,
        true /* map globals back to caller, too */);
    // We must also handle the special case if the returned value is a constant
    // literal, e.g. ret i32 42.
    if (exitInst) {
      if (const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(exitInst)) {
        const auto *RetVal = Ret->getReturnValue();
        if (RetVal) {
          if (const auto *CD = llvm::dyn_cast<llvm::ConstantData>(RetVal)) {
            // Generate the respective callsite. The callsite will receive its
            // value from this very return instruction cf.
            // getReturnEdgeFunction().
            auto ConstantRetGen = std::make_shared<GenAndKillAllOthers<d_t>>(
                callSite, this->getZeroValue());
            return std::make_shared<Union<d_t>>(
                std::vector<FlowFunctionPtrType>(
                    {AutoMapping, ConstantRetGen}));
          }
        }
      }
    }
    return AutoMapping;
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
    // Just use the auto mapping for values, pointer parameters and global
    // variables are killed and handled by getCallFlowfunction() and
    // getRetFlowFunction().
    // However, if only declarations are available as callee targets we would
    // lose the data-flow facts involved in the call which is usually not the
    // behavior that is intended. In that case, we must propagate all data-flow
    // facts alongside the call site.
    bool OnlyDecls = true;
    for (auto callee : callees) {
      if (!callee->isDeclaration()) {
        OnlyDecls = false;
      }
    }
    // Declarations only case
    return std::make_shared<MapFactsAlongsideCallSite<container_type>>(
        llvm::cast<llvm::CallBase>(callSite),
        true /* propagate globals alongsite the call site */,
        [](const llvm::CallBase *CS, const llvm::Value *V) {
          return false; // not involved in the call
        });
    // Otherwise
    return std::make_shared<MapFactsAlongsideCallSite<container_type>>(
        llvm::cast<llvm::CallBase>(callSite),
        false // do not propagate globals (as they are propagated via call- and
              // ret-functions)
    );
  }

  inline FlowFunctionPtrType getSummaryFlowFunction(n_t callSite,
                                                    f_t destMthd) override {
    // Do not use user-crafted summaries.
    return nullptr;
  }

  inline InitialSeeds<n_t, d_t, l_t> initialSeeds() override {
    InitialSeeds<n_t, d_t, l_t> Seeds;
    for (const auto &EntryPoint : this->EntryPoints) {
      for (const auto *StartPoint :
           this->ICF->getStartPointsOf(this->ICF->getFunction(EntryPoint))) {
        Seeds.addSeed(StartPoint, this->getZeroValue(), this->bottomElement());
      }
    }
    return Seeds;
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
    //
    // Zero --> Zero edges
    //
    // Edge function:
    //
    //                     0
    //                     |
    // %i = instruction    | \x.BOT
    //                     v
    //                     0
    //
    if (isZeroValue(currNode) && isZeroValue(succNode)) {
      return std::make_shared<AllBottom<l_t>>(bottomElement());
    }
    // check if the user has registered a fact generator function
    l_t UserEdgeFacts = BitVectorSet<e_t>();
    std::set<e_t> EdgeFacts;
    if (edgeFactGen) {
      EdgeFacts = edgeFactGen(curr);
      // fill BitVectorSet
      UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
    }
    //
    // Zero --> Alloca edges
    //
    // Edge function:
    //
    //                0
    //                 \
    // %a = alloca      \ \x.x \cup { commit of('%a = alloca') }
    //                   v
    //                   a
    //
    if (isZeroValue(currNode) && curr == succNode) {
      if (llvm::isa<llvm::AllocaInst>(curr)) {
        return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
      }
    }
    // TODO use new mechanism to handle globals.
    //
    // Zero --> Global edges
    //
    // Edge function:
    //
    // Let g be a global variable.
    //
    //                0
    //                 \
    // %a = alloca      \ \x.x \cup { commit of('@global') }
    //                   v
    //                   g
    //
    static auto Seeds = this->initialSeeds();
    static auto Globals = [this]() {
      std::set<d_t> Globals;
      for (const auto *Mod : this->IRDB->getAllModules()) {
        for (const auto &G : Mod->globals()) {
          Globals.insert(&G);
        }
      }
      return Globals;
    }();
    if (Seeds.containsInitialSeedsFor(curr) && isZeroValue(currNode) &&
        Globals.count(succNode)) {
      if (const auto *GlobalVarDef =
              llvm::dyn_cast<llvm::GlobalVariable>(succNode)) {
        EdgeFacts = edgeFactGen(GlobalVarDef);
        // fill BitVectorSet
        UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
        return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
      }
    }
    //
    // i --> i edges
    //
    // Edge function:
    //
    //                    i
    //                    |
    // %i = instruction   | \x.x \cup { commit of('%i = instruction') }
    //                    v
    //                    i
    //
    if (curr == currNode && currNode == succNode) {
      return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
    }
    // Handle loads in non-syntax only analysis
    if constexpr (!SyntacticAnalysisOnly) {
      if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
        //
        // y --> x
        //
        // Edge function:
        //
        //             y
        //              \
        // x = load y    \ \x.{ commit of('x = load y') } \cup { commits of y }
        //                v
        //                x
        //
        if ((currNode == Load->getPointerOperand() ||
             this->PT->isInReachableAllocationSites(
                 Load->getPointerOperand(), currNode,
                 OnlyConsiderLocalAliases)) &&
            Load == succNode) {
          IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
        } else {
          //
          // y --> y
          //
          // Edge function:
          //
          //             y
          //             |
          // x = load y  | \x.x (loads do not modify the value that is loaded
          // from)
          //             v
          //             y
          //
          return EdgeIdentity<l_t>::getInstance();
        }
      }
    }
    // Overrides at store instructions
    if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
      if (SyntacticAnalysisOnly) {
        //
        // x --> y
        //
        // Edge function:
        //
        //             x
        //              \
        // store x y     \ \x.{ commit of('store x y') }
        //                v
        //                y
        //
        if (currNode == Store->getValueOperand() &&
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
        // Kill all labels that are propagated along the edge of the value that
        // is overridden.
        //
        // y --> y
        //
        // Edge function:
        //
        //               y
        //               |
        // store x y     | \x.{ commit of('store x y') }
        //               v
        //               y
        //
        if ((currNode == succNode) && currNode == Store->getPointerOperand()) {
          // y obtains its value(s) from its original allocation and the store
          // instruction under analysis.
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
        }
      } else {
        // Use points-to information to find all possible overriding edges.

        // Overriding edge with literal: kill all labels that are propagated
        // along the edge of the value that is overridden.
        //
        // x --> y
        //
        // Edge function:
        //
        // Let x be a literal.
        //
        //               y
        //               |
        // store x y     | \x.{} \cup { commit of('store x y') }
        //               v
        //               y
        //
        if (llvm::isa<llvm::ConstantData>(Store->getValueOperand()) &&
            currNode == succNode &&
            (this->PT->isInReachableAllocationSites(Store->getPointerOperand(),
                                                    currNode,
                                                    OnlyConsiderLocalAliases) ||
             Store->getPointerOperand() == currNode)) {
          return IIAAKillOrReplaceEF::createEdgeFunction(UserEdgeFacts);
        }
        // Overriding edge: obtain labels from value to be stored (and may add
        // UserEdgeFacts, if any).
        //
        // x --> y
        //
        // Edge function:
        //
        //            x
        //             \
        // store x y    \ \x.x \cup { commit of('store x y') }
        //               v
        //               y
        //
        bool StoreValOpIsPointerTy =
            Store->getValueOperand()->getType()->isPointerTy();
        if ((currNode == Store->getValueOperand() ||
             (StoreValOpIsPointerTy &&
              this->PT->isInReachableAllocationSites(
                  Store->getValueOperand(), Store->getValueOperand(),
                  OnlyConsiderLocalAliases))) &&
            this->PT->isInReachableAllocationSites(Store->getPointerOperand(),
                                                   Store->getPointerOperand(),
                                                   OnlyConsiderLocalAliases)) {
          return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
        }
        // Kill all labels that are propagated along the edge of the
        // value/values that is/are overridden.
        //
        // y --> y
        //
        // Edge function:
        //
        //            y
        //            |
        // store x y  | \x.{}
        //            v
        //            y
        //
        if (currNode == succNode && this->PT->isInReachableAllocationSites(
                                        Store->getPointerOperand(), currNode,
                                        OnlyConsiderLocalAliases)) {
          return IIAAKillOrReplaceEF::createEdgeFunction(BitVectorSet<e_t>());
        }
      }
    }
    // Handle edge functions for general instructions.
    for (const auto &Op : curr->operands()) {
      //
      // 0 --> o_i
      //
      // Edge function:
      //
      //                        0
      //                         \
      // %i = instruction o_i     \ \x.x \cup { commit of('%i = instruction') }
      //                           v
      //                           o_i
      //
      if (isZeroValue(currNode) && Op == succNode) {
        return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
      }
      //
      // o_i --> o_i
      //
      // Edge function:
      //
      //                        o_i
      //                        |
      // %i = instruction o_i   | \x.x \cup { commit of('%i = instruction') }
      //                        v
      //                        o_i
      //
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
      //
      // o_i --> i
      //
      // Edge function:
      //
      //                        o_i
      //                         \
      // %i = instruction o_i     \ \x.x \cup { commit of('%i = instruction') }
      //                           v
      //                           i
      //
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
    // Otherwise stick to identity.
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getCallEdgeFunction(n_t callSite, d_t srcNode, f_t destinationMethod,
                      d_t destNode) override {
    // Handle the case in which a parameter that has been artificially
    // introduced by the compiler is passed. Such a value must be generated from
    // the zero value, to reflact the fact that the data flows from the callee
    // to the caller (at least) according to the source code.
    //
    // Let a_i be an argument that is annotated by the sret attribute.
    //
    // 0 --> a_i
    //
    // Edge function:
    //
    //                      0
    //                       \
    // call/invoke f(a_i)     \ \x.{}
    //                         v
    //                         a_i
    //
    std::set<d_t> SRetParams;
    const auto *CS = llvm::cast<llvm::CallBase>(callSite);
    for (unsigned Idx = 0; Idx < CS->arg_size(); ++Idx) {
      if (CS->paramHasAttr(Idx, llvm::Attribute::StructRet)) {
        SRetParams.insert(CS->getArgOperand(Idx));
      }
    }
    if (isZeroValue(srcNode) && SRetParams.count(destNode)) {
      return IIAAAddLabelsEF::createEdgeFunction(BitVectorSet<e_t>());
    }
    // Everything else can be passed as identity.
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getReturnEdgeFunction(n_t callSite, f_t calleeMethod, n_t exitInst,
                        d_t exitNode, n_t reSite, d_t retNode) override {
    // Handle the case in which constant data is returned, e.g. ret i32 42.
    //
    // Let c be the return instruction's corresponding call site.
    //
    // 0 --> c
    //
    // Edge function:
    //
    //               0
    //                \
    // ret x           \ \x.x \cup { commit of('ret x') }
    //                  v
    //                  c
    //
    if (isZeroValue(exitNode) && retNode == callSite) {
      const auto *Ret = llvm::dyn_cast<llvm::ReturnInst>(exitInst);
      if (const auto *CD =
              llvm::dyn_cast<llvm::ConstantData>(Ret->getReturnValue())) {
        // Check if the user has registered a fact generator function
        l_t UserEdgeFacts = BitVectorSet<e_t>();
        std::set<e_t> EdgeFacts;
        if (edgeFactGen) {
          EdgeFacts = edgeFactGen(exitInst);
          // fill BitVectorSet
          UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
        }
        return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
      }
    }
    // Everything else can be passed as identity.
    return EdgeIdentity<l_t>::getInstance();
  }

  inline std::shared_ptr<EdgeFunction<l_t>>
  getCallToRetEdgeFunction(n_t callSite, d_t callNode, n_t retSite,
                           d_t retSiteNode, std::set<f_t> callees) override {
    // Check if the user has registered a fact generator function
    l_t UserEdgeFacts = BitVectorSet<e_t>();
    std::set<e_t> EdgeFacts;
    if (edgeFactGen) {
      EdgeFacts = edgeFactGen(callSite);
      // fill BitVectorSet
      UserEdgeFacts = BitVectorSet<e_t>(EdgeFacts.begin(), EdgeFacts.end());
    }
    // Model call to heap allocating functions (new, new[], malloc, etc.) --
    // only model direct calls, though.
    if (callees.size() == 1) {
      for (const auto *Callee : callees) {
        if (this->ICF->isHeapAllocatingFunction(Callee)) {
          // Let H be a heap allocating function.
          //
          // 0 --> x
          //
          // Edge function:
          //
          //               0
          //                \
          // %i = call H     \ \x.x \cup { commit of('%i = call H') }
          //                  v
          //                  i
          //
          if (isZeroValue(callNode) && retSiteNode == callSite) {
            return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
          }
        }
      }
    }
    // Capture interactions of the call instruction and its operands.
    for (const auto &Op : callSite->operands()) {
      //
      // o_i --> o_i
      //
      // Edge function:
      //
      //                 o_i
      //                 |
      // %i = call o_i   | \ \x.x \cup { commit of('%i = call H') }
      //                 v
      //                 o_i
      //
      if (callNode == Op && callNode == retSiteNode) {
        return IIAAAddLabelsEF::createEdgeFunction(UserEdgeFacts);
      }
    }
    // Otherwise stick to identity
    return EdgeIdentity<l_t>::getInstance();
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

  /// Computes all variables where a result set has been computed using the
  /// edge functions (and respective value domain).
  inline std::unordered_set<d_t>
  getAllVariables(const SolverResults<n_t, d_t, l_t> &Solution) const {
    std::unordered_set<d_t> Variables;
    // collect all variables that are available
    for (const auto *M : this->IRDB->getAllModules()) {
      for (const auto &G : M->globals()) {
        Variables.insert(&G);
      }
      for (const auto &F : *M) {
        for (const auto &BB : F) {
          for (const auto &I : BB) {
            if (const auto *A = llvm::dyn_cast<llvm::AllocaInst>(&I)) {
              Variables.insert(A);
            }
            if (const auto *H = llvm::dyn_cast<llvm::CallBase>(&I)) {
              if (!H->isIndirectCall() && H->getCalledFunction() &&
                  this->ICF->isHeapAllocatingFunction(H->getCalledFunction())) {
                Variables.insert(H);
              }
            }
          }
        }
      }
    }

    return Variables;
  }

  /// Computes all variables for which an empty set has been computed using the
  /// edge functions (and respective value domain).
  inline std::unordered_set<d_t> getAllVariablesWithEmptySetValue(
      const SolverResults<n_t, d_t, l_t> &Solution) const {
    return removeVariablesWithoutEmptySetValue(Solution,
                                               getAllVariables(Solution));
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
      os << "(set size: " << lset.size() << ") values: ";
      if constexpr (std::is_same_v<e_t, vara::Taint *>) {
        for (const auto &s : lset) {
          std::string IRBuffer;
          llvm::raw_string_ostream RSO(IRBuffer);
          s->print(RSO);
          RSO.flush();
          os << IRBuffer << ", ";
        }
      } else {
        for (const auto &s : lset) {
          os << s << ", ";
        }
      }
    }
  }

  static inline l_t joinImpl(l_t Lhs, l_t Rhs) {
    if (Lhs == TopElement || Lhs == BottomElement) {
      return Rhs;
    }
    if (Rhs == TopElement || Rhs == BottomElement) {
      return Lhs;
    }
    auto LhsSet = std::get<BitVectorSet<e_t>>(Lhs);
    auto RhsSet = std::get<BitVectorSet<e_t>>(Rhs);
    return LhsSet.setUnion(RhsSet);
  }

private:
  /// Filters out all variables that had a non empty set during edge functions
  /// computations.
  inline std::unordered_set<d_t> removeVariablesWithoutEmptySetValue(
      const SolverResults<n_t, d_t, l_t> &Solution,
      std::unordered_set<d_t> Variables) const {
    // Check the solver results and remove all variables for which a
    // non-empty set has been computed
    auto Results = Solution.getAllResultEntries();
    for (const auto &Result : Results) {
      // We do not care for the concrete instruction at which data-flow facts
      // hold, instead we just wish to find out if a variable has been generated
      // at some point. Therefore, we only care for the variables and their
      // associated values and ignore at which point a variable may holds as a
      // data-flow fact.
      const auto *Variable = Result.getColumnKey();
      const auto &Value = Result.getValue();
      // skip result entry if variable is not in the set of all variables
      if (Variables.find(Variable) == Variables.end()) {
        continue;
      }
      // skip result entry if the computed value is not of type BitVectorSet
      if (!std::holds_alternative<BitVectorSet<e_t>>(Value)) {
        continue;
      }
      // remove variable from result set if a non-empty that has been computed
      auto &Values = std::get<BitVectorSet<e_t>>(Value);
      if (!Values.empty()) {
        Variables.erase(Variable);
      }
    }
    return Variables;
  }

  std::function<EdgeFactGeneratorTy> edgeFactGen;
  static inline const l_t BottomElement = Bottom{};
  static inline const l_t TopElement = Top{};
  const bool OnlyConsiderLocalAliases = true;

  inline BitVectorSet<e_t> edgeFactGenToBitVectorSet(n_t curr) {
    if (edgeFactGen) {
      auto Results = edgeFactGen(curr);
      BitVectorSet<e_t> BVS(Results.begin(), Results.end());
      return BVS;
    }
    return {};
  }

}; // namespace psr

using IDEInstInteractionAnalysis = IDEInstInteractionAnalysisT<>;

} // namespace psr

#endif
