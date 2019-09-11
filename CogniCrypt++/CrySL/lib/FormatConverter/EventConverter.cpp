
#include "CrySLTypechecker.h"
#include <Event.h>
#include <EventConverter.h>
#include <iostream>

namespace CCPP {
void checkFactoryConsumerFunc(CrySLParser::EventsOccurenceContext*  event, Event &eventObj);
// this function extracts events from AST node of eventscontext and convert them
// into Event entity class
std::vector<Event>
EventConverter::formatConverter(CrySLParser::EventsContext *eventCtx) {
  Event eventObj;
  std::vector<Object> objects;
  std::vector<Event> events;

  for (auto event : eventCtx->eventsOccurence()) {
    eventObj.eventName = event->eventName->getText();

    for (auto param : event->parametersList()->param()) {
      objects.emplace_back(param); // creates object of type Object using
                                   // parameterize constructor with param as a
                                   // parameter and inserts it in vector
    }

    checkFactoryConsumerFunc(
        event, eventObj); // checks if the event is factory or consumer funciton
                          // and finds it's event
    events.push_back(eventObj);
  }
  return events;
}

void checkFactoryConsumerFunc(CrySLParser::EventsOccurenceContext* event, Event &eventObj) {
  if (event->returnValue && event->returnValue->getText() == "this") {
    eventObj.isFactoryFunction = true;
    eventObj.factoryParamIdx = {-1};
  } else {
    eventObj.isConsumingFunction = true;

    int index = 0;
    std::set<int>
        consumerParamIdx; // we need it because this object can occurr nultiple
                          // times as a parameter of event
    for (auto param : event->parametersList()->param()) {

      if (param->getText() == "this") {
        consumerParamIdx.insert(index);
      }
      index++;
    }
    eventObj.consumerParamIdx = consumerParamIdx;
  }
}

} // namespace CCPP