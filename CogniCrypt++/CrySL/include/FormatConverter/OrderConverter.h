#pragma once
#include <FormatConverter/DFA/StateMachine.h>
#include <Parser/CrySLParser.h>
#include <Parser/CrySLTypechecker.h>
#include <memory>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>
#include <string>
#include <unordered_set>

namespace CCPP {
class NFACreator;
class OrderConverter {
  const std::string specName;
  CrySLParser::OrderContext *order;
  CrySLParser::EventsContext *evt;

  std::unordered_set<std::string> getFunctionNames(const std::string &evt);
  std::unique_ptr<DFA::StateMachine>
  createFromContext(CrySLParser::OrderContext *order,
                    CrySLParser::EventsContext *events);

public:
  OrderConverter(const std::string &specName, CrySLParser::OrderContext *order,
                 CrySLParser::EventsContext *evt);

  /// \brief Converts the order-regex, which is passed as ctor-argument, to a
  /// psr::TypeStateDescription
  ///
  /// The main work tbd in this function is to convert a regex to a DFA
  std::unique_ptr<psr::TypeStateDescription>
  convert(const CrySLTypechecker &ctc);
  friend class NFACreator;
};
} // namespace CCPP