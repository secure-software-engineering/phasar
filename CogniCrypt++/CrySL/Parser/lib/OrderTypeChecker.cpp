#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TokenHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
// using namespace std;

bool checkEventAndSymbols(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects) {

  if (!DefinedObjects.count(primaryContext->eventName->getText())) {
    std::cerr << Position(primaryContext) << ": The event '"
              << primaryContext->eventName->getText()
              << "' is not defined in EVENTS section" << std::endl;
    return false;
  }

  return true;
}

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

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {
  bool result = true;
  for (auto simpleOrder : order->orderSequence()->simpleOrder()) {
    for (auto unorderedSymbolsContext : simpleOrder->unorderedSymbols()) {
      result &= checkBound(unorderedSymbolsContext);
      for (auto primaryContext : unorderedSymbolsContext->primary()) {
        result &= checkEventAndSymbols(primaryContext, DefinedObjects);
      }
    }
  }
  return result;
}

} // namespace CCPP