/*
 * IFDSTaintAnalysis.hh
 *
 *  Created on: 15.09.2016
 *      Author: pdschbrt
 */
#ifndef ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_
#define ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_

#include "../../ifds_ide/DefaultIFDSTabulationProblem.hh"
#include "../../ifds_ide/DefaultSeeds.hh"
#include "../../ifds_ide/FlowFunction.hh"
#include "../../ifds_ide/flow_func/Gen.hh"
#include "../../ifds_ide/flow_func/Identity.hh"
#include "../../ifds_ide/flow_func/Kill.hh"
#include "../../../utils/utils.hh"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../../ifds_ide/ZeroValue.hh"
#include "../../ifds_ide/icfg/LLVMBasedICFG.hh"
using namespace std;

class IFDSTaintAnalysis
    : public DefaultIFDSTabulationProblem<
          const llvm::Instruction *, const llvm::Value *,
          const llvm::Function *, LLVMBasedICFG &> {
private:
  llvm::LLVMContext &context;

public:
  struct SourceFunction {
    string name;
    vector<unsigned> genargs;
    bool genreturn;
    SourceFunction() : genreturn(0) {}
    SourceFunction(const SourceFunction &sf) = default;
    SourceFunction(string n, vector<unsigned> gen, bool genret)
        : name(n), genargs(gen), genreturn(genret) {}
    friend ostream &operator<<(ostream &os, const SourceFunction &sf) {
      os << sf.name << "\n";
      for (auto arg : sf.genargs)
        os << arg << ",";
      return os << "\n" << sf.genreturn << endl;
    }
  };
  struct SinkFunction {
    string name;
    vector<unsigned> sinkargs;
    SinkFunction() {}
    SinkFunction(const SinkFunction &sf) = default;
    SinkFunction(string n, vector<unsigned> sink) : name(n), sinkargs(sink) {}
    friend ostream &operator<<(ostream &os, const SinkFunction &sf) {
      os << sf.name << "\n";
      for (auto arg : sf.sinkargs)
        os << arg << ",";
      return os << endl;
    }
  };

  const vector<SourceFunction> source_functions = {
      SourceFunction("fread", {0}, false), SourceFunction("read", {1}, false)};
  // keep in mind that 'char** argv' of main is a source for tainted values as
  // well
  const vector<SinkFunction> sink_functions = {
      SinkFunction("fwrite", {0}), SinkFunction("write", {1}),
      SinkFunction("printf", {1, 2, 3, 4, 5, 6, 7, 8, 9, 10})};

  SourceFunction findSourceFunction(const llvm::Function *f);

  SinkFunction findSinkFunction(const llvm::Function *f);

  bool isSourceFunction(const llvm::Function *f);

  bool isSinkFunction(const llvm::Function *f);

  IFDSTaintAnalysis(LLVMBasedICFG &icfg, llvm::LLVMContext &c);

  virtual ~IFDSTaintAnalysis() = default;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallFlowFuntion(const llvm::Instruction *callStmt,
                     const llvm::Function *destMthd) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  shared_ptr<FlowFunction<const llvm::Value *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite) override;

  map<const llvm::Instruction *, set<const llvm::Value *>>
  initialSeeds() override;

  const llvm::Value *createZeroValue() override;
};

#endif /* ANALYSIS_IFDS_IDE_PROBLEMS_IFDS_TAINT_ANALYSIS_IFDSTAINTANALYSIS_HH_ \
          */
