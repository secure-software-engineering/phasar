#include <ErrorHelper.h>
#include <iostream>

namespace CCPP {

void reportError(const Position &pos, const std::string &msg) {
  std::errs << "Error: " << pos << ": " << msg << std::endl;
}
void reportWarning(const Position &pos, const std::string &msg) {
  std::errs << "Warning: " << pos << ": " << msg << std::endl;
}
void reportError(const Position &pos,
                 std::initializer_list<std::string> &&ilist) {
  std::errs << "Error: " << pos << ": ";
  for (const auto &str : ilist) {
    std::errs << str;
  }
  std::endl;
}
void reportError(const Position &pos,
                 std::initializer_list<std::string> &&ilist) {
  std::errs << "Warning: " << pos << ": ";
  for (const auto &str : ilist) {
    std::errs << str;
  }
  std::endl;
}
} // namespace CCPP