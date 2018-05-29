/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenIf.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>
#include <phasar/PhasarLLVM/IfdsIde/SpecialSummaries.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>
#include <utility>
using namespace std;

// Source functions - critical argument(s) - signature:
//  -fread - 0 - size_t fread(void *ptr, size_t size, size_t nmemb, FILE
//  *stream);
//  -read - 1 - ssize_t read(int fd, void *buf, size_t count);
//  -fgetc - ret - int fgetc(FILE *stream);
//  -fgets - ret - char *fgets(char *s, int size, FILE *stream);
//  -getc - ret - int getc(FILE *stream);
//  -getchar - ret - int getchar(void);
//  -ungetc - ret - int ungetc(int c, FILE *stream);
//
// Sink functions - critical argument(s) - signature:
//  -fwrite - 0 - size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE
//  *stream);
//  -write - 1 - ssize_t write(int fd, const void *buf, size_t count);
//  -printf - everything - int printf(const char *format, ...);
//  -fputc - 0 - int fputc(int c, FILE *stream);
//  -fputs - 0 - int fputs(const char *s, FILE *stream);
//  -putc - 0 - int putc(int c, FILE *stream);
//  -putchar - 0 - int putchar(int c);
//  -puts - 0 - int puts(const char *s);

// Define what the source functions are
const map<string, IFDSTaintAnalysis::SourceFunction> IFDSTaintAnalysis::Sources{
    {"fread", IFDSTaintAnalysis::SourceFunction("fread", {0}, false)},
    {"read", IFDSTaintAnalysis::SourceFunction("read", {1}, false)},
    {"fgetc", IFDSTaintAnalysis::SourceFunction("fgetc", true)},
    {"fgets", IFDSTaintAnalysis::SourceFunction("fgets", {0}, true)},
    {"getc", IFDSTaintAnalysis::SourceFunction("getc", true)},
    {"getchar", IFDSTaintAnalysis::SourceFunction("getchar", true)},
    {"ungetc", IFDSTaintAnalysis::SourceFunction("ungetc", true)}};

// Define what the sink functions are
const map<string, IFDSTaintAnalysis::SinkFunction> IFDSTaintAnalysis::Sinks{
    {"fwrite", IFDSTaintAnalysis::SinkFunction("fwrite", {0})},
    {"write", IFDSTaintAnalysis::SinkFunction("write", {1})},
    {"printf",
     IFDSTaintAnalysis::SinkFunction("printf", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9})},
    {"fputc", IFDSTaintAnalysis::SinkFunction("fputc", {0})},
    {"fputs", IFDSTaintAnalysis::SinkFunction("fputs", {0})},
    {"putc", IFDSTaintAnalysis::SinkFunction("putc", {0})},
    {"putchar", IFDSTaintAnalysis::SinkFunction("putchar", {0})},
    {"puts", IFDSTaintAnalysis::SinkFunction("puts", {0})}};

IFDSTaintAnalysis::IFDSTaintAnalysis(IFDSTaintAnalysis::i_t icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  IFDSTaintAnalysis::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getNormalFlowFunction(IFDSTaintAnalysis::n_t curr,
                                         IFDSTaintAnalysis::n_t succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getNormalFlowFunction()";
  // Taint the commandline arguments
  if (curr->getFunction()->getName().str() == "main" &&
      icfg.isStartPoint(curr)) {
    set<IFDSTaintAnalysis::d_t> CmdArgs;
    for (auto &Arg : curr->getFunction()->args()) {
      CmdArgs.insert(&Arg);
      Arg.print(llvm::outs());
    }
    return make_shared<GenAll<IFDSTaintAnalysis::d_t>>(CmdArgs, zeroValue());
  }
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
        Load, zeroValue(), [Load](IFDSTaintAnalysis::d_t source) {
          return source == Load->getPointerOperand();
        });
  }
  // Check is a value is read from a pointer or struct
  if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(curr)) {
    return make_shared<GenIf<IFDSTaintAnalysis::d_t>>(
        GEP, zeroValue(), [GEP](IFDSTaintAnalysis::d_t source) {
          return source == GEP->getPointerOperand();
        });
  }
  // Otherwise we do not care and leave everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::v();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getCallFlowFunction(IFDSTaintAnalysis::n_t callStmt,
                                       IFDSTaintAnalysis::m_t destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getCallFlowFunction()";
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (Sources.count(destMthd->getName().str()) ||
      Sinks.count(destMthd->getName().str())) {
    return KillAll<IFDSTaintAnalysis::d_t>::v();
  }
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt),
                                         destMthd);
  }
  // Pass everything else as identity
  return Identity<IFDSTaintAnalysis::d_t>::v();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getRetFlowFunction(IFDSTaintAnalysis::n_t callSite,
                                      IFDSTaintAnalysis::m_t calleeMthd,
                                      IFDSTaintAnalysis::n_t exitStmt,
                                      IFDSTaintAnalysis::n_t retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getRetFlowFunction()";
  // We must check if the return value and formal parameter are tainted, if so
  // we must taint all user's of the function call. We are only interested in
  // formal parameters of pointer/reference type.
  return make_shared<MapFactsToCaller>(
      llvm::ImmutableCallSite(callSite), calleeMthd, exitStmt,
      [](IFDSTaintAnalysis::d_t formal) {
        return formal->getType()->isPointerTy();
      });
  // All other stuff is killed at this point
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getCallToRetFlowFunction(IFDSTaintAnalysis::n_t callSite,
                                            IFDSTaintAnalysis::n_t retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getCallToRetFlowFunction()";
  // Process the effects of source or sink functions that are called
  for (auto *Callee : icfg.getCalleesOfCallAt(callSite)) {
    string FunctionName = cxx_demangle(Callee->getName().str());
    if (Sources.count(FunctionName)) {
      // process generated taints
      BOOST_LOG_SEV(lg, DEBUG) << "Plugin SOURCE effects";
      auto Source = Sources.at(FunctionName);
      set<IFDSTaintAnalysis::d_t> ToGenerate;
      llvm::ImmutableCallSite CallSite(callSite);
      for (auto FormalIndex : Source.genargs) {
        IFDSTaintAnalysis::d_t V = CallSite.getArgOperand(FormalIndex);
        // Insert the value V that gets tainted
        ToGenerate.insert(V);
        // We also have to collect all aliases of V and generate them
        auto PTS = icfg.getWholeModulePTG().getPointsToSet(V);
        for (auto Alias : PTS) {
          ToGenerate.insert(Alias);
        }
      }
      if (Source.genreturn) {
        ToGenerate.insert(callSite);
      }
      return make_shared<GenAll<IFDSTaintAnalysis::d_t>>(ToGenerate,
                                                         zeroValue());
    }
    if (Sinks.count(FunctionName)) {
      // process leaks
      BOOST_LOG_SEV(lg, DEBUG) << "Plugin SINK effects";
      struct TAFF : FlowFunction<IFDSTaintAnalysis::d_t> {
        llvm::ImmutableCallSite callSite;
        IFDSTaintAnalysis::m_t calledMthd;
        SinkFunction sink;
        map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &Leaks;
        const IFDSTaintAnalysis *taintanalysis;
        TAFF(llvm::ImmutableCallSite cs, IFDSTaintAnalysis::m_t calledMthd,
             SinkFunction s,
             map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> &leaks,
             const IFDSTaintAnalysis *ta)
            : callSite(cs), calledMthd(calledMthd), sink(s), Leaks(leaks),
              taintanalysis(ta) {}
        set<IFDSTaintAnalysis::d_t>
        computeTargets(IFDSTaintAnalysis::d_t source) {
          // check if a tainted value flows into a sink
          // if so, add to Leaks and return id
          if (!taintanalysis->isZeroValue(source)) {
            for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
              if (source == callSite.getArgOperand(idx) &&
                  (find(sink.sinkargs.begin(), sink.sinkargs.end(), idx) !=
                   sink.sinkargs.end())) {
                cout << "FOUND LEAK" << endl;
                Leaks[callSite.getInstruction()].insert(source);
              }
            }
          }
          return {source};
        }
      };
      return make_shared<TAFF>(llvm::ImmutableCallSite(callSite), Callee,
                               Sinks.at(FunctionName), Leaks, this);
    }
  }
  // Otherwise pass everything as it is
  return Identity<IFDSTaintAnalysis::d_t>::v();
}

shared_ptr<FlowFunction<IFDSTaintAnalysis::d_t>>
IFDSTaintAnalysis::getSummaryFlowFunction(IFDSTaintAnalysis::n_t callStmt,
                                          IFDSTaintAnalysis::m_t destMthd) {
  SpecialSummaries<IFDSTaintAnalysis::d_t> &specialSummaries =
      SpecialSummaries<IFDSTaintAnalysis::d_t>::getInstance();
  string FunctionName = destMthd->getName().str();
  // If we have a special summary, which is neither a source function, nor
  // a sink function, then we provide it to the solver.
  if (specialSummaries.containsSpecialSummary(FunctionName) &&
      !Sources.count(FunctionName) && !Sinks.count(FunctionName)) {
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
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::initialSeeds()";
  // just start in main()
  map<IFDSTaintAnalysis::n_t, set<IFDSTaintAnalysis::d_t>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<IFDSTaintAnalysis::d_t>({zeroValue()})));
  }
  return SeedMap;
}

IFDSTaintAnalysis::d_t IFDSTaintAnalysis::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::createZeroValue()";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(IFDSTaintAnalysis::d_t d) const {
  return isLLVMZeroValue(d);
}

string IFDSTaintAnalysis::DtoString(IFDSTaintAnalysis::d_t d) const {
  return llvmIRToString(d);
}

string IFDSTaintAnalysis::NtoString(IFDSTaintAnalysis::n_t n) const {
  return llvmIRToString(n);
}

string IFDSTaintAnalysis::MtoString(IFDSTaintAnalysis::m_t m) const {
  return m->getName().str();
}
