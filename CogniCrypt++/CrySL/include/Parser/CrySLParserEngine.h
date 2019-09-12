#pragma once
#include <Parser/CrySLParser.h>
#include <Parser/CrySLTypechecker.h>
#include <Parser/ASTContext.h>
#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace CCPP {
/// \brief The top-level class for parsing and typechecking one or more CrySL++
/// files
class CrySLParserEngine {
  std::vector<std::unique_ptr<ASTContext>> ASTs;
  std::vector<std::string> FileNames;
  CrySLTypechecker::TypeCheckKind typechecksSucceeded = CrySLTypechecker::NONE;
  size_t numSyntaxErrors = 0;

public:
  CrySLParserEngine(std::vector<std::string> &&CrySL_FileNames);
  CrySLParserEngine(const std::vector<std::string> &CrySL_FileNames);
  /// \brief Parses and typechecks all CrySL files, which are passed to the
  /// constructor
  ///
  /// The parsing and parts of typechecking may be performed concurrently
  bool parseAndTypecheck();
  /// \brief Retrieves a vector of the root-nodes of the abstract syntax trees
  /// of all CrySL files, which are passed to the constructor
  const decltype(ASTs) &getAllASTs() const;
  /// \brief Retrieves a vector of the root-nodes of the abstract syntax trees
  /// of all CrySL files, which are passed to the constructor
  decltype(ASTs) &&getAllASTs();
  size_t getNumberOfSyntaxErrors() const;
  bool orderTypecheckSucceeded() const;
  bool objectsTypecheckSucceeded() const;
  bool eventsTypecheckSucceeded() const;
  bool ensuresTypecheckSucceeded() const;
  bool negatesTypecheckSucceeded() const;
  bool requiresTypecheckSucceeded() const;
  bool forbiddenTypecheckSucceeded() const;
  bool constraintsTypecheckSucceeded() const;
  bool typecheckSucceeded() const;
};
} // namespace CCPP