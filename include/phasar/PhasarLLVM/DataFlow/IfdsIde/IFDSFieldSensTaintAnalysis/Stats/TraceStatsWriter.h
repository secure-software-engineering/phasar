/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_TRACESTATSWRITER_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_TRACESTATSWRITER_H

#include "../Utils/Log.h"
#include "TraceStats.h"

#include <fstream>
#include <string>

namespace psr {

class TraceStatsWriter {
public:
  TraceStatsWriter(const TraceStats &TraceStats, std::string OutFile)
      : TrStats(TraceStats), OutFile(std::move(OutFile)) {}
  virtual ~TraceStatsWriter() = default;

  virtual void write() const = 0;

protected:
  [[nodiscard]] const TraceStats &getTraceStats() const { return TrStats; }
  [[nodiscard]] std::string getOutFile() const { return OutFile; }

private:
  const TraceStats &TrStats;
  const std::string OutFile;
};

} // namespace psr

#endif
