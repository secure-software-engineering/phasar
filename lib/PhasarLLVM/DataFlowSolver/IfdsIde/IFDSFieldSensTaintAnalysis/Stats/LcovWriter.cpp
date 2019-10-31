/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovWriter.h>

namespace psr {

void LcovWriter::write() const {
  std::ofstream writer(getOutFile());

  LOG_INFO("Writing lcov trace to: " << getOutFile());

  for (const auto &fileEntry : getTraceStats().getStats()) {
    const auto file = fileEntry.first;
    const auto functionStats = fileEntry.second;

    writer << "SF:" << file << "\n";

    for (const auto &functionEntry : functionStats) {
      const auto function = functionEntry.first;

      writer << "FNDA:"
             << "1," << function << "\n";
    }

    for (const auto &functionEntry : functionStats) {
      const auto lineNumberStats = functionEntry.second;

      for (const auto &lineNumberEntry : lineNumberStats) {

        writer << "DA:" << lineNumberEntry.getLineNumber() << ",1"
               << "\n";
      }
    }

    writer << "end_of_record"
           << "\n";
  }
}

} // namespace psr
