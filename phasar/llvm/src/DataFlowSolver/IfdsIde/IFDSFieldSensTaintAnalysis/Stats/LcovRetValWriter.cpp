/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovRetValWriter.h"

namespace psr {

static void filterReturnValues(TraceStats::FileStats &FileStats) {
  for (auto FileStatsIt = FileStats.begin(); FileStatsIt != FileStats.end();) {
    const auto File = FileStatsIt->first;
    auto &FunctionStats = FileStatsIt->second;

    for (auto FunctionStatsIt = FunctionStats.begin();
         FunctionStatsIt != FunctionStats.end();) {
      const auto Function = FunctionStatsIt->first;
      auto &LineNumberStats = FunctionStatsIt->second;

      for (auto LineNumberStatsIt = LineNumberStats.begin();
           LineNumberStatsIt != LineNumberStats.end();) {
        bool IsLineNumberRetVal = LineNumberStatsIt->isReturnValue();
        if (!IsLineNumberRetVal) {
          LineNumberStatsIt = LineNumberStats.erase(LineNumberStatsIt);
        } else {
          ++LineNumberStatsIt;
        }
      }

      bool IsLineNumberStatsEmpty = LineNumberStats.empty();
      if (IsLineNumberStatsEmpty) {
        FunctionStatsIt = FunctionStats.erase(FunctionStatsIt);
      } else {
        ++FunctionStatsIt;
      }
    }

    bool IsFunctionStatsEmpty = FunctionStats.empty();
    if (IsFunctionStatsEmpty) {
      FileStatsIt = FileStats.erase(FileStatsIt);
    } else {
      ++FileStatsIt;
    }
  }
}

void LcovRetValWriter::write() const {
  std::ofstream Writer(getOutFile());

  LOG_INFO("Writing lcov return value trace to: " << getOutFile());

  TraceStats::FileStats Stats = getTraceStats().getStats();

  filterReturnValues(Stats);

  for (const auto &FileEntry : Stats) {
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
