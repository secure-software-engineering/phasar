/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_UTILS_DATAFLOWUTILS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_UTILS_DATAFLOWUTILS_H

#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "llvm/IR/Instructions.h"

#include "phasar/PhasarLLVM/Domain/ExtendedValue.h"

namespace psr {

class DataFlowUtils {
public:
  DataFlowUtils() = delete;

  static bool isValueTainted(const llvm::Value *CurrentInst,
                             const ExtendedValue &Fact);

  static bool isMemoryLocationTainted(const llvm::Value *MemLocationMatr,
                                      const ExtendedValue &Fact);

  static std::vector<const llvm::Value *>
  getMemoryLocationSeqFromMatr(const llvm::Value *MemLocationMatr);
  static std::vector<const llvm::Value *>
  getMemoryLocationSeqFromFact(const ExtendedValue &MemLocationFact);
  static std::vector<const llvm::Value *>
  getVaListMemoryLocationSeqFromFact(const ExtendedValue &VaListFact);

  static bool isMemoryLocationSeqsEqual(
      const std::vector<const llvm::Value *> &MemLocationSeq1,
      const std::vector<const llvm::Value *> &MemLocationSeq2);

  static bool isSubsetMemoryLocationSeq(
      const std::vector<const llvm::Value *> &MemLocationSeqInst,
      const std::vector<const llvm::Value *> &MemLocationSeqFact);
  static std::vector<const llvm::Value *> getRelocatableMemoryLocationSeq(
      const std::vector<const llvm::Value *> &TaintedMemLocationSeq,
      const std::vector<const llvm::Value *> &SrcMemLocationSeq);
  static std::vector<const llvm::Value *> joinMemoryLocationSeqs(
      const std::vector<const llvm::Value *> &MemLocationSeq1,
      const std::vector<const llvm::Value *> &MemLocationSeq2);

  static bool isPatchableArgumentStore(const llvm::Value *SrcValue,
                                       const ExtendedValue &Fact);
  static bool isPatchableArgumentMemcpy(
      const llvm::Value *SrcValue,
      const std::vector<const llvm::Value *> &SrcMemLocationSeq,
      const ExtendedValue &Fact);
  static bool isPatchableVaListArgument(const llvm::Value *SrcValue,
                                        const ExtendedValue &Fact);
  static bool isPatchableReturnValue(const llvm::Value *SrcValue,
                                     const ExtendedValue &Fact);
  static std::vector<const llvm::Value *> patchMemoryLocationFrame(
      const std::vector<const llvm::Value *> &PatchableMemLocationSeq,
      const std::vector<const llvm::Value *> &PatchMemLocationSeq);

  static std::vector<
      std::tuple<const llvm::Value *, const std::vector<const llvm::Value *>,
                 const llvm::Value *>>
  getSanitizedArgList(const llvm::CallInst *CallInst,
                      const llvm::Function *DestFun,
                      const llvm::Value *ZeroValue);

  static const llvm::BasicBlock *
  getEndOfTaintedBlock(const llvm::BasicBlock *StartBasicBlock);
  static bool removeTaintedBlockInst(const ExtendedValue &Fact,
                                     const llvm::Instruction *CurrentInst);
  static bool isAutoGENInTaintedBlock(const llvm::Instruction *CurrentInst);

  static bool isMemoryLocationFact(const ExtendedValue &Ev);
  static bool isKillAfterStoreFact(const ExtendedValue &Ev);
  static bool isCheckOperandsInst(const llvm::Instruction *CurrentInst);
  static bool isAutoIdentity(const llvm::Instruction *CurrentInst,
                             const ExtendedValue &Fact);
  static bool isVarArgParam(const llvm::Value *Param,
                            const llvm::Value *ZeroValue);
  static bool isVaListType(const llvm::Type *Type);
  static bool isReturnValue(const llvm::Instruction *CurrentInst,
                            const llvm::Instruction *SuccessorInst);
  static bool isArrayDecay(const llvm::Value *MemLocationMatr);
  static bool isGlobalMemoryLocationSeq(
      const std::vector<const llvm::Value *> &MemLocationSeq);

  static void dumpFact(const ExtendedValue &Ev);

  static std::set<std::string> getTaintedFunctions();
  static std::set<std::string> getBlacklistedFunctions();

  static std::string getTraceFilenamePrefix(const std::string &EntryPoint);
};

} // namespace psr

#endif
