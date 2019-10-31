/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include <llvm/IR/DebugInfoMetadata.h>

namespace psr {

long TraceStats::add(const llvm::Instruction *instruction, bool isReturnValue) {
  const llvm::DebugLoc debugLocInst = instruction->getDebugLoc();
  if (!debugLocInst)
    return 0;

  const llvm::DebugLoc debugLocFn = debugLocInst.getFnDebugLoc();
  if (!debugLocFn)
    return 0;

  const auto function = instruction->getFunction();
  if (!function)
    return 0;

  const auto functionName = function->getName();

  const auto fnScope = llvm::cast<llvm::DIScope>(debugLocFn.getScope());

  const std::string file =
      fnScope->getDirectory().str() + "/" + fnScope->getFilename().str();

  unsigned int lineNumber = debugLocInst->getLine();

  LOG_DEBUG("Tainting " << file << ":" << functionName << ":" << lineNumber
                        << ":" << isReturnValue);

  TraceStats::LineNumberStats &lineNumberStats =
      getLineNumberStats(file, functionName);

  LineNumberEntry lineNumberEntry(lineNumber);

  if (isReturnValue) {
    lineNumberStats.erase(lineNumberEntry);
    lineNumberEntry.setReturnValue(true);
  }

  lineNumberStats.insert(lineNumberEntry);

  return 1;
}

long TraceStats::add(const llvm::Instruction *instruction,
                     const std::vector<const llvm::Value *> memLocationSeq) {
  bool isRetInstruction = llvm::isa<llvm::ReturnInst>(instruction);
  if (isRetInstruction) {
    const auto basicBlock = instruction->getParent();
    const auto basicBlockName = basicBlock->getName();

    bool isReturnBasicBlock = basicBlockName.compare_lower("return") == 0;
    if (isReturnBasicBlock)
      return 0;

    return add(instruction, true);
  }

  bool isGENMemoryLocation = !memLocationSeq.empty();
  if (isGENMemoryLocation) {
    const auto memLocationFrame = memLocationSeq.front();

    if (const auto allocaInst =
            llvm::dyn_cast<llvm::AllocaInst>(memLocationFrame)) {
      const auto instructionName = allocaInst->getName();
      bool isRetVal = instructionName.compare_lower("retval") == 0;

      if (isRetVal)
        return add(instruction, true);
    }
  }

  return add(instruction, false);
}

TraceStats::FunctionStats &TraceStats::getFunctionStats(std::string file) {
  auto functionStatsEntry = stats.find(file);
  if (functionStatsEntry != stats.end())
    return functionStatsEntry->second;

  stats.insert({file, FunctionStats()});

  return stats.find(file)->second;
}

TraceStats::LineNumberStats &
TraceStats::getLineNumberStats(std::string file, std::string function) {
  TraceStats::FunctionStats &functionStats = getFunctionStats(file);

  auto lineNumberEntry = functionStats.find(function);
  if (lineNumberEntry != functionStats.end())
    return lineNumberEntry->second;

  functionStats.insert({function, LineNumberStats()});

  return functionStats.find(function)->second;
}

} // namespace psr
