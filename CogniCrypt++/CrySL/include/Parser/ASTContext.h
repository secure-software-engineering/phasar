#pragma once
#include <Parser/CrySLLexer.h>
#include <Parser/CrySLParser.h>
#include <Parser/FileSpecificErrorListener.h>
#include <fstream>
#include <memory>
#include <string>

namespace CCPP {
/// \brief A wrapper over the ANTLR lexer/parser objects
///
/// This is required, since these objects own the AST nodes. Hence they have to
/// be alive as long as the AST nodes are accessible
class ASTContext {
  // std::unique_ptr<std::ifstream> fIn;
  std::unique_ptr<antlr4::ANTLRInputStream> input;
  std::unique_ptr<CrySLLexer> lexer;
  std::unique_ptr<antlr4::CommonTokenStream> tokens;
  std::unique_ptr<CrySLParser> parser;
  std::unique_ptr<FileSpecificErrorListener> err;
  CrySLParser::DomainModelContext *AST;
  std::string filename;
  size_t parsed = 0;

public:
  ASTContext(const std::string &filename);
  ASTContext(std::string &&filename);
  /// \brief Parses the CrySL file, which name was passed to the constructor
  ///
  /// Remarks: This method can be called multiple times, but the parsing will
  /// only be performed the first time this method is called.
  ///
  /// \returns True, iff there were no syntax errors
  bool parse();
  /// \brief Retrieves the root node of the AST for the current CrySL file.
  /// This is not valid before parse() was called the first time for this object
  CrySLParser::DomainModelContext *getAST() const;
  /// \brief Retrieves the name of the current CrySL file
  const std::string &getFilename() const;
  /// \brief After parse() was called, gives the number of syntax errors which
  /// occurred while parsing
  size_t getNumSyntaxErrors() const;
};
} // namespace CCPP