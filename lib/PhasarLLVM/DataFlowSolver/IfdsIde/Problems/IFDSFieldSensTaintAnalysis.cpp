/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <set>
#include <string>
#include <utility>

#include <vector>

#include "llvm/IR/IntrinsicInst.h"

#include "phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CallToRetFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GenerateFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/IdentityFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCallee.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCaller.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemSetInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemTransferInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/PHINodeFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/ReturnInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/StoreInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAEndInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAStartInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovRetValWriter.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovWriter.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LineNumberWriter.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStatsWriter.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSFieldSensTaintAnalysis.h"
#include "phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h"
#include "phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h"

namespace psr {

IFDSFieldSensTaintAnalysis::IFDSFieldSensTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, LLVMPointsToInfo *PT,
    const TaintConfiguration<ExtendedValue> &TaintConfig,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, std::move(EntryPoints)),
      taintConfig(TaintConfig) {
  IFDSFieldSensTaintAnalysis::ZeroValue = createZeroValue();
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getNormalFlowFunction(
    const llvm::Instruction *CurrentInst,
    const llvm::Instruction *SuccessorInst) {
  if (taintConfig.isSource(CurrentInst)) {
    // TODO: generate current inst wrapped in an ExtendedValue
  }

  if (taintConfig.isSink(CurrentInst)) {
    // TODO: report leak as done for the functions
  }

  if (DataFlowUtils::isReturnValue(CurrentInst, SuccessorInst)) {
    return std::make_shared<ReturnInstFlowFunction>(SuccessorInst, traceStats,
                                                    getZeroValue());
  }

  if (llvm::isa<llvm::StoreInst>(CurrentInst)) {
    return std::make_shared<StoreInstFlowFunction>(CurrentInst, traceStats,
                                                   getZeroValue());
  }

  if (llvm::isa<llvm::BranchInst>(CurrentInst) ||
      llvm::isa<llvm::SwitchInst>(CurrentInst)) {
    return std::make_shared<BranchSwitchInstFlowFunction>(
        CurrentInst, traceStats, getZeroValue());
  }

  if (llvm::isa<llvm::GetElementPtrInst>(CurrentInst)) {
    return std::make_shared<GEPInstFlowFunction>(CurrentInst, traceStats,
                                                 getZeroValue());
  }

  if (llvm::isa<llvm::PHINode>(CurrentInst)) {
    return std::make_shared<PHINodeFlowFunction>(CurrentInst, traceStats,
                                                 getZeroValue());
  }

  if (DataFlowUtils::isCheckOperandsInst(CurrentInst)) {
    return std::make_shared<CheckOperandsFlowFunction>(CurrentInst, traceStats,
                                                       getZeroValue());
  }

  return std::make_shared<IdentityFlowFunction>(CurrentInst, traceStats,
                                                getZeroValue());
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getCallFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  return std::make_shared<MapTaintedValuesToCallee>(
      llvm::cast<llvm::CallInst>(CallStmt), DestFun, traceStats,
      getZeroValue());
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function *CalleeFun,
    const llvm::Instruction *ExitStmt, const llvm::Instruction *RetSite) {
  return std::make_shared<MapTaintedValuesToCaller>(
      llvm::cast<llvm::CallInst>(CallSite),
      llvm::cast<llvm::ReturnInst>(ExitStmt), traceStats, getZeroValue());
}

/*
 * Every fact that was valid before call to function will be handled here
 * right after the function call has returned... We would like to keep all
 * previously generated facts. Facts from the returning functions are
 * handled in getRetFlowFunction.
 */
IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Instruction *RetSite,
    std::set<const llvm::Function *> Callees) {
  /*
   * It is important to wrap the identity call here. Consider the following
   * example:
   *
   * br i1 %cmp, label %cond.true, label %cond.false
   * cond.true:
   *   %call1 = call i32 (...) @foo()
   *   br label %cond.end
   * ...
   * cond.end:
   * %cond = phi i32 [ %call1, %cond.true ], [ 1, %cond.false ]
   *
   * Because we are in a tainted branch we must push %call1 to facts. We cannot
   * do that in the getSummaryFlowFunction() because if we return a flow
   * function we never follow the function. If we intercept here the call
   * instruction will be pushed when the flow function is called with the branch
   * instruction fact.
   */
  return std::make_shared<CallToRetFlowFunction>(CallSite, traceStats,
                                                 getZeroValue());
}

/*
 * If we return sth. different than a nullptr the callee will not be traversed.
 * Instead facts according to the defined flow function will be taken into
 * account.
 */
IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *CallStmt, const llvm::Function *DestFun) {
  const auto DestFunName = DestFun->getName();

  /*
   * We exclude function ptr calls as they will be applied to every
   * function matching its signature (@see LLVMBasedICFG.cpp:217).
   */
  const auto *const CallInst = llvm::cast<llvm::CallInst>(CallStmt);
  bool IsStaticCallSite = CallInst->getCalledFunction();
  if (!IsStaticCallSite) {
    return std::make_shared<IdentityFlowFunction>(CallStmt, traceStats,
                                                  getZeroValue());
  }

  /*
   * Exclude blacklisted functions here.
   */

  if (taintConfig.isSink(DestFunName)) {
    return std::make_shared<IdentityFlowFunction>(CallStmt, traceStats,
                                                  getZeroValue());
  }

  /*
   * Intrinsics.
   */
  if (llvm::isa<llvm::MemTransferInst>(CallStmt)) {
    return std::make_shared<MemTransferInstFlowFunction>(CallStmt, traceStats,
                                                         getZeroValue());
  }

  if (llvm::isa<llvm::MemSetInst>(CallStmt)) {
    return std::make_shared<MemSetInstFlowFunction>(CallStmt, traceStats,
                                                    getZeroValue());
  }

  if (llvm::isa<llvm::VAStartInst>(CallStmt)) {
    return std::make_shared<VAStartInstFlowFunction>(CallStmt, traceStats,
                                                     getZeroValue());
  }

  if (llvm::isa<llvm::VAEndInst>(CallStmt)) {
    return std::make_shared<VAEndInstFlowFunction>(CallStmt, traceStats,
                                                   getZeroValue());
  }

  /*
   * Provide summary for tainted functions.
   */
  if (taintConfig.isSource(DestFunName)) {
    return std::make_shared<GenerateFlowFunction>(CallStmt, traceStats,
                                                  getZeroValue());
  }

  /*
   * Skip all (other) declarations.
   */
  bool IsDeclaration = DestFun->isDeclaration();
  if (IsDeclaration) {
    return std::make_shared<IdentityFlowFunction>(CallStmt, traceStats,
                                                  getZeroValue());
  }

  /*
   * Follow call -> getCallFlowFunction()
   */
  return nullptr;
}

std::map<const llvm::Instruction *, std::set<ExtendedValue>>
IFDSFieldSensTaintAnalysis::initialSeeds() {
  std::map<const llvm::Instruction *, std::set<ExtendedValue>> SeedMap;
  for (const auto &EntryPoint : this->EntryPoints) {
    if (taintConfig.isSink(EntryPoint)) {
      continue;
    }
    SeedMap.insert(
        std::make_pair(&ICF->getFunction(EntryPoint)->front().front(),
                       std::set<ExtendedValue>({getZeroValue()})));
  }
  // additionally, add initial seeds if there are any
  auto TaintConfigSeeds = taintConfig.getInitialSeeds();
  for (auto &Seed : TaintConfigSeeds) {
    SeedMap[Seed.first].insert(Seed.second.begin(), Seed.second.end());
  }
  return SeedMap;
}

void IFDSFieldSensTaintAnalysis::emitTextReport(
    const SolverResults<const llvm::Instruction *, ExtendedValue, BinaryDomain>
        &SolverResults,
    std::ostream &OS) {
  std::string FirstEntryPoints = *EntryPoints.begin();
  const std::string LcovTraceFile =
      DataFlowUtils::getTraceFilenamePrefix(FirstEntryPoints + "-trace.txt");
  const std::string LcovRetValTraceFile = DataFlowUtils::getTraceFilenamePrefix(
      FirstEntryPoints + "-return-value-trace.txt");

#ifdef DEBUG_BUILD
  // Write line number trace (for tests only)
  LineNumberWriter lineNumberWriter(traceStats, "line-numbers.txt");
  LineNumberWriter.write();
#endif

  // Write lcov trace
  LcovWriter LcovWriter(traceStats, LcovTraceFile);
  LcovWriter.write();

  // Write lcov return value trace
  LcovRetValWriter LcovRetValWriter(traceStats, LcovRetValTraceFile);
  LcovRetValWriter.write();
}

} // namespace psr
