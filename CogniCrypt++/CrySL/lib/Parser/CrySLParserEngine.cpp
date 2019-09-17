#include <Parser/CrySLLexer.h>
#include <Parser/CrySLParserEngine.h>
#include <antlr4-runtime.h>
#include <fstream>
#include <future>
#include <optional>
#include <tuple>

namespace CCPP {
using namespace std;
using namespace antlr4;

CrySLParserEngine::CrySLParserEngine(vector<string> &&CrySL_FileNames)
    : FileNames(move(CrySL_FileNames)) {}
CrySLParserEngine::CrySLParserEngine(const vector<string> &CrySL_FileNames)
    : FileNames(CrySL_FileNames) {}

bool CrySLParserEngine::parseAndTypecheck() {
  vector<future<tuple<unique_ptr<ASTContext>,
                      optional<CrySLTypechecker::CrySLSpec>, bool>>>
      procs;
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
    procs.push_back(async(launch::async, [filename]() {
      auto ret = std::make_unique<ASTContext>(filename);
      bool succ = ret->parse();
      if (succ) {
        CrySLTypechecker::CrySLSpec spec(ret->getAST(), ret->getFilename());
        succ = spec.typecheck();
        return make_tuple(
            move(ret), optional<CrySLTypechecker::CrySLSpec>(std::move(spec)),
            succ);
      }
      return make_tuple(move(ret), optional<CrySLTypechecker::CrySLSpec>(),
                        succ);
    }));
    /*auto astCtx = std::make_unique<ASTContext>(filename);
    if (astCtx->parse())
      ASTs.push_back(std::move(astCtx));
    else {
      succ = false;
      numSyntaxErrors += astCtx->getNumSyntaxErrors();
    }*/
  }
  std::vector<CrySLTypechecker::CrySLSpec> specs;

  for (auto &fut : procs) {
    auto result = fut.get();

    numSyntaxErrors += get<0>(result)->getNumSyntaxErrors();

    if (get<2>(result)) {
      ASTs.push_back(move(get<0>(result)));

    } else {
      succ = false;
    }
    if (get<1>(result)) {
      specs.push_back(move(*get<1>(result)));
      typechecksSucceeded = (CrySLTypechecker::TypeCheckKind)(
          typechecksSucceeded | get<1>(result)->getErrors());
    }
  }
  //std::cout << "Error flag: " << (int)typechecksSucceeded << std::endl;
  if (succ) {
    CrySLTypechecker ctc(ASTs, move(specs));
    // succ = ctc.typecheck();
    succ = ctc.interSpecificationTypecheck();
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