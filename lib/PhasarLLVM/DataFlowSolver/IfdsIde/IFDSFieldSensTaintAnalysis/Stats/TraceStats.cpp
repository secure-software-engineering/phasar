/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <utility>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/TraceStats.h"

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Utils/Log.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

#include "llvm/IR/DebugInfoMetadata.h"

namespace psr {

long TraceStats::add(const llvm::Instruction *Instruction, bool IsReturnValue) {
  const llvm::DebugLoc &DebugLocInst = Instruction->getDebugLoc();
  if (!DebugLocInst) {
    return 0;
  }

  const llvm::DebugLoc DebugLocFn = DebugLocInst.getFnDebugLoc();
  if (!DebugLocFn) {
    return 0;
  }

  const auto *const Function = Instruction->getFunction();
  if (!Function) {
    return 0;
  }

  const auto FunctionName = Function->getName().str();

  auto *const FnScope = llvm::cast<llvm::DIScope>(DebugLocFn.getScope());

  const std::string File =
      FnScope->getDirectory().str() + "/" + FnScope->getFilename().str();

  unsigned int LineNumber = DebugLocInst->getLine();

  LOG_DEBUG("Tainting " << File << ":" << FunctionName << ":" << LineNumber
                        << ":" << IsReturnValue);

  TraceStats::LineNumberStats &LineNumberStats =
      getLineNumberStats(File, FunctionName);

  LineNumberEntry LineNumberEntry(LineNumber);

  if (IsReturnValue) {
    LineNumberStats.erase(LineNumberEntry);
    LineNumberEntry.setReturnValue(true);
  }

  LineNumberStats.insert(LineNumberEntry);

  return 1;
}

long TraceStats::add(const llvm::Instruction *Instruction,
                     const std::vector<const llvm::Value *> &MemLocationSeq) {
  bool IsRetInstruction = llvm::isa<llvm::ReturnInst>(Instruction);
  if (IsRetInstruction) {
    const auto *const BasicBlock = Instruction->getParent();
    const auto BasicBlockName = BasicBlock->getName();

    bool IsReturnBasicBlock = BasicBlockName.compare("return") == 0;
    if (IsReturnBasicBlock) {
      return 0;
    }

    return add(Instruction, true);
  }

  bool IsGENMemoryLocation = !MemLocationSeq.empty();
  if (IsGENMemoryLocation) {
    const auto *const MemLocationFrame = MemLocationSeq.front();

    if (const auto *const AllocaInst =
            llvm::dyn_cast<llvm::AllocaInst>(MemLocationFrame)) {
      const auto InstructionName = AllocaInst->getName();
      bool IsRetVal = InstructionName.compare("retval") == 0;

      if (IsRetVal) {
        return add(Instruction, true);
      }
    }
  }

  return add(Instruction, false);
}

TraceStats::FunctionStats &
TraceStats::getFunctionStats(const std::string &File) {
  auto FunctionStatsEntry = Stats.find(File);
  if (FunctionStatsEntry != Stats.end()) {
    return FunctionStatsEntry->second;
  }

  Stats.insert({File, FunctionStats()});

  return Stats.find(File)->second;
}

TraceStats::LineNumberStats &
TraceStats::getLineNumberStats(const std::string &File,
                               const std::string &Function) {
  TraceStats::FunctionStats &FunctionStats = getFunctionStats(File);

  auto LineNumberEntry = FunctionStats.find(Function);
  if (LineNumberEntry != FunctionStats.end()) {
    return LineNumberEntry->second;
  }

  FunctionStats.insert({Function, LineNumberStats()});

  return FunctionStats.find(Function)->second;
}

} // namespace psr
