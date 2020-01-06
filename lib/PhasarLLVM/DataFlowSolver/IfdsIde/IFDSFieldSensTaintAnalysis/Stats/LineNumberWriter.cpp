/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LineNumberWriter.h>

namespace psr {

void LineNumberWriter::write() const {
  std::ofstream writer(getOutFile());

  LOG_INFO("Writing line number trace to: " << getOutFile());

  for (const auto &fileEntry : getTraceStats().getStats()) {
    const auto functionStats = fileEntry.second;

    for (const auto &functionEntry : functionStats) {
      const auto lineNumberStats = functionEntry.second;

      for (const auto &lineNumberEntry : lineNumberStats) {

        writer << lineNumberEntry.getLineNumber() << "\n";
      }
    }
  }
}

} // namespace psr
