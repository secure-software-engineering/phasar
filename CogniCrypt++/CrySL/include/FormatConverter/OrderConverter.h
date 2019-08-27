#pragma once
#include <CrySLParser.h>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>

#include <string>

namespace CCPP {

class OrderConverter {
  std::string getFunctionName(CrySLParser::EventsOccurence *evt);
  
public:
  OrderConverter(CrySLParser::OrderContext *order,
                 CrySLParser::EventsContext *evt);

  std::unique_ptr<psr::TypeStateDescription> convert();
};
} // namespace CCPP