/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef IFDSENVIRONMENTVARIABLETRACING_H
#define IFDSENVIRONMENTVARIABLETRACING_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/Domain/ExtendedValue.h>
#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/IFDSEnvironmentVariableTracing/Stats/TraceStats.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/Utils/TaintConfiguration.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace psr {

class IFDSEnvironmentVariableTracing
    : public DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                          ExtendedValue, const llvm::Function *,
                                          LLVMBasedICFG &> {
public:
  IFDSEnvironmentVariableTracing(
      LLVMBasedICFG &ICFG, const TaintConfiguration<ExtendedValue> &TaintConfig,
      std::vector<std::string> EntryPoints);
  ~IFDSEnvironmentVariableTracing() override = default;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destMthd) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  std::shared_ptr<FlowFunction<ExtendedValue>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destMthd) override;

  std::map<const llvm::Instruction *, std::set<ExtendedValue>>
  initialSeeds() override;

  void printIFDSReport(std::ostream &os,
                       SolverResults<const llvm::Instruction *, ExtendedValue,
                                     BinaryDomain> &solverResults) override;

  ExtendedValue createZeroValue() override {
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

  void printMethod(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }

protected:
  std::vector<std::string> EntryPoints;

private:
  TaintConfiguration<ExtendedValue> taintConfig;

  TraceStats traceStats;
};

} // namespace psr

#endif // IFDSENVIRONMENTVARIABLETRACING_H
