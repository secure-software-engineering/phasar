/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LINENUMBERWRITER_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LINENUMBERWRITER_H

#include "TraceStatsWriter.h"

namespace psr {

class LineNumberWriter : public TraceStatsWriter {
public:
  LineNumberWriter(const TraceStats &TraceStats, const std::string &OutFile)
      : TraceStatsWriter(TraceStats, OutFile) {}
  ~LineNumberWriter() override = default;

  void write() const override;
};

} // namespace psr

#endif
