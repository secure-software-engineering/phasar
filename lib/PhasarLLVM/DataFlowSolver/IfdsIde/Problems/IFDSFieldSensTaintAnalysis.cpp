/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <set>
#include <string>
#include <vector>

#include <llvm/IR/IntrinsicInst.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CallToRetFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GenerateFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/IdentityFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCallee.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCaller.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemSetInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemTransferInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/PHINodeFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/ReturnInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/StoreInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAEndInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAStartInstFlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovRetValWriter.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovWriter.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LineNumberWriter.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStatsWriter.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/Problems/IFDSFieldSensTaintAnalysis.h>
#include <phasar/PhasarLLVM/Pointer/LLVMPointsToInfo.h>
#include <phasar/PhasarLLVM/TypeHierarchy/LLVMTypeHierarchy.h>

namespace psr {

IFDSFieldSensTaintAnalysis::IFDSFieldSensTaintAnalysis(
    const ProjectIRDB *IRDB, const LLVMTypeHierarchy *TH,
    const LLVMBasedICFG *ICF, const LLVMPointsToInfo *PT,
    const TaintConfiguration<ExtendedValue> &TaintConfig,
    std::set<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, TH, ICF, PT, EntryPoints),
      taintConfig(TaintConfig) {
  IFDSFieldSensTaintAnalysis::ZeroValue = createZeroValue();
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSFieldSensTaintAnalysis::getNormalFlowFunction(
    const llvm::Instruction *currentInst,
    const llvm::Instruction *successorInst) {
  if (taintConfig.isSource(currentInst)) {
    // TODO: generate current inst wrapped in an ExtendedValue
  }

  if (taintConfig.isSink(currentInst)) {
    // TODO: report leak as done for the functions
  }

  if (DataFlowUtils::isReturnValue(currentInst, successorInst))
    return std::make_shared<ReturnInstFlowFunction>(successorInst, traceStats,
                                                    getZeroValue());

  if (llvm::isa<llvm::StoreInst>(currentInst))
    return std::make_shared<StoreInstFlowFunction>(currentInst, traceStats,
                                                   getZeroValue());

  if (llvm::isa<llvm::BranchInst>(currentInst) ||
      llvm::isa<llvm::SwitchInst>(currentInst))
    return std::make_shared<BranchSwitchInstFlowFunction>(
        currentInst, traceStats, getZeroValue());

  if (llvm::isa<llvm::GetElementPtrInst>(currentInst))
    return std::make_shared<GEPInstFlowFunction>(currentInst, traceStats,
                                                 getZeroValue());

  if (llvm::isa<llvm::PHINode>(currentInst))
    return std::make_shared<PHINodeFlowFunction>(currentInst, traceStats,
                                                 getZeroValue());

  if (DataFlowUtils::isCheckOperandsInst(currentInst))
    return std::make_shared<CheckOperandsFlowFunction>(currentInst, traceStats,
                                                       getZeroValue());

  return std::make_shared<IdentityFlowFunction>(currentInst, traceStats,
                                                getZeroValue());
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSFieldSensTaintAnalysis::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destFun) {
  return std::make_shared<MapTaintedValuesToCallee>(
      llvm::cast<llvm::CallInst>(callStmt), destFun, traceStats,
      getZeroValue());
}

std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSFieldSensTaintAnalysis::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeFun,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  return std::make_shared<MapTaintedValuesToCaller>(
      llvm::cast<llvm::CallInst>(callSite),
      llvm::cast<llvm::ReturnInst>(exitStmt), traceStats, getZeroValue());
}

/*
 * Every fact that was valid before call to function will be handled here
 * right after the function call has returned... We would like to keep all
 * previously generated facts. Facts from the returning functions are
 * handled in getRetFlowFunction.
 */
std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSFieldSensTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    std::set<const llvm::Function *> callees) {
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
  return std::make_shared<CallToRetFlowFunction>(callSite, traceStats,
                                                 getZeroValue());
}

/*
 * If we return sth. different than a nullptr the callee will not be traversed.
 * Instead facts according to the defined flow function will be taken into
 * account.
 */
std::shared_ptr<FlowFunction<ExtendedValue>>
IFDSFieldSensTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destFun) {
  const auto destFunName = destFun->getName();

  /*
   * We exclude function ptr calls as they will be applied to every
   * function matching its signature (@see LLVMBasedICFG.cpp:217).
   */
  const auto callInst = llvm::cast<llvm::CallInst>(callStmt);
  bool isStaticCallSite = callInst->getCalledFunction();
  if (!isStaticCallSite)
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  getZeroValue());

  /*
   * Exclude blacklisted functions here.
   */

  if (taintConfig.isSink(destFunName))
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  getZeroValue());

  /*
   * Intrinsics.
   */
  if (llvm::isa<llvm::MemTransferInst>(callStmt))
    return std::make_shared<MemTransferInstFlowFunction>(callStmt, traceStats,
                                                         getZeroValue());

  if (llvm::isa<llvm::MemSetInst>(callStmt))
    return std::make_shared<MemSetInstFlowFunction>(callStmt, traceStats,
                                                    getZeroValue());

  if (llvm::isa<llvm::VAStartInst>(callStmt))
    return std::make_shared<VAStartInstFlowFunction>(callStmt, traceStats,
                                                     getZeroValue());

  if (llvm::isa<llvm::VAEndInst>(callStmt))
    return std::make_shared<VAEndInstFlowFunction>(callStmt, traceStats,
                                                   getZeroValue());

  /*
   * Provide summary for tainted functions.
   */
  if (taintConfig.isSource(destFunName))
    return std::make_shared<GenerateFlowFunction>(callStmt, traceStats,
                                                  getZeroValue());

  /*
   * Skip all (other) declarations.
   */
  bool isDeclaration = destFun->isDeclaration();
  if (isDeclaration)
    return std::make_shared<IdentityFlowFunction>(callStmt, traceStats,
                                                  getZeroValue());

  /*
   * Follow call -> getCallFlowFunction()
   */
  return nullptr;
}

std::map<const llvm::Instruction *, std::set<ExtendedValue>>
IFDSFieldSensTaintAnalysis::initialSeeds() {
  std::map<const llvm::Instruction *, std::set<ExtendedValue>> seedMap;
  for (const auto &entryPoint : this->EntryPoints) {
    if (taintConfig.isSink(entryPoint))
      continue;
    seedMap.insert(
        std::make_pair(&ICF->getFunction(entryPoint)->front().front(),
                       std::set<ExtendedValue>({getZeroValue()})));
  }
  // additionally, add initial seeds if there are any
  auto taintConfigSeeds = taintConfig.getInitialSeeds();
  for (auto &seed : taintConfigSeeds) {
    seedMap[seed.first].insert(seed.second.begin(), seed.second.end());
  }
  return seedMap;
}

void IFDSFieldSensTaintAnalysis::emitTextReport(
    const SolverResults<const llvm::Instruction *, ExtendedValue, BinaryDomain>
        &solverResults,
    std::ostream &os) {
  std::string FirstEntryPoints = *EntryPoints.begin();
  const std::string lcovTraceFile =
      DataFlowUtils::getTraceFilenamePrefix(FirstEntryPoints + "-trace.txt");
  const std::string lcovRetValTraceFile = DataFlowUtils::getTraceFilenamePrefix(
      FirstEntryPoints + "-return-value-trace.txt");

#ifdef DEBUG_BUILD
  // Write line number trace (for tests only)
  LineNumberWriter lineNumberWriter(traceStats, "line-numbers.txt");
  lineNumberWriter.write();
#endif

  // Write lcov trace
  LcovWriter lcovWriter(traceStats, lcovTraceFile);
  lcovWriter.write();

  // Write lcov return value trace
  LcovRetValWriter lcovRetValWriter(traceStats, lcovRetValTraceFile);
  lcovRetValWriter.write();
}

} // namespace psr
