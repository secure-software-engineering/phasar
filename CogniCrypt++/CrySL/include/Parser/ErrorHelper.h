#pragma once
#include <PositionHelper.h>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

namespace CCPP {

void reportError(const Position &pos, const std::string &msg);
void reportWarning(const Position &pos, const std::string &msg);
void reportError(const Position &pos,
                 std::initializer_list<std::string> &&ilist);
void reportWarning(const Position &pos,
                 std::initializer_list<std::string> &&ilist);

template <typename T, typename Msg = const char *>
std::shared_ptr<T> &reportIfNull(antlr4::ParserRuleContext *p,
                                 std::shared_ptr<T> &toCheck, Msg errMsg,
                                 const std::string &filename = "") {
  if (toCheck) {
    return toCheck;
  } else {
    // std::cerr << Position(p, filename) << ": " << errMsg << std::endl;
    reportError(Position(p, filename), errMsg);
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
    // std::cerr << Position(t, filename) << ": " << errMsg << std::endl;
    reportError(Position(t, filename), errMsg);
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
    // std::cerr << Position(p, filename) << ": " << errMsg << std::endl;
    reportError(Position(p, filename), errMsg);
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
    // std::cerr << Position(t, filename) << ": " << errMsg << std::endl;
    reportError(Position(t, filename), errMsg);
    return std::move(toCheck);
  }
}

} // namespace CCPP