#include <CrySLTypechecker.h>
#include <ErrorHelper.h>
#include <TypeParser.h>
#include <Types/Type.h>
#include <iostream>

namespace CCPP {
// using namespace std;

bool checkEvent(
    CrySLParser::PrimaryContext *primaryContext,
    std::unordered_map<std::string, std::shared_ptr<Type>> &DefinedObjects) {
  bool result = true;
  if (!DefinedObjects.count(primaryContext->eventName->getText())) {
    std::cerr << Position(primaryContext)
              << ": event is not defined in EVENTS section" << std::endl;
    return false;
  }
  return result;
}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::OrderContext *order) {
  bool result = true;
  for (auto simord : order->orderSequence()->simpleOrder()) {
    for (auto unordsym : simord->unorderedSymbols()) {
      for (auto primcon : unordsym->primary()) {
        result = checkEvent(primcon, DefinedObjects);
      }
    }
  }

  return result;
}

} // namespace CCPP