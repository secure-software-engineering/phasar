#pragma once
#include <antlr4-runtime.h>
#include <ostream>
#include <string>
namespace CCPP {
/// \brief A struct holding the line-number and column-number of the
/// start-position of a syntax element in the CrySL spec
struct Position {
  /// \brief The line number
  int line;
  /// \brief The column number
  int column;
  std::string filename;
  Position(int line, int col, const std::string &filename = "");
  Position(antlr4::ParserRuleContext *p, const std::string &filename = "");
  Position(antlr4::tree::TerminalNode *t, const std::string &filename = "");
  Position(antlr4::Token *tok, const std::string &filename = "");
  /// \brief Prints the position pos as tuple "(line, column)" to the os stream
  friend std::ostream &operator<<(std::ostream &os, const Position &pos);
};

} // namespace CCPP