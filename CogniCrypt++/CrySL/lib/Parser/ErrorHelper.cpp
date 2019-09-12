#include <Parser/ErrorHelper.h>
#include <iostream>

namespace CCPP {

void reportError(const Position &pos, const std::string &msg) {
  std::cerr << "Error: " << pos << ": " << msg << std::endl;
}
void reportWarning(const Position &pos, const std::string &msg) {
  std::cerr << "Warning: " << pos << ": " << msg << std::endl;
}
void reportError(const Position &pos,
                 std::initializer_list<std::string> &&ilist) {
  std::cerr << "Error: " << pos << ": ";
  for (const auto &str : ilist) {
    std::cerr << str;
  }
  std::cerr << std::endl;
}
void reportWarning(const Position &pos,
                   std::initializer_list<std::string> &&ilist) {
  std::cerr << "Warning: " << pos << ": ";
  for (const auto &str : ilist) {
    std::cerr << str;
  }
  std::cerr << std::endl;
}
} // namespace CCPP