#include <PositionHelper.h>
#include <iostream>
#include <memory>
#include <string>

namespace CCPP {
template <typename T, typename Msg = const char *>
std::shared_ptr<T> &reportIfNull(antlr4::ParserRuleContext *p,
                                 std::shared_ptr<T> &toCheck, Msg errMsg,
                                 const std::string &filename = "") {
  if (toCheck) {
    return toCheck;
  } else {
    std::cerr << Position(p, filename) << ": " << errMsg << std::endl;
    return toCheck;
  }
}
template <typename T, typename Msg = const char *>
std::shared_ptr<T> &reportIfNull(antlr4::tree::TerminalNode *t,
                                 std::shared_ptr<T> &toCheck, Msg errMsg,
                                 const std::string &filename = "") {
  if (toCheck) {
    return toCheck;
  } else {
    std::cerr << Position(t, filename) << ": " << errMsg << std::endl;
    return toCheck;
  }
}
template <typename T, typename Msg = const char *>
std::shared_ptr<T> &&reportIfNull(antlr4::ParserRuleContext *p,
                                  std::shared_ptr<T> &&toCheck, Msg errMsg,
                                  const std::string &filename = "") {
  if (toCheck) {
    return std::move(toCheck);
  } else {
    std::cerr << Position(p, filename) << ": " << errMsg << std::endl;
    return std::move(toCheck);
  }
}
template <typename T, typename Msg = const char *>
std::shared_ptr<T> &&reportIfNull(antlr4::tree::TerminalNode *t,
                                  std::shared_ptr<T> &&toCheck, Msg errMsg,
                                  const std::string &filename = "") {
  if (toCheck) {
    return std::move(toCheck);
  } else {
    std::cerr << Position(t, filename) << ": " << errMsg << std::endl;
    return std::move(toCheck);
  }
}
} // namespace CCPP