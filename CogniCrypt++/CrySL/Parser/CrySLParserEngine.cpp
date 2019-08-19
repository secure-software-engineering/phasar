#include "CrySLParserEngine.h"
#include "CrySLLexer.h"
#include <antlr4-runtime.h>
#include <fstream>
#include <future>
#include <tuple>
namespace CCPP {
using namespace std;

CrySLParserEngine::CrySLParserEngine(vector<string> &&CrySL_FileNames)
    : FileNames(move(CrySL_FileNames)) {}

bool CrySLParserEngine::parseAndTypecheck() {
  vector<future<tuple<CrySLParser::DomainModelContext *, bool>>> procs;
  for (auto &filename : FileNames) {
    procs.push_back(async(launch::async, [filename]() {
      ifstream fIn(filename);
      ANTLRInputStream input(fIn);
      CrySLLexer lexer(&input);
      CommonTokenStream tokens(&lexer);
      CrySLParser parser(&tokens);

      auto ast = parser.domainModel();

      return make_tuple(ast, parser.getNumberOfSyntaxErrors() == 0);
    }));
  }
  bool succ = true;
  for (auto &fut : procs) {
    auto result = fut.get();
    if (get<1>(result)) {
      ASTs.push_back(get<0>(result));
    } else
      succ = false;
  }
  return succ;
}
decltype(ASTs) &CrySLParserEngine::getAllASTs() { return ASTs; }
decltype(ASTs) &&CrySLParserEngine::getAllASTs() { return std::move(ASTs); }
} // namespace CCPP