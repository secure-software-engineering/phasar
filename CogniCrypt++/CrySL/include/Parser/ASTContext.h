#pragma once
#include <CrySLLexer.h>
#include <CrySLParser.h>
#include <FileSpecificErrorListener.h>
#include <fstream>
#include <memory>
#include <string>

namespace CCPP {
class ASTContext {
  std::unique_ptr<std::ifstream> fIn;
  std::unique_ptr<antlr4::ANTLRInputStream> input;
  std::unique_ptr<CrySLLexer> lexer;
  std::unique_ptr<antlr4::CommonTokenStream> tokens;
  std::unique_ptr<CrySLParser> parser;
  std::unique_ptr<FileSpecificErrorListener> err;
  CrySLParser::DomainModelContext *AST;
  std::string filename;
  int parsed = 0;

public:
  ASTContext(const std::string &filename);
  ASTContext(std::string &&filename);

  bool parse();
  CrySLParser::DomainModelContext *getAST() const;
  const std::string &getFilename() const;
};
} // namespace CCPP