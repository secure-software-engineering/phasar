#include "CrySLTypechecker.h"
#include "PositionHelper.h"
#include "Types/Type.h"
#include <iostream>

bool CCPP::CrySLTypechecker::CrySLSpec::typecheck(
    CrySLParser::ForbiddenContext *forb) {
  bool result = true;

  for (auto forbidden : forb->forbiddenOccurence()) {
    if (forbidden->eventName &&
        !DefinedEvents.count(forbidden->eventName->getText())) {
      std::cerr << Position(forbidden) << ":Forbidden event '"
                << forbidden->eventName->getText()
                << "' does not exist in the event section" << std::endl;
      return false;
    }
  }
  return result;
}