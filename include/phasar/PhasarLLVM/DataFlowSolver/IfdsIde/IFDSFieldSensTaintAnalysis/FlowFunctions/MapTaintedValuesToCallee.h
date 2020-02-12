/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef MAPTAINTEDVALUESTOCALLEE_H
#define MAPTAINTEDVALUESTOCALLEE_H

#include "../Stats/TraceStats.h"

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Instruction.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/LLVMZeroValue.h>
#include <phasar/PhasarLLVM/Domain/ExtendedValue.h>

namespace psr {

class MapTaintedValuesToCallee : public FlowFunction<ExtendedValue> {
public:
  MapTaintedValuesToCallee(const llvm::CallInst *_callInst,
                           const llvm::Function *_destFun,
                           TraceStats &_traceStats, ExtendedValue _zeroValue)
      : callInst(_callInst), destFun(_destFun), traceStats(_traceStats),
        zeroValue(_zeroValue) {}
  ~MapTaintedValuesToCallee() override = default;

  std::set<ExtendedValue> computeTargets(ExtendedValue fact) override;

private:
  const llvm::CallInst *callInst;
  const llvm::Function *destFun;
  TraceStats &traceStats;
  ExtendedValue zeroValue;
};

} // namespace psr

#endif // MAPTAINTEDVALUESTOCALLEE_H
