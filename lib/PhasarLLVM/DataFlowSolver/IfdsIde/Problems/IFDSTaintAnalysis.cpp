/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <utility>

#include "llvm/IR/CallSite.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMFlowFunctions.h"
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
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    const TaintConfiguration<const llvm::Value *> &TSF,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)),
      SourceSinkFunctions(TSF) {
  IFDSTaintAnalysis::ZeroValue = createZeroValue();
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getNormalFlowFunction(IFDSTaintAnalysis::n_t Curr,
                                         IFDSTaintAnalysis::n_t Succ) {
  // If a tainted value is stored, the store location must be tainted too
  if (const auto *Store = llvm::dyn_cast<llvm::StoreInst>(Curr)) {
    struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
      const llvm::StoreInst *Store;
      TAFF(const llvm::StoreInst *S) : Store(S){};
      set<IFDSTaintAnalysis::d_t>
      computeTargets(IFDSTaintAnalysis::d_t Source) override {
        if (Store->getValueOperand() == Source) {
          return set<IFDSTaintAnalysis::d_t>{Store->getPointerOperand(),
                                             Source};
        } else if (Store->getValueOperand() != Source &&
                   Store->getPointerOperand() == Source) {
          return {};
        } else {
          return {Source};
        }
      }
    };
    return make_shared<TAFF>(Store);
  }
  // If a tainted value is loaded, the loaded value is of course tainted
  if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(Curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        Load, [Load](IFDSTaintAnalysis::d_t Source) {
          return Source == Load->getPointerOperand();
        });
  }
  // Check if an address is computed from a tainted base pointer of an
  // aggregated object
  if (const auto *GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(Curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        GEP, [GEP](IFDSTaintAnalysis::d_t Source) {
          return Source == GEP->getPointerOperand();
        });
  }
  // Otherwise we do not care and leave everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getCallFlowFunction(IFDSTaintAnalysis::n_t CallStmt,
                                       IFDSTaintAnalysis::f_t DestFun) {
  string FunctionName = cxxDemangle(DestFun->getName().str());
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (SourceSinkFunctions.isSource(FunctionName) ||
      (SourceSinkFunctions.isSink(FunctionName))) {
    return KillAll<IFDSTaintAnalysis::d_t>::getInstance();
  }
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(CallStmt) ||
      llvm::isa<llvm::InvokeInst>(CallStmt)) {
    return make_shared<MapFactsToCallee<>>(llvm::ImmutableCallSite(CallStmt),
                                           DestFun);
  }
  // Pass everything else as identity
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

IFDSTaintAnalysis::FlowFunctionPtrType IFDSTaintAnalysis::getRetFlowFunction(
    IFDSTaintAnalysis::n_t CallSite, IFDSTaintAnalysis::f_t CalleeFun,
    IFDSTaintAnalysis::n_t ExitStmt, IFDSTaintAnalysis::n_t RetSite) {
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return make_shared<MapFactsToCaller<>>(
      llvm::ImmutableCallSite(CallSite), CalleeFun, ExitStmt,
      [](IFDSTaintAnalysis::d_t Formal) {
        return Formal->getType()->isPointerTy();
      });
  // All other stuff is killed at this point
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getCallToRetFlowFunction(
    IFDSTaintAnalysis::n_t CallSite, IFDSTaintAnalysis::n_t RetSite,
    set<IFDSTaintAnalysis::f_t> Callees) {
  // Process the effects of source or sink functions that are called
  for (const auto *Callee : ICF->getCalleesOfCallAt(CallSite)) {
    string FunctionName = cxxDemangle(Callee->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "F:" << Callee->getName().str());
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "demangled F:" << FunctionName);
    if (SourceSinkFunctions.isSource(FunctionName)) {
      // process generated taints
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Plugin SOURCE effects");
      auto Source = SourceSinkFunctions.getSource(FunctionName);
      set<IFDSTaintAnalysis::d_t> ToGenerate;
      llvm::ImmutableCallSite ICallSite(CallSite);
      if (auto *Pval =
              std::get_if<TaintConfiguration<IFDSTaintAnalysis::d_t>::All>(
                  &Source.TaintedArgs)) {
        for (unsigned I = 0; I < ICallSite.getNumArgOperands(); ++I) {
          IFDSTaintAnalysis::d_t V = ICallSite.getArgOperand(I);
          // Insert the value V that gets tainted
          ToGenerate.insert(V);
          // We also have to collect all aliases of V and generate them
          auto PTS = PT->getPointsToSet(V);
          for (const auto *Alias : *PTS) {
            ToGenerate.insert(Alias);
          }
        }
      } else if (auto *Pval = std::get_if<
                     TaintConfiguration<IFDSTaintAnalysis::d_t>::None>(
                     &Source.TaintedArgs)) {
        // don't do anything
      } else if (auto *Pval =
                     std::get_if<std::vector<unsigned>>(&Source.TaintedArgs)) {
        for (auto FormalIndex : *Pval) {
          IFDSTaintAnalysis::d_t V = ICallSite.getArgOperand(FormalIndex);
          // Insert the value V that gets tainted
          ToGenerate.insert(V);
          // We also have to collect all aliases of V and generate them
          auto PTS = PT->getPointsToSet(V);
          for (const auto *Alias : *PTS) {
            ToGenerate.insert(Alias);
          }
        }
      } else {
        throw std::runtime_error("Something went wrong, unexpected type");
      }

      if (Source.TaintsReturn) {
        ToGenerate.insert(CallSite);
      }
      return make_shared<GenAll<IFDSTaintAnalysis::d_t>>(ToGenerate,
                                                         getZeroValue());
    }
    if (SourceSinkFunctions.isSink(FunctionName)) {
      // process leaks
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "Plugin SINK effects");
      struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
        llvm::ImmutableCallSite CallSite;
        IFDSTaintAnalysis::f_t CalledMthd;
        TaintConfiguration<IFDSTaintAnalysis::d_t>::SinkFunction Sink;
        map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &Leaks;
        const IFDSTaintAnalysis *TaintAnalysis;
        TAFF(llvm::ImmutableCallSite CS, IFDSTaintAnalysis::f_t CalledMthd,
             TaintConfiguration<IFDSTaintAnalysis::d_t>::SinkFunction S,
             map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &Leaks,
             const IFDSTaintAnalysis *Ta)
            : CallSite(CS), CalledMthd(CalledMthd), Sink(std::move(S)),
              Leaks(Leaks), TaintAnalysis(Ta) {}
        set<IFDSTaintAnalysis::d_t>
        computeTargets(IFDSTaintAnalysis::d_t Source) override {
          // check if a tainted value flows into a sink
          // if so, add to Leaks and return id
          if (!TaintAnalysis->isZeroValue(Source)) {
            for (unsigned Idx = 0; Idx < CallSite.getNumArgOperands(); ++Idx) {
              if (Source == CallSite.getArgOperand(Idx) &&
                  Sink.isLeakedArg(Idx)) {
                cout << "FOUND LEAK" << endl;
                Leaks[CallSite.getInstruction()].insert(Source);
              }
            }
          }
          return {Source};
        }
      };
      return make_shared<TAFF>(llvm::ImmutableCallSite(CallSite), Callee,
                               SourceSinkFunctions.getSink(FunctionName), Leaks,
                               this);
    }
  }
  // Otherwise pass everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::getInstance();
}

IFDSTaintAnalysis::FlowFunctionPtrType
IFDSTaintAnalysis::getSummaryFlowFunction(IFDSTaintAnalysis::n_t CallStmt,
                                          IFDSTaintAnalysis::f_t DestFun) {
  SpecialSummaries<IFDSTaintAnalysis::d_t> &SS =
      SpecialSummaries<IFDSTaintAnalysis::d_t>::getInstance();
  string FunctionName = cxxDemangle(DestFun->getName().str());
  // If we have a special summary, which is neither a source function, nor
  // a sink function, then we provide it to the solver.
  if (SS.containsSpecialSummary(FunctionName) &&
      !SourceSinkFunctions.isSource(FunctionName) &&
      !SourceSinkFunctions.isSink(FunctionName)) {
    return SS.getSpecialFlowFunctionSummary(FunctionName);
  } else {
    // Otherwise we indicate, that not special summary exists
    // and the solver thus calls the call flow function instead
    return nullptr;
  }
}

map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::initialSeeds() {
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IFDSTaintAnalysis::initialSeeds()");
  // If main function is the entry point, commandline arguments have to be
  // tainted. Otherwise we just use the zero value to initialize the analysis.
  map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> SeedMap;
  for (const auto &EntryPoint : EntryPoints) {
    if (EntryPoint == "main") {
      set<IFDSTaintAnalysis::d_t> CmdArgs;
      for (const auto &Arg : ICF->getFunction(EntryPoint)->args()) {
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
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                << "IFDSTaintAnalysis::createZeroValue()");
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(IFDSTaintAnalysis::d_t D) const {
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(D);
}

void IFDSTaintAnalysis::printNode(ostream &OS, IFDSTaintAnalysis::n_t N) const {
  OS << llvmIRToString(N);
}

void IFDSTaintAnalysis::printDataFlowFact(ostream &OS,
                                          IFDSTaintAnalysis::d_t D) const {
  OS << llvmIRToString(D);
}

void IFDSTaintAnalysis::printFunction(ostream &OS,
                                      IFDSTaintAnalysis::f_t M) const {
  OS << M->getName().str();
}

void IFDSTaintAnalysis::emitTextReport(
    const SolverResults<n_t, d_t, BinaryDomain> &SR, std::ostream &OS) {
  OS << "\n----- Found the following leaks -----\n";
  if (Leaks.empty()) {
    OS << "No leaks found!\n";
  } else {
    for (const auto &Leak : Leaks) {
      OS << "At instruction\nIR  : " << llvmIRToString(Leak.first) << '\n';
      OS << "\n\nLeak(s):\n";
      for (const auto *LeakedValue : Leak.second) {
        OS << "IR  : ";
        // Get the actual leaked alloca instruction if possible
        if (const auto *Load = llvm::dyn_cast<llvm::LoadInst>(LeakedValue)) {
          OS << llvmIRToString(Load->getPointerOperand()) << '\n';
        } else {
          OS << llvmIRToString(LeakedValue) << '\n';
        }
      }
      OS << "-------------------\n";
    }
  }
}

} // namespace psr
