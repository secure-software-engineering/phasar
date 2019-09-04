#pragma once
#include <CrySLParser.h>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>

#include <string>

namespace CCPP {

class OrderConverter {
  std::string getFunctionName(CrySLParser::EventsOccurenceContext *evt);
  std::string specName;
public:
  OrderConverter(const std::string &specName, CrySLParser::OrderContext *order,
                 CrySLParser::EventsContext *evt);

  /// \brief Converts the order-regex, which is passed as ctor-argument, to a
  /// psr::TypeStateDescription
  ///
  /// The main work tbd in this function is to convert a regex to a DFA
  std::unique_ptr<psr::TypeStateDescription> convert();
};
} // namespace CCPP