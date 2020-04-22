/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef TRACESTATS_H
#define TRACESTATS_H

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

  TraceStats() {}
  ~TraceStats() = default;

  long add(const llvm::Instruction *instruction,
           const std::vector<const llvm::Value *> &memLocationSeq =
               std::vector<const llvm::Value *>());

  const FileStats getStats() const { return stats; }

private:
  long add(const llvm::Instruction *instruction, bool isReturnValue);

  FunctionStats &getFunctionStats(const std::string &file);
  LineNumberStats &getLineNumberStats(const std::string &file,
                                      const std::string &function);
  FileStats stats;
};

} // namespace psr

#endif // TRACESTATS_H
