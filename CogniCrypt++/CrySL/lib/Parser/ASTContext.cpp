#include <ASTContext.h>
#include <CrySLParserEngine.h>
#include <antlr4-runtime.h>
#include <fstream>

namespace CCPP {

using namespace antlr4;

ASTContext::ASTContext(const std::string &filename) : filename(filename) {}
ASTContext::ASTContext(std::string &&filename)
    : filename(std::move(filename)) {}
CrySLParser::DomainModelContext *ASTContext::getAST() const {
  return parsed ? AST : nullptr;
}
bool ASTContext::parse() {
  if (parsed)
    return parsed == 1;

  fIn = std::make_unique<std::ifstream>(filename);
  input = std::make_unique<ANTLRInputStream>(*fIn.get());
  lexer = std::make_unique<CrySLLexer>(input.get());
  tokens = std::make_unique<CommonTokenStream>(lexer.get());
  parser = std::make_unique<CrySLParser>(tokens.get());

  AST = parser->domainModel();
  parsed = 1 + parser->getNumberOfSyntaxErrors();
  return parsed == 1;
}
} // namespace CCPP