#include <Parser/PositionHelper.h>

namespace CCPP {
Position::Position(int line, int col, const std::string &filename)
    : line(line), column(col), filename(filename) {}
Position::Position(antlr4::ParserRuleContext *p, const std::string &filename)
    : Position(p ? p->getStart() : nullptr, filename) {}
Position::Position(antlr4::tree::TerminalNode *t, const std::string &filename)
    : Position(t ? t->getSymbol() : nullptr, filename) {}
Position::Position(antlr4::Token *tok, const std::string &filename)
    : filename(filename) {
  if (tok) {
    line = tok->getLine();
    column = tok->getCharPositionInLine();
  } else {
    line = column = 0;
  }
}
std::ostream &operator<<(std::ostream &os, const CCPP::Position &pos) {
  if (!pos.filename.empty()) {
    os << pos.filename << " at ";
  }
  return os << "(" << pos.line << ", " << pos.column << ")";
}
} // namespace CCPP
