#include <Parser/ASTContext.h>
#include <Parser/CrySLParserEngine.h>

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
const std::string &ASTContext::getFilename() const { return filename; }
bool ASTContext::parse() {
  if (parsed)
    return parsed == 1;

  // fIn = std::make_unique<std::ifstream>(filename);
  std::ifstream fIn(filename);
  input = std::make_unique<ANTLRInputStream>(fIn);
  lexer = std::make_unique<CrySLLexer>(input.get());
  tokens = std::make_unique<CommonTokenStream>(lexer.get());
  parser = std::make_unique<CrySLParser>(tokens.get());
  err = std::make_unique<FileSpecificErrorListener>(filename);
  parser->removeErrorListeners();
  parser->addErrorListener(err.get());

  AST = parser->domainModel();
  parsed = 1 + parser->getNumberOfSyntaxErrors();
  return parsed == 1;
}
size_t ASTContext::getNumSyntaxErrors() const { return parsed - 1; }
} // namespace CCPP