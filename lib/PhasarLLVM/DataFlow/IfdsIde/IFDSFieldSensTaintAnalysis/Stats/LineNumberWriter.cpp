/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlow/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LineNumberWriter.h"

namespace psr {

void LineNumberWriter::write() const {
  std::ofstream Writer(getOutFile());

  LOG_INFO("Writing line number trace to: " << getOutFile());

  for (const auto &FileEntry : getTraceStats().getStats()) {
    const auto FunctionStats = FileEntry.second;

    for (const auto &FunctionEntry : FunctionStats) {
      const auto LineNumberStats = FunctionEntry.second;

      for (const auto &LineNumberEntry : LineNumberStats) {

        Writer << LineNumberEntry.getLineNumber() << "\n";
      }
    }
  }
}

} // namespace psr
