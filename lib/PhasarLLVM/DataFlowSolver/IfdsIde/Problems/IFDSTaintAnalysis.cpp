/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "llvm/IR/CallSite.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/GenIf.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSTaintAnalysis.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/SpecialSummaries.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

#include "phasar/Utils/LLVMIRToSrc.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"

using namespace std;
using namespace psr;

namespace psr {

IFDSTaintAnalysis::IFDSTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    const TaintConfiguration<const llvm::Value *> &TSF,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints),
      SourceSinkFunctions(TSF) {
  IFDSTaintAnalysis::ZeroValue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getNormalFlowFunction(IFDSTaintAnalysis::n_t curr,
                                         IFDSTaintAnalysis::n_t succ) {
  // If a tainted value is stored, the store location must be tainted too
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
      const llvm::StoreInst *store;
      TAFF(const llvm::StoreInst *s) : store(s){};
      set<IFDSTaintAnalysis::d_t>
      computeTargets(IFDSTaintAnalysis::d_t source) override {
        if (store->getValueOperand() == source) {
          return set<IFDSTaintAnalysis::d_t>{store->getPointerOperand(),
                                             source};
        } else if (store->getValueOperand() != source &&
                   store->getPointerOperand() == source) {
          return {};
        } else {
          return {source};
        }
      }
    };
    return make_shared<TAFF>(Store);
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        Load, [Load](IFDSTaintAnalysis::d_t source) {
          return source == Load->getPointerOperand();
        });
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        GEP, [GEP](IFDSTaintAnalysis::d_t source) {
          return source == GEP->getPointerOperand();
        });
  }
  // Otherwise we do not care and leave everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getCallFlowFunction(IFDSTaintAnalysis::n_t callStmt,
                                       IFDSTaintAnalysis::f_t destFun) {
  string FunctionName = cxx_demangle(destFun->getName().str());
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (SourceSinkFunctions.isSource(FunctionName) ||
      (SourceSinkFunctions.isSink(FunctionName))) {
    return KillAll<IFDSTaintAnalysis::d_t>::getInstance();
  }
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destFun);
  }
  // Pass everything else as identity
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getRetFlowFunction(IFDSTaintAnalysis::n_t callSite,
                                      IFDSTaintAnalysis::f_t calleeFun,
                                      IFDSTaintAnalysis::n_t exitStmt,
                                      IFDSTaintAnalysis::n_t retSite) {
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeFun, exitStmt,
      [](IFDSTaintAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      });
  // All other stuff is killed at this point
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getCallToRetFlowFunction(
    IFDSTaintAnalysis::n_t callSite, IFDSTaintAnalysis::n_t retSite,
    set<IFDSTaintAnalysis::f_t> callees) {
  auto &lg = lg::get();
  // Process the effects of source or sink functions that are called
  for (auto *Callee : ICF->getCalleesOfCallAt(callSite)) {
    string FunctionName = cxx_demangle(Callee->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "F:" << Callee->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "demangled F:" << FunctionName);
    if (SourceSinkFunctions.isSource(FunctionName)) {
      // process generated taints
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Plugin SOURCE effects");
      auto Source = SourceSinkFunctions.getSource(FunctionName);
      set<IFDSTaintAnalysis::d_t> ToGenerate;
      llvm::ImmutableCallSite CallSite(callSite);
      if (auto pval =
              std::get_if<TaintConfiguration<IFDSTaintAnalysis::d_t>::All>(
                  &Source.TaintedArgs)) {
        for (unsigned i = 0; i < CallSite.getNumArgOperands(); ++i) {
          IFDSTaintAnalysis::d_t V = CallSite.getArgOperand(i);
          // Insert the value V that gets tainted
          ToGenerate.insert(V);
          // We also have to collect all aliases of V and generate them
          auto PTS = ICF->getWholeModulePTG().getPointsToSet(V);
          for (auto Alias : PTS) {
            ToGenerate.insert(Alias);
          }
        }
      } else if (auto pval = std::get_if<
                     TaintConfiguration<IFDSTaintAnalysis::d_t>::None>(
                     &Source.TaintedArgs)) {
        // don't do anything
      } else if (auto pval =
                     std::get_if<std::vector<unsigned>>(&Source.TaintedArgs)) {
        for (auto FormalIndex : *pval) {
          IFDSTaintAnalysis::d_t V = CallSite.getArgOperand(FormalIndex);
          // Insert the value V that gets tainted
          ToGenerate.insert(V);
          // We also have to collect all aliases of V and generate them
          auto PTS = ICF->getWholeModulePTG().getPointsToSet(V);
          for (auto Alias : PTS) {
            ToGenerate.insert(Alias);
          }
        }
      } else {
        throw std::runtime_error("Something went wrong, unexpected type");
      }

      if (Source.TaintsReturn) {
        ToGenerate.insert(callSite);
      }
      return make_shared<GenAll<IFDSTaintAnalysis::d_t>>(ToGenerate,
                                                         getZeroValue());
    }
    if (SourceSinkFunctions.isSink(FunctionName)) {
      // process leaks
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Plugin SINK effects");
      struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
        llvm::ImmutableCallSite callSite;
        IFDSTaintAnalysis::f_t calledMthd;
        TaintConfiguration<IFDSTaintAnalysis::d_t>::SinkFunction Sink;
        map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &Leaks;
        const IFDSTaintAnalysis *taintanalysis;
        TAFF(llvm::ImmutableCallSite cs, IFDSTaintAnalysis::f_t calledMthd,
             TaintConfiguration<IFDSTaintAnalysis::d_t>::SinkFunction s,
             map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &leaks,
             const IFDSTaintAnalysis *ta)
            : callSite(cs), calledMthd(calledMthd), Sink(s), Leaks(leaks),
              taintanalysis(ta) {}
        set<IFDSTaintAnalysis::d_t>
        computeTargets(IFDSTaintAnalysis::d_t source) override {
          // check if a tainted value flows into a sink
          // if so, add to Leaks and return id
          if (!taintanalysis->isZeroValue(source)) {
            for (unsigned Idx = 0; Idx < callSite.getNumArgOperands(); ++Idx) {
              if (source == callSite.getArgOperand(Idx) &&
                  Sink.isLeakedArg(Idx)) {
                cout << "FOUND LEAK" << endl;
                Leaks[callSite.getInstruction()].insert(source);
              }
            }
          }
          return {source};
        }
      };
      return make_shared<TAFF>(llvm::ImmutableCallSite(callSite), Callee,
                               SourceSinkFunctions.getSink(FunctionName), Leaks,
                               this);
    }
  }
  // Otherwise pass everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getSummaryFlowFunction(IFDSTaintAnalysis::n_t callStmt,
                                          IFDSTaintAnalysis::f_t destFun) {
  SpecialSummaries<IFDSTaintAnalysis::d_t> &specialSummaries =
      SpecialSummaries<IFDSTaintAnalysis::d_t>::getInstance();
  string FunctionName = cxx_demangle(destFun->getName().str());
  // If we have a special summary, which is neither a source function, nor
  // a sink function, then we provide it to the solver.
  if (specialSummaries.containsSpecialSummary(FunctionName) &&
      !SourceSinkFunctions.isSource(FunctionName) &&
      !SourceSinkFunctions.isSink(FunctionName)) {
    return specialSummaries.getSpecialFlowFunctionSummary(FunctionName);
  } else {
    // Otherwise we indicate, that not special summary exists
    // and the solver thus calls the call flow function instead
    return nullptr;
  }
}

map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::initialSeeds() {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSTaintAnalysis::initialSeeds()");
  // If main function is the entry point, commandline arguments have to be
  // tainted. Otherwise we just use the zero value to initialize the analysis.
  map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    if (EntryPoint == "main") {
      set<IFDSTaintAnalysis::d_t> CmdArgs;
      for (auto &Arg : ICF->getFunction(EntryPoint)->args()) {
        CmdArgs.insert(&Arg);
      }
      CmdArgs.insert(getZeroValue());
      SeedMap.insert(
          make_pair(&ICF->getFunction(EntryPoint)->front().front(), CmdArgs));
    } else {
      SeedMap.insert(make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                               set<IFDSTaintAnalysis::d_t>({getZeroValue()})));
    }
  }
  return SeedMap;
}

IFDSTaintAnalysis::d_t IFDSTaintAnalysis::createZeroValue() const {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IFDSTaintAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(IFDSTaintAnalysis::d_t d) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d);
}

void IFDSTaintAnalysis::printNode(ostream &os, IFDSTaintAnalysis::n_t n) const {
  os << llvmIRToString(n);
}

void IFDSTaintAnalysis::printDataFlowFact(ostream &os,
                                          IFDSTaintAnalysis::d_t d) const {
  os << llvmIRToString(d);
}

void IFDSTaintAnalysis::printFunction(ostream &os,
                                      IFDSTaintAnalysis::f_t m) const {
  os << m->getName().str();
}

void IFDSTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, BinaryDomain> &SR, std::ostream &os) {
  os << "\n----- Found the following leaks -----\n";
  if (Leaks.empty()) {
    os << "No leaks found!\n";
  } else {
    for (auto Leak : Leaks) {
      os << "At instruction\nIR  : " << llvmIRToString(Leak.first) << '\n';
      os << "\n\nLeak(s):\n";
      for (auto LeakedValue : Leak.second) {
        os << "IR  : ";
        // Get the actual leaked alloca instruction if possible
        if (auto Load = llvm::dyn_cast<llvm::LoadInst>(LeakedValue)) {
          os << llvmIRToString(Load->getPointerOperand()) << '\n';
        } else {
          os << llvmIRToString(LeakedValue) << '\n';
        }
      }
      os << "-------------------\n";
    }
  }
}

} // namespace psr
