/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_TRACESTATS_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_TRACESTATS_H

#include "LineNumberEntry.h"

#include <map>
#include <set>

#include "llvm/IR/Instruction.h"

namespace psr {

class TraceStats {
public:
  using FileStats =
      std::map<std::string, std::map<std::string, std::set<LineNumberEntry>>>;
  using FunctionStats = std::map<std::string, std::set<LineNumberEntry>>;
  using LineNumberStats = std::set<LineNumberEntry>;

  TraceStats() = default;
  ~TraceStats() = default;

  long add(const llvm::Instruction *Inst,
           const std::vector<const llvm::Value *> &MemLocationSeq =
               std::vector<const llvm::Value *>());

  [[nodiscard]] FileStats getStats() const { return Stats; }

private:
  long add(const llvm::Instruction *Inst, bool IsReturnValue);

  FunctionStats &getFunctionStats(const std::string &File);
  LineNumberStats &getLineNumberStats(const std::string &File,
                                      const std::string &FunctionName);
  FileStats Stats;
};

} // namespace psr

#endif
