#include <CrySLLexer.h>
#include <CrySLParserEngine.h>
#include <antlr4-runtime.h>
#include <fstream>
#include <future>
#include <tuple>

namespace CCPP {
using namespace std;
using namespace antlr4;

CrySLParserEngine::CrySLParserEngine(vector<string> &&CrySL_FileNames)
    : FileNames(move(CrySL_FileNames)) {}
CrySLParserEngine::CrySLParserEngine(const vector<string> &CrySL_FileNames)
    : FileNames(CrySL_FileNames) {}

bool CrySLParserEngine::parseAndTypecheck() {
  // vector<future<tuple<CrySLParser::DomainModelContext *, bool>>> procs;
  bool succ = true;
  for (auto &filename : FileNames) {
    /*procs.push_back(async(launch::async, [filename]() {
      ifstream fIn(filename);
      ANTLRInputStream input(fIn);
      CrySLLexer lexer(&input);
      CommonTokenStream tokens(&lexer);
      CrySLParser parser(&tokens);

      auto ast = parser.domainModel();
      std::cout << ast->getText()<<std::endl;

      return make_tuple(ast, parser.getNumberOfSyntaxErrors() == 0);
    }));*/
    auto astCtx = std::make_unique<ASTContext>(filename);
    if (astCtx->parse())
      ASTs.push_back(std::move(astCtx));
    else {
      succ = false;
      numSyntaxErrors += astCtx->getNumSyntaxErrors();
    }
  }

  /*for (auto &fut : procs) {
    auto result = fut.get();
    if (get<1>(result)) {
      ASTs.push_back(get<0>(result));
    } else
      succ = false;
  }*/
  // TODO make typechecking as parallel as possible
  if (succ) {
    CrySLTypechecker ctc(ASTs);
    succ = ctc.typecheck();
  }
  return succ;
}
const decltype(CrySLParserEngine::ASTs) &
CrySLParserEngine::CrySLParserEngine::getAllASTs() const {
  return ASTs;
}
decltype(CrySLParserEngine::ASTs) &&
CrySLParserEngine::CrySLParserEngine::getAllASTs() {
  return std::move(ASTs);
}
size_t CrySLParserEngine::getNumberOfSyntaxErrors() const {
  return numSyntaxErrors;
}
bool CrySLParserEngine::orderTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::ORDER);
}
bool CrySLParserEngine::objectsTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::OBJECTS);
}
bool CrySLParserEngine::eventsTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::EVENTS);
}
bool CrySLParserEngine::ensuresTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::ENSURES);
}
bool CrySLParserEngine::negatesTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::NEGATES);
}
bool CrySLParserEngine::requiresTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::REQUIRES);
}
bool CrySLParserEngine::forbiddenTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::FORBIDDEN);
}
bool CrySLParserEngine::constraintsTypecheckSucceeded() const {
  return !(typechecksSucceeded & CrySLTypechecker::CONSTRAINTS);
}
bool CrySLParserEngine::typecheckSucceeded() const {
  return typechecksSucceeded == CrySLTypechecker::NONE;
}
} // namespace CCPP