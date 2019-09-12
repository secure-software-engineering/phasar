
#include <Parser/CrySLTypechecker.h>
#include <Parser/PositionHelper.h>
#include <Parser/Types/Type.h>
#include <Parser/ErrorHelper.h>
#include <iostream>

namespace CCPP {
bool checkEventObjects(
    CrySLParser::ParametersListContext *paramList,
    std::unordered_map<std::string, std::shared_ptr<Types::Type>>
        &definedObjects,
    const std::string &filename) {
  if (!paramList)
    return true;
  bool result = true;
  for (auto prm : paramList->param()) {
    if (prm->memberAccess()) {
      auto parameters = prm->memberAccess()->Ident();

      for (auto pObject : parameters) {
        if (!definedObjects.count(pObject->getText())) {
          // std::cerr << Position(pObject, filename)
          //          << ": object does not exist in the objects section"
          //          << std::endl;
          reportError(Position(pObject, filename),
                      {"The object '", pObject->getText(),
                       "' is not defined in the OBJECTS section"});
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
        // std::cerr << Position(event->returnValue, filename)
        //          << ": The return-object is not defined" << std::endl;
        reportError(Position(event->returnValue, filename),
                    {"The return-object '", event->returnValue->getText(),
                     "' is not defined in the OBJECTS section"});
        return false;
      }
    }

    if (DefinedEvents.count(eventName)) {
      checkEventObjects(event->parametersList(), DefinedObjects, filename);
      result = false;
      // std::cerr << Position(event, filename) << ": The event '" << eventName
      //          << "' is already defined" << std::endl;
      reportError(Position(event, filename),
                  {"The event '", eventName, "' is already defined"});
    } else {
      // std::cout << "Define event '" << eventName << "'" << std::endl;
      // DefinedEvents.insert(eventName);
      DefinedEvents[eventName] = {eventName};
      result =
          checkEventObjects(event->parametersList(), DefinedObjects, filename);
    }
  }
  for (auto aggregate : evt->eventAggregate()) {
    auto aggs = aggregate->agg()->Ident();
    std::unordered_set<std::string> aggsSet;
    for (auto agg : aggs) {
      aggsSet.insert(agg->getText());
      if (!DefinedEvents.count(agg->getText())) {
        result = false;
        // std::cerr << Position(agg, filename) << ": The event '"
        //          << agg->getText() << "' is not defined" << std::endl;
        reportError(Position(agg, filename),
                    {"The event '", agg->getText(), "' is not defined"});
      }
    }
    // DefinedEvents.insert(aggregate->eventName->getText());
    DefinedEvents[aggregate->eventName->getText()] = std::move(aggsSet);
  }
  return result;
}

} // namespace CCPP