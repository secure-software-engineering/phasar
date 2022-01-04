/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LCOVWRITER_H
#define PHASAR_PHASARLLVM_DATAFLOWSOLVER_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LCOVWRITER_H

#include "TraceStatsWriter.h"

namespace psr {

class LcovWriter : public TraceStatsWriter {
public:
  LcovWriter(const TraceStats &TStats, const std::string &OutFile)
      : TraceStatsWriter(TStats, OutFile) {}
  ~LcovWriter() override = default;

  void write() const override;
};

} // namespace psr

#endif
