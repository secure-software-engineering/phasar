#pragma once
#include "CrySLParser.h"
#include "CrySLTypechecker.h"
#include <iostream>
#include <string>
#include <vector>

namespace CCPP {
class CrySLParserEngine {
  std::vector<CrySLParser::DomainModelContext *> ASTs;
  std::vector<std::string> FileNames;

public:
  CrySLParserEngine(std::vector<std::string> &&CrySL_FileNames);
  bool parseAndTypecheck();
  const decltype(ASTs) &getAllASTs() const;
  decltype(ASTs) &&getAllASTs();
};
} // namespace CCPP