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
  Position(antlr4::ParserRuleContext *p) {
    if (p) {
      line = p->getStart()->getLine();
      column = p->getStart()->getCharPositionInLine();
    } else {
      line = column = 0;
    }
  }
  Position(antlr4::tree::TerminalNode *t) {
    if (t) {
      line = t->getSymbol()->getLine();
      column = t->getSymbol()->getCharPositionInLine();
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