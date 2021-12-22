/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef LINENUMBERWRITER_H
#define LINENUMBERWRITER_H

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

#endif // LINENUMBERWRITER_H
