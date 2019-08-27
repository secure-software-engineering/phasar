#pragma once
#include <antlr4-runtime.h>
#include <ostream>

/// \brief A struct holding the line-number and column-number of the
/// start-position of a syntax element in the CrySL spec
struct Position {
  /// \brief The line number
  int line;
  /// \brief The column number
  int column;
  Position(int line, int col) : line(line), column(col) {}
  Position(antlr4::ParserRuleContext *p)
      : Position(p ? p->getStart() : nullptr) {}
  Position(antlr4::tree::TerminalNode *t)
      : Position(t ? t->getSymbol() : nullptr) {}
  Position(antlr4::Token *tok) {
    if (tok) {
      line = tok->getLine();
      column = tok->getCharPositionInLine();
    } else {
      line = column = 0;
    }
  }
  /// \brief Prints the position pos as tuple "(line, column)" to the os stream
  friend std::ostream &operator<<(std::ostream &os, const Position &pos);
};
std::ostream &operator<<(std::ostream &os, const Position &pos) {
  return os << "(" << pos.line << ", " << pos.column << ")";
}