
#include "CrySLTypechecker.h"
#include "PositionHelper.h"
#include "Types/Type.h"
#include <iostream>
namespace CCPP {
bool checkEventObjects(
    CrySLParser::ParametersListContext *paramList,
    std::unordered_map<std::string, std::shared_ptr<Types::Type>>
        &definedObjects) {
  if (!paramList)
    return true;
  bool result = true;
  for (auto prm : paramList->param()) {
    if (prm->memberAccess()) {
      auto parameters = prm->memberAccess()->Ident();

      for (auto pObject : parameters) {
        if (!definedObjects.count(pObject->getText())) {
          std::cerr << Position(pObject)
                    << ": object does not exist in the objects section"
                    << std::endl;
          return false;
        }
      }
    } else if (!prm->thisPtr || !prm->wildCard) {
      return true;
    }
  }
  return result;
}

bool CrySLTypechecker::CrySLSpec::typecheck(CrySLParser::EventsContext *evt) {

  bool result = true;

  for (auto event : evt->eventsOccurence()) {
    auto eventName = event->eventName->getText();

    if (event->returnValue) {
      if (!DefinedObjects.count(event->returnValue->getText())) {
        std::cerr << Position(event->returnValue)
                  << ": The return-object is not defined" << std::endl;
        return false;
      }
    }

    if (DefinedEvents.count(eventName)) {
      result = checkEventObjects(event->parametersList(), DefinedObjects);
    } else {
      DefinedEvents.insert(eventName);
      result = checkEventObjects(event->parametersList(), DefinedObjects);
    }
    return result;
  }
  for (auto aggregate : evt->eventAggregate()) {
    for (auto agg : aggregate->agg()->Ident()) {
      if (!DefinedEvents.count(agg->getText())) {
        result = false;
        std::cerr << Position(agg) << ": The event is not defined" << std::endl;
      }
    }
    DefinedEvents.insert(aggregate->eventName->getText());
  }
}

} // namespace CCPP