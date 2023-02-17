/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/Problems/IFDSFieldSensTaintAnalysis.h"

#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/BranchSwitchInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CallToRetFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/CheckOperandsFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GEPInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/GenerateFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/IdentityFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCallee.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MapTaintedValuesToCaller.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemSetInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/MemTransferInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/PHINodeFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/ReturnInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/StoreInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAEndInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/FlowFunctions/VAStartInstFlowFunction.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovRetValWriter.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovWriter.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LineNumberWriter.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStatsWriter.h"
#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/DataFlowUtils.h"

#include "llvm/IR/IntrinsicInst.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace psr {

IFDSFieldSensTaintAnalysis::IFDSFieldSensTaintAnalysis(
    const LLVMProjectIRDB *IRDB, const LLVMTaintConfig *TaintConfig,
    std::vector<std::string> EntryPoints)
    : IFDSTabulationProblem(IRDB, std::move(EntryPoints), createZeroValue()),
      Config(TaintConfig) {
  assert(Config != nullptr);
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getNormalFlowFunction(
    const llvm::Instruction *CurrentInst,
    const llvm::Instruction *SuccessorInst) {
  if (Config->isSource(CurrentInst)) {
    // TODO: generate current inst wrapped in an ExtendedValue
  }

  if (Config->isSink(CurrentInst)) {
    // TODO: report leak as done for the functions
  }

  if (DataFlowUtils::isReturnValue(CurrentInst, SuccessorInst)) {
    return std::make_shared<ReturnInstFlowFunction>(SuccessorInst, Stats,
                                                    getZeroValue());
  }

  if (llvm::isa<llvm::StoreInst>(CurrentInst)) {
    return std::make_shared<StoreInstFlowFunction>(CurrentInst, Stats,
                                                   getZeroValue());
  }

  if (llvm::isa<llvm::BranchInst>(CurrentInst) ||
      llvm::isa<llvm::SwitchInst>(CurrentInst)) {
    return std::make_shared<BranchSwitchInstFlowFunction>(CurrentInst, Stats,
                                                          getZeroValue());
  }

  if (llvm::isa<llvm::GetElementPtrInst>(CurrentInst)) {
    return std::make_shared<GEPInstFlowFunction>(CurrentInst, Stats,
                                                 getZeroValue());
  }

  if (llvm::isa<llvm::PHINode>(CurrentInst)) {
    return std::make_shared<PHINodeFlowFunction>(CurrentInst, Stats,
                                                 getZeroValue());
  }

  if (DataFlowUtils::isCheckOperandsInst(CurrentInst)) {
    return std::make_shared<CheckOperandsFlowFunction>(CurrentInst, Stats,
                                                       getZeroValue());
  }

  return std::make_shared<IdentityFlowFunction>(CurrentInst, Stats,
                                                getZeroValue());
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getCallFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function *DestFun) {
  return std::make_shared<MapTaintedValuesToCallee>(
      llvm::cast<llvm::CallInst>(CallSite), DestFun, Stats, getZeroValue());
}

IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function * /*CalleeFun*/,
    const llvm::Instruction *ExitStmt, const llvm::Instruction * /*RetSite*/) {
  return std::make_shared<MapTaintedValuesToCaller>(
      llvm::cast<llvm::CallInst>(CallSite),
      llvm::cast<llvm::ReturnInst>(ExitStmt), Stats, getZeroValue());
}

/*
 * Every fact that was valid before call to function will be handled here
 * right after the function call has returned... We would like to keep all
 * previously generated facts. Facts from the returning functions are
 * handled in getRetFlowFunction.
 */
IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Instruction * /*RetSite*/,
    llvm::ArrayRef<f_t> /*Callees*/) {
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
  return std::make_shared<CallToRetFlowFunction>(CallSite, Stats,
                                                 getZeroValue());
}

/*
 * If we return sth. different than a nullptr the callee will not be traversed.
 * Instead facts according to the defined flow function will be taken into
 * account.
 */
IFDSFieldSensTaintAnalysis::FlowFunctionPtrType
IFDSFieldSensTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *CallSite, const llvm::Function *DestFun) {
  /*
   * We exclude function ptr calls as they will be applied to every
   * function matching its signature (@see LLVMBasedICFG.cpp:217).
   */
  const auto *const CS = llvm::cast<llvm::CallInst>(CallSite);
  bool IsStaticCallSite = CS->getCalledFunction();
  if (!IsStaticCallSite) {
    return std::make_shared<IdentityFlowFunction>(CS, Stats, getZeroValue());
  }

  /*
   * Exclude blacklisted functions here.
   */
  bool IsSink = Config->mayLeakValuesAt(CallSite, DestFun);

  if (IsSink) {
    return std::make_shared<IdentityFlowFunction>(CS, Stats, getZeroValue());
  }

  /*
   * Intrinsics.
   */
  if (llvm::isa<llvm::MemTransferInst>(CallSite)) {
    return std::make_shared<MemTransferInstFlowFunction>(CallSite, Stats,
                                                         getZeroValue());
  }

  if (llvm::isa<llvm::MemSetInst>(CallSite)) {
    return std::make_shared<MemSetInstFlowFunction>(CallSite, Stats,
                                                    getZeroValue());
  }

  if (llvm::isa<llvm::VAStartInst>(CallSite)) {
    return std::make_shared<VAStartInstFlowFunction>(CallSite, Stats,
                                                     getZeroValue());
  }

  if (llvm::isa<llvm::VAEndInst>(CallSite)) {
    return std::make_shared<VAEndInstFlowFunction>(CallSite, Stats,
                                                   getZeroValue());
  }

  /*
   * Provide summary for tainted functions.
   */
  bool TaintRet =
      Config->isSource(CallSite) ||
      (Config->getRegisteredSourceCallBack() &&
       Config->getRegisteredSourceCallBack()(CallSite).count(CallSite));
  /// TODO: What about source parameters? They are not handled in the original
  /// implementation, so skip them for now and add them later.
  if (TaintRet) {
    return std::make_shared<GenerateFlowFunction>(CallSite, Stats,
                                                  getZeroValue());
  }

  /*
   * Skip all (other) declarations.
   */
  bool IsDeclaration = DestFun->isDeclaration();
  if (IsDeclaration) {
    return std::make_shared<IdentityFlowFunction>(CallSite, Stats,
                                                  getZeroValue());
  }

  /*
   * Follow call -> getCallFlowFunction()
   */
  return nullptr;
}

InitialSeeds<const llvm::Instruction *, ExtendedValue,
             IFDSFieldSensTaintAnalysis::l_t>
IFDSFieldSensTaintAnalysis::initialSeeds() {
  InitialSeeds<const llvm::Instruction *, ExtendedValue,
               IFDSFieldSensTaintAnalysis::l_t>
      Seeds;
  auto TaintSeeds = Config->makeInitialSeeds();
  for (const auto &[Inst, Facts] : TaintSeeds) {
    for (const auto &Fact : Facts) {
      Seeds.addSeed(Inst, ExtendedValue(Fact));
    }
  }
  return Seeds;
}

void IFDSFieldSensTaintAnalysis::emitTextReport(
    const SolverResults<const llvm::Instruction *, ExtendedValue, BinaryDomain>
        & /*SolverResults*/,
    llvm::raw_ostream & /*OS*/) {
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
  LcovWriter LcovWriter(Stats, LcovTraceFile);
  LcovWriter.write();

  // Write lcov return value trace
  LcovRetValWriter LcovRetValWriter(Stats, LcovRetValTraceFile);
  LcovRetValWriter.write();
}

} // namespace psr
