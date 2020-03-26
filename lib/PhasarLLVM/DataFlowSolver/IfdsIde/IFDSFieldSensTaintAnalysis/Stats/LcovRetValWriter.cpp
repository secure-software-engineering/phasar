/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IFDSFieldSensTaintAnalysis/Stats/LcovRetValWriter.h"

namespace psr {

static void filterReturnValues(TraceStats::FileStats &fileStats) {
  for (auto fileStatsIt = fileStats.begin(); fileStatsIt != fileStats.end();) {
    const auto file = fileStatsIt->first;
    auto &functionStats = fileStatsIt->second;

    for (auto functionStatsIt = functionStats.begin();
         functionStatsIt != functionStats.end();) {
      const auto function = functionStatsIt->first;
      auto &lineNumberStats = functionStatsIt->second;

      for (auto lineNumberStatsIt = lineNumberStats.begin();
           lineNumberStatsIt != lineNumberStats.end();) {
        bool isLineNumberRetVal = lineNumberStatsIt->isReturnValue();
        if (!isLineNumberRetVal) {
          lineNumberStatsIt = lineNumberStats.erase(lineNumberStatsIt);
        } else {
          ++lineNumberStatsIt;
        }
      }

      bool isLineNumberStatsEmpty = lineNumberStats.empty();
      if (isLineNumberStatsEmpty) {
        functionStatsIt = functionStats.erase(functionStatsIt);
      } else {
        ++functionStatsIt;
      }
    }

    bool isFunctionStatsEmpty = functionStats.empty();
    if (isFunctionStatsEmpty) {
      fileStatsIt = fileStats.erase(fileStatsIt);
    } else {
      ++fileStatsIt;
    }
  }
}

void LcovRetValWriter::write() const {
  std::ofstream writer(getOutFile());

  LOG_INFO("Writing lcov return value trace to: " << getOutFile());

  TraceStats::FileStats stats = getTraceStats().getStats();

  filterReturnValues(stats);

  for (const auto &fileEntry : stats) {
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
