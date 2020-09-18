/**
 * @author Sebastian Roland <seroland86@gmail.com>
 */

#ifndef LINENUMBERENTRY_H
#define LINENUMBERENTRY_H

#include <functional>

namespace psr {

class LineNumberEntry {
public:
  LineNumberEntry(unsigned int _lineNumber) : lineNumber(_lineNumber) {}
  ~LineNumberEntry() = default;

  bool operator<(const LineNumberEntry &rhs) const {
    return std::less<unsigned int>{}(lineNumber, rhs.lineNumber);
  }

  unsigned int getLineNumber() const { return lineNumber; }

  bool isReturnValue() const { return returnValue; }
  void setReturnValue(bool _returnValue) { returnValue = _returnValue; }

private:
  unsigned int lineNumber;
  bool returnValue = false;
};

} // namespace psr

#endif // LINENUMBERENTRY_H
