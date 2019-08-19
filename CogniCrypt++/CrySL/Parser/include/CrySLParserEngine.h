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
  decltype(ASTs) &getAllASTs();
  decltype(ASTs) &&getAllASTs();
};
} // namespace CCPP