/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovWriter.h"

namespace psr {

void LcovWriter::write() const {
  std::ofstream Writer(getOutFile());

  LOG_INFO("Writing lcov trace to: " << getOutFile());

  for (const auto &FileEntry : getTraceStats().getStats()) {
    const auto File = FileEntry.first;
    const auto FunctionStats = FileEntry.second;

    Writer << "SF:" << File << "\n";

    for (const auto &FunctionEntry : FunctionStats) {
      const auto Function = FunctionEntry.first;

      Writer << "FNDA:"
             << "1," << Function << "\n";
    }

    for (const auto &FunctionEntry : FunctionStats) {
      const auto LineNumberStats = FunctionEntry.second;

      for (const auto &LineNumberEntry : LineNumberStats) {

        Writer << "DA:" << LineNumberEntry.getLineNumber() << ",1"
               << "\n";
      }
    }

    Writer << "end_of_record"
           << "\n";
  }
}

} // namespace psr
