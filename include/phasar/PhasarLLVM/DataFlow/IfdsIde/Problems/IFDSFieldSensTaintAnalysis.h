/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSFIELDSENSTAINTANALYSIS_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_PROBLEMS_IFDSFIELDSENSTAINTANALYSIS_H

#include "phasar/DataFlow/IfdsIde/IFDSTabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde//IFDSFieldSensTaintAnalysis/Utils/ExtendedValue.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/LLVMZeroValue.h"
#include "phasar/PhasarLLVM/Domain/LLVMAnalysisDomain.h"
#include "phasar/PhasarLLVM/TaintConfig/LLVMTaintConfig.h"
#include "phasar/PhasarLLVM/Utils/LLVMShorthands.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Value.h"

#include <map>
#include <memory>
#include <set>
#include <string>

namespace llvm {
class Value;
class Function;
class StructType;
} // namespace llvm

namespace psr {

struct IFDSFieldSensTaintAnalysisDomain : public LLVMIFDSAnalysisDomainDefault {
  using d_t = ExtendedValue;
};

class IFDSFieldSensTaintAnalysis
    : public IFDSTabulationProblem<IFDSFieldSensTaintAnalysisDomain> {
public:
  using ConfigurationTy = LLVMTaintConfig;

  IFDSFieldSensTaintAnalysis(const LLVMProjectIRDB *IRDB,
                             const LLVMTaintConfig *TaintConfig,
                             std::vector<std::string> EntryPoints = {"main"});

  ~IFDSFieldSensTaintAnalysis() override = default;

  FlowFunctionPtrType
  getNormalFlowFunction(const llvm::Instruction *Curr,
                        const llvm::Instruction *Succ) override;

  FlowFunctionPtrType
  getCallFlowFunction(const llvm::Instruction *CallSite,
                      const llvm::Function *DestFun) override;

  FlowFunctionPtrType
  getRetFlowFunction(const llvm::Instruction *CallSite,
                     const llvm::Function *CalleeFun,
                     const llvm::Instruction *ExitStmt,
                     const llvm::Instruction *RetSite) override;

  FlowFunctionPtrType
  getCallToRetFlowFunction(const llvm::Instruction *CallSite,
                           const llvm::Instruction *RetSite,
                           llvm::ArrayRef<f_t> Callees) override;

  FlowFunctionPtrType
  getSummaryFlowFunction(const llvm::Instruction *CallSite,
                         const llvm::Function *DestFun) override;

  InitialSeeds<const llvm::Instruction *, ExtendedValue, l_t>
  initialSeeds() override;

  void
  emitTextReport(const SolverResults<const llvm::Instruction *, ExtendedValue,
                                     BinaryDomain> &SolverResults,
                 llvm::raw_ostream &OS = llvm::outs()) override;

  [[nodiscard]] ExtendedValue createZeroValue() const {
    // create a special value to represent the zero value!
    return ExtendedValue(LLVMZeroValue::getInstance());
  }

  [[nodiscard]] bool isZeroValue(ExtendedValue EV) const override {
    return LLVMZeroValue::isLLVMZeroValue(EV.getValue());
  }

  void printNode(llvm::raw_ostream &OS,
                 const llvm::Instruction *Stmt) const override {
    OS << llvmIRToString(Stmt);
  }

  void printDataFlowFact(llvm::raw_ostream &OS,
                         ExtendedValue EV) const override {
    OS << llvmIRToString(EV.getValue()) << "\n";
    for (const auto *MemLocationPart : EV.getMemLocationSeq()) {
      OS << "A:\t" << llvmIRToString(MemLocationPart) << "\n";
    }
    if (!EV.getEndOfTaintedBlockLabel().empty()) {
      OS << "L:\t" << EV.getEndOfTaintedBlockLabel() << "\n";
    }
    if (EV.isVarArg()) {
      OS << "VT:\t" << EV.isVarArgTemplate() << "\n";
      for (const auto *VAListMemLocationPart : EV.getVaListMemLocationSeq()) {
        OS << "VLA:\t" << llvmIRToString(VAListMemLocationPart) << "\n";
      }
      OS << "VI:\t" << EV.getVarArgIndex() << "\n";
      OS << "CI:\t" << EV.getCurrentVarArgIndex() << "\n";
    }
  }

  void printFunction(llvm::raw_ostream &OS,
                     const llvm::Function *Func) const override {
    OS << Func->getName();
  }

private:
  const LLVMTaintConfig *Config{};

  TraceStats Stats{};
};

} // namespace psr

#endif
