#pragma once
#include <Parser/PositionHelper.h>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

// Use this unified interface for reporting typechecking-errors/warnings

namespace CCPP {

/// \brief Report an error which occurred at CrySL-position pos and attach the
/// error-message msg to it
void reportError(const Position &pos, const std::string &msg);
/// \brief Report a warning which occurred at CrySL-position pos and attach the
/// warning-message msg to it
void reportWarning(const Position &pos, const std::string &msg);
/// \brief Report an error which occurred at CrySL-position pos and attach the
/// error-message ilist to it
void reportError(const Position &pos,
                 std::initializer_list<std::string> &&ilist);
/// \brief Report a warning which occurred at CrySL-position pos and attach the
/// warning-message ilist to it
void reportWarning(const Position &pos,
                   std::initializer_list<std::string> &&ilist);

/// \brief Report an error with the message errMsg at the CrySL-position defined
/// by p and filename, iff toCheck is nullptr
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
/// \brief Report an error with the message errMsg at the CrySL-position defined
/// by t and filename, iff toCheck is nullptr
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
/// \brief Report an error with the message errMsg at the CrySL-position defined
/// by p and filename, iff toCheck is nullptr
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
/// \brief Report an error with the message errMsg at the CrySL-position defined
/// by t and filename, iff toCheck is nullptr
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