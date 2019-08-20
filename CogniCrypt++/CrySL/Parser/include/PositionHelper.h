#pragma once
#include <antlr4-runtime.h>
#include <ostream>

struct Position {
  int line, column;
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
  friend std::ostream &operator<<(std::ostream &os, const Position &pos);
};
std::ostream &operator<<(std::ostream &os, const Position &pos) {
  return os << "(" << pos.line << ", " << pos.column << ")";
}