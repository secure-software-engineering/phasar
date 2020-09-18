/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef LINENUMBERWRITER_H
#define LINENUMBERWRITER_H

#include "TraceStatsWriter.h"

namespace psr {

class LineNumberWriter : public TraceStatsWriter {
public:
  LineNumberWriter(const TraceStats &_traceStats, const std::string _outFile)
      : TraceStatsWriter(_traceStats, _outFile) {}
  ~LineNumberWriter() override = default;

  void write() const override;
};

} // namespace psr

#endif // LINENUMBERWRITER_H
