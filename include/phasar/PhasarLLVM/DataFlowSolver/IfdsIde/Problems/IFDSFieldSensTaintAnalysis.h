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

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/Domain/ExtendedValue.h>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace llvm {
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;
class LLVMTypeHierarchy;
class LLVMPointsToInfo;

class IFDSFieldSensTaintAnalysis
    : public IFDSTabulationProblem<
          const llvm::Instruction *, ExtendedValue, const llvm::Function *,
          const llvm::StructType *, const llvm::Value *, LLVMBasedICFG> {
public:
  typedef ExtendedValue d_t;
  typedef const llvm::Instruction *n_t;
  typedef const llvm::Function *f_t;
  typedef const llvm::StructType *t_t;
  typedef const llvm::Value *v_t;
  typedef LLVMBasedICFG i_t;
  using ConfigurationTy = TaintConfiguration<ExtendedValue>;

  IFDSFieldSensTaintAnalysis(
      const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
      const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
      const TaintConfiguration<ExtendedValue> &TaintConfig,
      std::set<std::string> EntryPoints = {"main"});
  ~IFDSFieldSensTaintAnalysis() override = default;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destFun) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeFun,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
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
