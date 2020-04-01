/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "llvm/IR/DebugInfoMetadata.h"

namespace psr {

long TraceStats::add(const llvm::Instruction *Instruction, bool IsReturnValue) {
  const llvm::DebugLoc debugLocInst = Instruction->getDebugLoc();
  if (!debugLocInst)
    return 0;

  const llvm::DebugLoc debugLocFn = debugLocInst.getFnDebugLoc();
  if (!debugLocFn)
    return 0;

  const auto function = Instruction->getFunction();
  if (!function)
    return 0;

  const auto functionName = function->getName();

  const auto fnScope = llvm::cast<llvm::DIScope>(debugLocFn.getScope());

  const std::string file =
      fnScope->getDirectory().str() + "/" + fnScope->getFilename().str();

  unsigned int lineNumber = debugLocInst->getLine();

  LOG_DEBUG("Tainting " << file << ":" << functionName << ":" << lineNumber
                        << ":" << IsReturnValue);

  TraceStats::LineNumberStats &lineNumberStats =
      getLineNumberStats(file, functionName);

  LineNumberEntry lineNumberEntry(lineNumber);

  if (IsReturnValue) {
    lineNumberStats.erase(lineNumberEntry);
    lineNumberEntry.setReturnValue(true);
  }

  lineNumberStats.insert(lineNumberEntry);

  return 1;
}

long TraceStats::add(const llvm::Instruction *Instruction,
                     const std::vector<const llvm::Value *> MemLocationSeq) {
  bool isRetInstruction = llvm::isa<llvm::ReturnInst>(Instruction);
  if (isRetInstruction) {
    const auto basicBlock = Instruction->getParent();
    const auto basicBlockName = basicBlock->getName();

    bool isReturnBasicBlock = basicBlockName.compare_lower("return") == 0;
    if (isReturnBasicBlock)
      return 0;

    return add(Instruction, true);
  }

  bool isGENMemoryLocation = !MemLocationSeq.empty();
  if (isGENMemoryLocation) {
    const auto memLocationFrame = MemLocationSeq.front();

    if (const auto allocaInst =
            llvm::dyn_cast<llvm::AllocaInst>(memLocationFrame)) {
      const auto instructionName = allocaInst->getName();
      bool isRetVal = instructionName.compare_lower("retval") == 0;

      if (isRetVal)
        return add(Instruction, true);
    }
  }

  return add(Instruction, false);
}

TraceStats::FunctionStats &TraceStats::getFunctionStats(std::string File) {
  auto functionStatsEntry = stats.find(File);
  if (functionStatsEntry != stats.end())
    return functionStatsEntry->second;

  stats.insert({File, FunctionStats()});

  return stats.find(File)->second;
}

TraceStats::LineNumberStats &
TraceStats::getLineNumberStats(std::string File, std::string Function) {
  TraceStats::FunctionStats &functionStats = getFunctionStats(File);

  auto lineNumberEntry = functionStats.find(Function);
  if (lineNumberEntry != functionStats.end())
    return lineNumberEntry->second;

  functionStats.insert({Function, LineNumberStats()});

  return functionStats.find(Function)->second;
}

} // namespace psr
