#include <PositionHelper.h>

namespace CCPP {
Position::Position(int line, int col) : line(line), column(col) {}
Position::Position(antlr4::ParserRuleContext *p)
    : Position(p ? p->getStart() : nullptr) {}
Position::Position(antlr4::tree::TerminalNode *t)
    : Position(t ? t->getSymbol() : nullptr) {}
Position::Position(antlr4::Token *tok) {
  if (tok) {
    line = tok->getLine();
    column = tok->getCharPositionInLine();
  } else {
    line = column = 0;
  }
}
std::ostream &operator<<(std::ostream &os, const CCPP::Position &pos) {
  return os << "(" << pos.line << ", " << pos.column << ")";
}
} // namespace CCPP
