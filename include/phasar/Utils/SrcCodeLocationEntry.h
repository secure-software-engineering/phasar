#ifndef PHASAR_UTILS_SRCCODELOCATIONENTRY_H
#define PHASAR_UTILS_SRCCODELOCATIONENTRY_H

#include <set>

#include <sys/types.h>

namespace psr {

struct SrcCodeLocationEntry {
  SrcCodeLocationEntry(u_int32_t Line, std::set<u_int32_t> Column)
      : Line(Line), Column(std::move(Column)) {}
  u_int32_t Line{};
  std::set<u_int32_t> Column;
  bool operator==(const SrcCodeLocationEntry &Other) const {
    return Line == Other.Line && Column == Other.Column;
  }
  bool operator<(const SrcCodeLocationEntry &Other) const {
    return Line < Other.Line && Column < Other.Column;
  }
};

} // namespace psr

#endif
