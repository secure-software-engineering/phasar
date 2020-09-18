/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef LCOVWRITER_H
#define LCOVWRITER_H

#include "TraceStatsWriter.h"

namespace psr {

class LcovWriter : public TraceStatsWriter {
public:
  LcovWriter(const TraceStats &_traceStats, const std::string _outFile)
      : TraceStatsWriter(_traceStats, _outFile) {}
  ~LcovWriter() override = default;

  void write() const override;
};

} // namespace psr

#endif // LCOVWRITER_H
