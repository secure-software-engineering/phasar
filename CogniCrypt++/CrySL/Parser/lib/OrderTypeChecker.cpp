#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TokenHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
// using namespace std;

bool checkBound(CrySLParser::UnorderedSymbolsContext *uno) {
  bool succ = true;
  if (uno->bound) {
    long long lo, hi;
    long long max_hi = (long long)uno->primary().size();
    lo = uno->lower ? parseInt(uno->lower->getText()) : 0;
    hi = uno->upper ? parseInt(uno->upper->getText()) : max_hi;

    if (lo > hi) {
      std::cerr << Position(uno->bound)
                << ": The lower bound must not be greater than the upper bound"
                << std::endl;
      succ = false;
    }
    if (hi > max_hi) {
      std::cerr << Position(uno->upper)
                << ": The upper bound must not be greater than the number of "
                   "primary expressions in the unordered set"
                << std::endl;
      succ = false;
    }
  }
  return succ;
}

bool checkEvent(CrySLParser::PrimaryContext *primaryContext,
                std::unordered_set<std::string> &DefinedEventsDummy,
                std::unordered_set<std::string> &DefinedEvents) {

  DefinedEventsDummy.erase(primaryContext->eventName->getText());

  if (!DefinedEvents.count(primaryContext->eventName->getText())) {
    std::cerr << Position(primaryContext)
              << ": event is not defined in EVENTS section but is called in "
                 "ORDER section"
              << std::endl;
    return false;
  }
}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {
  bool result = true;
  std::unordered_set<std::string> DefinedEventsDummy = DefinedEvents;
  for (auto simpleOrder : order->orderSequence()->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      result &= checkBound(unorderedSymbolsContext);
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        result &= checkEvent(primaryContext, DefinedEventsDummy, DefinedEvents);
      }
    }
  }
  if (!DefinedEventsDummy.empty()) {
    std::cerr << Position(order)
              << ": event is defined in EVENTS section but not called in ORDER "
                 "section"
              << std::endl;
  }
  return result;
}

} // namespace CCPP