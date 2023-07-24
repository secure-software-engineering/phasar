/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LINENUMBERENTRY_H
#define PHASAR_PHASARLLVM_DATAFLOW_IFDSIDE_IFDSFIELDSENSTAINTANALYSIS_STATS_LINENUMBERENTRY_H

#include <functional>

namespace psr {

class LineNumberEntry {
public:
  LineNumberEntry(unsigned int LineNumber) : LineNumber(LineNumber) {}
  ~LineNumberEntry() = default;

  bool operator<(const LineNumberEntry &Rhs) const {
    return std::less<unsigned int>{}(LineNumber, Rhs.LineNumber);
  }

  [[nodiscard]] unsigned int getLineNumber() const { return LineNumber; }

  [[nodiscard]] bool isReturnValue() const { return ReturnVal; }
  void setReturnValue(bool ReturnVal) { this->ReturnVal = ReturnVal; }

private:
  unsigned int LineNumber;
  bool ReturnVal = false;
};

} // namespace psr

#endif
