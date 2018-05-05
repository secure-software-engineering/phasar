/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/IfdsIde/Problems/IFDSTaintAnalysis.h>

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

IFDSTaintAnalysis::IFDSTaintAnalysis(LLVMBasedICFG &icfg,
                                     vector<string> EntryPoints)
    : DefaultIFDSTabulationProblem(icfg), EntryPoints(EntryPoints) {
  DefaultIFDSTabulationProblem::zerovalue = createZeroValue();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                         const llvm::Instruction *succ) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getNormalFlowFunction()";
  // Taint the commandline arguments
  if (curr->getFunction()->getName().str() == "main" &&
      icfg.isStartPoint(curr)) {
    set<const llvm::Value *> CmdArgs;
    for (auto &Arg : curr->getFunction()->args()) {
      CmdArgs.insert(&Arg);
      Arg.print(llvm::outs());
    }
    return make_shared<GenAll<const llvm::Value *>>(CmdArgs, zeroValue());
  }
  // If a tainted value is stored, the store location must be tainted too
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct TAFF : FlowFunction<const llvm::Value *> {
      const llvm::StoreInst *store;
      TAFF(const llvm::StoreInst *s) : store(s){};
      set<const llvm::Value *>
      computeTargets(const llvm::Value *source) override {
        if (store->getValueOperand() == source) {
          return set<const llvm::Value *>{store->getPointerOperand(), source};
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
    // struct TAFF : FlowFunction<const llvm::Value *> {
    //   const llvm::LoadInst *load;
    //   TAFF(const llvm::LoadInst *l) : load(l) {}
    //   set<const llvm::Value *>
    //   computeTargets(const llvm::Value *source) override {
    //     if (source == load->getPointerOperand()) {
    //       return {load, source};
    //     } else {
    //       return {source};
    //     }
    //   }
    // };
    // return make_shared<TAFF>(Load);
    return make_shared<GenIf<const llvm::Value *>>(
      Load, zeroValue(), [Load](const llvm::Value *source) {
        return source == Load->getPointerOperand();
      });
  }
  // Check is a value is read from a pointer or struct
  if (auto GEP = llvm::dyn_cast<llvm::GetElementPtrInst>(curr)) {
    // struct TAFF : FlowFunction<const llvm::Value *> {
    //   const llvm::GetElementPtrInst *gep;
    //   TAFF(const llvm::GetElementPtrInst *g) : gep(g) {}
    //   set<const llvm::Value *>
    //   computeTargets(const llvm::Value *source) override {
    //     if (source == gep->getPointerOperand()) {
    //       return {gep, source};
    //     } else {
    //       return {source};
    //     }
    //   }
    // };
    // return make_shared<TAFF>(GEP);
    return make_shared<GenIf<const llvm::Value *>>(
        GEP, zeroValue(), [GEP](const llvm::Value *source) {
          return source == GEP->getPointerOperand();
        });
  }
  // Otherwise we do not care and leave everything as it is
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                       const llvm::Function *destMthd) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getCallFlowFunction()";
  // Check if a source or sink function is called:
  // We then can kill all data-flow facts not following the called function.
  // The respective taints or leaks are then generated in the corresponding
  // call to return flow function.
  if (Sources.count(destMthd->getName().str()) ||
      Sinks.count(destMthd->getName().str())) {
    return KillAll<const llvm::Value *>::v();
  }
  // Map the actual into the formal parameters
  if (llvm::isa<llvm::CallInst>(callStmt) ||
      llvm::isa<llvm::InvokeInst>(callStmt)) {
    // Do the mapping
    // struct TAFF : FlowFunction<const llvm::Value *> {
    //   llvm::ImmutableCallSite callSite;
    //   const llvm::Function *destMthd;
    //   const llvm::Value *zerovalue;
    //   const IFDSTaintAnalysis *taintanalysis;
    //   vector<const llvm::Value *> actuals;
    //   vector<const llvm::Value *> formals;
    //   TAFF(llvm::ImmutableCallSite cs, const llvm::Function *dm,
    //        const llvm::Value *zv, const IFDSTaintAnalysis *ta)
    //       : callSite(cs), destMthd(dm), zerovalue(zv), taintanalysis(ta) {
    //     // set up the actual parameters
    //     for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
    //       actuals.push_back(callSite.getArgOperand(idx));
    //     }
    //     // set up the actual parameters
    //     for (unsigned idx = 0; idx < destMthd->arg_size(); ++idx) {
    //       formals.push_back(getNthFunctionArgument(destMthd, idx));
    //     }
    //   }
    //   set<const llvm::Value *> computeTargets(const llvm::Value *source) {
    //     if (!taintanalysis->isZeroValue(source)) {
    //       set<const llvm::Value *> res;
    //       for (unsigned idx = 0; idx < actuals.size(); ++idx) {
    //         if (source == actuals[idx]) {
    //           res.insert(formals[idx]); // corresponding formal
    //           // res.insert(source); // corresponding actual
    //           res.insert(zerovalue);
    //         }
    //       }
    //       return res;
    //     } else {
    //       return {source};
    //     }
    //   }
    // };
    // return make_shared<TAFF>(llvm::ImmutableCallSite(callStmt), destMthd, zeroValue(), this);
    return make_shared<MapFactsToCallee>(llvm::ImmutableCallSite(callStmt), destMthd);
  }
  // Pass everything else as identity
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                      const llvm::Function *calleeMthd,
                                      const llvm::Instruction *exitStmt,
                                      const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getRetFlowFunction()";
  // We must check if the return value is tainted, if so we must taint
  // all useres of the function call.
  return make_shared<MapFactsToCaller>(llvm::ImmutableCallSite(callSite),
                                       calleeMthd, exitStmt);
  // struct TAFF : FlowFunction<const llvm::Value *> {
  //   llvm::ImmutableCallSite callSite;
  //   const llvm::Function *calleeMthd;
  //   const llvm::ReturnInst *exitStmt;
  //   const llvm::Value *zerovalue;
  //   const IFDSTaintAnalysis *taintanalysis;
  //   vector<const llvm::Value *> actuals;
  //   vector<const llvm::Value *> formals;
  //   TAFF(llvm::ImmutableCallSite callsite, const llvm::Function *callemthd,
  //        const llvm::Instruction *exitstmt, const llvm::Value *zv,
  //        const IFDSTaintAnalysis *ta)
  //       : callSite(callsite), calleeMthd(callemthd),
  //         exitStmt(llvm::dyn_cast<llvm::ReturnInst>(exitstmt)),
  //         zerovalue(zv), taintanalysis(ta) {
  //     // set up the actual parameters
  //     for (unsigned idx = 0; idx < callSite.getNumArgOperands(); ++idx) {
  //       actuals.push_back(callSite.getArgOperand(idx));
  //     }
  //     // set up the actual parameters
  //     for (unsigned idx = 0; idx < calleeMthd->arg_size(); ++idx) {
  //       formals.push_back(getNthFunctionArgument(calleeMthd, idx));
  //     }
  //   }
  //   set<const llvm::Value *>
  //   computeTargets(const llvm::Value *source) override {
  //     if (!taintanalysis->isZeroValue(source)) {
  //       set<const llvm::Value *> res;
  //       res.insert(zerovalue);
  //       // collect everything that is returned by value and pointer/
  //       reference for (unsigned idx = 0; idx < formals.size(); ++idx) {
  //         if (source == formals[idx] &&
  //             formals[idx]->getType()->isPointerTy()) {
  //           res.insert(actuals[idx]);
  //         }
  //       }
  //       // collect taints returned by return value
  //       if (source == exitStmt->getReturnValue()) {
  //         res.insert(callSite.getInstruction());
  //       }
  //       return res;
  //     }
  //     // else just draw the zero edge
  //     return {source};
  //   }
  // };
  // return make_shared<TAFF>(llvm::ImmutableCallSite(callSite), calleeMthd,
  //                          exitStmt, zeroValue(), this);
  // All other stuff is killed at this point
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getCallToRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Instruction *retSite) {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::getCallToRetFlowFunction()";
  // Process the effects of source or sink functions that are called
  for (auto *Callee : icfg.getCalleesOfCallAt(callSite)) {
    string FunctionName = cxx_demangle(Callee->getName().str());
    if (Sources.count(FunctionName)) {
      // process generated taints
      BOOST_LOG_SEV(lg, DEBUG) << "Plugin SOURCE effects";
      auto Source = Sources.at(FunctionName);
      set<const llvm::Value *> ToGenerate;
      llvm::ImmutableCallSite CallSite(callSite);
      for (auto FormalIndex : Source.genargs) {
        const llvm::Value *V = CallSite.getArgOperand(FormalIndex);
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
      return make_shared<GenAll<const llvm::Value *>>(ToGenerate, zeroValue());
    }
    if (Sinks.count(FunctionName)) {
      // process leaks
      BOOST_LOG_SEV(lg, DEBUG) << "Plugin SINK effects";
      struct TAFF : FlowFunction<const llvm::Value *> {
        llvm::ImmutableCallSite callSite;
        const llvm::Function *calledMthd;
        SinkFunction sink;
        map<const llvm::Instruction *, set<const llvm::Value *>> &Leaks;
        const IFDSTaintAnalysis *taintanalysis;
        TAFF(llvm::ImmutableCallSite cs, const llvm::Function *calledMthd,
             SinkFunction s,
             map<const llvm::Instruction *, set<const llvm::Value *>> &leaks,
             const IFDSTaintAnalysis *ta)
            : callSite(cs), calledMthd(calledMthd), sink(s), Leaks(leaks),
              taintanalysis(ta) {}
        set<const llvm::Value *> computeTargets(const llvm::Value *source) {
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
  return Identity<const llvm::Value *>::v();
}

shared_ptr<FlowFunction<const llvm::Value *>>
IFDSTaintAnalysis::getSummaryFlowFunction(const llvm::Instruction *callStmt,
                                          const llvm::Function *destMthd) {
  SpecialSummaries<const llvm::Value *> &specialSummaries =
      SpecialSummaries<const llvm::Value *>::getInstance();
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

map<const llvm::Instruction *, set<const llvm::Value *>>
IFDSTaintAnalysis::initialSeeds() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::initialSeeds()";
  // just start in main()
  map<const llvm::Instruction *, set<const llvm::Value *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const llvm::Value *>({zeroValue()})));
  }
  return SeedMap;
}

const llvm::Value *IFDSTaintAnalysis::createZeroValue() {
  auto &lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "IFDSTaintAnalysis::createZeroValue()";
  // create a special value to represent the zero value!
  return LLVMZeroValue::getInstance();
}

bool IFDSTaintAnalysis::isZeroValue(const llvm::Value *d) const {
  return isLLVMZeroValue(d);
}

string IFDSTaintAnalysis::DtoString(const llvm::Value *d) const {
  return llvmIRToString(d);
}

string IFDSTaintAnalysis::NtoString(const llvm::Instruction *n) const {
  return llvmIRToString(n);
}

string IFDSTaintAnalysis::MtoString(const llvm::Function *m) const {
  return m->getName().str();
}
