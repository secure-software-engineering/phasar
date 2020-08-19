/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef IFDSFIELDSENSTAINTANALYSIS_H
#define IFDSFIELDSENSTAINTANALYSIS_H

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/AnalysisDomain.h"
#include "phasar/PhasarLLVM/Domain/ExtendedValue.h"
#include "phasar/PhasarLLVM/Utils/TaintConfiguration.h"
#include "phasar/Utils/LLVMShorthands.h"

namespace llvm {
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

struct IFDSFieldSensTaintAnalysisDomain : public LLVMAnalysisDomainDefault {
  using d_t = ExtendedValue;
};

class IFDSFieldSensTaintAnalysis
    : public IFDSTabulationProblem<IFDSFieldSensTaintAnalysisDomain> {
public:
  using ConfigurationTy = TaintConfiguration<ExtendedValue>;

  IFDSFieldSensTaintAnalysis(
      const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
      const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
      const TaintConfiguration<ExtendedValue> &TaintConfig,
      std::set<std::string> EntryPoints = {"main"});
  ~IFDSFieldSensTaintAnalysis() override = default;

  FlowFunctionPtrType
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  FlowFunctionPtrType
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destFun) override;

  FlowFunctionPtrType
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeFun,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  FlowFunctionPtrType
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destFun) override;

  std::map<const llvm::Instruction *, std::set<ExtendedValue>>
  initialSeeds() override;

  void
  emitTextReport(const SolverResults<const llvm::Instruction *, ExtendedValue,
                                     BinaryDomain> &solverResults,
                 std::ostream &OS = std::cout) override;

  ExtendedValue createZeroValue() const override {
    // create a special value to represent the zero value!
    return ExtendedValue(LLVMZeroValue::getInstance());
  }

  bool isZeroValue(ExtendedValue ev) const override {
    return LLVMZeroValue::getInstance()->isLLVMZeroValue(ev.getValue());
  }

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printDataFlowFact(std::ostream &os, ExtendedValue ev) const override {
    os << llvmIRToString(ev.getValue()) << "\n";
    for (const auto memLocationPart : ev.getMemLocationSeq()) {
      os << "A:\t" << llvmIRToString(memLocationPart) << "\n";
    }
    if (!ev.getEndOfTaintedBlockLabel().empty()) {
      os << "L:\t" << ev.getEndOfTaintedBlockLabel() << "\n";
    }
    if (ev.isVarArg()) {
      os << "VT:\t" << ev.isVarArgTemplate() << "\n";
      for (const auto vaListMemLocationPart : ev.getVaListMemLocationSeq()) {
        os << "VLA:\t" << llvmIRToString(vaListMemLocationPart) << "\n";
      }
      os << "VI:\t" << ev.getVarArgIndex() << "\n";
      os << "CI:\t" << ev.getCurrentVarArgIndex() << "\n";
    }
  }

  void printFunction(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }

private:
  TaintConfiguration<ExtendedValue> taintConfig;

  TraceStats traceStats;
};

} // namespace psr

#endif // IFDSFIELDSENSTAINTANALYSIS_H
