
#include "CrySLTypechecker.h"
#include <Event.h>
#include <EventConverter.h>
#include <iostream>

namespace CCPP {

void EventConverter::formatConverter(CrySLParser::EventsContext *eventCtx) {
  EventConverter eventConverter;
  Event eventObj;
  std::vector<Object> objects;
  std::vector<Event> events;

  for (auto event : eventCtx->eventsOccurence()) {
    eventObj.eventName = event->eventName->getText();
    events.push_back(eventObj);

    for (auto param : event->parametersList()->param()) {
    }
  }
}
} // namespace CCPP